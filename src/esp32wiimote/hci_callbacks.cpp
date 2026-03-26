// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_callbacks.h"

#include "../utils/serial_logging.h"
#include "Arduino.h"
#include "TinyWiimote.h"
#include "queue/hci_queue.h"

// Static member initialization
HciQueueManager *HciCallbacksHandler::queueManager = nullptr;

HciCallbacksHandler::HciCallbacksHandler() {
    hciInterface_.hciSendPacket = hciHostSendPacket;
}

void HciCallbacksHandler::setQueueManager(HciQueueManager *queueManager) {
    HciCallbacksHandler::queueManager = queueManager;
}

const struct TwHciInterface *HciCallbacksHandler::getHciInterface() const {
    return &hciInterface_;
}

esp_vhci_host_callback_t *HciCallbacksHandler::getVhciCallback() {
    vhciCallback_.notify_host_recv = notifyHostRecv;
    vhciCallback_.notify_host_send_available = notifyHostSendAvailable;
    return &vhciCallback_;
}

void HciCallbacksHandler::notifyHostSendAvailable() {
    wiimoteLogDebug("notifyHostSendAvailable\n");
    if (!tinyWiimoteDeviceIsInited()) {
        tinyWiimoteResetDevice();
    }
}

int HciCallbacksHandler::notifyHostRecv(uint8_t *data, uint16_t len) {
    wiimoteLogDebug("notifyHostRecv:");
    for (int i = 0; i < len; i++) {
        wiimoteLogDebug(" %02x", data[i]);
    }
    wiimoteLogDebug("\n");

    if ((queueManager != nullptr) && queueManager->sendToRxQueue(data, len)) {
        return ESP_OK;
    }
    wiimoteLogError("HciCallback: Failed to send data to RX queue\n");
    return ESP_FAIL;
}

void HciCallbacksHandler::hciHostSendPacket(uint8_t *data, size_t len) {
    if (queueManager != nullptr) {
        queueManager->sendToTxQueue(data, len);
    } else {
        wiimoteLogWarn("HciCallback: Queue manager not set, cannot send packet\n");
    }
}
