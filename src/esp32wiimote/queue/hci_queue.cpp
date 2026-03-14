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
    : _txQueue(nullptr), _rxQueue(nullptr), _rxQueueSize(rxQueueSize), _txQueueSize(txQueueSize) {}

HciQueueManager::~HciQueueManager() {
    // FreeRTOS queues cleanup would happen here if needed
}

bool HciQueueManager::createQueues() {
    LOG_DEBUG("HciQueue: Creating TX and RX queues...\n");
    _txQueue = xQueueCreate(_txQueueSize, sizeof(struct HciQueueData *));
    if (_txQueue == nullptr) {
        LOG_ERROR("HciQueue: xQueueCreate(txQueue) failed\n");
        return false;
    }

    _rxQueue = xQueueCreate(_rxQueueSize, sizeof(struct HciQueueData *));
    if (_rxQueue == nullptr) {
        LOG_ERROR("HciQueue: xQueueCreate(rxQueue) failed\n");
        return false;
    }

    LOG_DEBUG("HciQueue: Queues created successfully\n");
    return true;
}

bool HciQueueManager::sendToQueue(xQueueHandle queue,
                                  uint8_t *data,
                                  size_t len,
                                  const char *debugLabel) {
    LOG_DEBUG("%s\n", debugLabel);

    if ((data == nullptr) || (len == 0U)) {
        LOG_DEBUG("HciQueue: No data to send (len=%d)\n", len);
        return true;
    }

    HciQueueData *queuedata = (HciQueueData *)malloc(sizeof(HciQueueData) + len);
    if (queuedata == nullptr) {
        LOG_ERROR("HciQueue: malloc failed for %d bytes\n", sizeof(HciQueueData) + len);
        return false;
    }

    queuedata->len = len;
    memcpy(queuedata->data, data, len);

    if (xQueueSend(queue, &queuedata, portMAX_DELAY) != pdPASS) {
        LOG_ERROR("HciQueue: xQueueSend failed\n");
        free(queuedata);
        return false;
    }

    return true;
}

bool HciQueueManager::sendToTxQueue(uint8_t *data, size_t len) {
    bool result = sendToQueue(_txQueue, data, len, "sendToTxQueue");
    if (result && (data != nullptr) && (len != 0U)) {
        LOG_DEBUG("RECV <= %s\n", format2Hex(data, len));
    }
    return result;
}

bool HciQueueManager::sendToRxQueue(uint8_t *data, size_t len) {
    bool result = sendToQueue(_rxQueue, data, len, "sendToRxQueue");
    if (result && (data != nullptr) && (len != 0U)) {
        LOG_DEBUG("SEND => %s\n", format2Hex(data, len));
    }
    return result;
}

void HciQueueManager::processTxQueue() {
    if (uxQueueMessagesWaiting(_txQueue) != 0U) {
        bool ok = esp_vhci_host_check_send_available();
        LOG_DEBUG("esp_vhci_host_check_send_available=%d\n", ok);

        if (ok) {
            HciQueueData *queuedata = nullptr;
            if (xQueueReceive(_txQueue, reinterpret_cast<void *>(&queuedata), 0) == pdTRUE) {
                esp_vhci_host_send_packet(queuedata->data, queuedata->len);
                LOG_DEBUG("SEND => %s\n", format2Hex(queuedata->data, queuedata->len));
                free(queuedata);
            }
        }
    }
}

void HciQueueManager::processRxQueue() {
    if (uxQueueMessagesWaiting(_rxQueue) != 0U) {
        HciQueueData *queuedata = nullptr;
        if (xQueueReceive(_rxQueue, reinterpret_cast<void *>(&queuedata), 0) == pdTRUE) {
            handleHciData(queuedata->data, queuedata->len);
            free(queuedata);
        }
    }
}

bool HciQueueManager::hasTxPending() const {
    return uxQueueMessagesWaiting(_txQueue) > 0;
}

bool HciQueueManager::hasRxPending() const {
    return uxQueueMessagesWaiting(_rxQueue) > 0;
}
