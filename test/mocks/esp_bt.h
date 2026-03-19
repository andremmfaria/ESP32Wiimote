#ifndef ESP_BT_MOCK_H
#define ESP_BT_MOCK_H

// Minimal esp_bt.h mock for native unit tests.

#include <stddef.h>
#include <stdint.h>

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_FAIL (-1)

typedef struct {
    void (*notify_host_send_available)(void);
    int (*notify_host_recv)(uint8_t *data, uint16_t len);
} esp_vhci_host_callback_t;

// Declared here; defined in test_mocks.cpp.
bool esp_vhci_host_check_send_available(void);
void esp_vhci_host_send_packet(uint8_t *data, size_t len);
esp_err_t esp_vhci_host_register_callback(esp_vhci_host_callback_t *callback);

#endif  // ESP_BT_MOCK_H
