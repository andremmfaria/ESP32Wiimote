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
    LOG_DEBUG("BtController: Starting Bluetooth controller initialization...\n");
    
    if (!hciCallbacks || !queueManager) {
        LOG_ERROR("BtController: Invalid parameters!\n");
        return false;
    }
    
    // Start Bluetooth controller FIRST (must be done before VHCI registration)
    LOG_DEBUG("BtController: Starting Bluetooth controller with btStart()...\n");
    if (!btStart()) {
        LOG_ERROR("BtController: btStart() failed!\n");
        return false;
    }
    LOG_DEBUG("BtController: Bluetooth controller started successfully!\n");
    
    // Initialize TinyWiimote with HCI interface
    LOG_DEBUG("BtController: Initializing TinyWiimote...\n");
    TinyWiimoteInit(*hciCallbacks->getHciInterface());
    LOG_DEBUG("BtController: TinyWiimote initialized\n");
    
    // Set queue manager for callbacks
    LOG_DEBUG("BtController: Setting queue manager...\n");
    hciCallbacks->setQueueManager(queueManager);
    
    // Create FreeRTOS queues
    LOG_DEBUG("BtController: Creating HCI queues...\n");
    if (!queueManager->createQueues()) {
        LOG_ERROR("BtController: Failed to create HCI queues!\n");
        return false;
    }
    LOG_DEBUG("BtController: HCI queues created successfully\n");
    
    // Register VHCI callbacks (must be done AFTER btStart)
    LOG_DEBUG("BtController: Registering VHCI callbacks...\n");
    esp_err_t ret = esp_vhci_host_register_callback(hciCallbacks->getVhciCallback());
    if (ret != ESP_OK) {
        LOG_ERROR("BtController: Failed to register VHCI callback! Error: 0x%x\n", ret);
        return false;
    }
    LOG_DEBUG("BtController: VHCI callbacks registered successfully!\n");
    
    _initialized = true;
    LOG_INFO("BtController: Bluetooth controller initialization complete!\n");
    return true;
}

bool BluetoothController::isStarted(void) const
{
    return btStarted();
}
