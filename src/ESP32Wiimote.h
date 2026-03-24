// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_ES_P32_WIIMOTE_H
#define ESP32_WIIMOTE_ES_P32_WIIMOTE_H

#include "TinyWiimote.h"
#include "config/runtime_config_store.h"
#include "esp32wiimote/bt_controller.h"
#include "esp32wiimote/data_parser.h"
#include "esp32wiimote/hci_callbacks.h"
#include "esp32wiimote/queue/hci_queue.h"
#include "esp32wiimote/state/button_state.h"
#include "esp32wiimote/state/sensor_state.h"
#include "serial/serial_command_parser.h"
#include "serial/serial_command_session.h"
#include "utils/serial_logging.h"
#include "wiimote_config.h"

/**
 * Action types for filters
 */
enum class FilterAction : uint8_t {
    Ignore = 0,
};

struct ESP32WiimoteConfig {
    int nunchukStickThreshold = 1;
    int txQueueSize = 32;
    int rxQueueSize = 32;
    uint32_t fastReconnectTtlMs = 3UL * 60UL * 1000UL;
};

enum class ReportingMode : uint8_t {
    CoreButtons = 0x30,
    CoreButtonsAccel = 0x31,
    CoreButtonsAccelIr = 0x33,
    CoreButtonsAccelExt = 0x35,
};

enum class WifiDeliveryMode : uint8_t {
    RestOnly = 0,
    RestAndWebSocket = 1,
};

/**
 * ESP32Wiimote - Main Wiimote controller interface for ESP32
 * Provides high-level API for Wiimote button, sensor, and nunchuk data
 *
 * Delegates responsibilities to:
 * - BluetoothController: BT initialization
 * - HciCallbacksHandler: HCI callbacks
 * - HciQueueManager: Packet queuing
 * - ButtonStateManager: Button state tracking
 * - SensorStateManager: Sensor data tracking
 * - WiimoteDataParser: Data parsing
 */
class ESP32Wiimote {
   public:
    enum class DisconnectReason : uint8_t {
        LocalHostTerminated = 0x16,
        RemoteUserTerminated = 0x13,
        AuthenticationFailure = 0x05,
        PowerOff = 0x15,
    };

    struct BluetoothControllerState {
        bool initialized;
        bool started;
        bool scanning;
        bool connected;
        uint16_t activeConnectionHandle;
        bool fastReconnectActive;
        bool autoReconnectEnabled;
    };

    struct WifiControlState {
        bool enabled;
        bool initializing;
        bool ready;
        bool networkCredentialsConfigured;
        bool networkConnectAttempted;
        bool networkConnected;
        bool networkConnectFailed;
        WifiDeliveryMode deliveryMode;
        bool wifiLayerStarted;
        bool littleFsMounted;
        bool staticRoutesRegistered;
        bool apiRoutesRegistered;
        bool websocketRoutesRegistered;
    };

    /**
     * Create ESP32Wiimote instance with default configuration
     */
    ESP32Wiimote();

    /**
     * Create ESP32Wiimote instance with explicit config
     */
    explicit ESP32Wiimote(const ESP32WiimoteConfig &config);

    /**
     * Configure runtime credentials and Wi-Fi enablement policy.
     * This configuration is independent from initialization and can be
     * applied before init() to define auth behavior.
     */
    void configure(const WiimoteConfig &config);

    /**
     * Enable or disable Wi-Fi control lifecycle.
     * Lifecycle startup is asynchronous and progresses during task() calls.
     */
    void enableWifiControl(bool enabled,
                           WifiDeliveryMode deliveryMode = WifiDeliveryMode::RestOnly);

    /**
     * Returns true when Wi-Fi control lifecycle is enabled.
     */
    bool isWifiControlEnabled() const;

    /**
     * Returns true when Wi-Fi lifecycle completed all startup stages.
     */
    bool isWifiControlReady() const;

    /**
     * Returns a snapshot of Wi-Fi control lifecycle state.
     */
    WifiControlState getWifiControlState() const;

    /**
     * Initialize Bluetooth and HCI queues
     * Must be called before using other methods
     */
    bool init();

    /**
     * Process HCI tasks
     * Must be called regularly (e.g., in main loop)
     */
    void task();

    /**
     * Check if new data is available
     * @return 1 if data changed, 0 otherwise
     */
    int available();

    /**
     * Get current button state
     * @return ButtonState enum value
     */
    ButtonState getButtonState();

    /**
     * Get current accelerometer state
     * @return AccelState with x, y, z values
     */
    struct AccelState getAccelState();

    /**
     * Get current nunchuk state
     * @return NunchukState with stick and accelerometer values
     */
    struct NunchukState getNunchukState();

    /**
     * Check if Wiimote is connected
     * @return true if connected, false otherwise
     */
    static bool isConnected();

    /**
     * Get battery level
     * @return Battery level (0-100)
     */
    static uint8_t getBatteryLevel();

    /**
     * Request battery status update from Wiimote
     * Battery level will be updated when response is received
     */
    static void requestBatteryUpdate();

