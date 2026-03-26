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
#include "wifi/web_api_router.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr size_t kSerialResponseBufSize = 192U;
constexpr uint16_t kWifiHttpPort = 80U;
constexpr const char *kWifiMockFailSsid = "__fail__";
constexpr uint32_t kWifiConnectTimeoutMs = 15000U;

enum WifiInitStage : uint8_t {
    KWifiInitStageStartWifi = 0U,
    KWifiInitStageWaitNetworkConnected = 1U,
    KWifiInitStageRegisterStaticRoutes = 2U,
    KWifiInitStageRegisterApiRoutes = 3U,
    KWifiInitStageRegisterWebSocketRoutes = 4U,
    KWifiInitStageReady = 5U,
};

const char *wifiDeliveryModeToString(const WifiDeliveryMode kMode) {
    return kMode == WifiDeliveryMode::RestAndWebSocket ? "RestAndWebSocket" : "RestOnly";
}

const char *safeSsid(const WiimoteNetworkCredentials &network) {
    return (network.ssid != nullptr && network.ssid[0] != '\0') ? network.ssid : "<unset>";
}

const char *copyOwnedString(char *dst, size_t dstSize, const char *src) {
    if (dst == nullptr || dstSize == 0U) {
        return nullptr;
    }

    dst[0] = '\0';
    if (src == nullptr || src[0] == '\0') {
        return nullptr;
    }

    snprintf(dst, dstSize, "%s", src);
    return dst;
}

void logWifiConnectedDetails(const WiimoteNetworkCredentials &network,
                             const WifiDeliveryMode kMode) {
#if defined(ARDUINO_ARCH_ESP32)
    String mac = WiFi.macAddress();
    if (mac.length() == 0) {
        mac = "n/a";
    }

    String ip = WiFi.localIP().toString();
    if (ip == "0.0.0.0") {
        ip = "not-assigned";
    }

    wiimoteLogInfo("ESP32Wiimote: wifi.connected ssid=%s mode=%s mac=%s ip=%s\n", safeSsid(network),
             wifiDeliveryModeToString(kMode), mac.c_str(), ip.c_str());
#else
    wiimoteLogInfo("ESP32Wiimote: wifi.connected ssid=%s mode=%s\n", safeSsid(network),
             wifiDeliveryModeToString(kMode));
#endif
}

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

    bool setWifiControlEnabled(bool enabled) override {
        device_->enableWifiControl(enabled, device_->getConfig().wifi.deliveryMode);
        return device_->isWifiControlEnabled() == enabled;
    }

    bool setWifiDeliveryMode(uint8_t mode) override {
        if (mode > static_cast<uint8_t>(WifiDeliveryMode::RestAndWebSocket)) {
            return false;
        }
        return device_->setWifiDeliveryMode(static_cast<WifiDeliveryMode>(mode));
    }

    bool setWifiNetworkCredentials(const char *ssid, const char *password) override {
        return device_->updateWifiNetworkCredentials(ssid, password);
    }

    bool restartWifiControl() override { return device_->restartWifiControl(); }

    bool setWifiApiToken(const char *token) override { return device_->updateWifiApiToken(token); }

    bool isWifiApiTokenMutationAllowed() const override { return false; }

    bool isConnected() const override { return ESP32Wiimote::isConnected(); }

    uint8_t getBatteryLevel() const override { return ESP32Wiimote::getBatteryLevel(); }

    bool isControllerInitialized() const override {
        return device_->getBluetoothControllerState().started;
    }

    bool isControllerScanning() const override {
        return device_->getBluetoothControllerState().scanning;
    }

    bool isDiscoveryActive() const override {
        const auto kState = device_->getBluetoothControllerState();
        return kState.started && kState.scanning;
    }

    uint16_t getActiveConnectionHandle() const override {
        return device_->getBluetoothControllerState().activeConnectionHandle;
    }

   private:
    ESP32Wiimote *device_;
};

WebWiimoteStatusSnapshot webGetWiimoteStatus(void *userData) {
    ESP32Wiimote *device = static_cast<ESP32Wiimote *>(userData);
    WebWiimoteStatusSnapshot snapshot = {};
    snapshot.connected = ESP32Wiimote::isConnected();
    snapshot.batteryLevel = ESP32Wiimote::getBatteryLevel();
    return snapshot;
}

