// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_callbacks.h"
#include "queue/hci_queue.h"
#include "TinyWiimote.h"
#include "Arduino.h"
#include "../utils/serial_logging.h"

// Static member initialization
HciQueueManager* HciCallbacksHandler::_queueManager = NULL;

HciCallbacksHandler::HciCallbacksHandler()
{
    _hciInterface.hci_send_packet = hciHostSendPacket;
}

void HciCallbacksHandler::setQueueManager(HciQueueManager* queueManager)
{
    _queueManager = queueManager;
}

const struct TwHciInterface* HciCallbacksHandler::getHciInterface(void) const
{
    return &_hciInterface;
}

esp_vhci_host_callback_t* HciCallbacksHandler::getVhciCallback(void)
{
    _vhciCallback.notify_host_recv = notifyHostRecv;
    _vhciCallback.notify_host_send_available = notifyHostSendAvailable;
    return &_vhciCallback;
}

void HciCallbacksHandler::notifyHostSendAvailable(void)
{
    VERBOSE_PRINT("notifyHostSendAvailable\n");
    if (!TinyWiimoteDeviceIsInited()) {
        TinyWiimoteResetDevice();
    }
}

int HciCallbacksHandler::notifyHostRecv(uint8_t* data, uint16_t len)
{
    VERBOSE_PRINT("notifyHostRecv:");
    for (int i = 0; i < len; i++) {
        VERBOSE_PRINT(" %02x", data[i]);
    }
    VERBOSE_PRINTLN("");

    if (_queueManager && _queueManager->sendToRxQueue(data, len)) {
        return ESP_OK;
    } else {
        UNVERBOSE_PRINT("[HciCallback] ERROR: Failed to send data to RX queue\n");
        return ESP_FAIL;
    }
}

void HciCallbacksHandler::hciHostSendPacket(uint8_t* data, size_t len)
{
    if (_queueManager) {
        _queueManager->sendToTxQueue(data, len);
    } else {
        VERBOSE_PRINT("[HciCallback] WARNING: Queue manager not set, cannot send packet\n");
    }
}
