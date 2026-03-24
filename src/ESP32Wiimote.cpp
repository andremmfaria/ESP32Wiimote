// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "ESP32Wiimote.h"

#include "Arduino.h"
#include "TinyWiimote.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "serial/serial_command_dispatcher.h"
#include "serial/serial_response_formatter.h"
#include "utils/serial_logging.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr size_t kSerialResponseBufSize = 192U;
constexpr const char *kWifiMockFailSsid = "__fail__";

enum WifiInitStage : uint8_t {
    KWifiInitStageStartWifi = 0U,
    KWifiInitStageMountLittleFs = 1U,
    KWifiInitStageRegisterStaticRoutes = 2U,
    KWifiInitStageRegisterApiRoutes = 3U,
    KWifiInitStageRegisterWebSocketRoutes = 4U,
    KWifiInitStageReady = 5U,
};

class Esp32SerialCommandTarget : public SerialCommandTarget {
   public:
    explicit Esp32SerialCommandTarget(ESP32Wiimote *device) : device_(device) {}

    bool setLeds(uint8_t ledMask) override { return device_->setLeds(ledMask); }

    bool setReportingMode(uint8_t mode, bool continuous) override {
        return device_->setReportingMode(static_cast<ReportingMode>(mode), continuous);
    }

    bool setAccelerometerEnabled(bool enabled) override {
        return device_->setAccelerometerEnabled(enabled);
    }

    bool requestStatus() override { return device_->requestStatus(); }

    void setScanEnabled(bool enabled) override { device_->setScanEnabled(enabled); }

    bool startDiscovery() override { return device_->startDiscovery(); }

    bool stopDiscovery() override { return device_->stopDiscovery(); }

    bool disconnectActiveController(uint8_t reason) override {
        return device_->disconnectActiveController(
            static_cast<ESP32Wiimote::DisconnectReason>(reason));
    }

    void setAutoReconnectEnabled(bool enabled) override {
        device_->setAutoReconnectEnabled(enabled);
    }

    void clearReconnectCache() override { device_->clearReconnectCache(); }

    bool isConnected() const override { return ESP32Wiimote::isConnected(); }

    uint8_t getBatteryLevel() const override { return ESP32Wiimote::getBatteryLevel(); }

   private:
    ESP32Wiimote *device_;
};

}  // namespace

/**
 * Constructor - Initialize all component managers
 */
ESP32Wiimote::ESP32Wiimote() : ESP32Wiimote(ESP32WiimoteConfig()) {}

ESP32Wiimote::ESP32Wiimote(const ESP32WiimoteConfig &config)
    : config_(config)
    , serialPrivilegedToken_(nullptr)
    , wifiApiToken_(nullptr)
    , wifiEnabled_(false)
    , wifiTokenFallbackToSerial_(false)
    , runtimeConfigStoreReady_(false)
    , runtimeConfigSnapshot_()
    , serialControlEnabled_(false)
    , serialPrivilegedCommandsRequireUnlock_(true)
    , serialInputLine_{0}
    , serialInputLen_(0)
    , serialInputOverflow_(false)
    , wifiControlEnabled_(false)
    , wifiControlInitializing_(false)
    , wifiControlReady_(false)
    , wifiNetworkCredentialsConfigured_(false)
    , wifiNetworkConnectAttempted_(false)
    , wifiNetworkConnected_(false)
    , wifiNetworkConnectFailed_(false)
    , wifiDeliveryMode_(WifiDeliveryMode::RestOnly)
    , wifiInitStage_(KWifiInitStageStartWifi)
    , wifiLayerStarted_(false)
    , littleFsMounted_(false)
    , staticRoutesRegistered_(false)
    , apiRoutesRegistered_(false)
    , websocketRoutesRegistered_(false) {
    hciCallbacks_ = new HciCallbacksHandler();
    queueManager_ = new HciQueueManager(config_.txQueueSize, config_.rxQueueSize);
    buttonState_ = new ButtonStateManager();
    sensorState_ = new SensorStateManager(config_.nunchukStickThreshold);
    dataParser_ = new WiimoteDataParser(buttonState_, sensorState_);

    runtimeConfigSnapshot_.autoReconnectEnabled = false;
    runtimeConfigSnapshot_.fastReconnectTtlMs = config_.fastReconnectTtlMs;
    runtimeConfigSnapshot_.ledMask = 0U;
    runtimeConfigSnapshot_.reportingMode = static_cast<uint8_t>(ReportingMode::CoreButtons);
    runtimeConfigSnapshot_.reportingContinuous = false;
}

