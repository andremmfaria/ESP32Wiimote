// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_queue.h"

#include "../../utils/serial_logging.h"
#include "Arduino.h"
#include "TinyWiimote.h"
#include "esp_bt.h"

#include <stdlib.h>
#include <string.h>

HciQueueManager::HciQueueManager(size_t rxQueueSize, size_t txQueueSize)
    : txQueue_(nullptr), rxQueue_(nullptr), rxQueueSize_(rxQueueSize), txQueueSize_(txQueueSize) {}

bool HciQueueManager::createQueues() {
    wiimoteLogDebug("HciQueue: Creating TX and RX queues...\n");
    txQueue_ = xQueueCreate(txQueueSize_, sizeof(struct HciQueueData *));
    if (txQueue_ == nullptr) {
        wiimoteLogError("HciQueue: xQueueCreate(txQueue) failed\n");
        return false;
    }

    rxQueue_ = xQueueCreate(rxQueueSize_, sizeof(struct HciQueueData *));
    if (rxQueue_ == nullptr) {
        wiimoteLogError("HciQueue: xQueueCreate(rxQueue) failed\n");
        return false;
    }

    wiimoteLogDebug("HciQueue: Queues created successfully\n");
    return true;
}

bool HciQueueManager::sendToQueue(xQueueHandle queue,
                                  uint8_t *data,
                                  size_t len,
                                  const char *debugLabel) {
    wiimoteLogDebug("%s\n", debugLabel);

    if ((data == nullptr) || (len == 0U)) {
        wiimoteLogDebug("HciQueue: No data to send (len=%d)\n", len);
        return true;
    }

    HciQueueData *queuedata = (HciQueueData *)malloc(sizeof(HciQueueData) + len);
    if (queuedata == nullptr) {
        wiimoteLogError("HciQueue: malloc failed for %d bytes\n", sizeof(HciQueueData) + len);
        return false;
    }

    queuedata->len = len;
    memcpy(queuedata->data, data, len);

    const void *queueItem = reinterpret_cast<const void *>(&queuedata);
    if (xQueueSend(queue, queueItem, portMAX_DELAY) != pdPASS) {
        wiimoteLogError("HciQueue: xQueueSend failed\n");
        free(queuedata);
        return false;
    }

    return true;
}

bool HciQueueManager::sendToTxQueue(uint8_t *data, size_t len) {
    if (txQueue_ == nullptr) {
        wiimoteLogWarn("HciQueue: TX queue is not initialized\n");
        return false;
    }

    bool result = sendToQueue(txQueue_, data, len, "sendToTxQueue");
    if (result && (data != nullptr) && (len != 0U)) {
        wiimoteLogDebug("RECV <= %s\n", format2Hex(data, len));
    }
    return result;
}

bool HciQueueManager::sendToRxQueue(uint8_t *data, size_t len) {
    if (rxQueue_ == nullptr) {
        wiimoteLogWarn("HciQueue: RX queue is not initialized\n");
        return false;
    }

    bool result = sendToQueue(rxQueue_, data, len, "sendToRxQueue");
    if (result && (data != nullptr) && (len != 0U)) {
        wiimoteLogDebug("SEND => %s\n", format2Hex(data, len));
    }
    return result;
}

void HciQueueManager::processTxQueue() {
    if (txQueue_ != nullptr && uxQueueMessagesWaiting(txQueue_) != 0U) {
        bool ok = esp_vhci_host_check_send_available();
        wiimoteLogDebug("esp_vhci_host_check_send_available=%d\n", ok);

        if (ok) {
            HciQueueData *queuedata = nullptr;
            if (xQueueReceive(txQueue_, reinterpret_cast<void *>(&queuedata), 0) == pdTRUE) {
                esp_vhci_host_send_packet(queuedata->data, queuedata->len);
                wiimoteLogDebug("SEND => %s\n", format2Hex(queuedata->data, queuedata->len));
                free(queuedata);
            }
        }
    }
}

void HciQueueManager::processRxQueue() {
    if (rxQueue_ != nullptr && uxQueueMessagesWaiting(rxQueue_) != 0U) {
        HciQueueData *queuedata = nullptr;
        if (xQueueReceive(rxQueue_, reinterpret_cast<void *>(&queuedata), 0) == pdTRUE) {
            handleHciData(queuedata->data, queuedata->len);
            free(queuedata);
        }
    }
}

bool HciQueueManager::hasTxPending() const {
    return txQueue_ != nullptr && uxQueueMessagesWaiting(txQueue_) > 0U;
}

bool HciQueueManager::hasRxPending() const {
    return rxQueue_ != nullptr && uxQueueMessagesWaiting(rxQueue_) > 0U;
}
