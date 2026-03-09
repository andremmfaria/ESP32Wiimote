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
#include "../utils/serial_logging.h"

BluetoothController::BluetoothController()
    : _initialized(false)
{
}

bool BluetoothController::init(HciCallbacksHandler* hciCallbacks, HciQueueManager* queueManager)
{
    VERBOSE_PRINTLN("[BtController] Starting Bluetooth controller initialization...");
    
    if (!hciCallbacks || !queueManager) {
        UNVERBOSE_PRINT("[BtController] ERROR: Invalid parameters!\n");
        return false;
    }
    
    // Start Bluetooth controller FIRST (must be done before VHCI registration)
    VERBOSE_PRINTLN("[BtController] Starting Bluetooth controller with btStart()...");
    if (!btStart()) {
        UNVERBOSE_PRINT("[BtController] ERROR: btStart() failed!\n");
        return false;
    }
    VERBOSE_PRINTLN("[BtController] Bluetooth controller started successfully!");
    
    // Initialize TinyWiimote with HCI interface
    VERBOSE_PRINTLN("[BtController] Initializing TinyWiimote...");
    TinyWiimoteInit(*hciCallbacks->getHciInterface());
    VERBOSE_PRINTLN("[BtController] TinyWiimote initialized");
    
    // Set queue manager for callbacks
    VERBOSE_PRINTLN("[BtController] Setting queue manager...");
    hciCallbacks->setQueueManager(queueManager);
    
    // Create FreeRTOS queues
    VERBOSE_PRINTLN("[BtController] Creating HCI queues...");
    if (!queueManager->createQueues()) {
        UNVERBOSE_PRINT("[BtController] ERROR: Failed to create HCI queues!\n");
        return false;
    }
    VERBOSE_PRINTLN("[BtController] HCI queues created successfully");
    
    // Register VHCI callbacks (must be done AFTER btStart)
    VERBOSE_PRINTLN("[BtController] Registering VHCI callbacks...");
    esp_err_t ret = esp_vhci_host_register_callback(hciCallbacks->getVhciCallback());
    if (ret != ESP_OK) {
        UNVERBOSE_PRINT("[BtController] ERROR: Failed to register VHCI callback! Error: 0x%x\n", ret);
        return false;
    }
    VERBOSE_PRINTLN("[BtController] VHCI callbacks registered successfully!");
    
    _initialized = true;
    VERBOSE_PRINTLN("[BtController] Bluetooth controller initialization complete!");
    return true;
}

bool BluetoothController::isStarted(void) const
{
    return btStarted();
}