WebControllerStatusSnapshot webGetControllerStatus(void *userData) {
    ESP32Wiimote *device = static_cast<ESP32Wiimote *>(userData);
    const ESP32Wiimote::BluetoothControllerState kState = device->getBluetoothControllerState();

    WebControllerStatusSnapshot snapshot = {};
    snapshot.initialized = kState.initialized;
    snapshot.started = kState.started;
    snapshot.scanning = kState.scanning;
    snapshot.connected = kState.connected;
    snapshot.activeConnectionHandle = kState.activeConnectionHandle;
    snapshot.fastReconnectActive = kState.fastReconnectActive;
    snapshot.autoReconnectEnabled = kState.autoReconnectEnabled;
    return snapshot;
}

WebConfigSnapshot webGetConfig(void *userData) {
    ESP32Wiimote *device = static_cast<ESP32Wiimote *>(userData);
    const ESP32WiimoteConfig &config = device->getConfig();

    WebConfigSnapshot snapshot = {};
    snapshot.nunchukStickThreshold = config.nunchukStickThreshold;
    snapshot.txQueueSize = config.txQueueSize;
    snapshot.rxQueueSize = config.rxQueueSize;
    snapshot.fastReconnectTtlMs = config.fastReconnectTtlMs;
    return snapshot;
}

WebWifiControlStateSnapshot webGetWifiControlState(void *userData) {
    ESP32Wiimote *device = static_cast<ESP32Wiimote *>(userData);
    const ESP32Wiimote::WifiControlState kState = device->getWifiControlState();

    WebWifiControlStateSnapshot snapshot = {};
    snapshot.enabled = kState.enabled;
    snapshot.ready = kState.ready;
    snapshot.networkCredentialsConfigured = kState.networkCredentialsConfigured;
    snapshot.networkConnectAttempted = kState.networkConnectAttempted;
    snapshot.networkConnected = kState.networkConnected;
    snapshot.networkConnectFailed = kState.networkConnectFailed;
    snapshot.restAndWebSocket = kState.deliveryMode == WifiDeliveryMode::RestAndWebSocket;
    snapshot.serverStarted = kState.serverStarted;
    snapshot.serverBindFailed = kState.serverBindFailed;
    snapshot.hasToken = device->hasWifiApiToken();
    return snapshot;
}

bool webSetLeds(uint8_t ledMask, void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->setLeds(ledMask);
}

bool webSetReportingMode(uint8_t mode, bool continuous, void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->setReportingMode(static_cast<ReportingMode>(mode),
                                                                   continuous);
}

bool webSetAccelEnabled(bool enabled, void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->setAccelerometerEnabled(enabled);
}

bool webRequestStatus(void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->requestStatus();
}

void webSetScanEnabled(bool enabled, void *userData) {
    static_cast<ESP32Wiimote *>(userData)->setScanEnabled(enabled);
}

bool webStartDiscovery(void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->startDiscovery();
}

bool webStopDiscovery(void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->stopDiscovery();
}

bool webDisconnect(uint8_t reason, void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->disconnectActiveController(
        static_cast<ESP32Wiimote::DisconnectReason>(reason));
}

void webSetAutoReconnect(bool enabled, void *userData) {
    static_cast<ESP32Wiimote *>(userData)->setAutoReconnectEnabled(enabled);
}

bool webSetWifiControlEnabled(bool enabled, void *userData) {
    ESP32Wiimote *device = static_cast<ESP32Wiimote *>(userData);
    device->enableWifiControl(enabled, device->getConfig().wifi.deliveryMode);
    return device->isWifiControlEnabled() == enabled;
}

bool webSetWifiDeliveryMode(bool restAndWebSocket, void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->setWifiDeliveryMode(
        restAndWebSocket ? WifiDeliveryMode::RestAndWebSocket : WifiDeliveryMode::RestOnly);
}

bool webSetWifiNetwork(const char *ssid, const char *password, void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->updateWifiNetworkCredentials(ssid, password);
}

bool webRestartWifiControl(void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->restartWifiControl();
}

bool webSetWifiApiToken(const char *token, void *userData) {
    return static_cast<ESP32Wiimote *>(userData)->updateWifiApiToken(token);
}

const char *wifiHttpMethodToString(const WifiHttpMethod kMethod) {
    switch (kMethod) {
        case WifiHttpMethod::Get:
            return "GET";
        case WifiHttpMethod::Post:
            return "POST";
        default:
            return "UNKNOWN";
    }
}

