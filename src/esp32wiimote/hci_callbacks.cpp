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
HciQueueManager *HciCallbacksHandler::_queueManager = nullptr;

HciCallbacksHandler::HciCallbacksHandler() {
    _hciInterface.hci_send_packet = hciHostSendPacket;
}

void HciCallbacksHandler::setQueueManager(HciQueueManager *queueManager) {
    _queueManager = queueManager;
}

const struct TwHciInterface *HciCallbacksHandler::getHciInterface() const {
    return &_hciInterface;
}

esp_vhci_host_callback_t *HciCallbacksHandler::getVhciCallback() {
    _vhciCallback.notify_host_recv = notifyHostRecv;
    _vhciCallback.notify_host_send_available = notifyHostSendAvailable;
    return &_vhciCallback;
}

void HciCallbacksHandler::notifyHostSendAvailable() {
    LOG_DEBUG("notifyHostSendAvailable\n");
    if (!TinyWiimoteDeviceIsInited()) {
        TinyWiimoteResetDevice();
    }
}

int HciCallbacksHandler::notifyHostRecv(uint8_t *data, uint16_t len) {
    LOG_DEBUG("notifyHostRecv:");
    for (int i = 0; i < len; i++) {
        LOG_DEBUG(" %02x", data[i]);
    }
    LOG_DEBUG("\n");

    if ((_queueManager != nullptr) && _queueManager->sendToRxQueue(data, len)) {
        return ESP_OK;
    }
    LOG_ERROR("HciCallback: Failed to send data to RX queue\n");
    return ESP_FAIL;
}

void HciCallbacksHandler::hciHostSendPacket(uint8_t *data, size_t len) {
    if (_queueManager != nullptr) {
        _queueManager->sendToTxQueue(data, len);
    } else {
        LOG_WARN("HciCallback: Queue manager not set, cannot send packet\n");
    }
}
