#include "../../../src/ESP32Wiimote.h"
#include "../../../src/config/runtime_config_store.h"
#include "../../mocks/test_mocks.h"

#include <cstring>
#include <type_traits>
#include <unity.h>

namespace {

class FakeWifiHttpServer : public WifiHttpServer {
   public:
    void reset() {
        beginResult_ = true;
        beginCallCount_ = 0;
        endCallCount_ = 0;
        endDuringDispatchCallCount_ = 0;
        pollCallCount_ = 0;
        started_ = false;
        dispatchingRequest_ = false;
        handler_ = nullptr;
        userData_ = nullptr;
        lastStartError_ = WifiHttpServerStartError::None;
    }

    void setBeginResult(bool result) { beginResult_ = result; }

    int beginCallCount() const { return beginCallCount_; }

    int endCallCount() const { return endCallCount_; }

    int endDuringDispatchCallCount() const { return endDuringDispatchCallCount_; }

    int pollCallCount() const { return pollCallCount_; }

    bool dispatchRequest(const char *method,
                         const char *path,
                         const char *authHeader,
                         const char *body,
                         int *status,
                         const char **contentType,
                         char *responseBuf,
                         size_t responseBufSize) {
        if (!started_ || handler_ == nullptr || responseBuf == nullptr || responseBufSize == 0U) {
            return false;
        }

        WifiHttpResponse response = {};
        const WifiHttpRequest kRequest = {parseMethod(method), path, authHeader, body,
                                          body != nullptr ? std::strlen(body) : 0U};
        dispatchingRequest_ = true;
        handler_(&kRequest, responseBuf, responseBufSize, &response, userData_);
        dispatchingRequest_ = false;

        if (status != nullptr) {
            *status = response.status;
        }
        if (contentType != nullptr) {
            *contentType = response.contentType;
        }

        return true;
    }

    void setHandler(wifi_http_request_handler_fn handler, void *userData) override {
        handler_ = handler;
        userData_ = userData;
    }

    bool begin(uint16_t /*port*/) override {
        if (started_) {
            lastStartError_ = WifiHttpServerStartError::None;
            return true;
        }

        if (handler_ == nullptr) {
            lastStartError_ = WifiHttpServerStartError::MissingHandler;
            return false;
        }

        ++beginCallCount_;
        if (!beginResult_) {
            lastStartError_ = WifiHttpServerStartError::BackendUnavailable;
            return false;
        }

        started_ = true;
        lastStartError_ = WifiHttpServerStartError::None;
        return true;
    }

    void end() override {
        if (!started_) {
            return;
        }

        if (dispatchingRequest_) {
            ++endDuringDispatchCallCount_;
        }
        ++endCallCount_;
        started_ = false;
        handler_ = nullptr;
        userData_ = nullptr;
    }

    void poll() const override {
        if (!started_) {
            return;
        }

        ++pollCallCount_;
    }

    bool isStarted() const override { return started_; }

    bool isDispatchingRequest() const override { return dispatchingRequest_; }

    WifiHttpServerStartError lastStartError() const override { return lastStartError_; }

   private:
    static WifiHttpMethod parseMethod(const char *method) {
        if (method == nullptr) {
            return WifiHttpMethod::Unsupported;
        }
        if (std::strcmp(method, "GET") == 0) {
            return WifiHttpMethod::Get;
        }
        if (std::strcmp(method, "POST") == 0) {
            return WifiHttpMethod::Post;
        }
        return WifiHttpMethod::Unsupported;
    }

    wifi_http_request_handler_fn handler_ = nullptr;
    void *userData_ = nullptr;
    mutable int pollCallCount_ = 0;
    int beginCallCount_ = 0;
    int endCallCount_ = 0;
    int endDuringDispatchCallCount_ = 0;
    bool beginResult_ = true;
    bool started_ = false;
    bool dispatchingRequest_ = false;
    WifiHttpServerStartError lastStartError_ = WifiHttpServerStartError::None;
};

FakeWifiHttpServer gFakeWifiHttpServer;

class TestESP32Wiimote : public ::ESP32Wiimote {
   public:
    TestESP32Wiimote() : ::ESP32Wiimote(ESP32WiimoteConfig(), &gFakeWifiHttpServer) {}

    explicit TestESP32Wiimote(const ESP32WiimoteConfig &config)
        : ::ESP32Wiimote(config, &gFakeWifiHttpServer) {}
};

void wifiHttpServerMockReset() {
    gFakeWifiHttpServer.reset();
}

void wifiHttpServerMockSetBeginResult(bool result) {
    gFakeWifiHttpServer.setBeginResult(result);
}

int wifiHttpServerMockGetBeginCallCount() {
    return gFakeWifiHttpServer.beginCallCount();
}

int wifiHttpServerMockGetEndCallCount() {
    return gFakeWifiHttpServer.endCallCount();
}

int wifiHttpServerMockGetEndDuringDispatchCallCount() {
    return gFakeWifiHttpServer.endDuringDispatchCallCount();
}

int wifiHttpServerMockGetPollCallCount() {
    return gFakeWifiHttpServer.pollCallCount();
}

bool wifiHttpServerMockDispatchRequest(const char *method,
                                       const char *path,
                                       const char *authHeader,
                                       const char *body,
                                       int *status,
                                       const char **contentType,
                                       char *responseBuf,
                                       size_t responseBufSize) {
    return gFakeWifiHttpServer.dispatchRequest(method, path, authHeader, body, status, contentType,
                                               responseBuf, responseBufSize);
}

}  // namespace

void setUp(void) {
    RuntimeConfigStore nvsReset;
    nvsReset.init();
    nvsReset.clear();
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
    mockSetMillis(0UL);
    mockSerialClearInput();
    mockSerialClearOutput();
    wifiHttpServerMockReset();
    HciCallbacksHandler::setQueueManager(nullptr);
    wiimoteSetLogLevel(kWiimoteLogWarning);
}

void tearDown(void) {}

void testESP32WiimoteDefaultConstructorBuilds() {
    TestESP32Wiimote device;
    ESP32Wiimote::setLogLevel(kWiimoteLogError);
    TEST_ASSERT_EQUAL_UINT8(kWiimoteLogError, ESP32Wiimote::getLogLevel());
}

