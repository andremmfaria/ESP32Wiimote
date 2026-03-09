// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __HCI_QUEUE_H__
#define __HCI_QUEUE_H__

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * HCI Queue Data Structure
 */
struct HciQueueData {
    size_t len;
    uint8_t data[];
};

/**
 * HCI Queue Manager - Handles TX/RX queues for HCI packets
 */
class HciQueueManager {
public:
    HciQueueManager(size_t rxQueueSize = 32, size_t txQueueSize = 32);
    ~HciQueueManager();
    
    /**
     * Create and initialize the TX and RX queues
     * @return true if queues created successfully, false otherwise
     */
    bool createQueues(void);
    
    /**
     * Send data to TX queue (for outgoing packets)
     * @param data Pointer to packet data
     * @param len Length of packet
     * @return true if successful, false otherwise
     */
    bool sendToTxQueue(uint8_t* data, size_t len);
    
    /**
     * Send data to RX queue (for incoming packets)
     * @param data Pointer to packet data
     * @param len Length of packet
     * @return true if successful, false otherwise
     */
    bool sendToRxQueue(uint8_t* data, size_t len);
    
    /**
     * Process TX queue - sends queued packets if possible
     * Must be called from main task
     */
    void processTxQueue(void);
    
    /**
     * Process RX queue - handles received packets
     * Must be called from main task
     */
    void processRxQueue(void);
    
    /**
     * Check if TX queue has pending packets
     */
    bool hasTxPending(void) const;
    
    /**
     * Check if RX queue has pending packets
     */
    bool hasRxPending(void) const;

private:
    xQueueHandle _txQueue;
    xQueueHandle _rxQueue;
    size_t _rxQueueSize;
    size_t _txQueueSize;
};

#endif // __HCI_QUEUE_H__
