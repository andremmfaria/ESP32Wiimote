// In-memory NVS mock for native tests.
// Implements the NVS flash and open/read/write/erase/commit API using a
// std::map so the real production code can run unchanged on the host.

#include "nvs.h"

#include "nvs_flash.h"

#include <map>
#include <string>

namespace {

using NvsStore = std::map<std::string, uint32_t>;
std::map<std::string, NvsStore> gStorage;

nvs_handle_t gNextHandle = 1U;
std::map<nvs_handle_t, std::string> gOpenHandles;

NvsStore *storeFor(nvs_handle_t handle) {
    auto it = gOpenHandles.find(handle);
    if (it == gOpenHandles.end()) {
        return nullptr;
    }
    return &gStorage[it->second];
}

}  // namespace

bool gMockNvsOpenFail = false;

esp_err_t nvs_flash_init() {
    return ESP_OK;
}

esp_err_t nvs_flash_erase() {
    gStorage.clear();
    gOpenHandles.clear();
    gNextHandle = 1U;
    return ESP_OK;
}

esp_err_t nvs_open(const char *name, nvs_open_mode_t /*mode*/, nvs_handle_t *out_handle) {
    if (gMockNvsOpenFail || name == nullptr || out_handle == nullptr) {
        return ESP_FAIL;
    }
    *out_handle = gNextHandle++;
    gOpenHandles[*out_handle] = name;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) {
    gOpenHandles.erase(handle);
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value) {
    NvsStore *store = storeFor(handle);
    if (store == nullptr || key == nullptr) {
        return ESP_FAIL;
    }
    (*store)[key] = value;
    return ESP_OK;
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value) {
    NvsStore *store = storeFor(handle);
    if (store == nullptr || key == nullptr || out_value == nullptr) {
        return ESP_FAIL;
    }
    auto it = store->find(key);
    if (it == store->end()) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    *out_value = static_cast<uint8_t>(it->second);
    return ESP_OK;
}

esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value) {
    NvsStore *store = storeFor(handle);
    if (store == nullptr || key == nullptr) {
        return ESP_FAIL;
    }
    (*store)[key] = value;
    return ESP_OK;
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char *key, uint32_t *out_value) {
    NvsStore *store = storeFor(handle);
    if (store == nullptr || key == nullptr || out_value == nullptr) {
        return ESP_FAIL;
    }
    auto it = store->find(key);
    if (it == store->end()) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    *out_value = it->second;
    return ESP_OK;
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char *key) {
    NvsStore *store = storeFor(handle);
    if (store == nullptr || key == nullptr) {
        return ESP_FAIL;
    }
    return store->erase(key) > 0U ? ESP_OK : ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_commit(nvs_handle_t handle) {
    return gOpenHandles.count(handle) > 0U ? ESP_OK : ESP_FAIL;
}