    /**
     * Set Wiimote LED mask
     * @param ledMask Bitmask of LEDs to enable
     * @return true if command was queued, false if not connected
     */
    bool setLeds(uint8_t ledMask);

    /**
     * Set Wiimote reporting mode
     * @param mode Reporting mode to apply
     * @param continuous true for continuous reports, false for change-triggered
     * @return true if command was queued, false if not connected
     */
    bool setReportingMode(ReportingMode mode, bool continuous = false);

    /**
     * Enable or disable accelerometer parsing
     * @param enabled true to enable accelerometer, false to disable
     * @return true when request is accepted
     */
    bool setAccelerometerEnabled(bool enabled);

    /**
     * Request status report from Wiimote
     * @return true if command was queued, false if not connected
     */
    bool requestStatus();

    /**
     * Write data to Wiimote memory/register space
     * @param addressSpace WiimoteAddressSpace value
     * @param offset Address offset in selected space
     * @param data Pointer to payload bytes
     * @param len Number of bytes to write
     * @return true if command was queued, false if not connected
     */
    bool writeMemory(uint8_t addressSpace, uint32_t offset, const uint8_t *data, uint8_t len);

    /**
     * Read data from Wiimote memory/register space
     * @param addressSpace WiimoteAddressSpace value
     * @param offset Address offset in selected space
     * @param size Number of bytes to read
     * @return true if command was queued, false if not connected
     */
    bool readMemory(uint8_t addressSpace, uint32_t offset, uint16_t size);

    /**
     * Set Bluetooth scan mode
     * @param enabled true to enable scan, false to disable
     */
    void setScanEnabled(bool enabled);

    /**
     * Start discovery flow
     * @return true if discovery request was accepted, false otherwise
     */
    bool startDiscovery();

    /**
     * Stop discovery flow
     * @return true if stop request was accepted, false otherwise
     */
    bool stopDiscovery();

    /**
     * Disconnect active controller
     * @param reason HCI disconnect reason code
     * @return true if disconnect request was sent, false otherwise
     */
    bool disconnectActiveController(DisconnectReason reason);

    /**
     * Enable or disable automatic reconnect behavior
     * @param enabled true to enable auto reconnect
     */
    void setAutoReconnectEnabled(bool enabled);

    /**
     * Clear cached reconnect target
     */
    void clearReconnectCache();

    /**
     * Enable/disable serial command processing in task loop
     * Disabled by default.
     */
    void enableSerialControl(bool enabled);

    /**
     * Returns true when serial command processing is enabled.
     */
    bool isSerialControlEnabled() const;

    /**
     * Get Bluetooth controller runtime state snapshot
     * @return Current controller state
     */
    BluetoothControllerState getBluetoothControllerState();

    /**
     * Set runtime log level for all library components
     * @param level WiimoteLogLevel value
     */
    static void setLogLevel(uint8_t level);

    /**
     * Get current runtime log level
     * @return Current WiimoteLogLevel value
     */
    static uint8_t getLogLevel();

    /**
     * Add filter to ignore certain data types
     * @param action Filter action (ACTION_IGNORE)
     * @param filter Filter type (FilterButton, FilterAccel, etc.)
     */
    void addFilter(FilterAction action, int filter);

   private:
    ESP32WiimoteConfig config_;
    const char *serialPrivilegedToken_;
    const char *wifiApiToken_;
    WiimoteNetworkCredentials networkCredentials_;
    bool wifiEnabled_;
    bool wifiTokenFallbackToSerial_;
    RuntimeConfigStore runtimeConfigStore_;
    bool runtimeConfigStoreReady_;
    RuntimeConfigSnapshot runtimeConfigSnapshot_;

    // Component managers
    HciCallbacksHandler *hciCallbacks_;
    HciQueueManager *queueManager_;
    ButtonStateManager *buttonState_;
    SensorStateManager *sensorState_;
    WiimoteDataParser *dataParser_;

    bool serialControlEnabled_;
    bool serialPrivilegedCommandsRequireUnlock_;
    SerialCommandSession serialCommandSession_;
    char serialInputLine_[kSerialMaxLineLength + 1U];
    uint8_t serialInputLen_;
    bool serialInputOverflow_;

    bool wifiControlEnabled_;
    bool wifiControlInitializing_;
    bool wifiControlReady_;
    bool wifiNetworkCredentialsConfigured_;
    bool wifiNetworkConnectAttempted_;
    bool wifiNetworkConnected_;
    bool wifiNetworkConnectFailed_;
    WifiDeliveryMode wifiDeliveryMode_;
    uint8_t wifiInitStage_;
    bool wifiLayerStarted_;
    bool littleFsMounted_;
    bool staticRoutesRegistered_;
    bool apiRoutesRegistered_;
    bool websocketRoutesRegistered_;

    void processSerialControl();
    void processSerialCommandLine(const char *line);
    void processWifiControl();
    void resetWifiLifecycleState();
    void persistRuntimeConfigSnapshot();
};

#endif  // ESP32_WIIMOTE_ES_P32_WIIMOTE_H
