// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __ESP32_WIIMOTE_H__
#define __ESP32_WIIMOTE_H__

#include "TinyWiimote.h"
#include "esp32wiimote/bt_controller.h"
#include "esp32wiimote/data_parser.h"
#include "esp32wiimote/hci_callbacks.h"
#include "esp32wiimote/queue/hci_queue.h"
#include "esp32wiimote/state/button_state.h"
#include "esp32wiimote/state/sensor_state.h"

/**
 * Action types for filters
 */
enum {
    ACTION_IGNORE,
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
    /**
     * Create ESP32Wiimote instance
     * @param NUNCHUK_STICK_THRESHOLD Sensitivity threshold for nunchuk stick changes (default: 1)
     */
    ESP32Wiimote(int NUNCHUK_STICK_THRESHOLD = 1);

    /**
     * Initialize Bluetooth and HCI queues
     * Must be called before using other methods
     */
    bool init(void);

    /**
     * Process HCI tasks
     * Must be called regularly (e.g., in main loop)
     */
    void task(void);

    /**
     * Check if new data is available
     * @return 1 if data changed, 0 otherwise
     */
    int available(void);

    /**
     * Get current button state
     * @return ButtonState enum value
     */
    ButtonState getButtonState(void);

    /**
     * Get current accelerometer state
     * @return AccelState with x, y, z values
     */
    struct AccelState getAccelState(void);

    /**
     * Get current nunchuk state
     * @return NunchukState with stick and accelerometer values
     */
    struct NunchukState getNunchukState(void);

    /**
     * Check if Wiimote is connected
     * @return true if connected, false otherwise
     */
    static bool isConnected(void);

    /**
     * Get battery level
     * @return Battery level (0-100)
     */
    static uint8_t getBatteryLevel(void);

    /**
     * Request battery status update from Wiimote
     * Battery level will be updated when response is received
     */
    static void requestBatteryUpdate(void);

    /**
     * Add filter to ignore certain data types
     * @param action Filter action (ACTION_IGNORE)
     * @param filter Filter type (FILTER_BUTTON, FILTER_ACCEL, etc.)
     */
    void addFilter(int action, int filter);

   private:
    // Component managers
    BluetoothController *_btController;
    HciCallbacksHandler *_hciCallbacks;
    HciQueueManager *_queueManager;
    ButtonStateManager *_buttonState;
    SensorStateManager *_sensorState;
    WiimoteDataParser *_dataParser;
};

#endif  // __ESP32_WIIMOTE_H__
