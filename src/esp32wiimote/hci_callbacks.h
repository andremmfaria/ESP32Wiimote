// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __HCI_CALLBACKS_H__
#define __HCI_CALLBACKS_H__

#include <stdint.h>
#include <stddef.h>
#include "esp_bt.h"
#include "TinyWiimote.h"

// Forward declaration
class HciQueueManager;

/**
 * HCI Callbacks Handler
 * Manages Bluetooth host controller callbacks and HCI interface setup
 */
class HciCallbacksHandler {
public:
    HciCallbacksHandler();
    
    /**
     * Set the queue manager for HCI packet routing
     */
    void setQueueManager(HciQueueManager* queueManager);
    
    /**
     * Get the HCI interface structure for TinyWiimote
     */
    const struct TwHciInterface* getHciInterface(void) const;
    
    /**
     * Get the VHCI callback structure for ESP32
     */
    esp_vhci_host_callback_t* getVhciCallback(void);
    
    /**
     * Static callback - Called when host is ready to send
     */
    static void notifyHostSendAvailable(void);
    
    /**
     * Static callback - Called when host receives data
     */
    static int notifyHostRecv(uint8_t* data, uint16_t len);
    
    /**
     * Static callback - Send HCI packet to host
     */
    static void hciHostSendPacket(uint8_t* data, size_t len);

private:
    static HciQueueManager* _queueManager;
    struct TwHciInterface _hciInterface;
    esp_vhci_host_callback_t _vhciCallback;
};

#endif // __HCI_CALLBACKS_H__