void testESP32WiimoteGetConfigReflectsRuntimeAuthAndWifiSettings() {
    ESP32WiimoteConfig config;
    config.auth.serialPrivilegedToken = "serial_token";
    config.auth.wifiApiToken = "wifi_token";
    config.wifi.enabled = true;
    config.wifi.deliveryMode = WifiDeliveryMode::RestAndWebSocket;
    config.wifi.network = {"ssid", "password"};

    TestESP32Wiimote device;
    device.configure(config);

    const ESP32WiimoteConfig &current = device.getConfig();
    TEST_ASSERT_TRUE(current.wifi.enabled);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(WifiDeliveryMode::RestAndWebSocket),
                            static_cast<uint8_t>(current.wifi.deliveryMode));
    TEST_ASSERT_EQUAL_STRING("serial_token", current.auth.serialPrivilegedToken);
    TEST_ASSERT_EQUAL_STRING("wifi_token", current.auth.wifiApiToken);
    TEST_ASSERT_EQUAL_STRING("ssid", current.wifi.network.ssid);
    TEST_ASSERT_EQUAL_STRING("password", current.wifi.network.password);
}

void testESP32WiimoteConfigurePartialConfigPreservesPreviousAuthAndWifiValues() {
    ESP32WiimoteConfig initial;
    initial.auth.serialPrivilegedToken = "serial_old";
    initial.auth.wifiApiToken = "wifi_old";
    initial.wifi.enabled = true;
    initial.wifi.deliveryMode = WifiDeliveryMode::RestAndWebSocket;
    initial.wifi.network = {"ssid_old", "password_old"};

    TestESP32Wiimote device(initial);

    ESP32WiimoteConfig partial;
    partial.auth.serialPrivilegedToken = "serial_new";
    device.configure(partial);

    const ESP32WiimoteConfig &current = device.getConfig();
    TEST_ASSERT_EQUAL_STRING("serial_new", current.auth.serialPrivilegedToken);
    TEST_ASSERT_EQUAL_STRING("wifi_old", current.auth.wifiApiToken);
    TEST_ASSERT_TRUE(current.wifi.enabled);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(WifiDeliveryMode::RestAndWebSocket),
                            static_cast<uint8_t>(current.wifi.deliveryMode));
    TEST_ASSERT_EQUAL_STRING("ssid_old", current.wifi.network.ssid);
    TEST_ASSERT_EQUAL_STRING("password_old", current.wifi.network.password);
}

void testESP32WiimoteInitFailureDoesNotSetFastReconnectTtl() {
    ESP32WiimoteConfig config;
    config.fastReconnectTtlMs = 1234U;
    TestESP32Wiimote device(config);

    mockBtStartResult = false;

    TEST_ASSERT_FALSE(device.init());
    TEST_ASSERT_EQUAL_UINT32(0U, mockLastFastReconnectTtlMs);
}

void testESP32WiimoteInitSuccessProcessesQueuesDuringTask() {
    ESP32WiimoteConfig config;
    config.fastReconnectTtlMs = 3210U;
    TestESP32Wiimote device(config);

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
    TestESP32Wiimote device;

    device.task();

    TEST_ASSERT_EQUAL(0, gMockVhciSendCount);
    TEST_ASSERT_EQUAL(0, mockHandleHciDataCallCount);
}

void testESP32WiimoteAvailableAndStateAccessorsUseDataParser() {
    TestESP32Wiimote device;

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
    TestESP32Wiimote device;
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
    TestESP32Wiimote device;

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
    TestESP32Wiimote device;

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
    TEST_ASSERT_TRUE(mockLastWriteMemoryData == payload);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), mockLastWriteMemoryLen);

    mockReadMemoryResult = true;
    TEST_ASSERT_TRUE(device.readMemory(0x00, 0x000FA0BCU, 32U));
    TEST_ASSERT_EQUAL(1, mockReadMemoryCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastReadMemoryAddressSpace);
    TEST_ASSERT_EQUAL_UINT32(0x000FA0BCU, mockLastReadMemoryOffset);
    TEST_ASSERT_EQUAL_UINT16(32U, mockLastReadMemorySize);
}

void testESP32WiimoteControllerMethodsDelegateAndMapState() {
    TestESP32Wiimote device;

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
    TestESP32Wiimote device;

    TEST_ASSERT_FALSE(device.isSerialControlEnabled());

    device.enableSerialControl(true);
    TEST_ASSERT_TRUE(device.isSerialControlEnabled());

    device.enableSerialControl(false);
    TEST_ASSERT_FALSE(device.isSerialControlEnabled());
}

void testESP32WiimoteSerialControlProcessesOneLinePerTaskCall() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = false;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    TestESP32Wiimote device(config);
    mockBtStarted = true;
    mockTinyWiimoteConnected = true;
    mockSetLedsResult = true;

    device.enableSerialControl(true);
    mockSerialSetInput("wm unlock token 60\nwm led 0x01\n");

    device.task();
    TEST_ASSERT_EQUAL_STRING("@wm: ok\n", mockSerialGetOutput());
    TEST_ASSERT_EQUAL(0, mockSetLedsCallCount);

    device.task();
    TEST_ASSERT_EQUAL(1, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x01, mockLastLedsMask);
    TEST_ASSERT_EQUAL_STRING("@wm: ok\n@wm: ok\n", mockSerialGetOutput());
}

