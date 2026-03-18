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

    UNITY_END();
}

void loop() {}
#endif
