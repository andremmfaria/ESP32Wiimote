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

#define WIIMOTE_VERBOSE 0

#if WIIMOTE_VERBOSE
#define VERBOSE_PRINT(...) Serial.printf(__VA_ARGS__)
#define VERBOSE_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define VERBOSE_PRINT(...) do {} while(0)
#define VERBOSE_PRINTLN(...) do {} while(0)
#endif

#define UNVERBOSE_PRINT(...) do {} while(0)

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
    _txQueue = xQueueCreate(_txQueueSize, sizeof(struct HciQueueData*));
    if (_txQueue == NULL) {
        VERBOSE_PRINTLN("xQueueCreate(txQueue) failed");
        return false;
    }
    
    _rxQueue = xQueueCreate(_rxQueueSize, sizeof(struct HciQueueData*));
    if (_rxQueue == NULL) {
        VERBOSE_PRINTLN("xQueueCreate(rxQueue) failed");
        return false;
    }
    
    return true;
}

bool HciQueueManager::sendToTxQueue(uint8_t* data, size_t len)
{
    VERBOSE_PRINTLN("sendToTxQueue");
    
    if (!data || !len) {
        VERBOSE_PRINTLN("no data");
        return true;
    }
    
    HciQueueData* queuedata = (HciQueueData*)malloc(sizeof(HciQueueData) + len);
    if (!queuedata) {
        VERBOSE_PRINTLN("malloc failed");
        return false;
    }
    
    queuedata->len = len;
    memcpy(queuedata->data, data, len);
    
    UNVERBOSE_PRINT("RECV <= %s\n", format2Hex(queuedata->data, queuedata->len));
    
    if (xQueueSend(_txQueue, &queuedata, portMAX_DELAY) != pdPASS) {
        VERBOSE_PRINTLN("xQueueSend failed");
        free(queuedata);
        return false;
    }
    
    return true;
}

bool HciQueueManager::sendToRxQueue(uint8_t* data, size_t len)
{
    VERBOSE_PRINTLN("sendToRxQueue");
    
    if (!data || !len) {
        VERBOSE_PRINTLN("no data");
        return true;
    }
    
    HciQueueData* queuedata = (HciQueueData*)malloc(sizeof(HciQueueData) + len);
    if (!queuedata) {
        VERBOSE_PRINTLN("malloc failed");
        return false;
    }
    
    queuedata->len = len;
    memcpy(queuedata->data, data, len);
    
    UNVERBOSE_PRINT("SEND => %s\n", format2Hex(queuedata->data, queuedata->len));
    
    if (xQueueSend(_rxQueue, &queuedata, portMAX_DELAY) != pdPASS) {
        VERBOSE_PRINTLN("xQueueSend failed");
        free(queuedata);
        return false;
    }
    
    return true;
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
                UNVERBOSE_PRINT("SEND => %s\n", format2Hex(queuedata->data, queuedata->len));
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
