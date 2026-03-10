// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "bt_controller.h"

#include "../utils/serial_logging.h"
#include "Arduino.h"
#include "TinyWiimote.h"
#include "esp32-hal-bt.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "hci_callbacks.h"
#include "queue/hci_queue.h"

// Keep BT memory reserved on cores that weak-link btInUse().
// This avoids relying on esp32-hal-bt-mem.h being present.
extern "C" __attribute__((weak)) bool btInUse(void) {
    return true;
}

BluetoothController::BluetoothController() : _initialized(false) {}

bool BluetoothController::init(HciCallbacksHandler *hciCallbacks, HciQueueManager *queueManager) {
    LOG_DEBUG("BtController: Starting Bluetooth controller initialization...\n");

    if ((hciCallbacks == nullptr) || (queueManager == nullptr)) {
        LOG_ERROR("BtController: Invalid parameters!\n");
        return false;
    }

    // Start Bluetooth controller FIRST (must be done before VHCI registration)
    LOG_DEBUG("BtController: About to initialize Bluetooth controller...\n");
    LOG_DEBUG("BtController: Free heap before init: %d bytes\n", ESP.getFreeHeap());

    // Get and display detailed status info
    esp_bt_controller_status_t status = esp_bt_controller_get_status();
    LOG_DEBUG("BtController: Initial controller status: %d (0=IDLE, 1=INITED, 2=ENABLED)\n",
              status);

    // Initialize Bluetooth controller using Arduino core function
    LOG_DEBUG("BtController: Calling btStart()...\n");
    LOG_DEBUG("BtController: Controller status before btStart: %d\n",
              esp_bt_controller_get_status());

    if (!btStart()) {
        LOG_ERROR("BtController: btStart() failed!\n");
        return false;
    }

    LOG_DEBUG("BtController: btStart() succeeded!\n");
    LOG_DEBUG("BtController: Controller status after btStart: %d\n",
              esp_bt_controller_get_status());
    LOG_DEBUG("BtController: Free heap after init: %d bytes\n", ESP.getFreeHeap());

    // Initialize TinyWiimote with HCI interface
    LOG_DEBUG("BtController: Initializing TinyWiimote...\n");
    TinyWiimoteInit(*hciCallbacks->getHciInterface());
    LOG_DEBUG("BtController: TinyWiimote initialized\n");

    // Set queue manager for callbacks
    LOG_DEBUG("BtController: Setting queue manager...\n");
    HciCallbacksHandler::setQueueManager(queueManager);

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

bool BluetoothController::isStarted() {
    return btStarted();
}
