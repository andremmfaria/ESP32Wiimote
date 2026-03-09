// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include <string.h>
#include <stdlib.h>
#include "hci_queue.h"
#include "Arduino.h"
#include "esp_bt.h"
#include "TinyWiimote.h"
#include "../../utils/serial_logging.h"

HciQueueManager::HciQueueManager(size_t rxQueueSize, size_t txQueueSize)
    : _txQueue(NULL), _rxQueue(NULL),
      _rxQueueSize(rxQueueSize), _txQueueSize(txQueueSize)
{
}

HciQueueManager::~HciQueueManager()
{
    // FreeRTOS queues cleanup would happen here if needed
}

bool HciQueueManager::createQueues(void)
{
    VERBOSE_PRINTLN("[HciQueue] Creating TX and RX queues...");
    _txQueue = xQueueCreate(_txQueueSize, sizeof(struct HciQueueData*));
    if (_txQueue == NULL) {
        UNVERBOSE_PRINT("[HciQueue] ERROR: xQueueCreate(txQueue) failed\n");
        return false;
    }
    
    _rxQueue = xQueueCreate(_rxQueueSize, sizeof(struct HciQueueData*));
    if (_rxQueue == NULL) {
        UNVERBOSE_PRINT("[HciQueue] ERROR: xQueueCreate(rxQueue) failed\n");
        return false;
    }
    
    VERBOSE_PRINTLN("[HciQueue] Queues created successfully");
    return true;
}

bool HciQueueManager::sendToQueue(xQueueHandle queue, uint8_t* data, size_t len, const char* debugLabel)
{
    VERBOSE_PRINTLN(debugLabel);
    
    if (!data || !len) {
        VERBOSE_PRINT("[HciQueue] No data to send (len=%d)\n", len);
        return true;
    }
    
    HciQueueData* queuedata = (HciQueueData*)malloc(sizeof(HciQueueData) + len);
    if (!queuedata) {
        UNVERBOSE_PRINT("[HciQueue] ERROR: malloc failed for %d bytes\n", sizeof(HciQueueData) + len);
        return false;
    }
    
    queuedata->len = len;
    memcpy(queuedata->data, data, len);
    
    if (xQueueSend(queue, &queuedata, portMAX_DELAY) != pdPASS) {
        UNVERBOSE_PRINT("[HciQueue] ERROR: xQueueSend failed\n");
        free(queuedata);
        return false;
    }
    
    return true;
}

bool HciQueueManager::sendToTxQueue(uint8_t* data, size_t len)
{
    bool result = sendToQueue(_txQueue, data, len, "sendToTxQueue");
    if (result && data && len) {
        VERBOSE_PRINT("RECV <= %s\n", format2Hex(data, len));
    }
    return result;
}

bool HciQueueManager::sendToRxQueue(uint8_t* data, size_t len)
{
    bool result = sendToQueue(_rxQueue, data, len, "sendToRxQueue");
    if (result && data && len) {
        VERBOSE_PRINT("SEND => %s\n", format2Hex(data, len));
    }
    return result;
}

void HciQueueManager::processTxQueue(void)
{
    if (uxQueueMessagesWaiting(_txQueue)) {
        bool ok = esp_vhci_host_check_send_available();
        VERBOSE_PRINT("esp_vhci_host_check_send_available=%d", ok);
        
        if (ok) {
            HciQueueData* queuedata = NULL;
            if (xQueueReceive(_txQueue, &queuedata, 0) == pdTRUE) {
                esp_vhci_host_send_packet(queuedata->data, queuedata->len);
                VERBOSE_PRINT("SEND => %s\n", format2Hex(queuedata->data, queuedata->len));
                free(queuedata);
            }
        }
    }
}

void HciQueueManager::processRxQueue(void)
{
    if (uxQueueMessagesWaiting(_rxQueue)) {
        HciQueueData* queuedata = NULL;
        if (xQueueReceive(_rxQueue, &queuedata, 0) == pdTRUE) {
            handleHciData(queuedata->data, queuedata->len);
            free(queuedata);
        }
    }
}

bool HciQueueManager::hasTxPending(void) const
{
    return uxQueueMessagesWaiting(_txQueue) > 0;
}

bool HciQueueManager::hasRxPending(void) const
{
    return uxQueueMessagesWaiting(_rxQueue) > 0;
}
