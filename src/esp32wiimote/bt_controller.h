// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_BT_CONTROLLER_H
#define ESP32_WIIMOTE_BT_CONTROLLER_H

#include <stdint.h>

// Forward declarations
class HciCallbacksHandler;
class HciQueueManager;

/**
 * Bluetooth Controller Manager
 * Initializes and manages ESP32 Bluetooth controller
 */
class BluetoothController {
   public:
    BluetoothController();

    /**
     * Initialize Bluetooth controller and HCI interface
     * @param hciCallbacks Callback handler
     * @param queueManager Queue manager for packet routing
     * @return true if initialization successful, false otherwise
     */
    static bool init(HciCallbacksHandler *hciCallbacks, HciQueueManager *queueManager);

    /**
     * Check if Bluetooth is started
     */
    static bool isStarted();

   private:
    static bool initialized;
};

#endif  // ESP32_WIIMOTE_BT_CONTROLLER_H
