// Test mock implementations for native tests
// Provides TinyWiimote boundary stubs and mock state variables

#include "test_mocks.h"

#include "esp32-hal-bt.h"

#include <string.h>

// Mock state variables with external linkage
bool mockHasData = false;
TinyWiimoteData mockData = {0, {0}, 0};

uint8_t mockLastPacket[256] = {0};
int mockLastPacketLen = 0;
int mockSendCallCount = 0;
uint16_t mockLastChannelHandle = 0;
uint16_t mockLastRemoteCID = 0;

// Additional TinyWiimote boundary mock state
bool mockDeviceIsInited = false;
int mockResetDeviceCallCount = 0;
int mockHandleHciDataCallCount = 0;
int mockTinyWiimoteInitCallCount = 0;
struct TwHciInterface mockLastHciInterface = {nullptr};
bool mockTinyWiimoteConnected = false;
uint8_t mockBatteryLevel = 0;
int mockRequestBatteryUpdateCallCount = 0;
int mockReqAccelerometerCallCount = 0;
bool mockLastReqAccelerometerUse = false;
uint32_t mockLastFastReconnectTtlMs = 0;

// ESP32 BT VHCI mock state
bool gMockVhciSendAvailable = true;
uint8_t gMockVhciSentData[512] = {0};
size_t gMockVhciSentLen = 0;
int gMockVhciSendCount = 0;
esp_err_t mockVhciRegisterResult = ESP_OK;
int mockVhciRegisterCallCount = 0;
esp_vhci_host_callback_t *mockLastVhciCallback = nullptr;

// ESP32 controller / Arduino runtime mock state
bool mockBtStartResult = true;
bool mockBtStarted = false;
uint8_t mockBtControllerStatus = static_cast<uint8_t>(ESP_BT_CONTROLLER_STATUS_IDLE);
size_t mockEspFreeHeap = 0;
MockEspClass ESP;

// FreeRTOS queue creation control
int mockQueueCreateCallCount = 0;
int mockQueueCreateFailOnCall = 0;

// ---- TinyWiimote input boundary mocks ----

int tinyWiimoteAvailable() {
    return mockHasData ? 1 : 0;
}

TinyWiimoteData tinyWiimoteRead() {
    mockHasData = false;
    return mockData;
}

// ---- Additional TinyWiimote stubs (used by hci_callbacks / hci_queue) ----

void tinyWiimoteInit(struct TwHciInterface hciInterface) {
    mockTinyWiimoteInitCallCount++;
    mockLastHciInterface = hciInterface;
}

bool tinyWiimoteDeviceIsInited() {
    return mockDeviceIsInited;
}

void tinyWiimoteResetDevice() {
    mockResetDeviceCallCount++;
}

// NOLINTNEXTLINE(readability-non-const-parameter)
void handleHciData(uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    mockHandleHciDataCallCount++;
}

bool tinyWiimoteIsConnected() {
    return mockTinyWiimoteConnected;
}

uint8_t tinyWiimoteGetBatteryLevel() {
    return mockBatteryLevel;
}

void tinyWiimoteRequestBatteryUpdate() {
    mockRequestBatteryUpdateCallCount++;
}

void tinyWiimoteReqAccelerometer(bool use) {
    mockReqAccelerometerCallCount++;
    mockLastReqAccelerometerUse = use;
}

void tinyWiimoteSetFastReconnectTtlMs(uint32_t ttlMs) {
    mockLastFastReconnectTtlMs = ttlMs;
}

// ---- ESP32 BT VHCI mock implementations ----

bool esp_vhci_host_check_send_available() {
    return gMockVhciSendAvailable;
}

void esp_vhci_host_send_packet(uint8_t *data, size_t len) {
    gMockVhciSendCount++;
    if (len <= sizeof(gMockVhciSentData)) {
        memcpy(gMockVhciSentData, data, len);
        gMockVhciSentLen = len;
    }
}

esp_err_t esp_vhci_host_register_callback(esp_vhci_host_callback_t *callback) {
    mockVhciRegisterCallCount++;
    mockLastVhciCallback = callback;
    return mockVhciRegisterResult;
}

bool btStart() {
    mockBtStarted = mockBtStartResult;
    return mockBtStartResult;
}

bool btStarted() {
    return mockBtStarted;
}

// NOLINTNEXTLINE(readability-identifier-naming)
esp_bt_controller_status_t esp_bt_controller_get_status() {
    return static_cast<esp_bt_controller_status_t>(mockBtControllerStatus);
}
