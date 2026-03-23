#include "runtime_config_store.h"

#include "nvs.h"
#include "nvs_flash.h"

namespace {

constexpr const char *kRuntimeConfigNamespace = "wm_cfg";
constexpr const char *kKeyAutoReconnect = "auto_reconn";
constexpr const char *kKeyFastReconnectTtlMs = "fast_ttl";
constexpr const char *kKeyLedMask = "led_mask";
constexpr const char *kKeyReportingMode = "rep_mode";
constexpr const char *kKeyReportingContinuous = "rep_cont";

bool ensureNvsReady() {
    esp_err_t error = nvs_flash_init();
    if (error == ESP_OK) {
        return true;
    }

    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() != ESP_OK) {
            return false;
        }
        error = nvs_flash_init();
    }

    return error == ESP_OK;
}

bool openStore(nvs_open_mode openMode, nvs_handle_t *handle) {
    if (handle == nullptr) {
        return false;
    }

    const esp_err_t kOpenError = nvs_open(kRuntimeConfigNamespace, openMode, handle);
    return kOpenError == ESP_OK;
}

bool writeSnapshot(nvs_handle_t handle, const RuntimeConfigSnapshot &snapshot) {
    if (nvs_set_u8(handle, kKeyAutoReconnect, snapshot.autoReconnectEnabled ? 1U : 0U) != ESP_OK) {
        return false;
    }

    if (nvs_set_u32(handle, kKeyFastReconnectTtlMs, snapshot.fastReconnectTtlMs) != ESP_OK) {
        return false;
    }

    if (nvs_set_u8(handle, kKeyLedMask, snapshot.ledMask) != ESP_OK) {
        return false;
    }

    if (nvs_set_u8(handle, kKeyReportingMode, snapshot.reportingMode) != ESP_OK) {
        return false;
    }

    if (nvs_set_u8(handle, kKeyReportingContinuous, snapshot.reportingContinuous ? 1U : 0U) !=
        ESP_OK) {
        return false;
    }

    return nvs_commit(handle) == ESP_OK;
}

bool readSnapshot(nvs_handle_t handle, RuntimeConfigSnapshot *snapshot) {
    uint8_t autoReconnect = 0U;
    uint32_t fastReconnectTtlMs = 0U;
    uint8_t ledMask = 0U;
    uint8_t reportingMode = 0U;
    uint8_t reportingContinuous = 0U;

    if (nvs_get_u8(handle, kKeyAutoReconnect, &autoReconnect) != ESP_OK) {
        return false;
    }

    if (nvs_get_u32(handle, kKeyFastReconnectTtlMs, &fastReconnectTtlMs) != ESP_OK) {
        return false;
    }

    if (nvs_get_u8(handle, kKeyLedMask, &ledMask) != ESP_OK) {
        return false;
    }

    if (nvs_get_u8(handle, kKeyReportingMode, &reportingMode) != ESP_OK) {
        return false;
    }

    if (nvs_get_u8(handle, kKeyReportingContinuous, &reportingContinuous) != ESP_OK) {
        return false;
    }

    snapshot->autoReconnectEnabled = autoReconnect != 0U;
    snapshot->fastReconnectTtlMs = fastReconnectTtlMs;
    snapshot->ledMask = ledMask;
    snapshot->reportingMode = reportingMode;
    snapshot->reportingContinuous = reportingContinuous != 0U;
    return true;
}

}  // namespace

RuntimeConfigStore::RuntimeConfigStore() : initialized_(false) {}

bool RuntimeConfigStore::init() {
    initialized_ = ensureNvsReady();
    return initialized_;
}

bool RuntimeConfigStore::save(const RuntimeConfigSnapshot &snapshot) const {
    if (!initialized_) {
        return false;
    }

    nvs_handle_t handle = 0;
    if (!openStore(NVS_READWRITE, &handle)) {
        return false;
    }

    const bool kSaved = writeSnapshot(handle, snapshot);
    nvs_close(handle);
    return kSaved;
}

bool RuntimeConfigStore::load(RuntimeConfigSnapshot *snapshot) const {
    if (!initialized_ || snapshot == nullptr) {
        return false;
    }

    nvs_handle_t handle = 0;
    if (!openStore(NVS_READONLY, &handle)) {
        return false;
    }

    const bool kLoaded = readSnapshot(handle, snapshot);
    nvs_close(handle);
    return kLoaded;
}

bool RuntimeConfigStore::clear() const {
    if (!initialized_) {
        return false;
    }

    nvs_handle_t handle = 0;
    if (!openStore(NVS_READWRITE, &handle)) {
        return false;
    }

    const esp_err_t kErrAutoReconnect = nvs_erase_key(handle, kKeyAutoReconnect);
    const esp_err_t kErrFastReconnectTtlMs = nvs_erase_key(handle, kKeyFastReconnectTtlMs);
    const esp_err_t kErrLedMask = nvs_erase_key(handle, kKeyLedMask);
    const esp_err_t kErrReportingMode = nvs_erase_key(handle, kKeyReportingMode);
    const esp_err_t kErrReportingContinuous = nvs_erase_key(handle, kKeyReportingContinuous);

    const bool kEraseOk =
        (kErrAutoReconnect == ESP_OK || kErrAutoReconnect == ESP_ERR_NVS_NOT_FOUND) &&
        (kErrFastReconnectTtlMs == ESP_OK || kErrFastReconnectTtlMs == ESP_ERR_NVS_NOT_FOUND) &&
        (kErrLedMask == ESP_OK || kErrLedMask == ESP_ERR_NVS_NOT_FOUND) &&
        (kErrReportingMode == ESP_OK || kErrReportingMode == ESP_ERR_NVS_NOT_FOUND) &&
        (kErrReportingContinuous == ESP_OK || kErrReportingContinuous == ESP_ERR_NVS_NOT_FOUND);

    const bool kCommitOk = nvs_commit(handle) == ESP_OK;
    nvs_close(handle);

    return kEraseOk && kCommitOk;
}
