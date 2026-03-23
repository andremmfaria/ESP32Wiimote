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
bool mockSetLedsResult = false;
int mockSetLedsCallCount = 0;
uint8_t mockLastLedsMask = 0;
bool mockSetReportingModeResult = false;
int mockSetReportingModeCallCount = 0;
uint8_t mockLastReportingMode = 0;
bool mockLastReportingContinuous = false;
bool mockRequestStatusResult = false;
int mockRequestStatusCallCount = 0;
bool mockWriteMemoryResult = false;
int mockWriteMemoryCallCount = 0;
uint8_t mockLastWriteMemoryAddressSpace = 0;
uint32_t mockLastWriteMemoryOffset = 0;
const uint8_t *mockLastWriteMemoryData = nullptr;
uint8_t mockLastWriteMemoryLen = 0;
bool mockReadMemoryResult = false;
int mockReadMemoryCallCount = 0;
uint8_t mockLastReadMemoryAddressSpace = 0;
uint32_t mockLastReadMemoryOffset = 0;
uint16_t mockLastReadMemorySize = 0;
int mockSetScanEnabledCallCount = 0;
bool mockLastScanEnabled = false;
bool mockStartDiscoveryResult = false;
int mockStartDiscoveryCallCount = 0;
bool mockStopDiscoveryResult = false;
int mockStopDiscoveryCallCount = 0;
bool mockDisconnectResult = false;
int mockDisconnectCallCount = 0;
uint8_t mockLastDisconnectReason = 0;
int mockSetAutoReconnectEnabledCallCount = 0;
bool mockLastAutoReconnectEnabled = false;
int mockClearReconnectCacheCallCount = 0;
BluetoothControllerState mockControllerState = {false, false, false, false, 0, false, false};

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
unsigned long mockMillis = 0UL;
MockEspClass ESP;

char mockSerialInputBuffer[kMockSerialInputBufferSize] = {0};
size_t mockSerialInputLen = 0;
size_t mockSerialInputPos = 0;

char mockSerialOutputBuffer[kMockSerialOutputBufferSize] = {0};
size_t mockSerialOutputLen = 0;

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

bool tinyWiimoteSetLeds(uint8_t ledMask) {
    mockSetLedsCallCount++;
    mockLastLedsMask = ledMask;
    return mockSetLedsResult;
}

bool tinyWiimoteSetReportingMode(uint8_t mode, bool continuous) {
    mockSetReportingModeCallCount++;
    mockLastReportingMode = mode;
    mockLastReportingContinuous = continuous;
    return mockSetReportingModeResult;
}

bool tinyWiimoteRequestStatus() {
    mockRequestStatusCallCount++;
    return mockRequestStatusResult;
}

bool tinyWiimoteWriteMemory(uint8_t addressSpace,
                            uint32_t offset,
                            const uint8_t *data,
                            uint8_t len) {
    mockWriteMemoryCallCount++;
    mockLastWriteMemoryAddressSpace = addressSpace;
    mockLastWriteMemoryOffset = offset;
    mockLastWriteMemoryData = data;
    mockLastWriteMemoryLen = len;
    return mockWriteMemoryResult;
}

bool tinyWiimoteReadMemory(uint8_t addressSpace, uint32_t offset, uint16_t size) {
    mockReadMemoryCallCount++;
    mockLastReadMemoryAddressSpace = addressSpace;
    mockLastReadMemoryOffset = offset;
    mockLastReadMemorySize = size;
    return mockReadMemoryResult;
}

void tinyWiimoteSetScanEnabled(bool enabled) {
    mockSetScanEnabledCallCount++;
    mockLastScanEnabled = enabled;
}

bool tinyWiimoteStartDiscovery() {
    mockStartDiscoveryCallCount++;
    return mockStartDiscoveryResult;
}

bool tinyWiimoteStopDiscovery() {
    mockStopDiscoveryCallCount++;
    return mockStopDiscoveryResult;
}

bool tinyWiimoteDisconnect(uint8_t reason) {
    mockDisconnectCallCount++;
    mockLastDisconnectReason = reason;
    return mockDisconnectResult;
}

void tinyWiimoteSetAutoReconnectEnabled(bool enabled) {
    mockSetAutoReconnectEnabledCallCount++;
    mockLastAutoReconnectEnabled = enabled;
}

void tinyWiimoteClearReconnectCache() {
    mockClearReconnectCacheCallCount++;
}

BluetoothControllerState tinyWiimoteGetControllerState() {
    return mockControllerState;
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
