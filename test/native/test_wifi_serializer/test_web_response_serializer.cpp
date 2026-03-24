#include "../../../src/wifi/web_response_serializer.h"

#include <string.h>
#include <unity.h>

// ===== Helpers =====

static char buf[512];

static void clearBuf() {
    memset(buf, 0, sizeof(buf));
}

// ===== serializeOk =====

void testSerializeOkBasic() {
    clearBuf();
    WebSerializeResult r = serializeOk(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"ok\"}", buf);
}

void testSerializeOkNullBuf() {
    WebSerializeResult r = serializeOk(nullptr, 32);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

void testSerializeOkZeroSize() {
    clearBuf();
    WebSerializeResult r = serializeOk(buf, 0);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

void testSerializeOkTinyBuffer() {
    char small[5];
    WebSerializeResult r = serializeOk(small, sizeof(small));
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

// ===== serializeError =====

void testSerializeErrorBasic() {
    clearBuf();
    WebSerializeResult r = serializeError(buf, sizeof(buf), "not found");
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"error\",\"message\":\"not found\"}", buf);
}

void testSerializeErrorEmptyMessage() {
    clearBuf();
    WebSerializeResult r = serializeError(buf, sizeof(buf), "");
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"error\",\"message\":\"\"}", buf);
}

void testSerializeErrorNullBuf() {
    WebSerializeResult r = serializeError(nullptr, 64, "err");
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

void testSerializeErrorNullMessage() {
    clearBuf();
    WebSerializeResult r = serializeError(buf, sizeof(buf), nullptr);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

void testSerializeErrorTinyBuffer() {
    char small[10];
    WebSerializeResult r = serializeError(small, sizeof(small), "toolong");
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

// ===== serializeWiimoteInput =====

void testSerializeWiimoteInputBasic() {
    clearBuf();
    WebWiimoteInputSnapshot in{};
    in.buttonMask = 0x00000008U;  // ButtonState::A
    in.accelX = 128;
    in.accelY = 64;
    in.accelZ = 32;
    in.nunchukX = 0;
    in.nunchukY = 0;
    WebSerializeResult r = serializeWiimoteInput(buf, sizeof(buf), in);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING(
        "{\"buttons\":\"00000008\",\"accelX\":128,\"accelY\":64,\"accelZ\":32,"
        "\"nunchukX\":0,\"nunchukY\":0}",
        buf);
}

void testSerializeWiimoteInputAllZero() {
    clearBuf();
    WebWiimoteInputSnapshot in{};
    WebSerializeResult r = serializeWiimoteInput(buf, sizeof(buf), in);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING(
        "{\"buttons\":\"00000000\",\"accelX\":0,\"accelY\":0,\"accelZ\":0,"
        "\"nunchukX\":0,\"nunchukY\":0}",
        buf);
}

void testSerializeWiimoteInputWithNunchuk() {
    clearBuf();
    WebWiimoteInputSnapshot in{};
    in.buttonMask = 0U;
    in.accelX = 0;
    in.accelY = 0;
    in.accelZ = 0;
    in.nunchukX = 127;
    in.nunchukY = 200;
    WebSerializeResult r = serializeWiimoteInput(buf, sizeof(buf), in);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING(
        "{\"buttons\":\"00000000\",\"accelX\":0,\"accelY\":0,\"accelZ\":0,"
        "\"nunchukX\":127,\"nunchukY\":200}",
        buf);
}

void testSerializeWiimoteInputNullBuf() {
    WebWiimoteInputSnapshot in{};
    WebSerializeResult r = serializeWiimoteInput(nullptr, 128, in);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

void testSerializeWiimoteInputTinyBuffer() {
    char small[10];
    WebWiimoteInputSnapshot in{};
    WebSerializeResult r = serializeWiimoteInput(small, sizeof(small), in);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

// ===== serializeWiimoteStatus =====

void testSerializeWiimoteStatusConnected() {
    clearBuf();
    WebWiimoteStatusSnapshot st{};
    st.connected = true;
    st.batteryLevel = 75;
    WebSerializeResult r = serializeWiimoteStatus(buf, sizeof(buf), st);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING("{\"connected\":true,\"batteryLevel\":75}", buf);
}

void testSerializeWiimoteStatusDisconnected() {
    clearBuf();
    WebWiimoteStatusSnapshot st{};
    st.connected = false;
    st.batteryLevel = 0;
    WebSerializeResult r = serializeWiimoteStatus(buf, sizeof(buf), st);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING("{\"connected\":false,\"batteryLevel\":0}", buf);
}

void testSerializeWiimoteStatusMaxBattery() {
    clearBuf();
    WebWiimoteStatusSnapshot st{};
    st.connected = true;
    st.batteryLevel = 100;
    WebSerializeResult r = serializeWiimoteStatus(buf, sizeof(buf), st);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING("{\"connected\":true,\"batteryLevel\":100}", buf);
}

void testSerializeWiimoteStatusNullBuf() {
    WebWiimoteStatusSnapshot st{};
    WebSerializeResult r = serializeWiimoteStatus(nullptr, 64, st);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

void testSerializeWiimoteStatusTinyBuffer() {
    char small[5];
    WebWiimoteStatusSnapshot st{};
    WebSerializeResult r = serializeWiimoteStatus(small, sizeof(small), st);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

// ===== serializeControllerStatus =====

void testSerializeControllerStatusFullyActive() {
    clearBuf();
    WebControllerStatusSnapshot st{};
    st.initialized = true;
    st.started = true;
    st.scanning = false;
    st.connected = true;
    st.activeConnectionHandle = 0x0040;
    st.fastReconnectActive = false;
    st.autoReconnectEnabled = true;
    WebSerializeResult r = serializeControllerStatus(buf, sizeof(buf), st);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING(
        "{\"initialized\":true,\"started\":true,\"scanning\":false,"
        "\"connected\":true,\"activeConnectionHandle\":64,"
        "\"fastReconnectActive\":false,\"autoReconnectEnabled\":true}",
        buf);
}

void testSerializeControllerStatusAllFalse() {
    clearBuf();
    WebControllerStatusSnapshot st{};
    WebSerializeResult r = serializeControllerStatus(buf, sizeof(buf), st);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING(
        "{\"initialized\":false,\"started\":false,\"scanning\":false,"
        "\"connected\":false,\"activeConnectionHandle\":0,"
        "\"fastReconnectActive\":false,\"autoReconnectEnabled\":false}",
        buf);
}

void testSerializeControllerStatusNullBuf() {
    WebControllerStatusSnapshot st{};
    WebSerializeResult r = serializeControllerStatus(nullptr, 128, st);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

void testSerializeControllerStatusTinyBuffer() {
    char small[10];
    WebControllerStatusSnapshot st{};
    WebSerializeResult r = serializeControllerStatus(small, sizeof(small), st);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

// ===== serializeConfig =====

void testSerializeConfigDefaults() {
    clearBuf();
    WebConfigSnapshot cfg{};
    cfg.nunchukStickThreshold = 1;
    cfg.txQueueSize = 32;
    cfg.rxQueueSize = 32;
    cfg.fastReconnectTtlMs = 180000UL;
    WebSerializeResult r = serializeConfig(buf, sizeof(buf), cfg);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING(
        "{\"nunchukStickThreshold\":1,\"txQueueSize\":32,\"rxQueueSize\":32,"
        "\"fastReconnectTtlMs\":180000}",
        buf);
}

void testSerializeConfigZeroes() {
    clearBuf();
    WebConfigSnapshot cfg{};
    WebSerializeResult r = serializeConfig(buf, sizeof(buf), cfg);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING(
        "{\"nunchukStickThreshold\":0,\"txQueueSize\":0,\"rxQueueSize\":0,"
        "\"fastReconnectTtlMs\":0}",
        buf);
}

void testSerializeConfigNullBuf() {
    WebConfigSnapshot cfg{};
    WebSerializeResult r = serializeConfig(nullptr, 128, cfg);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

void testSerializeConfigTinyBuffer() {
    char small[5];
    WebConfigSnapshot cfg{};
    WebSerializeResult r = serializeConfig(small, sizeof(small), cfg);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

// ===== serializeWifiControlState =====

void testSerializeWifiControlStateBasic() {
    clearBuf();
    WebWifiControlStateSnapshot state{};
    state.enabled = true;
    state.ready = false;
    state.networkCredentialsConfigured = true;
    state.networkConnectAttempted = true;
    state.networkConnected = false;
    state.networkConnectFailed = true;
    state.restAndWebSocket = true;

    WebSerializeResult r = serializeWifiControlState(buf, sizeof(buf), state);
    TEST_ASSERT_EQUAL(WebSerializeResult::Ok, r);
    TEST_ASSERT_EQUAL_STRING(
        "{\"enabled\":true,\"ready\":false,\"networkCredentialsConfigured\":true,"
        "\"networkConnectAttempted\":true,\"networkConnected\":false,"
        "\"networkConnectFailed\":true,\"deliveryMode\":\"rest-ws\"}",
        buf);
}

void testSerializeWifiControlStateNullBuffer() {
    WebWifiControlStateSnapshot state{};
    WebSerializeResult r = serializeWifiControlState(nullptr, 10U, state);
    TEST_ASSERT_EQUAL(WebSerializeResult::BufferTooSmall, r);
}

// ===== Main =====

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testSerializeOkBasic);
    RUN_TEST(testSerializeOkNullBuf);
    RUN_TEST(testSerializeOkZeroSize);
    RUN_TEST(testSerializeOkTinyBuffer);

    RUN_TEST(testSerializeErrorBasic);
    RUN_TEST(testSerializeErrorEmptyMessage);
    RUN_TEST(testSerializeErrorNullBuf);
    RUN_TEST(testSerializeErrorNullMessage);
    RUN_TEST(testSerializeErrorTinyBuffer);

    RUN_TEST(testSerializeWiimoteInputBasic);
    RUN_TEST(testSerializeWiimoteInputAllZero);
    RUN_TEST(testSerializeWiimoteInputWithNunchuk);
    RUN_TEST(testSerializeWiimoteInputNullBuf);
    RUN_TEST(testSerializeWiimoteInputTinyBuffer);

    RUN_TEST(testSerializeWiimoteStatusConnected);
    RUN_TEST(testSerializeWiimoteStatusDisconnected);
    RUN_TEST(testSerializeWiimoteStatusMaxBattery);
    RUN_TEST(testSerializeWiimoteStatusNullBuf);
    RUN_TEST(testSerializeWiimoteStatusTinyBuffer);

    RUN_TEST(testSerializeControllerStatusFullyActive);
    RUN_TEST(testSerializeControllerStatusAllFalse);
    RUN_TEST(testSerializeControllerStatusNullBuf);
    RUN_TEST(testSerializeControllerStatusTinyBuffer);

    RUN_TEST(testSerializeConfigDefaults);
    RUN_TEST(testSerializeConfigZeroes);
    RUN_TEST(testSerializeConfigNullBuf);
    RUN_TEST(testSerializeConfigTinyBuffer);

    RUN_TEST(testSerializeWifiControlStateBasic);
    RUN_TEST(testSerializeWifiControlStateNullBuffer);

    return UNITY_END();
}
