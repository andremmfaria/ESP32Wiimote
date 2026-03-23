#include "../../../src/ESP32Wiimote.h"
#include "../../mocks/test_mocks.h"

#include <type_traits>
#include <unity.h>

void setUp(void) {
    mockBtStartResult = true;
    mockBtStarted = false;
    mockBtControllerStatus = static_cast<uint8_t>(ESP_BT_CONTROLLER_STATUS_IDLE);
    mockEspFreeHeap = 8192U;
    mockTinyWiimoteInitCallCount = 0;
    mockTinyWiimoteConnected = false;
    mockBatteryLevel = 0;
    mockRequestBatteryUpdateCallCount = 0;
    mockReqAccelerometerCallCount = 0;
    mockLastReqAccelerometerUse = true;
    mockLastFastReconnectTtlMs = 0;
    mockSetLedsResult = false;
    mockSetLedsCallCount = 0;
    mockLastLedsMask = 0;
    mockSetReportingModeResult = false;
    mockSetReportingModeCallCount = 0;
    mockLastReportingMode = 0;
    mockLastReportingContinuous = false;
    mockRequestStatusResult = false;
    mockRequestStatusCallCount = 0;
    mockWriteMemoryResult = false;
    mockWriteMemoryCallCount = 0;
    mockLastWriteMemoryAddressSpace = 0;
    mockLastWriteMemoryOffset = 0;
    mockLastWriteMemoryData = nullptr;
    mockLastWriteMemoryLen = 0;
    mockReadMemoryResult = false;
    mockReadMemoryCallCount = 0;
    mockLastReadMemoryAddressSpace = 0;
    mockLastReadMemoryOffset = 0;
    mockLastReadMemorySize = 0;
    mockSetScanEnabledCallCount = 0;
    mockLastScanEnabled = false;
    mockStartDiscoveryResult = false;
    mockStartDiscoveryCallCount = 0;
    mockStopDiscoveryResult = false;
    mockStopDiscoveryCallCount = 0;
    mockDisconnectResult = false;
    mockDisconnectCallCount = 0;
    mockLastDisconnectReason = 0;
    mockSetAutoReconnectEnabledCallCount = 0;
    mockLastAutoReconnectEnabled = false;
    mockClearReconnectCacheCallCount = 0;
    mockControllerState = {false, false, false, false, 0, false, false};
    mockHandleHciDataCallCount = 0;
    mockHasData = false;
    mockData = {0, {0}, 0};
    gMockVhciSendCount = 0;
    gMockVhciSentLen = 0;
    mockVhciRegisterResult = ESP_OK;
    mockVhciRegisterCallCount = 0;
    mockLastVhciCallback = nullptr;
    mockQueueCreateCallCount = 0;
    mockQueueCreateFailOnCall = 0;
    mockSerialClearInput();
    mockSerialClearOutput();
    HciCallbacksHandler::setQueueManager(nullptr);
    wiimoteSetLogLevel(kWiimoteLogWarning);
}

void tearDown(void) {}

void testESP32WiimoteDefaultConstructorBuilds() {
    ESP32Wiimote device;
    ESP32Wiimote::setLogLevel(kWiimoteLogError);
    TEST_ASSERT_EQUAL_UINT8(kWiimoteLogError, ESP32Wiimote::getLogLevel());
}

void testESP32WiimoteInitFailureDoesNotSetFastReconnectTtl() {
    ESP32WiimoteConfig config;
    config.fastReconnectTtlMs = 1234U;
    ESP32Wiimote device(config);

    mockBtStartResult = false;

    TEST_ASSERT_FALSE(device.init());
    TEST_ASSERT_EQUAL_UINT32(0U, mockLastFastReconnectTtlMs);
}

void testESP32WiimoteInitSuccessProcessesQueuesDuringTask() {
    ESP32WiimoteConfig config;
    config.fastReconnectTtlMs = 3210U;
    ESP32Wiimote device(config);

    TEST_ASSERT_TRUE(device.init());
    TEST_ASSERT_EQUAL_UINT32(config.fastReconnectTtlMs, mockLastFastReconnectTtlMs);

    uint8_t txPacket[] = {0x01, 0x02, 0x03};
    uint8_t rxPacket[] = {0x04, 0x05, 0x06};
    HciCallbacksHandler::hciHostSendPacket(txPacket, sizeof(txPacket));
    TEST_ASSERT_EQUAL(ESP_OK, HciCallbacksHandler::notifyHostRecv(rxPacket, sizeof(rxPacket)));

    device.task();

    TEST_ASSERT_EQUAL(1, gMockVhciSendCount);
    TEST_ASSERT_EQUAL(1, mockHandleHciDataCallCount);
}

