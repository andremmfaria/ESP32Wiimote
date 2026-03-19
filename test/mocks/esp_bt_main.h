#ifndef ESP_BT_MAIN_MOCK_H
#define ESP_BT_MAIN_MOCK_H

#include <stdint.h>

typedef enum {
    ESP_BT_CONTROLLER_STATUS_IDLE = 0,
    ESP_BT_CONTROLLER_STATUS_INITED = 1,
    ESP_BT_CONTROLLER_STATUS_ENABLED = 2,
} esp_bt_controller_status_t;

esp_bt_controller_status_t esp_bt_controller_get_status(void);

#endif  // ESP_BT_MAIN_MOCK_H