WebApiContext makeWebApiContext(ESP32Wiimote *device) {
    WebApiContext context = {};
    context.wifiApiToken = device->getConfig().auth.wifiApiToken;
    context.getWiimoteStatus = webGetWiimoteStatus;
    context.getControllerStatus = webGetControllerStatus;
    context.getConfig = webGetConfig;
    context.getWifiControlState = webGetWifiControlState;
    context.setLeds = webSetLeds;
    context.setReportingMode = webSetReportingMode;
    context.setAccelEnabled = webSetAccelEnabled;
    context.requestStatus = webRequestStatus;
    context.setScanEnabled = webSetScanEnabled;
    context.startDiscovery = webStartDiscovery;
    context.stopDiscovery = webStopDiscovery;
    context.disconnect = webDisconnect;
    context.setAutoReconnect = webSetAutoReconnect;
    context.setWifiControlEnabled = webSetWifiControlEnabled;
    context.setWifiDeliveryMode = webSetWifiDeliveryMode;
    context.setWifiNetwork = webSetWifiNetwork;
    context.restartWifiControl = webRestartWifiControl;
    context.setWifiApiToken = webSetWifiApiToken;
    context.allowWifiApiTokenMutation = false;
    context.userData = device;
    return context;
}

void handleWifiHttpRequest(const WifiHttpRequest *request,
                           char *responseBuf,
                           size_t responseBufSize,
                           WifiHttpResponse *response,
                           void *userData) {
    if (request == nullptr || responseBuf == nullptr || responseBufSize == 0U ||
        response == nullptr || userData == nullptr) {
        return;
    }

    ESP32Wiimote *device = static_cast<ESP32Wiimote *>(userData);
    WebApiContext context = makeWebApiContext(device);
    const WebApiRouteResult kRouteResult = webApiRoute(
        &context, wifiHttpMethodToString(request->method), request->path, request->authHeader,
        request->body, request->bodyLen, responseBuf, responseBufSize);

    if (kRouteResult.httpStatus == 101) {
        serializeError(responseBuf, responseBufSize, "websocket upgrade not implemented");
        response->status = 501;
        response->contentType = "application/json";
        return;
    }

    response->status = kRouteResult.httpStatus;
    response->contentType = kRouteResult.contentType;
}

}  // namespace

/**
 * Constructor - Initialize all component managers
 */
ESP32Wiimote::ESP32Wiimote() : ESP32Wiimote(ESP32WiimoteConfig(), nullptr) {}

ESP32Wiimote::ESP32Wiimote(const ESP32WiimoteConfig &config) : ESP32Wiimote(config, nullptr) {}

ESP32Wiimote::ESP32Wiimote(const ESP32WiimoteConfig &config, WifiHttpServer *httpServer)
    : config_(config)
    , serialPrivilegedToken_(nullptr)
    , wifiApiToken_(nullptr)
    , wifiEnabled_(false)
    , wifiTokenFallbackToSerial_(false)
    , serialPrivilegedTokenStorage_{0}
    , wifiApiTokenStorage_{0}
    , wifiSsidStorage_{0}
    , wifiPasswordStorage_{0}
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
    , wifiNetworkConnectStartMs_(0U)
    , wifiDeliveryMode_(WifiDeliveryMode::RestOnly)
    , wifiInitStage_(KWifiInitStageStartWifi)
    , wifiLayerStarted_(false)
    , staticRoutesRegistered_(false)
    , apiRoutesRegistered_(false)
    , websocketRoutesRegistered_(false)
    , serverStarted_(false)
    , serverBindFailed_(false)
    , httpServer_(httpServer != nullptr ? httpServer : &ownedHttpServer_) {
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

    applyRuntimeConfig(config_, false);
}

void ESP32Wiimote::configure(const ESP32WiimoteConfig &config) {
    applyRuntimeConfig(config, true);
}

const ESP32WiimoteConfig &ESP32Wiimote::getConfig() const {
    return config_;
}