void ESP32Wiimote::configure(const WiimoteConfig &config) {
    wifiEnabled_ = config.wifiEnabled;
    serialPrivilegedToken_ = config.serialPrivilegedToken;
    wifiApiToken_ = config.wifiApiToken;
    networkCredentials_ = config.network;

    if (serialPrivilegedToken_ == nullptr || serialPrivilegedToken_[0] == '\0') {
        LOG_ERROR("ESP32Wiimote: serialPrivilegedToken must be provided\n");
    }

    wifiTokenFallbackToSerial_ = false;
    if (wifiApiToken_ == nullptr || wifiApiToken_[0] == '\0') {
        wifiApiToken_ = serialPrivilegedToken_;
        wifiTokenFallbackToSerial_ = true;
    }

    wifiNetworkCredentialsConfigured_ =
        networkCredentials_.ssid != nullptr && networkCredentials_.ssid[0] != '\0' &&
        networkCredentials_.password != nullptr && networkCredentials_.password[0] != '\0';
    serialCommandSession_.setToken(serialPrivilegedToken_);

    if (!wifiEnabled_) {
        enableWifiControl(false, wifiDeliveryMode_);
    }
}

void ESP32Wiimote::enableWifiControl(bool enabled, WifiDeliveryMode deliveryMode) {
    const WifiDeliveryMode kPreviousDeliveryMode = wifiDeliveryMode_;
    wifiDeliveryMode_ = deliveryMode;

    if (!enabled || !wifiEnabled_) {
        wifiControlEnabled_ = false;
        resetWifiLifecycleState();
        return;
    }

    if (wifiControlEnabled_ && wifiControlReady_ && kPreviousDeliveryMode == deliveryMode) {
        return;
    }

    wifiControlEnabled_ = true;
    resetWifiLifecycleState();
    wifiControlInitializing_ = true;
}

bool ESP32Wiimote::isWifiControlEnabled() const {
    return wifiControlEnabled_;
}

bool ESP32Wiimote::isWifiControlReady() const {
    return wifiControlReady_;
}

ESP32Wiimote::WifiControlState ESP32Wiimote::getWifiControlState() const {
    WifiControlState state = {};
    state.enabled = wifiControlEnabled_;
    state.initializing = wifiControlInitializing_;
    state.ready = wifiControlReady_;
    state.networkCredentialsConfigured = wifiNetworkCredentialsConfigured_;
    state.networkConnectAttempted = wifiNetworkConnectAttempted_;
    state.networkConnected = wifiNetworkConnected_;
    state.networkConnectFailed = wifiNetworkConnectFailed_;
    state.deliveryMode = wifiDeliveryMode_;
    state.wifiLayerStarted = wifiLayerStarted_;
    state.littleFsMounted = littleFsMounted_;
    state.staticRoutesRegistered = staticRoutesRegistered_;
    state.apiRoutesRegistered = apiRoutesRegistered_;
    state.websocketRoutesRegistered = websocketRoutesRegistered_;
    return state;
}

/**
 * Initialize Bluetooth and HCI interface
 * Orchestrates initialization of all components
 * Returns true if initialization succeeded, false otherwise
 */