void testESP32WiimoteTaskSkipsQueuesWhenControllerStopped() {
    ESP32Wiimote device;

    device.task();

    TEST_ASSERT_EQUAL(0, gMockVhciSendCount);
    TEST_ASSERT_EQUAL(0, mockHandleHciDataCallCount);
}

void testESP32WiimoteAvailableAndStateAccessorsUseDataParser() {
    ESP32Wiimote device;

    mockHasData = true;
    mockData.len = 7;
    mockData.data[0] = 0xA1;
    mockData.data[1] = 0x31;
    mockData.data[2] = 0x00;
    mockData.data[3] = 0x08;
    mockData.data[4] = 0x10;
    mockData.data[5] = 0x20;
    mockData.data[6] = 0x30;

    TEST_ASSERT_EQUAL(1, device.available());
    TEST_ASSERT_TRUE(buttonStateHas(device.getButtonState(), kButtonA));

    AccelState accel = device.getAccelState();
    TEST_ASSERT_EQUAL_UINT8(0x10, accel.xAxis);
    TEST_ASSERT_EQUAL_UINT8(0x20, accel.yAxis);
    TEST_ASSERT_EQUAL_UINT8(0x30, accel.zAxis);

    NunchukState nunchuk = device.getNunchukState();
    TEST_ASSERT_EQUAL_UINT8(0x00, nunchuk.xStick);
    TEST_ASSERT_EQUAL_UINT8(0x00, nunchuk.yStick);
}

void testESP32WiimoteStaticDelegationMethodsCallTinyWiimoteLayer() {
    ESP32Wiimote device;
    mockTinyWiimoteConnected = true;
    mockBatteryLevel = 87;

    TEST_ASSERT_TRUE(device.isConnected());
    TEST_ASSERT_EQUAL_UINT8(87, device.getBatteryLevel());

    ESP32Wiimote::requestBatteryUpdate();
    TEST_ASSERT_EQUAL(1, mockRequestBatteryUpdateCallCount);

    ESP32Wiimote::setLogLevel(kWiimoteLogInfo);
    TEST_ASSERT_EQUAL_UINT8(kWiimoteLogInfo, ESP32Wiimote::getLogLevel());
}

void testESP32WiimoteAddFilterUpdatesParserAndAccelRequest() {
    ESP32Wiimote device;

    device.addFilter(FilterAction::Ignore, kFilterButton);
    TEST_ASSERT_EQUAL(0, mockReqAccelerometerCallCount);

    device.addFilter(FilterAction::Ignore, kFilterAccel);
    TEST_ASSERT_EQUAL(1, mockReqAccelerometerCallCount);
    TEST_ASSERT_FALSE(mockLastReqAccelerometerUse);
}

void testESP32WiimotePublicControllerTypesHaveExpectedShape() {
    static_assert(
        std::is_same<uint8_t, std::underlying_type<ESP32Wiimote::DisconnectReason>::type>::value,
        "DisconnectReason must keep uint8_t ABI");

    TEST_ASSERT_EQUAL_UINT8(
        0x16, static_cast<uint8_t>(ESP32Wiimote::DisconnectReason::LocalHostTerminated));
    TEST_ASSERT_EQUAL_UINT8(
        0x13, static_cast<uint8_t>(ESP32Wiimote::DisconnectReason::RemoteUserTerminated));
    TEST_ASSERT_EQUAL_UINT8(
        0x05, static_cast<uint8_t>(ESP32Wiimote::DisconnectReason::AuthenticationFailure));
    TEST_ASSERT_EQUAL_UINT8(0x15, static_cast<uint8_t>(ESP32Wiimote::DisconnectReason::PowerOff));

    ESP32Wiimote::BluetoothControllerState state = {true, false, true, false, 0x1234, true, false};
    TEST_ASSERT_TRUE(state.initialized);
    TEST_ASSERT_FALSE(state.started);
    TEST_ASSERT_TRUE(state.scanning);
    TEST_ASSERT_FALSE(state.connected);
    TEST_ASSERT_EQUAL_UINT16(0x1234, state.activeConnectionHandle);
    TEST_ASSERT_TRUE(state.fastReconnectActive);
    TEST_ASSERT_FALSE(state.autoReconnectEnabled);
}