void ESP32Wiimote::applyRuntimeConfig(const ESP32WiimoteConfig &config, bool logValidationErrors) {
    const bool kHasSerialToken = config.auth.serialPrivilegedToken != nullptr &&
                                 config.auth.serialPrivilegedToken[0] != '\0';
    const bool kHasWifiApiToken =
        config.auth.wifiApiToken != nullptr && config.auth.wifiApiToken[0] != '\0';
    const bool kHasWifiNetwork =
        config.wifi.network.ssid != nullptr && config.wifi.network.ssid[0] != '\0' &&
        config.wifi.network.password != nullptr && config.wifi.network.password[0] != '\0';

    if (kHasSerialToken) {
        config_.auth.serialPrivilegedToken =
            copyOwnedString(serialPrivilegedTokenStorage_, sizeof(serialPrivilegedTokenStorage_),
                            config.auth.serialPrivilegedToken);
    }

    if (kHasWifiApiToken) {
        config_.auth.wifiApiToken = copyOwnedString(
            wifiApiTokenStorage_, sizeof(wifiApiTokenStorage_), config.auth.wifiApiToken);
    }

    if (kHasSerialToken || kHasWifiApiToken || !config.auth.wifiTokenFallbackToSerial) {
        config_.auth.wifiTokenFallbackToSerial = config.auth.wifiTokenFallbackToSerial;
    }

    if (config.wifi.enabled) {
        config_.wifi.enabled = true;
    }

    if (config.wifi.deliveryMode == WifiDeliveryMode::RestAndWebSocket) {
        config_.wifi.deliveryMode = WifiDeliveryMode::RestAndWebSocket;
    }

    if (kHasWifiNetwork) {
        config_.wifi.network = WiimoteNetworkCredentials(
            copyOwnedString(wifiSsidStorage_, sizeof(wifiSsidStorage_), config.wifi.network.ssid),
            copyOwnedString(wifiPasswordStorage_, sizeof(wifiPasswordStorage_),
                            config.wifi.network.password));
    }

    wifiEnabled_ = config_.wifi.enabled;
    serialPrivilegedToken_ = config_.auth.serialPrivilegedToken;
    wifiApiToken_ = config_.auth.wifiApiToken;
    networkCredentials_ = config_.wifi.network;

    wifiDeliveryMode_ = config_.wifi.deliveryMode;

    if (logValidationErrors &&
        (serialPrivilegedToken_ == nullptr || serialPrivilegedToken_[0] == '\0')) {
        wiimoteLogError("ESP32Wiimote: serialPrivilegedToken must be provided\n");
    }

    wifiTokenFallbackToSerial_ = false;
    if (config_.auth.wifiTokenFallbackToSerial &&
        (wifiApiToken_ == nullptr || wifiApiToken_[0] == '\0')) {
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

    if (!enabled) {
        wifiControlEnabled_ = false;
        resetWifiLifecycleState();
        return;
    }

    if (!wifiEnabled_) {
        wifiEnabled_ = true;
        config_.wifi.enabled = true;
    }

    if (wifiControlEnabled_ && wifiControlReady_ && kPreviousDeliveryMode == deliveryMode) {
        return;
    }

    wifiControlEnabled_ = true;
    resetWifiLifecycleState();
    wifiControlInitializing_ = true;
}

bool ESP32Wiimote::updateWifiNetworkCredentials(const char *ssid, const char *password) {
    if (ssid == nullptr || ssid[0] == '\0' || password == nullptr || password[0] == '\0') {
        return false;
    }

    config_.wifi.network = WiimoteNetworkCredentials(
        copyOwnedString(wifiSsidStorage_, sizeof(wifiSsidStorage_), ssid),
        copyOwnedString(wifiPasswordStorage_, sizeof(wifiPasswordStorage_), password));
    networkCredentials_ = config_.wifi.network;
    wifiNetworkCredentialsConfigured_ = true;

    if (wifiControlEnabled_) {
        resetWifiLifecycleState();
        wifiControlInitializing_ = true;
    }

    return true;
}

bool ESP32Wiimote::restartWifiControl() {
    if (!wifiEnabled_ || !wifiControlEnabled_) {
        return false;
    }

    resetWifiLifecycleState();
    wifiControlInitializing_ = true;
    return true;
}

bool ESP32Wiimote::updateWifiApiToken(const char *token) {
    if (token == nullptr || token[0] == '\0') {
        return false;
    }

    config_.auth.wifiApiToken =
        copyOwnedString(wifiApiTokenStorage_, sizeof(wifiApiTokenStorage_), token);
    wifiApiToken_ = config_.auth.wifiApiToken;
    wifiTokenFallbackToSerial_ = false;
    return true;
}

bool ESP32Wiimote::setWifiDeliveryMode(WifiDeliveryMode deliveryMode) {
    config_.wifi.deliveryMode = deliveryMode;
    wifiDeliveryMode_ = deliveryMode;

    if (wifiControlEnabled_) {
        resetWifiLifecycleState();
        wifiControlInitializing_ = true;
    }

    return true;
}

bool ESP32Wiimote::isWifiControlEnabled() const {
    return wifiControlEnabled_;
}

bool ESP32Wiimote::isWifiControlReady() const {
    return wifiControlReady_;
}

bool ESP32Wiimote::hasWifiApiToken() const {
    return wifiApiToken_ != nullptr && wifiApiToken_[0] != '\0';
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
    state.staticRoutesRegistered = staticRoutesRegistered_;
    state.apiRoutesRegistered = apiRoutesRegistered_;
    state.websocketRoutesRegistered = websocketRoutesRegistered_;
    state.serverStarted = serverStarted_;
    state.serverBindFailed = serverBindFailed_;
    return state;
}

/**
 * Initialize Bluetooth and HCI interface
 * Orchestrates initialization of all components
 * Returns true if initialization succeeded, false otherwise
 */
bool ESP32Wiimote::init() {
    wiimoteLogInfo("ESP32Wiimote: Starting initialization...\n");
    wiimoteLogInfo("ESP32Wiimote: wifi.config enabled=%d mode=%s ssid=%s\n",
             static_cast<int>(wifiEnabled_), wifiDeliveryModeToString(wifiDeliveryMode_),
             safeSsid(networkCredentials_));
    if (!wifiEnabled_) {
        wiimoteLogInfo("ESP32Wiimote: wifi.startup skipped (config.wifi.enabled=0)\n");
    }

    // Initialize Bluetooth controller (which initializes TinyWiimote, queues, and VHCI callbacks)
    wiimoteLogDebug("ESP32Wiimote: Calling BluetoothController::init()...\n");
    if (!BluetoothController::init(hciCallbacks_, queueManager_)) {
        wiimoteLogError("ESP32Wiimote: Bluetooth controller initialization failed!\n");
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

    wiimoteLogInfo("ESP32Wiimote: Initialization complete!\n");
    return true;
}

/**
 * Process HCI tasks
 * Should be called regularly in the main loop
 */
void ESP32Wiimote::task() {
    if (!BluetoothController::isStarted()) {
        processWifiControl();
        httpServer_->poll();
        return;
    }

    // Process pending HCI packets
    queueManager_->processTxQueue();
    queueManager_->processRxQueue();

    if (serialControlEnabled_) {
        processSerialControl();
    }

    processWifiControl();
    httpServer_->poll();
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

    if (parsed.tokenCount >= 2U && strcmp(parsed.tokens[1], "wifi-status") == 0) {
        const WifiControlState kWifiState = getWifiControlState();
        serialFormatWifiStatus(response, sizeof(response), kWifiState.enabled, kWifiState.ready,
                               kWifiState.networkConnected, kWifiState.networkConnectFailed,
                               kWifiState.deliveryMode == WifiDeliveryMode::RestAndWebSocket);
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
                wiimoteLogWarn("ESP32Wiimote: wifi.startup failed (missing credentials)\n");
                wifiNetworkConnectFailed_ = true;
                wifiControlEnabled_ = false;
                wifiControlInitializing_ = false;
                return;
            }

            if (strcmp(networkCredentials_.ssid, kWifiMockFailSsid) == 0) {
                wiimoteLogWarn("ESP32Wiimote: wifi.startup failed (mock join failure) ssid=%s\n",
                         safeSsid(networkCredentials_));
                wifiNetworkConnectFailed_ = true;
                wifiControlEnabled_ = false;
                wifiControlInitializing_ = false;
                return;
            }

#if defined(ARDUINO_ARCH_ESP32)
            WiFi.mode(WIFI_STA);
            WiFi.begin(networkCredentials_.ssid, networkCredentials_.password);
            wifiNetworkConnectStartMs_ = static_cast<uint32_t>(millis());
            wifiNetworkConnected_ = false;
            wifiNetworkConnectFailed_ = false;
            wifiLayerStarted_ = true;
            wiimoteLogInfo("ESP32Wiimote: wifi.startup connecting ssid=%s mode=%s timeout_ms=%lu\n",
                     safeSsid(networkCredentials_), wifiDeliveryModeToString(wifiDeliveryMode_),
                     static_cast<unsigned long>(kWifiConnectTimeoutMs));
            wifiInitStage_ = KWifiInitStageWaitNetworkConnected;
#else
            wifiNetworkConnected_ = true;
            wifiNetworkConnectFailed_ = false;
            wifiLayerStarted_ = true;
            logWifiConnectedDetails(networkCredentials_, wifiDeliveryMode_);
            wifiInitStage_ = KWifiInitStageRegisterStaticRoutes;
#endif
            break;
        case KWifiInitStageWaitNetworkConnected:
#if defined(ARDUINO_ARCH_ESP32)
            if (WiFi.status() == WL_CONNECTED) {
                wifiNetworkConnected_ = true;
                wifiNetworkConnectFailed_ = false;
                logWifiConnectedDetails(networkCredentials_, wifiDeliveryMode_);
                wifiInitStage_ = KWifiInitStageRegisterStaticRoutes;
                break;
            }

            if (static_cast<uint32_t>(millis()) - wifiNetworkConnectStartMs_ >=
                kWifiConnectTimeoutMs) {
                wiimoteLogWarn("ESP32Wiimote: wifi.startup failed (timeout) ssid=%s status=%d\n",
                         safeSsid(networkCredentials_), static_cast<int>(WiFi.status()));
                wifiNetworkConnectFailed_ = true;
                wifiControlEnabled_ = false;
                wifiControlInitializing_ = false;
                return;
            }
#endif
            break;
        case KWifiInitStageRegisterStaticRoutes:
            staticRoutesRegistered_ = true;
            wifiInitStage_ = KWifiInitStageRegisterApiRoutes;
            break;
        case KWifiInitStageRegisterApiRoutes:
            apiRoutesRegistered_ = true;
            if (!startWifiHttpServer()) {
                wifiControlEnabled_ = false;
                wifiControlInitializing_ = false;
                return;
            }
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
            wiimoteLogInfo("ESP32Wiimote: wifi.startup ready static=%d api=%d ws=%d\n",
                     static_cast<int>(staticRoutesRegistered_),
                     static_cast<int>(apiRoutesRegistered_),
                     static_cast<int>(websocketRoutesRegistered_));
            break;
        default:
            wifiControlEnabled_ = false;
            resetWifiLifecycleState();
            break;
    }
}

void ESP32Wiimote::resetWifiLifecycleState() {
    stopWifiHttpServer();
    wifiControlInitializing_ = false;
    wifiControlReady_ = false;
    wifiNetworkConnectAttempted_ = false;
    wifiNetworkConnected_ = false;
    wifiNetworkConnectFailed_ = false;
    wifiNetworkConnectStartMs_ = 0U;
    wifiInitStage_ = KWifiInitStageStartWifi;
    wifiLayerStarted_ = false;
    staticRoutesRegistered_ = false;
    apiRoutesRegistered_ = false;
    websocketRoutesRegistered_ = false;
    serverStarted_ = false;
    serverBindFailed_ = false;
}

bool ESP32Wiimote::startWifiHttpServer() {
    if (serverStarted_) {
        return true;
    }

    wiimoteLogInfo("ESP32Wiimote: wifi.http starting port=%u\n",
             static_cast<unsigned int>(kWifiHttpPort));
    httpServer_->setHandler(handleWifiHttpRequest, this);
    if (!httpServer_->begin(kWifiHttpPort)) {
        serverBindFailed_ = true;
        serverStarted_ = false;
        const unsigned int kErrorCode = static_cast<unsigned int>(httpServer_->lastStartError());
        wiimoteLogWarn("ESP32Wiimote: wifi.http start failed port=%u error_code=%u\n",
                 static_cast<unsigned int>(kWifiHttpPort), kErrorCode);
        return false;
    }

    serverStarted_ = true;
    serverBindFailed_ = false;
    wiimoteLogInfo("ESP32Wiimote: wifi.http started port=%u\n", static_cast<unsigned int>(kWifiHttpPort));
    return true;
}

void ESP32Wiimote::stopWifiHttpServer() {
    if (!httpServer_->isStarted()) {
        return;
    }

    httpServer_->end();
    if (serverStarted_) {
        wiimoteLogInfo("ESP32Wiimote: wifi.http stopped port=%u\n",
                 static_cast<unsigned int>(kWifiHttpPort));
    }
    serverStarted_ = false;
}

void ESP32Wiimote::persistRuntimeConfigSnapshot() {
    if (!runtimeConfigStoreReady_) {
        return;
    }

    if (!runtimeConfigStore_.save(runtimeConfigSnapshot_)) {
        runtimeConfigStoreReady_ = false;
    }
}