bool ESP32Wiimote::init() {
    LOG_INFO("ESP32Wiimote: Starting initialization...\n");

    // Initialize Bluetooth controller (which initializes TinyWiimote, queues, and VHCI callbacks)
    LOG_DEBUG("ESP32Wiimote: Calling BluetoothController::init()...\n");
    if (!BluetoothController::init(hciCallbacks_, queueManager_)) {
        LOG_ERROR("ESP32Wiimote: Bluetooth controller initialization failed!\n");
        return false;
    }

    tinyWiimoteSetFastReconnectTtlMs(config_.fastReconnectTtlMs);

    runtimeConfigSnapshot_.fastReconnectTtlMs = config_.fastReconnectTtlMs;
    runtimeConfigStoreReady_ = runtimeConfigStore_.init();
    if (runtimeConfigStoreReady_) {
        RuntimeConfigSnapshot persisted = {};
        if (runtimeConfigStore_.load(&persisted)) {
            runtimeConfigSnapshot_ = persisted;
            tinyWiimoteSetFastReconnectTtlMs(runtimeConfigSnapshot_.fastReconnectTtlMs);
            tinyWiimoteSetAutoReconnectEnabled(runtimeConfigSnapshot_.autoReconnectEnabled);
        } else {
            runtimeConfigStoreReady_ = runtimeConfigStore_.save(runtimeConfigSnapshot_);
        }
    }

    if (wifiTokenFallbackToSerial_) {
        Serial.println("@wm: info wifi_api_token_missing_using_serial_token");
    }

    LOG_INFO("ESP32Wiimote: Initialization complete!\n");
    return true;
}

/**
 * Process HCI tasks
 * Should be called regularly in the main loop
 */
void ESP32Wiimote::task() {
    if (!BluetoothController::isStarted()) {
        processWifiControl();
        return;
    }

    // Process pending HCI packets
    queueManager_->processTxQueue();
    queueManager_->processRxQueue();

    if (serialControlEnabled_) {
        processSerialControl();
    }

    processWifiControl();
}

/**
 * Check if new sensor/button data is available
 * Delegates to data parser
 */
int ESP32Wiimote::available() {
    return dataParser_->parseData();
}

/**
 * Get current button state
 */
ButtonState ESP32Wiimote::getButtonState() {
    return buttonState_->getCurrent();
}

/**
 * Get current accelerometer state
 */
struct AccelState ESP32Wiimote::getAccelState() {
    return sensorState_->getAccel();
}

/**
 * Get current nunchuk state
 */
struct NunchukState ESP32Wiimote::getNunchukState() {
    return sensorState_->getNunchuk();
}

/**
 * Check if Wiimote is connected
 */
bool ESP32Wiimote::isConnected() {
    return tinyWiimoteIsConnected();
}

/**
 * Get battery level
 */
uint8_t ESP32Wiimote::getBatteryLevel() {
    return tinyWiimoteGetBatteryLevel();
}

/**
 * Request battery status update
 */
void ESP32Wiimote::requestBatteryUpdate() {
    tinyWiimoteRequestBatteryUpdate();
}

bool ESP32Wiimote::setLeds(uint8_t ledMask) {
    const bool kAccepted = tinyWiimoteSetLeds(ledMask);
    if (kAccepted) {
        runtimeConfigSnapshot_.ledMask = ledMask;
        persistRuntimeConfigSnapshot();
    }
    return kAccepted;
}

bool ESP32Wiimote::setReportingMode(ReportingMode mode, bool continuous) {
    const bool kAccepted = tinyWiimoteSetReportingMode(static_cast<uint8_t>(mode), continuous);
    if (kAccepted) {
        runtimeConfigSnapshot_.reportingMode = static_cast<uint8_t>(mode);
        runtimeConfigSnapshot_.reportingContinuous = continuous;
        persistRuntimeConfigSnapshot();
    }
    return kAccepted;
}

bool ESP32Wiimote::setAccelerometerEnabled(bool enabled) {
    tinyWiimoteReqAccelerometer(enabled);
    return true;
}

bool ESP32Wiimote::requestStatus() {
    return tinyWiimoteRequestStatus();
}

bool ESP32Wiimote::writeMemory(uint8_t addressSpace,
                               uint32_t offset,
                               const uint8_t *data,
                               uint8_t len) {
    return tinyWiimoteWriteMemory(addressSpace, offset, data, len);
}

