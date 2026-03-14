// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32WIIMOTE_HCI_CALLBACKS_H_
#define ESP32WIIMOTE_HCI_CALLBACKS_H_

#include "TinyWiimote.h"
#include "esp_bt.h"

#include <stddef.h>
#include <stdint.h>

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
    static void setQueueManager(HciQueueManager *queueManager);

    /**
     * Get the HCI interface structure for TinyWiimote
     */
    const struct TwHciInterface *getHciInterface() const;

    /**
     * Get the VHCI callback structure for ESP32
     */
    esp_vhci_host_callback_t *getVhciCallback();

    /**
     * Static callback - Called when host is ready to send
     */
    static void notifyHostSendAvailable();

    /**
     * Static callback - Called when host receives data
     */
    static int notifyHostRecv(uint8_t *data, uint16_t len);

    /**
     * Static callback - Send HCI packet to host
     */
    static void hciHostSendPacket(uint8_t *data, size_t len);

   private:
    static HciQueueManager *_queueManager;
    struct TwHciInterface _hciInterface;
    esp_vhci_host_callback_t _vhciCallback;
};

#endif  // ESP32WIIMOTE_HCI_CALLBACKS_H_
