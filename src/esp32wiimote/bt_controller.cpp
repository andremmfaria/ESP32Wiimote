// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "bt_controller.h"
#include "hci_callbacks.h"
#include "queue/hci_queue.h"
#include "TinyWiimote.h"
#include "Arduino.h"
#include "esp_bt.h"

#define WIIMOTE_VERBOSE 0

#if WIIMOTE_VERBOSE
#define VERBOSE_PRINT(...) Serial.printf(__VA_ARGS__)
#define VERBOSE_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define VERBOSE_PRINT(...) do {} while(0)
#define VERBOSE_PRINTLN(...) do {} while(0)
#endif

BluetoothController::BluetoothController()
    : _initialized(false)
{
}

bool BluetoothController::init(HciCallbacksHandler* hciCallbacks, HciQueueManager* queueManager)
{
    if (!hciCallbacks || !queueManager) {
        VERBOSE_PRINTLN("Invalid parameters for Bluetooth initialization");
        return false;
    }
    
    // Initialize TinyWiimote with HCI interface
    TinyWiimoteInit(*hciCallbacks->getHciInterface());
    
    // Set queue manager for callbacks
    hciCallbacks->setQueueManager(queueManager);
    
    // Create FreeRTOS queues
    if (!queueManager->createQueues()) {
        VERBOSE_PRINTLN("Failed to create HCI queues");
        return false;
    }
    
    // Register VHCI callbacks
    esp_err_t ret = esp_vhci_host_register_callback(hciCallbacks->getVhciCallback());
    if (ret != ESP_OK) {
        VERBOSE_PRINTLN("Failed to register VHCI callback");
        return false;
    }
    
    // Start Bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (!btStart()) {
        Serial.printf("btStart() failed\n");
        return false;
    }
    
    _initialized = true;
    return true;
}

bool BluetoothController::isStarted(void) const
{
    return btStarted();
}