bool ESP32Wiimote::readMemory(uint8_t addressSpace, uint32_t offset, uint16_t size) {
    return tinyWiimoteReadMemory(addressSpace, offset, size);
}

void ESP32Wiimote::setScanEnabled(bool enabled) {
    tinyWiimoteSetScanEnabled(enabled);
}

bool ESP32Wiimote::startDiscovery() {
    return tinyWiimoteStartDiscovery();
}

bool ESP32Wiimote::stopDiscovery() {
    return tinyWiimoteStopDiscovery();
}

bool ESP32Wiimote::disconnectActiveController(DisconnectReason reason) {
    return tinyWiimoteDisconnect(static_cast<uint8_t>(reason));
}

void ESP32Wiimote::setAutoReconnectEnabled(bool enabled) {
    tinyWiimoteSetAutoReconnectEnabled(enabled);
    runtimeConfigSnapshot_.autoReconnectEnabled = enabled;
    persistRuntimeConfigSnapshot();
}

void ESP32Wiimote::clearReconnectCache() {
    tinyWiimoteClearReconnectCache();
}

void ESP32Wiimote::enableSerialControl(bool enabled) {
    serialControlEnabled_ = enabled;

    if (!enabled) {
        serialCommandSession_.lock();
        serialInputLen_ = 0U;
        serialInputOverflow_ = false;
        serialInputLine_[0] = '\0';
    }
}

bool ESP32Wiimote::isSerialControlEnabled() const {
    return serialControlEnabled_;
}

ESP32Wiimote::BluetoothControllerState ESP32Wiimote::getBluetoothControllerState() {
    const ::BluetoothControllerState kState = tinyWiimoteGetControllerState();
    BluetoothControllerState apiState = {};
    apiState.initialized = kState.initialized;
    apiState.started = kState.started;
    apiState.scanning = kState.scanning;
    apiState.connected = kState.connected;
    apiState.activeConnectionHandle = kState.activeConnectionHandle;
    apiState.fastReconnectActive = kState.fastReconnectActive;
    apiState.autoReconnectEnabled = kState.autoReconnectEnabled;
    return apiState;
}

void ESP32Wiimote::setLogLevel(uint8_t level) {
    wiimoteSetLogLevel(level);
}

uint8_t ESP32Wiimote::getLogLevel() {
    return wiimoteGetLogLevel();
}
/**
 * Add filter to ignore certain data types
 */
void ESP32Wiimote::addFilter(FilterAction action, int filter) {
    if (action == FilterAction::Ignore) {
        dataParser_->setFilter(dataParser_->getFilter() | filter);

        if ((filter & kFilterAccel) != 0) {
            tinyWiimoteReqAccelerometer(false);
        }
    }
}

void ESP32Wiimote::processSerialControl() {
    while (Serial.available() > 0) {
        const int kRaw = Serial.read();
        if (kRaw < 0) {
            return;
        }

        const char kCh = static_cast<char>(kRaw);
        if (kCh == '\r') {
            continue;
        }

        if (kCh == '\n') {
            if (serialInputOverflow_) {
                char response[kSerialResponseBufSize] = {0};
                serialFormatParseResult(response, sizeof(response), SerialParseResult::LineTooLong);
                Serial.println(response);
            } else if (serialInputLen_ > 0U) {
                serialInputLine_[serialInputLen_] = '\0';
                processSerialCommandLine(serialInputLine_);
            }

            serialInputLen_ = 0U;
            serialInputOverflow_ = false;
            serialInputLine_[0] = '\0';

            // Bounded loop: process at most one completed line per task() call.
            return;
        }

        if (serialInputOverflow_) {
            continue;
        }

        if (serialInputLen_ < kSerialMaxLineLength) {
            serialInputLine_[serialInputLen_] = kCh;
            serialInputLen_++;
        } else {
            serialInputOverflow_ = true;
        }
    }
}