void testESP32WiimoteSerialControlPrivilegedCommandIsLockedByDefault() {
    TestESP32Wiimote device;
    mockBtStarted = true;
    mockTinyWiimoteConnected = true;
    mockSetLedsResult = true;

    device.enableSerialControl(true);
    mockSerialSetInput("wm led 0x01\n");

    device.task();

    TEST_ASSERT_EQUAL(0, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_STRING("@wm: error locked\n", mockSerialGetOutput());
}

void testESP32WiimoteSerialControlUnlockExpiresByTime() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = false;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    TestESP32Wiimote device(config);
    mockBtStarted = true;
    mockTinyWiimoteConnected = true;
    mockSetLedsResult = true;

    device.enableSerialControl(true);

    mockSetMillis(1000UL);
    mockSerialSetInput("wm unlock token 1\nwm led 0x01\n");

    device.task();
    TEST_ASSERT_EQUAL_STRING("@wm: ok\n", mockSerialGetOutput());

    mockSetMillis(1500UL);
    device.task();
    TEST_ASSERT_EQUAL(1, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_STRING("@wm: ok\n@wm: ok\n", mockSerialGetOutput());

    mockSetMillis(2500UL);
    mockSerialSetInput("wm led 0x02\n");
    device.task();

    TEST_ASSERT_EQUAL(1, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_STRING("@wm: ok\n@wm: ok\n@wm: error locked\n", mockSerialGetOutput());
}

void testESP32WiimoteConfigureWithWifiDisabledRejectsBadCredentials() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = false;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    TestESP32Wiimote device(config);

    mockBtStarted = true;
    device.enableSerialControl(true);
    mockSerialSetInput("wm unlock wrong 60\n");

    device.task();

    TEST_ASSERT_EQUAL_STRING("@wm: error bad_credentials\n", mockSerialGetOutput());
}

void testESP32WiimoteConfigurePropagatesCredentialsToSerialUnlock() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = false;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    TestESP32Wiimote device(config);

    mockBtStarted = true;
    mockTinyWiimoteConnected = true;
    mockSetLedsResult = true;
    device.enableSerialControl(true);
    mockSerialSetInput("wm unlock token 60\nwm led 0x01\n");

    device.task();
    device.task();

    TEST_ASSERT_EQUAL(1, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_STRING("@wm: ok\n@wm: ok\n", mockSerialGetOutput());
}

void testESP32WiimoteWifiControlDisabledByDefault() {
    TestESP32Wiimote device;

    TEST_ASSERT_FALSE(device.isWifiControlEnabled());
    TEST_ASSERT_FALSE(device.isWifiControlReady());

    const ESP32Wiimote::WifiControlState kState = device.getWifiControlState();
    TEST_ASSERT_FALSE(kState.enabled);
    TEST_ASSERT_FALSE(kState.ready);
    TEST_ASSERT_FALSE(kState.networkCredentialsConfigured);
    TEST_ASSERT_FALSE(kState.networkConnectAttempted);
    TEST_ASSERT_FALSE(kState.networkConnected);
    TEST_ASSERT_FALSE(kState.networkConnectFailed);
    TEST_ASSERT_FALSE(kState.wifiLayerStarted);
    TEST_ASSERT_FALSE(kState.serverStarted);
    TEST_ASSERT_FALSE(kState.serverBindFailed);
}

void testESP32WiimoteWifiControlCanBeEnabledAtRuntimeWhenConfigStartsDisabled() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = false;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);

    TEST_ASSERT_TRUE(device.isWifiControlEnabled());
    TEST_ASSERT_FALSE(device.isWifiControlReady());
}

void testESP32WiimoteWifiControlAsyncLifecycleRestOnly() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);

    TEST_ASSERT_TRUE(device.isWifiControlEnabled());
    TEST_ASSERT_FALSE(device.isWifiControlReady());
    TEST_ASSERT_TRUE(device.getWifiControlState().networkCredentialsConfigured);

    device.task();
    TEST_ASSERT_TRUE(device.getWifiControlState().networkConnectAttempted);
    TEST_ASSERT_TRUE(device.getWifiControlState().networkConnected);
    TEST_ASSERT_FALSE(device.getWifiControlState().networkConnectFailed);
    TEST_ASSERT_TRUE(device.getWifiControlState().wifiLayerStarted);
    TEST_ASSERT_FALSE(device.getWifiControlState().staticRoutesRegistered);

    device.task();
    TEST_ASSERT_TRUE(device.getWifiControlState().staticRoutesRegistered);
    TEST_ASSERT_FALSE(device.getWifiControlState().apiRoutesRegistered);

    device.task();
    TEST_ASSERT_TRUE(device.getWifiControlState().apiRoutesRegistered);
    TEST_ASSERT_FALSE(device.getWifiControlState().websocketRoutesRegistered);

    device.task();
    TEST_ASSERT_TRUE(device.isWifiControlReady());
    TEST_ASSERT_FALSE(device.getWifiControlState().websocketRoutesRegistered);
    TEST_ASSERT_TRUE(device.getWifiControlState().serverStarted);
    TEST_ASSERT_FALSE(device.getWifiControlState().serverBindFailed);
}

void testESP32WiimoteWifiControlStartsHttpServerAndRoutesRequests() {
    static const size_t kResponseBufSize = 8192U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);

    for (int i = 0; i < 5; ++i) {
        device.task();
    }

    TEST_ASSERT_TRUE(device.isWifiControlReady());
    TEST_ASSERT_TRUE(device.getWifiControlState().serverStarted);
    TEST_ASSERT_FALSE(device.getWifiControlState().serverBindFailed);
    TEST_ASSERT_EQUAL(1, wifiHttpServerMockGetBeginCallCount());

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;

    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest("GET", "/openapi.json", nullptr, nullptr,
                                                       &status, &contentType, responseBuf,
                                                       sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_EQUAL_STRING("application/json", contentType);
    TEST_ASSERT_NOT_NULL(std::strstr(responseBuf, "\"openapi\":\"3.0.3\""));

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest("GET", "/app.js", nullptr, nullptr, &status,
                                                       &contentType, responseBuf,
                                                       sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_EQUAL_STRING("application/javascript", contentType);
    TEST_ASSERT_NOT_NULL(std::strstr(responseBuf, "fetchWithAuth"));

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest("GET", "/api/wifi/control", "Bearer token",
                                                       nullptr, &status, &contentType, responseBuf,
                                                       sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_NOT_NULL(std::strstr(responseBuf, "\"serverStarted\":true"));
}

void testESP32WiimoteWifiControlHttpBridgePreservesUnauthorizedContract() {
    static const size_t kResponseBufSize = 512U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    for (int i = 0; i < 5; ++i) {
        device.task();
    }

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;

    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest("GET", "/api/wifi/control", nullptr, nullptr,
                                                       &status, &contentType, responseBuf,
                                                       sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(401, status);
    TEST_ASSERT_EQUAL_STRING("application/json", contentType);
    TEST_ASSERT_NOT_NULL(std::strstr(responseBuf, "unauthorized"));
}

void testESP32WiimoteWifiControlHttpBridgeMapsUnsupportedMethodToNotFound() {
    static const size_t kResponseBufSize = 512U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    for (int i = 0; i < 5; ++i) {
        device.task();
    }

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;

    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest("PATCH", "/api/wifi/control", "Bearer token",
                                                       nullptr, &status, &contentType, responseBuf,
                                                       sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(404, status);
    TEST_ASSERT_EQUAL_STRING("application/json", contentType);
    TEST_ASSERT_NOT_NULL(std::strstr(responseBuf, "not found"));
}

void testESP32WiimoteWifiControlReportsHttpBindFailure() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    wifiHttpServerMockSetBeginResult(false);
    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);

    for (int i = 0; i < 4; ++i) {
        device.task();
    }

    const ESP32Wiimote::WifiControlState kState = device.getWifiControlState();
    TEST_ASSERT_FALSE(kState.enabled);
    TEST_ASSERT_FALSE(kState.ready);
    TEST_ASSERT_FALSE(kState.serverStarted);
    TEST_ASSERT_TRUE(kState.serverBindFailed);
    TEST_ASSERT_EQUAL(1, wifiHttpServerMockGetBeginCallCount());
}

void testESP32WiimoteDisablingWifiStopsHttpServer() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    for (int i = 0; i < 5; ++i) {
        device.task();
    }

    TEST_ASSERT_TRUE(device.getWifiControlState().serverStarted);
    device.enableWifiControl(false, WifiDeliveryMode::RestOnly);

    TEST_ASSERT_FALSE(device.isWifiControlEnabled());
    TEST_ASSERT_FALSE(device.getWifiControlState().serverStarted);
    TEST_ASSERT_EQUAL(1, wifiHttpServerMockGetEndCallCount());
}

void testESP32WiimoteWifiControlRestAndWebSocketAddsWebSocketStage() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestAndWebSocket);

    for (int i = 0; i < 4; ++i) {
        device.task();
    }

    TEST_ASSERT_TRUE(device.getWifiControlState().apiRoutesRegistered);
    TEST_ASSERT_TRUE(device.getWifiControlState().websocketRoutesRegistered);
    TEST_ASSERT_FALSE(device.isWifiControlReady());

    device.task();
    TEST_ASSERT_TRUE(device.isWifiControlReady());
}

