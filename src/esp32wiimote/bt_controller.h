// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __BT_CONTROLLER_H__
#define __BT_CONTROLLER_H__

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
    bool init(HciCallbacksHandler* hciCallbacks, HciQueueManager* queueManager);
    
    /**
     * Check if Bluetooth is started
     */
    bool isStarted(void) const;

private:
    bool _initialized;
};

#endif // __BT_CONTROLLER_H__