void ESP32Wiimote::processSerialCommandLine(const char *line) {
    SerialParsedCommand parsed = {};
    const SerialParseResult kParseResult = serialCommandParse(line, &parsed);

    if (kParseResult == SerialParseResult::NotACommand ||
        kParseResult == SerialParseResult::EmptyLine) {
        return;
    }

    char response[kSerialResponseBufSize] = {0};

    if (kParseResult != SerialParseResult::Ok) {
        serialFormatParseResult(response, sizeof(response), kParseResult);
        Serial.println(response);
        return;
    }

    Esp32SerialCommandTarget target(this);
    SerialDispatchOptions options = {};
    options.session = &serialCommandSession_;
    options.privilegedCommandsRequireUnlock = serialPrivilegedCommandsRequireUnlock_;
    options.nowMs = static_cast<uint32_t>(millis());

    const SerialDispatchResult kDispatchResult = serialCommandDispatch(parsed, &target, options);
    serialFormatDispatchResult(response, sizeof(response), kDispatchResult);
    Serial.println(response);
}

void ESP32Wiimote::processWifiControl() {
    if (!wifiControlEnabled_ || !wifiEnabled_) {
        return;
    }

    if (wifiControlReady_) {
        return;
    }

    if (!wifiControlInitializing_) {
        wifiControlInitializing_ = true;
    }

    switch (wifiInitStage_) {
        case KWifiInitStageStartWifi:
            wifiNetworkConnectAttempted_ = true;
            if (!wifiNetworkCredentialsConfigured_) {
                wifiNetworkConnectFailed_ = true;
                wifiControlEnabled_ = false;
                wifiControlInitializing_ = false;
                return;
            }

            if (strcmp(networkCredentials_.ssid, kWifiMockFailSsid) == 0) {
                wifiNetworkConnectFailed_ = true;
                wifiControlEnabled_ = false;
                wifiControlInitializing_ = false;
                return;
            }

            wifiNetworkConnected_ = true;
            wifiNetworkConnectFailed_ = false;
            wifiLayerStarted_ = true;
            wifiInitStage_ = KWifiInitStageMountLittleFs;
            break;
        case KWifiInitStageMountLittleFs:
            littleFsMounted_ = true;
            wifiInitStage_ = KWifiInitStageRegisterStaticRoutes;
            break;
        case KWifiInitStageRegisterStaticRoutes:
            staticRoutesRegistered_ = true;
            wifiInitStage_ = KWifiInitStageRegisterApiRoutes;
            break;
        case KWifiInitStageRegisterApiRoutes:
            apiRoutesRegistered_ = true;
            if (wifiDeliveryMode_ == WifiDeliveryMode::RestAndWebSocket) {
                wifiInitStage_ = KWifiInitStageRegisterWebSocketRoutes;
            } else {
                wifiInitStage_ = KWifiInitStageReady;
            }
            break;
        case KWifiInitStageRegisterWebSocketRoutes:
            websocketRoutesRegistered_ = true;
            wifiInitStage_ = KWifiInitStageReady;
            break;
        case KWifiInitStageReady:
            wifiControlInitializing_ = false;
            wifiControlReady_ = true;
            break;
        default:
            wifiControlEnabled_ = false;
            resetWifiLifecycleState();
            break;
    }
}

void ESP32Wiimote::resetWifiLifecycleState() {
    wifiControlInitializing_ = false;
    wifiControlReady_ = false;
    wifiNetworkConnectAttempted_ = false;
    wifiNetworkConnected_ = false;
    wifiNetworkConnectFailed_ = false;
    wifiInitStage_ = KWifiInitStageStartWifi;
    wifiLayerStarted_ = false;
    littleFsMounted_ = false;
    staticRoutesRegistered_ = false;
    apiRoutesRegistered_ = false;
    websocketRoutesRegistered_ = false;
}

void ESP32Wiimote::persistRuntimeConfigSnapshot() {
    if (!runtimeConfigStoreReady_) {
        return;
    }

    if (!runtimeConfigStore_.save(runtimeConfigSnapshot_)) {
        runtimeConfigStoreReady_ = false;
    }
}