void testESP32WiimoteWifiControlModeSwitchRestToWebSocketRestartsLifecycle() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);

    for (int i = 0; i < 5; ++i) {
        device.task();
    }

    TEST_ASSERT_TRUE(device.isWifiControlReady());
    TEST_ASSERT_TRUE(device.getWifiControlState().apiRoutesRegistered);
    TEST_ASSERT_FALSE(device.getWifiControlState().websocketRoutesRegistered);

    device.enableWifiControl(true, WifiDeliveryMode::RestAndWebSocket);
    TEST_ASSERT_FALSE(device.isWifiControlReady());
    TEST_ASSERT_FALSE(device.getWifiControlState().apiRoutesRegistered);
    TEST_ASSERT_FALSE(device.getWifiControlState().websocketRoutesRegistered);

    for (int i = 0; i < 4; ++i) {
        device.task();
    }

    TEST_ASSERT_TRUE(device.getWifiControlState().apiRoutesRegistered);
    TEST_ASSERT_TRUE(device.getWifiControlState().websocketRoutesRegistered);
    TEST_ASSERT_FALSE(device.isWifiControlReady());
    device.task();
    TEST_ASSERT_TRUE(device.isWifiControlReady());
}

void testESP32WiimoteWifiControlModeSwitchWebSocketToRestDisablesWebSocketStage() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestAndWebSocket);

    for (int i = 0; i < 6; ++i) {
        device.task();
    }

    TEST_ASSERT_TRUE(device.isWifiControlReady());
    TEST_ASSERT_TRUE(device.getWifiControlState().apiRoutesRegistered);
    TEST_ASSERT_TRUE(device.getWifiControlState().websocketRoutesRegistered);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    TEST_ASSERT_FALSE(device.isWifiControlReady());
    TEST_ASSERT_FALSE(device.getWifiControlState().apiRoutesRegistered);
    TEST_ASSERT_FALSE(device.getWifiControlState().websocketRoutesRegistered);

    for (int i = 0; i < 3; ++i) {
        device.task();
    }

    TEST_ASSERT_TRUE(device.getWifiControlState().apiRoutesRegistered);
    TEST_ASSERT_FALSE(device.getWifiControlState().websocketRoutesRegistered);
    TEST_ASSERT_FALSE(device.isWifiControlReady());
    device.task();
    TEST_ASSERT_TRUE(device.isWifiControlReady());
}

void testESP32WiimoteWifiControlFailsWithoutNetworkCredentials() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);

    TEST_ASSERT_TRUE(device.isWifiControlEnabled());
    device.task();

    const ESP32Wiimote::WifiControlState kState = device.getWifiControlState();
    TEST_ASSERT_FALSE(kState.enabled);
    TEST_ASSERT_FALSE(kState.ready);
    TEST_ASSERT_TRUE(kState.networkConnectAttempted);
    TEST_ASSERT_FALSE(kState.networkConnected);
    TEST_ASSERT_TRUE(kState.networkConnectFailed);
    TEST_ASSERT_FALSE(kState.wifiLayerStarted);
}

void testESP32WiimoteWifiControlFailsWhenJoinFails() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "__fail__";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    device.task();

    const ESP32Wiimote::WifiControlState kState = device.getWifiControlState();
    TEST_ASSERT_FALSE(kState.enabled);
    TEST_ASSERT_FALSE(kState.ready);
    TEST_ASSERT_TRUE(kState.networkCredentialsConfigured);
    TEST_ASSERT_TRUE(kState.networkConnectAttempted);
    TEST_ASSERT_FALSE(kState.networkConnected);
    TEST_ASSERT_TRUE(kState.networkConnectFailed);
    TEST_ASSERT_FALSE(kState.wifiLayerStarted);
}

void testESP32WiimoteUpdateWifiNetworkCredentialsChangesConfig() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    TestESP32Wiimote device(config);

    TEST_ASSERT_TRUE(device.updateWifiNetworkCredentials("ssid_new", "pass_new"));
    TEST_ASSERT_EQUAL_STRING("ssid_new", device.getConfig().wifi.network.ssid);
    TEST_ASSERT_EQUAL_STRING("pass_new", device.getConfig().wifi.network.password);
}

void testESP32WiimoteUpdateWifiApiTokenChangesConfig() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "old_token";
    TestESP32Wiimote device(config);

    TEST_ASSERT_TRUE(device.updateWifiApiToken("new_token"));
    TEST_ASSERT_EQUAL_STRING("new_token", device.getConfig().auth.wifiApiToken);
    TEST_ASSERT_TRUE(device.hasWifiApiToken());
}

void testESP32WiimoteWifiTokenFallsBackToSerialTokenWhenConfigured() {
    ESP32WiimoteConfig config;
    config.auth.serialPrivilegedToken = "serial_token";
    config.auth.wifiApiToken = nullptr;
    config.auth.wifiTokenFallbackToSerial = true;
    TestESP32Wiimote device(config);

    TEST_ASSERT_TRUE(device.hasWifiApiToken());
    TEST_ASSERT_TRUE(device.init());
    TEST_ASSERT_NOT_NULL(
        std::strstr(mockSerialGetOutput(), "wifi_api_token_missing_using_serial_token"));
}

