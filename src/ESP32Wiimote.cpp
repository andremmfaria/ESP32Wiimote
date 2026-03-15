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
ESP32Wiimote::ESP32Wiimote(int nunchukStickThreshold) {
    btController_ = new BluetoothController();
    hciCallbacks_ = new HciCallbacksHandler();
    queueManager_ = new HciQueueManager(32, 32);
    buttonState_ = new ButtonStateManager();
    sensorState_ = new SensorStateManager(nunchukStickThreshold);
    dataParser_ = new WiimoteDataParser(buttonState_, sensorState_);
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
    queueManager_->processTxQueue();
    queueManager_->processRxQueue();
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

        if ((filter & FilterAccel) != 0) {
            tinyWiimoteReqAccelerometer(false);
        }
    }
}
