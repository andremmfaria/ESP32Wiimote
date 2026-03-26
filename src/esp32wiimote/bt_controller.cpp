// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "bt_controller.h"

#include "../utils/protocol_codes.h"
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

bool BluetoothController::initialized = false;

BluetoothController::BluetoothController() = default;

bool BluetoothController::init(HciCallbacksHandler *hciCallbacks, HciQueueManager *queueManager) {
    wiimoteLogDebug("BtController: Starting Bluetooth controller initialization...\n");

    if ((hciCallbacks == nullptr) || (queueManager == nullptr)) {
        wiimoteLogError("BtController: Invalid parameters!\n");
        return false;
    }

    // Start Bluetooth controller FIRST (must be done before VHCI registration)
    wiimoteLogDebug("BtController: About to initialize Bluetooth controller...\n");
    wiimoteLogDebug("BtController: Free heap before init: %d bytes\n", ESP.getFreeHeap());

    // Get and display detailed status info
    esp_bt_controller_status_t status = esp_bt_controller_get_status();
    wiimoteLogDebug("BtController: Initial controller status: %d (%s)\n", status,
              btControllerStatusToString((uint8_t)status));

    // Initialize Bluetooth controller using Arduino core function
    wiimoteLogDebug("BtController: Calling btStart()...\n");
    wiimoteLogDebug("BtController: Controller status before btStart: %d (%s)\n",
              esp_bt_controller_get_status(),
              btControllerStatusToString((uint8_t)esp_bt_controller_get_status()));

    if (!btStart()) {
        wiimoteLogError("BtController: btStart() failed!\n");
        return false;
    }

    wiimoteLogDebug("BtController: btStart() succeeded!\n");
    wiimoteLogDebug("BtController: Controller status after btStart: %d (%s)\n",
              esp_bt_controller_get_status(),
              btControllerStatusToString((uint8_t)esp_bt_controller_get_status()));
    wiimoteLogDebug("BtController: Free heap after init: %d bytes\n", ESP.getFreeHeap());

    // Initialize TinyWiimote with HCI interface
    wiimoteLogDebug("BtController: Initializing TinyWiimote...\n");
    tinyWiimoteInit(*hciCallbacks->getHciInterface());
    wiimoteLogDebug("BtController: TinyWiimote initialized\n");

    // Set queue manager for callbacks
    wiimoteLogDebug("BtController: Setting queue manager...\n");
    HciCallbacksHandler::setQueueManager(queueManager);

    // Create FreeRTOS queues
    wiimoteLogDebug("BtController: Creating HCI queues...\n");
    if (!queueManager->createQueues()) {
        wiimoteLogError("BtController: Failed to create HCI queues!\n");
        return false;
    }
    wiimoteLogDebug("BtController: HCI queues created successfully\n");

    // Register VHCI callbacks (must be done AFTER btStart)
    wiimoteLogDebug("BtController: Registering VHCI callbacks...\n");
    esp_err_t ret = esp_vhci_host_register_callback(hciCallbacks->getVhciCallback());
    if (ret != ESP_OK) {
        wiimoteLogError("BtController: Failed to register VHCI callback! Error: 0x%x\n", ret);
        return false;
    }
    wiimoteLogDebug("BtController: VHCI callbacks registered successfully!\n");

    initialized = true;
    wiimoteLogInfo("BtController: Bluetooth controller initialization complete!\n");
    return true;
}

bool BluetoothController::isStarted() {
    return initialized && btStarted();
}
