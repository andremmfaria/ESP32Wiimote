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
#include "utils/serial_logging.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * Constructor - Initialize all component managers
 */
ESP32Wiimote::ESP32Wiimote(int NUNCHUK_STICK_THRESHOLD) {
    _btController = new BluetoothController();
    _hciCallbacks = new HciCallbacksHandler();
    _queueManager = new HciQueueManager(32, 32);
    _buttonState = new ButtonStateManager();
    _sensorState = new SensorStateManager(NUNCHUK_STICK_THRESHOLD);
    _dataParser = new WiimoteDataParser(_buttonState, _sensorState);
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
    if (!_btController->init(_hciCallbacks, _queueManager)) {
        LOG_ERROR("ESP32Wiimote: Bluetooth controller initialization failed!\n");
        return false;
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
        return;
    }

    // Process pending HCI packets
    _queueManager->processTxQueue();
    _queueManager->processRxQueue();
}

/**
 * Check if new sensor/button data is available
 * Delegates to data parser
 */
int ESP32Wiimote::available() {
    return _dataParser->parseData();
}

/**
 * Get current button state
 */
ButtonState ESP32Wiimote::getButtonState() {
    return _buttonState->getCurrent();
}

/**
 * Get current accelerometer state
 */
struct AccelState ESP32Wiimote::getAccelState() {
    return _sensorState->getAccel();
}

/**
 * Get current nunchuk state
 */
struct NunchukState ESP32Wiimote::getNunchukState() {
    return _sensorState->getNunchuk();
}

/**
 * Check if Wiimote is connected
 */
bool ESP32Wiimote::isConnected() {
    return TinyWiimoteIsConnected();
}

/**
 * Get battery level
 */
uint8_t ESP32Wiimote::getBatteryLevel() {
    return TinyWiimoteGetBatteryLevel();
}

/**
 * Request battery status update
 */
void ESP32Wiimote::requestBatteryUpdate() {
    TinyWiimoteRequestBatteryUpdate();
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
void ESP32Wiimote::addFilter(int action, int filter) {
    if (action == ACTION_IGNORE) {
        _dataParser->setFilter(_dataParser->getFilter() | filter);

        if ((filter & FILTER_ACCEL) != 0) {
            TinyWiimoteReqAccelerometer(false);
        }
    }
}