void testESP32WiimoteUpdateWifiNetworkCredentialsRejectsEmptyValues() {
    TestESP32Wiimote device;

    TEST_ASSERT_FALSE(device.updateWifiNetworkCredentials(nullptr, "password"));
    TEST_ASSERT_FALSE(device.updateWifiNetworkCredentials("", "password"));
    TEST_ASSERT_FALSE(device.updateWifiNetworkCredentials("ssid", nullptr));
    TEST_ASSERT_FALSE(device.updateWifiNetworkCredentials("ssid", ""));
}

void testESP32WiimoteUpdateWifiApiTokenRejectsEmptyValues() {
    TestESP32Wiimote device;

    TEST_ASSERT_FALSE(device.updateWifiApiToken(nullptr));
    TEST_ASSERT_FALSE(device.updateWifiApiToken(""));
    TEST_ASSERT_FALSE(device.hasWifiApiToken());
}

void testESP32WiimoteRestartWifiControlRequiresEnabledControl() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    TEST_ASSERT_FALSE(device.restartWifiControl());
    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    TEST_ASSERT_TRUE(device.restartWifiControl());
}

void testESP32WiimoteSerialCommandsExerciseWifiAndControllerWrappers() {
    ESP32WiimoteConfig config;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    TestESP32Wiimote device(config);

    mockBtStarted = true;
    mockTinyWiimoteConnected = true;
    mockSetLedsResult = true;
    mockSetReportingModeResult = true;
    mockRequestStatusResult = true;
    mockStartDiscoveryResult = true;
    mockStopDiscoveryResult = true;

    device.enableSerialControl(true);
    mockSerialSetInput(
        "wm unlock token 60\n"
        "wm wifi-set-network ssid_new pass_new\n"
        "wm wifi-control on\n"
        "wm wifi-mode rest-ws\n"
        "wm wifi-restart\n"
        "wm reconnect on\n"
        "wm reconnect clear\n"
        "wm scan on\n"
        "wm discover start\n"
        "wm discover stop\n"
        "wm request-status\n"
        "wm accel on\n"
        "wm mode 0x31 1\n"
        "wm led 0x01\n");

    for (int i = 0; i < 14; ++i) {
        device.task();
    }

    TEST_ASSERT_TRUE(device.isWifiControlEnabled());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(WifiDeliveryMode::RestAndWebSocket),
                            static_cast<uint8_t>(device.getWifiControlState().deliveryMode));
    TEST_ASSERT_EQUAL_STRING("ssid_new", device.getConfig().wifi.network.ssid);
    TEST_ASSERT_EQUAL_STRING("pass_new", device.getConfig().wifi.network.password);
    TEST_ASSERT_EQUAL(1, mockSetAutoReconnectEnabledCallCount);
    TEST_ASSERT_TRUE(mockLastAutoReconnectEnabled);
    TEST_ASSERT_EQUAL(1, mockClearReconnectCacheCallCount);
    TEST_ASSERT_EQUAL(1, mockSetScanEnabledCallCount);
    TEST_ASSERT_TRUE(mockLastScanEnabled);
    TEST_ASSERT_EQUAL(1, mockStartDiscoveryCallCount);
    TEST_ASSERT_EQUAL(1, mockStopDiscoveryCallCount);
    TEST_ASSERT_EQUAL(1, mockRequestStatusCallCount);
    TEST_ASSERT_EQUAL(1, mockReqAccelerometerCallCount);
    TEST_ASSERT_TRUE(mockLastReqAccelerometerUse);
    TEST_ASSERT_EQUAL(1, mockSetReportingModeCallCount);
    TEST_ASSERT_EQUAL(1, mockSetLedsCallCount);
}

