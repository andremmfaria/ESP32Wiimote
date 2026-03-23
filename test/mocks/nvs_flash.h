#ifndef NVS_FLASH_MOCK_H
#define NVS_FLASH_MOCK_H

#include "esp_bt.h"  // esp_err_t

esp_err_t nvs_flash_init();
esp_err_t nvs_flash_erase();

#endif  // NVS_FLASH_MOCK_H