void testESP32WiimoteOutputMethodsDelegateAndPropagateResults() {
    ESP32Wiimote device;

    mockSetLedsResult = true;
    TEST_ASSERT_TRUE(device.setLeds(0x0A));
    TEST_ASSERT_EQUAL(1, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x0A, mockLastLedsMask);

    mockSetReportingModeResult = true;
    TEST_ASSERT_TRUE(device.setReportingMode(ReportingMode::CoreButtonsAccel, true));
    TEST_ASSERT_EQUAL(1, mockSetReportingModeCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x31, mockLastReportingMode);
    TEST_ASSERT_TRUE(mockLastReportingContinuous);

    TEST_ASSERT_TRUE(device.setAccelerometerEnabled(false));
    TEST_ASSERT_EQUAL(1, mockReqAccelerometerCallCount);
    TEST_ASSERT_FALSE(mockLastReqAccelerometerUse);

    mockRequestStatusResult = true;
    TEST_ASSERT_TRUE(device.requestStatus());
    TEST_ASSERT_EQUAL(1, mockRequestStatusCallCount);

    uint8_t payload[] = {0xAA, 0xBB, 0xCC};
    mockWriteMemoryResult = false;
    TEST_ASSERT_FALSE(device.writeMemory(0x04, 0x12345678U, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL(1, mockWriteMemoryCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x04, mockLastWriteMemoryAddressSpace);
    TEST_ASSERT_EQUAL_UINT32(0x12345678U, mockLastWriteMemoryOffset);
    TEST_ASSERT_EQUAL_PTR(payload, mockLastWriteMemoryData);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), mockLastWriteMemoryLen);

    mockReadMemoryResult = true;
    TEST_ASSERT_TRUE(device.readMemory(0x00, 0x000FA0BCU, 32U));
    TEST_ASSERT_EQUAL(1, mockReadMemoryCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastReadMemoryAddressSpace);
    TEST_ASSERT_EQUAL_UINT32(0x000FA0BCU, mockLastReadMemoryOffset);
    TEST_ASSERT_EQUAL_UINT16(32U, mockLastReadMemorySize);
}

void testESP32WiimoteControllerMethodsDelegateAndMapState() {
    ESP32Wiimote device;

    device.setScanEnabled(true);
    TEST_ASSERT_EQUAL(1, mockSetScanEnabledCallCount);
    TEST_ASSERT_TRUE(mockLastScanEnabled);

    mockStartDiscoveryResult = true;
    TEST_ASSERT_TRUE(device.startDiscovery());
    TEST_ASSERT_EQUAL(1, mockStartDiscoveryCallCount);

    mockStopDiscoveryResult = false;
    TEST_ASSERT_FALSE(device.stopDiscovery());
    TEST_ASSERT_EQUAL(1, mockStopDiscoveryCallCount);

    mockDisconnectResult = true;
    TEST_ASSERT_TRUE(
        device.disconnectActiveController(ESP32Wiimote::DisconnectReason::AuthenticationFailure));
    TEST_ASSERT_EQUAL(1, mockDisconnectCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x05, mockLastDisconnectReason);

    device.setAutoReconnectEnabled(true);
    TEST_ASSERT_EQUAL(1, mockSetAutoReconnectEnabledCallCount);
    TEST_ASSERT_TRUE(mockLastAutoReconnectEnabled);

    device.clearReconnectCache();
    TEST_ASSERT_EQUAL(1, mockClearReconnectCacheCallCount);

    mockControllerState = {true, true, true, true, 0xBEEF, true, true};
    ESP32Wiimote::BluetoothControllerState state = device.getBluetoothControllerState();
    TEST_ASSERT_TRUE(state.initialized);
    TEST_ASSERT_TRUE(state.started);
    TEST_ASSERT_TRUE(state.scanning);
    TEST_ASSERT_TRUE(state.connected);
    TEST_ASSERT_EQUAL_UINT16(0xBEEF, state.activeConnectionHandle);
    TEST_ASSERT_TRUE(state.fastReconnectActive);
    TEST_ASSERT_TRUE(state.autoReconnectEnabled);
}

void testESP32WiimoteSerialControlDisabledByDefaultAndOptIn() {
    ESP32Wiimote device;

    TEST_ASSERT_FALSE(device.isSerialControlEnabled());

    device.enableSerialControl(true);
    TEST_ASSERT_TRUE(device.isSerialControlEnabled());

    device.enableSerialControl(false);
    TEST_ASSERT_FALSE(device.isSerialControlEnabled());
}