void testESP32WiimoteHttpRoutesExerciseStatusAndMutationWrappers() {
    static const size_t kResponseBufSize = 2048U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    mockTinyWiimoteConnected = true;
    mockBatteryLevel = 66;
    mockSetLedsResult = true;
    mockSetReportingModeResult = true;
    mockRequestStatusResult = true;
    mockStartDiscoveryResult = true;
    mockStopDiscoveryResult = true;
    mockDisconnectResult = true;

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    for (int i = 0; i < 5; ++i) {
        device.task();
    }

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;

    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest("GET", "/api/wiimote/status", "Bearer token",
                                                       nullptr, &status, &contentType, responseBuf,
                                                       sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_NOT_NULL(std::strstr(responseBuf, "\"batteryLevel\":66"));

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest("GET", "/api/wiimote/config", "Bearer token",
                                                       nullptr, &status, &contentType, responseBuf,
                                                       sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(
        wifiHttpServerMockDispatchRequest("POST", "/api/wiimote/commands/leds", "Bearer token",
                                          "{\"command\":\"set_leds\",\"mask\":\"3\"}", &status,
                                          &contentType, responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest(
        "POST", "/api/wiimote/commands/reporting-mode", "Bearer token",
        "{\"command\":\"set_reporting_mode\",\"mode\":\"49\",\"continuous\":\"true\"}", &status,
        &contentType, responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest(
        "POST", "/api/wiimote/commands/accelerometer", "Bearer token",
        "{\"command\":\"set_accelerometer\",\"enabled\":\"true\"}", &status, &contentType,
        responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(
        wifiHttpServerMockDispatchRequest("POST", "/api/wiimote/commands/request-status",
                                          "Bearer token", "{\"command\":\"request_status\"}",
                                          &status, &contentType, responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(
        wifiHttpServerMockDispatchRequest("POST", "/api/wiimote/commands/discovery", "Bearer token",
                                          "{\"command\":\"discovery_start\"}", &status,
                                          &contentType, responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(
        wifiHttpServerMockDispatchRequest("POST", "/api/wiimote/commands/discovery", "Bearer token",
                                          "{\"command\":\"discovery_stop\"}", &status, &contentType,
                                          responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest(
        "POST", "/api/wiimote/commands/disconnect", "Bearer token",
        "{\"command\":\"disconnect\",\"reason\":\"22\"}", &status, &contentType, responseBuf,
        sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    memset(responseBuf, 0, sizeof(responseBuf));
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest(
        "POST", "/api/wiimote/commands/reconnect-policy", "Bearer token",
        "{\"command\":\"set_reconnect_policy\",\"enabled\":\"true\"}", &status, &contentType,
        responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);

    TEST_ASSERT_EQUAL(1, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL(1, mockSetReportingModeCallCount);
    TEST_ASSERT_EQUAL(1, mockReqAccelerometerCallCount);
    TEST_ASSERT_EQUAL(1, mockRequestStatusCallCount);
    TEST_ASSERT_EQUAL(1, mockStartDiscoveryCallCount);
    TEST_ASSERT_EQUAL(1, mockStopDiscoveryCallCount);
    TEST_ASSERT_EQUAL(1, mockDisconnectCallCount);
    TEST_ASSERT_EQUAL(1, mockSetAutoReconnectEnabledCallCount);
}

void testESP32WiimoteHttpModeSwitchDefersServerStopUntilAfterDispatch() {
    static const size_t kResponseBufSize = 512U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    for (int i = 0; i < 5; ++i) {
        device.task();
    }
    TEST_ASSERT_TRUE(device.isWifiControlReady());

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;
    TEST_ASSERT_TRUE(
        wifiHttpServerMockDispatchRequest("POST", "/api/wifi/delivery-mode", "Bearer token",
                                          "{\"command\":\"set_wifi_mode\",\"mode\":\"rest-ws\"}",
                                          &status, &contentType, responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_EQUAL(0, wifiHttpServerMockGetEndDuringDispatchCallCount());

    // Deferred restart executes in the next loop iteration.
    device.task();
    TEST_ASSERT_EQUAL(1, wifiHttpServerMockGetEndCallCount());
    TEST_ASSERT_FALSE(device.isWifiControlReady());
}

void testESP32WiimoteHttpModeSwitchToRestDefersServerStopUntilAfterDispatch() {
    static const size_t kResponseBufSize = 512U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestAndWebSocket);
    for (int i = 0; i < 6; ++i) {
        device.task();
    }
    TEST_ASSERT_TRUE(device.isWifiControlReady());

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;
    TEST_ASSERT_TRUE(
        wifiHttpServerMockDispatchRequest("POST", "/api/wifi/delivery-mode", "Bearer token",
                                          "{\"command\":\"set_wifi_mode\",\"mode\":\"rest\"}",
                                          &status, &contentType, responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_EQUAL(0, wifiHttpServerMockGetEndDuringDispatchCallCount());

    device.task();
    TEST_ASSERT_EQUAL(1, wifiHttpServerMockGetEndCallCount());
    TEST_ASSERT_FALSE(device.isWifiControlReady());
}

void testESP32WiimoteHttpRestartDefersServerStopUntilAfterDispatch() {
    static const size_t kResponseBufSize = 512U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    for (int i = 0; i < 5; ++i) {
        device.task();
    }
    TEST_ASSERT_TRUE(device.isWifiControlReady());

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest(
        "POST", "/api/wifi/restart", "Bearer token", "{\"command\":\"restart_wifi\"}", &status,
        &contentType, responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_EQUAL(0, wifiHttpServerMockGetEndDuringDispatchCallCount());

    device.task();
    TEST_ASSERT_EQUAL(1, wifiHttpServerMockGetEndCallCount());
    TEST_ASSERT_FALSE(device.isWifiControlReady());
}

void testESP32WiimoteHttpSetNetworkDefersServerStopUntilAfterDispatch() {
    static const size_t kResponseBufSize = 512U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    for (int i = 0; i < 5; ++i) {
        device.task();
    }
    TEST_ASSERT_TRUE(device.isWifiControlReady());

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest(
        "POST", "/api/wifi/network", "Bearer token",
        "{\"command\":\"set_wifi_network\",\"ssid\":\"ssid_new\",\"password\":\"pass_new\"}",
        &status, &contentType, responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_EQUAL(0, wifiHttpServerMockGetEndDuringDispatchCallCount());

    device.task();
    TEST_ASSERT_EQUAL(1, wifiHttpServerMockGetEndCallCount());
    TEST_ASSERT_FALSE(device.isWifiControlReady());
}

void testESP32WiimoteHttpControlDisableDefersServerStopUntilAfterDispatch() {
    static const size_t kResponseBufSize = 512U;
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    for (int i = 0; i < 5; ++i) {
        device.task();
    }
    TEST_ASSERT_TRUE(device.isWifiControlReady());

    char responseBuf[kResponseBufSize] = {0};
    int status = 0;
    const char *contentType = nullptr;
    TEST_ASSERT_TRUE(wifiHttpServerMockDispatchRequest(
        "POST", "/api/wifi/control", "Bearer token",
        "{\"command\":\"set_wifi_control\",\"enabled\":\"false\"}", &status, &contentType,
        responseBuf, sizeof(responseBuf)));
    TEST_ASSERT_EQUAL(200, status);
    TEST_ASSERT_EQUAL(0, wifiHttpServerMockGetEndDuringDispatchCallCount());
    TEST_ASSERT_FALSE(device.isWifiControlEnabled());

    device.task();
    TEST_ASSERT_EQUAL(1, wifiHttpServerMockGetEndCallCount());
    TEST_ASSERT_FALSE(device.isWifiControlReady());
}

void testESP32WiimoteSerialWifiStatusCommandPrintsWifiState() {
    ESP32WiimoteConfig config;
    config.wifi.enabled = true;
    config.auth.serialPrivilegedToken = "token";
    config.auth.wifiApiToken = "token";
    config.wifi.network.ssid = "ssid";
    config.wifi.network.password = "wifi_password";
    TestESP32Wiimote device(config);

    mockBtStarted = true;
    device.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    device.enableSerialControl(true);
    mockSerialSetInput("wm wifi-status\n");

    device.task();

    TEST_ASSERT_NOT_NULL(std::strstr(mockSerialGetOutput(), "@wm: wifi enabled=1"));
}

void testESP32WiimoteSerialControlIgnoresNonCommandInput() {
    TestESP32Wiimote device;
    mockBtStarted = true;

    device.enableSerialControl(true);
    mockSerialSetInput("hello world\n");

    device.task();

    TEST_ASSERT_EQUAL(0, mockSetLedsCallCount);
    TEST_ASSERT_EQUAL_STRING("", mockSerialGetOutput());
}

void testESP32WiimoteSerialControlReportsLineTooLong() {
    TestESP32Wiimote device;
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

void testESP32WiimoteInitPersistsDefaultRuntimeConfigSnapshot() {
    ESP32WiimoteConfig config;
    config.fastReconnectTtlMs = 54321U;
    TestESP32Wiimote device(config);

    TEST_ASSERT_TRUE(device.init());

    RuntimeConfigStore store;
    RuntimeConfigSnapshot snapshot = {};
    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_TRUE(store.load(&snapshot));

    TEST_ASSERT_FALSE(snapshot.autoReconnectEnabled);
    TEST_ASSERT_EQUAL_UINT32(54321U, snapshot.fastReconnectTtlMs);
    TEST_ASSERT_EQUAL_UINT8(0U, snapshot.ledMask);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ReportingMode::CoreButtons),
                            snapshot.reportingMode);
    TEST_ASSERT_FALSE(snapshot.reportingContinuous);
}

void testESP32WiimotePersistsRuntimePolicyUpdates() {
    TestESP32Wiimote device;

    TEST_ASSERT_TRUE(device.init());

    mockSetLedsResult = true;
    TEST_ASSERT_TRUE(device.setLeds(0x0CU));

    mockSetReportingModeResult = true;
    TEST_ASSERT_TRUE(device.setReportingMode(ReportingMode::CoreButtonsAccel, true));

    device.setAutoReconnectEnabled(true);

    RuntimeConfigStore store;
    RuntimeConfigSnapshot snapshot = {};
    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_TRUE(store.load(&snapshot));

    TEST_ASSERT_TRUE(snapshot.autoReconnectEnabled);
    TEST_ASSERT_EQUAL_UINT32(180000U, snapshot.fastReconnectTtlMs);
    TEST_ASSERT_EQUAL_UINT8(0x0CU, snapshot.ledMask);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ReportingMode::CoreButtonsAccel),
                            snapshot.reportingMode);
    TEST_ASSERT_TRUE(snapshot.reportingContinuous);
}

void testESP32WiimoteRejectedOutputCommandsDoNotPersist() {
    TestESP32Wiimote device;
    TEST_ASSERT_TRUE(device.init());

    mockSetLedsResult = false;
    TEST_ASSERT_FALSE(device.setLeds(0x07U));

    mockSetReportingModeResult = false;
    TEST_ASSERT_FALSE(device.setReportingMode(ReportingMode::CoreButtonsAccelExt, true));

    RuntimeConfigStore store;
    RuntimeConfigSnapshot snapshot = {};
    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_TRUE(store.load(&snapshot));

    TEST_ASSERT_EQUAL_UINT8(0U, snapshot.ledMask);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ReportingMode::CoreButtons),
                            snapshot.reportingMode);
    TEST_ASSERT_FALSE(snapshot.reportingContinuous);
}

void testESP32WiimoteInitRestoresPersistedFieldsFromStore() {
    // First boot: persist non-default values via API calls.
    {
        TestESP32Wiimote first;
        TEST_ASSERT_TRUE(first.init());
        first.setAutoReconnectEnabled(true);
    }

    // Second boot: a new device instance restores from the store.
    mockLastFastReconnectTtlMs = 0U;
    mockLastAutoReconnectEnabled = false;
    mockSetAutoReconnectEnabledCallCount = 0;

    TestESP32Wiimote second;
    TEST_ASSERT_TRUE(second.init());

    // autoReconnectEnabled was persisted as true and must be re-applied.
    TEST_ASSERT_EQUAL(1, mockSetAutoReconnectEnabledCallCount);
    TEST_ASSERT_TRUE(mockLastAutoReconnectEnabled);

    // The in-memory snapshot must also reflect the restored value.
    RuntimeConfigStore store;
    RuntimeConfigSnapshot snapshot = {};
    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_TRUE(store.load(&snapshot));
    TEST_ASSERT_TRUE(snapshot.autoReconnectEnabled);
}

void testESP32WiimoteInitWithNoPersistedDataUsesConfigDefaults() {
    // NVS is empty (cleared in setUp); init() should bootstrap defaults and
    // NOT call setAutoReconnectEnabled (default is false, TinyWiimote init
    // already sets false state, but we verify no spurious restore-path call).
    mockSetAutoReconnectEnabledCallCount = 0;
    mockLastFastReconnectTtlMs = 0U;

    ESP32WiimoteConfig config;
    config.fastReconnectTtlMs = 99000U;
    TestESP32Wiimote device(config);
    TEST_ASSERT_TRUE(device.init());

    // Defaults applied: fastReconnectTtlMs from config, no extra autoReconnect apply.
    TEST_ASSERT_EQUAL_UINT32(99000U, mockLastFastReconnectTtlMs);
    TEST_ASSERT_EQUAL(0, mockSetAutoReconnectEnabledCallCount);

    // Store now bootstrapped with defaults.
    RuntimeConfigStore store;
    RuntimeConfigSnapshot snapshot = {};
    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_TRUE(store.load(&snapshot));
    TEST_ASSERT_FALSE(snapshot.autoReconnectEnabled);
    TEST_ASSERT_EQUAL_UINT32(99000U, snapshot.fastReconnectTtlMs);
}

void testESP32WiimoteInitRestoresFastReconnectTtlFromStore() {
    // First boot: call init with a specific ttl, then persist a different ttl
    // by directly saving to the store (simulating a prior runtime update path).
    {
        TestESP32Wiimote first;
        TEST_ASSERT_TRUE(first.init());
        // Manually patch the store with a custom TTL to simulate persisted state.
        RuntimeConfigStore store;
        RuntimeConfigSnapshot snap = {};
        TEST_ASSERT_TRUE(store.init());
        TEST_ASSERT_TRUE(store.load(&snap));
        snap.fastReconnectTtlMs = 77777U;
        TEST_ASSERT_TRUE(store.save(snap));
    }

    // Second boot restores the custom TTL.
    mockLastFastReconnectTtlMs = 0U;

    TestESP32Wiimote second;
    TEST_ASSERT_TRUE(second.init());

    TEST_ASSERT_EQUAL_UINT32(77777U, mockLastFastReconnectTtlMs);
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testESP32WiimoteDefaultConstructorBuilds);
    RUN_TEST(testESP32WiimoteGetConfigReflectsRuntimeAuthAndWifiSettings);
    RUN_TEST(testESP32WiimoteConfigurePartialConfigPreservesPreviousAuthAndWifiValues);
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
    RUN_TEST(testESP32WiimoteSerialControlPrivilegedCommandIsLockedByDefault);
    RUN_TEST(testESP32WiimoteSerialControlUnlockExpiresByTime);
    RUN_TEST(testESP32WiimoteConfigureWithWifiDisabledRejectsBadCredentials);
    RUN_TEST(testESP32WiimoteConfigurePropagatesCredentialsToSerialUnlock);
    RUN_TEST(testESP32WiimoteWifiControlDisabledByDefault);
    RUN_TEST(testESP32WiimoteWifiControlCanBeEnabledAtRuntimeWhenConfigStartsDisabled);
    RUN_TEST(testESP32WiimoteWifiControlAsyncLifecycleRestOnly);
    RUN_TEST(testESP32WiimoteWifiControlStartsHttpServerAndRoutesRequests);
    RUN_TEST(testESP32WiimoteWifiControlHttpBridgePreservesUnauthorizedContract);
    RUN_TEST(testESP32WiimoteWifiControlHttpBridgeMapsUnsupportedMethodToNotFound);
    RUN_TEST(testESP32WiimoteWifiControlReportsHttpBindFailure);
    RUN_TEST(testESP32WiimoteDisablingWifiStopsHttpServer);
    RUN_TEST(testESP32WiimoteWifiControlRestAndWebSocketAddsWebSocketStage);
    RUN_TEST(testESP32WiimoteWifiControlModeSwitchRestToWebSocketRestartsLifecycle);
    RUN_TEST(testESP32WiimoteWifiControlModeSwitchWebSocketToRestDisablesWebSocketStage);
    RUN_TEST(testESP32WiimoteWifiControlFailsWithoutNetworkCredentials);
    RUN_TEST(testESP32WiimoteWifiControlFailsWhenJoinFails);
    RUN_TEST(testESP32WiimoteUpdateWifiNetworkCredentialsChangesConfig);
    RUN_TEST(testESP32WiimoteUpdateWifiApiTokenChangesConfig);
    RUN_TEST(testESP32WiimoteWifiTokenFallsBackToSerialTokenWhenConfigured);
    RUN_TEST(testESP32WiimoteUpdateWifiNetworkCredentialsRejectsEmptyValues);
    RUN_TEST(testESP32WiimoteUpdateWifiApiTokenRejectsEmptyValues);
    RUN_TEST(testESP32WiimoteRestartWifiControlRequiresEnabledControl);
    RUN_TEST(testESP32WiimoteSerialCommandsExerciseWifiAndControllerWrappers);
    RUN_TEST(testESP32WiimoteHttpRoutesExerciseStatusAndMutationWrappers);
    RUN_TEST(testESP32WiimoteHttpModeSwitchDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteHttpModeSwitchToRestDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteHttpRestartDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteHttpSetNetworkDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteHttpControlDisableDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteSerialWifiStatusCommandPrintsWifiState);
    RUN_TEST(testESP32WiimoteSerialControlIgnoresNonCommandInput);
    RUN_TEST(testESP32WiimoteSerialControlReportsLineTooLong);
    RUN_TEST(testESP32WiimoteInitPersistsDefaultRuntimeConfigSnapshot);
    RUN_TEST(testESP32WiimotePersistsRuntimePolicyUpdates);
    RUN_TEST(testESP32WiimoteRejectedOutputCommandsDoNotPersist);
    RUN_TEST(testESP32WiimoteInitRestoresPersistedFieldsFromStore);
    RUN_TEST(testESP32WiimoteInitWithNoPersistedDataUsesConfigDefaults);
    RUN_TEST(testESP32WiimoteInitRestoresFastReconnectTtlFromStore);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testESP32WiimoteDefaultConstructorBuilds);
    RUN_TEST(testESP32WiimoteGetConfigReflectsRuntimeAuthAndWifiSettings);
    RUN_TEST(testESP32WiimoteConfigurePartialConfigPreservesPreviousAuthAndWifiValues);
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
    RUN_TEST(testESP32WiimoteSerialControlPrivilegedCommandIsLockedByDefault);
    RUN_TEST(testESP32WiimoteSerialControlUnlockExpiresByTime);
    RUN_TEST(testESP32WiimoteConfigureWithWifiDisabledRejectsBadCredentials);
    RUN_TEST(testESP32WiimoteConfigurePropagatesCredentialsToSerialUnlock);
    RUN_TEST(testESP32WiimoteWifiControlDisabledByDefault);
    RUN_TEST(testESP32WiimoteWifiControlCanBeEnabledAtRuntimeWhenConfigStartsDisabled);
    RUN_TEST(testESP32WiimoteWifiControlAsyncLifecycleRestOnly);
    RUN_TEST(testESP32WiimoteWifiControlStartsHttpServerAndRoutesRequests);
    RUN_TEST(testESP32WiimoteWifiControlHttpBridgePreservesUnauthorizedContract);
    RUN_TEST(testESP32WiimoteWifiControlHttpBridgeMapsUnsupportedMethodToNotFound);
    RUN_TEST(testESP32WiimoteWifiControlReportsHttpBindFailure);
    RUN_TEST(testESP32WiimoteDisablingWifiStopsHttpServer);
    RUN_TEST(testESP32WiimoteWifiControlRestAndWebSocketAddsWebSocketStage);
    RUN_TEST(testESP32WiimoteWifiControlModeSwitchRestToWebSocketRestartsLifecycle);
    RUN_TEST(testESP32WiimoteWifiControlModeSwitchWebSocketToRestDisablesWebSocketStage);
    RUN_TEST(testESP32WiimoteWifiControlFailsWithoutNetworkCredentials);
    RUN_TEST(testESP32WiimoteWifiControlFailsWhenJoinFails);
    RUN_TEST(testESP32WiimoteUpdateWifiNetworkCredentialsChangesConfig);
    RUN_TEST(testESP32WiimoteUpdateWifiApiTokenChangesConfig);
    RUN_TEST(testESP32WiimoteWifiTokenFallsBackToSerialTokenWhenConfigured);
    RUN_TEST(testESP32WiimoteUpdateWifiNetworkCredentialsRejectsEmptyValues);
    RUN_TEST(testESP32WiimoteUpdateWifiApiTokenRejectsEmptyValues);
    RUN_TEST(testESP32WiimoteRestartWifiControlRequiresEnabledControl);
    RUN_TEST(testESP32WiimoteSerialCommandsExerciseWifiAndControllerWrappers);
    RUN_TEST(testESP32WiimoteHttpRoutesExerciseStatusAndMutationWrappers);
    RUN_TEST(testESP32WiimoteHttpModeSwitchDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteHttpModeSwitchToRestDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteHttpRestartDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteHttpSetNetworkDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteHttpControlDisableDefersServerStopUntilAfterDispatch);
    RUN_TEST(testESP32WiimoteSerialWifiStatusCommandPrintsWifiState);
    RUN_TEST(testESP32WiimoteSerialControlIgnoresNonCommandInput);
    RUN_TEST(testESP32WiimoteSerialControlReportsLineTooLong);
    RUN_TEST(testESP32WiimoteInitPersistsDefaultRuntimeConfigSnapshot);
    RUN_TEST(testESP32WiimotePersistsRuntimePolicyUpdates);
    RUN_TEST(testESP32WiimoteRejectedOutputCommandsDoNotPersist);
    RUN_TEST(testESP32WiimoteInitRestoresPersistedFieldsFromStore);
    RUN_TEST(testESP32WiimoteInitWithNoPersistedDataUsesConfigDefaults);
    RUN_TEST(testESP32WiimoteInitRestoresFastReconnectTtlFromStore);

    UNITY_END();
}

void loop() {}
#endif