void testESP32WiimoteSerialControlProcessesOneLinePerTaskCall() {
    ESP32Wiimote device;
    mockBtStarted = true;
    mockTinyWiimoteConnected = true;
    mockSetLedsResult = true;

    device.enableSerialControl(true);
    mockSerialSetInput("wm led 0x01\nwm led 0x02\n");

    device.task();
    TEST_ASSERT_EQUAL(1, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x01, mockLastLedsMask);
    TEST_ASSERT_EQUAL_STRING("@wm: ok\n", mockSerialGetOutput());

    device.task();
    TEST_ASSERT_EQUAL(2, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x02, mockLastLedsMask);
    TEST_ASSERT_EQUAL_STRING("@wm: ok\n@wm: ok\n", mockSerialGetOutput());
}

void testESP32WiimoteSerialControlIgnoresNonCommandInput() {
    ESP32Wiimote device;
    mockBtStarted = true;

    device.enableSerialControl(true);
    mockSerialSetInput("hello world\n");

    device.task();

    TEST_ASSERT_EQUAL(0, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_STRING("", mockSerialGetOutput());
}

void testESP32WiimoteSerialControlReportsLineTooLong() {
    ESP32Wiimote device;
    mockBtStarted = true;

    char line[kSerialMaxLineLength + 8U];
    memset(line, 'a', sizeof(line));
    line[0] = 'w';
    line[1] = 'm';
    line[2] = ' ';
    line[kSerialMaxLineLength + 6U] = '\n';
    line[kSerialMaxLineLength + 7U] = '\0';

    device.enableSerialControl(true);
    mockSerialSetInput(line);

    device.task();

    TEST_ASSERT_EQUAL_STRING("@wm: error line_too_long\n", mockSerialGetOutput());
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testESP32WiimoteDefaultConstructorBuilds);
    RUN_TEST(testESP32WiimoteInitFailureDoesNotSetFastReconnectTtl);
    RUN_TEST(testESP32WiimoteInitSuccessProcessesQueuesDuringTask);
    RUN_TEST(testESP32WiimoteTaskSkipsQueuesWhenControllerStopped);
    RUN_TEST(testESP32WiimoteAvailableAndStateAccessorsUseDataParser);
    RUN_TEST(testESP32WiimoteStaticDelegationMethodsCallTinyWiimoteLayer);
    RUN_TEST(testESP32WiimoteAddFilterUpdatesParserAndAccelRequest);
    RUN_TEST(testESP32WiimotePublicControllerTypesHaveExpectedShape);
    RUN_TEST(testESP32WiimoteOutputMethodsDelegateAndPropagateResults);
    RUN_TEST(testESP32WiimoteControllerMethodsDelegateAndMapState);
    RUN_TEST(testESP32WiimoteSerialControlDisabledByDefaultAndOptIn);
    RUN_TEST(testESP32WiimoteSerialControlProcessesOneLinePerTaskCall);
    RUN_TEST(testESP32WiimoteSerialControlIgnoresNonCommandInput);
    RUN_TEST(testESP32WiimoteSerialControlReportsLineTooLong);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testESP32WiimoteDefaultConstructorBuilds);
    RUN_TEST(testESP32WiimoteInitFailureDoesNotSetFastReconnectTtl);
    RUN_TEST(testESP32WiimoteInitSuccessProcessesQueuesDuringTask);
    RUN_TEST(testESP32WiimoteTaskSkipsQueuesWhenControllerStopped);
    RUN_TEST(testESP32WiimoteAvailableAndStateAccessorsUseDataParser);
    RUN_TEST(testESP32WiimoteStaticDelegationMethodsCallTinyWiimoteLayer);
    RUN_TEST(testESP32WiimoteAddFilterUpdatesParserAndAccelRequest);
    RUN_TEST(testESP32WiimotePublicControllerTypesHaveExpectedShape);
    RUN_TEST(testESP32WiimoteOutputMethodsDelegateAndPropagateResults);
    RUN_TEST(testESP32WiimoteControllerMethodsDelegateAndMapState);
    RUN_TEST(testESP32WiimoteSerialControlDisabledByDefaultAndOptIn);
    RUN_TEST(testESP32WiimoteSerialControlProcessesOneLinePerTaskCall);
    RUN_TEST(testESP32WiimoteSerialControlIgnoresNonCommandInput);
    RUN_TEST(testESP32WiimoteSerialControlReportsLineTooLong);

    UNITY_END();
}

void loop() {}
#endif
