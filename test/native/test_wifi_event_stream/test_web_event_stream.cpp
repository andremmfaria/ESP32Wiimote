#include "../../../src/wifi/web_api_router.h"
#include "../../../src/wifi/web_command_queue.h"
#include "../../../src/wifi/web_event_stream.h"

#include <cstring>
#include <unity.h>

// ===== Shared Fixtures =====

static WebEventStream gStream;
static char gBuf[512];

// ===== Send-Callback Spies =====

struct SendCapture {
    bool called;
    char frame[256];
    size_t frameLen;
    void *userData;
};

static SendCapture gInputCapture;
static SendCapture gStatusCapture;

static void spyInputSend(const char *frame, size_t frameLen, void *userData) {
    gInputCapture.called = true;
    gInputCapture.frameLen = frameLen;
    gInputCapture.userData = userData;
    std::strncpy(gInputCapture.frame, frame, sizeof(gInputCapture.frame) - 1U);
    gInputCapture.frame[sizeof(gInputCapture.frame) - 1U] = '\0';
}

static void spyStatusSend(const char *frame, size_t frameLen, void *userData) {
    gStatusCapture.called = true;
    gStatusCapture.frameLen = frameLen;
    gStatusCapture.userData = userData;
    std::strncpy(gStatusCapture.frame, frame, sizeof(gStatusCapture.frame) - 1U);
    gStatusCapture.frame[sizeof(gStatusCapture.frame) - 1U] = '\0';
}

// ===== Unity Hooks =====

void setUp() {
    webEventStreamInit(&gStream);
    gInputCapture = {};
    gStatusCapture = {};
}

void tearDown() {}

// ===== Init Tests =====

void testWebEventStreamInitSetsAllChannelsDisconnected() {
    TEST_ASSERT_FALSE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Input));
    TEST_ASSERT_FALSE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Status));
}

// ===== Path Matching Tests =====

void testWebEventStreamMatchPathInputChannel() {
    WebEventStreamChannel ch = WebEventStreamChannel::Status;
    const bool kMatched = webEventStreamMatchPath("/api/wiimote/input/events", &ch);
    TEST_ASSERT_TRUE(kMatched);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WebEventStreamChannel::Input), static_cast<int>(ch));
}

void testWebEventStreamMatchPathStatusChannel() {
    WebEventStreamChannel ch = WebEventStreamChannel::Input;
    const bool kMatched = webEventStreamMatchPath("/api/wiimote/status/events", &ch);
    TEST_ASSERT_TRUE(kMatched);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WebEventStreamChannel::Status), static_cast<int>(ch));
}

void testWebEventStreamMatchPathUnknownReturnsFalse() {
    WebEventStreamChannel ch = WebEventStreamChannel::Input;
    TEST_ASSERT_FALSE(webEventStreamMatchPath("/api/wiimote/other", &ch));
    TEST_ASSERT_FALSE(webEventStreamMatchPath("/api/wiimote/input/events/extra", &ch));
    TEST_ASSERT_FALSE(webEventStreamMatchPath("", &ch));
}

void testWebEventStreamMatchPathNullPathReturnsFalse() {
    WebEventStreamChannel ch = WebEventStreamChannel::Input;
    TEST_ASSERT_FALSE(webEventStreamMatchPath(nullptr, &ch));
}

void testWebEventStreamMatchPathNullChannelReturnsFalse() {
    TEST_ASSERT_FALSE(webEventStreamMatchPath("/api/wiimote/input/events", nullptr));
}

// ===== Connect / HasClient Tests =====

void testWebEventStreamConnectSetsHasClient() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, nullptr);
    TEST_ASSERT_TRUE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Input));
    TEST_ASSERT_FALSE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Status));
}

void testWebEventStreamConnectStatusChannel() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Status, spyStatusSend, nullptr);
    TEST_ASSERT_FALSE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Input));
    TEST_ASSERT_TRUE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Status));
}

void testWebEventStreamConnectNullSendFnIsNoOp() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, nullptr, nullptr);
    TEST_ASSERT_FALSE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Input));
}

void testWebEventStreamConnectReplacesExistingClient() {
    int sentinel1 = 1;
    int sentinel2 = 2;
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, &sentinel1);
    TEST_ASSERT_TRUE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Input));
    // replace
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, &sentinel2);
    TEST_ASSERT_TRUE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Input));
    // userData should now point to sentinel2
    WebWiimoteInputSnapshot snap = {};
    webEventStreamPublishInput(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_EQUAL_PTR(&sentinel2, gInputCapture.userData);
}

// ===== Disconnect Tests =====

void testWebEventStreamDisconnectClearsClient() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, nullptr);
    webEventStreamDisconnect(&gStream, WebEventStreamChannel::Input);
    TEST_ASSERT_FALSE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Input));
}

void testWebEventStreamDisconnectStatusChannel() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Status, spyStatusSend, nullptr);
    webEventStreamDisconnect(&gStream, WebEventStreamChannel::Status);
    TEST_ASSERT_FALSE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Status));
}

void testWebEventStreamDisconnectWhenNotConnectedIsNoOp() {
    // Should not crash or affect the other channel
    webEventStreamConnect(&gStream, WebEventStreamChannel::Status, spyStatusSend, nullptr);
    webEventStreamDisconnect(&gStream, WebEventStreamChannel::Input);
    TEST_ASSERT_TRUE(webEventStreamHasClient(&gStream, WebEventStreamChannel::Status));
}

// ===== PublishInput Tests =====

void testWebEventStreamPublishInputCallsSendFn() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, nullptr);
    WebWiimoteInputSnapshot snap = {};
    webEventStreamPublishInput(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_TRUE(gInputCapture.called);
    TEST_ASSERT_GREATER_THAN(0U, gInputCapture.frameLen);
}

void testWebEventStreamPublishInputNoOpWhenNoClient() {
    WebWiimoteInputSnapshot snap = {};
    webEventStreamPublishInput(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_FALSE(gInputCapture.called);
}

void testWebEventStreamPublishInputDoesNotCallStatusSendFn() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, nullptr);
    webEventStreamConnect(&gStream, WebEventStreamChannel::Status, spyStatusSend, nullptr);
    WebWiimoteInputSnapshot snap = {};
    webEventStreamPublishInput(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_TRUE(gInputCapture.called);
    TEST_ASSERT_FALSE(gStatusCapture.called);
}

void testWebEventStreamPublishInputForwardsUserData() {
    int token = 42;
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, &token);
    WebWiimoteInputSnapshot snap = {};
    webEventStreamPublishInput(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_EQUAL_PTR(&token, gInputCapture.userData);
}

// ===== PublishStatus Tests =====

void testWebEventStreamPublishStatusCallsSendFn() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Status, spyStatusSend, nullptr);
    WebWiimoteStatusSnapshot snap = {};
    webEventStreamPublishStatus(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_TRUE(gStatusCapture.called);
    TEST_ASSERT_GREATER_THAN(0U, gStatusCapture.frameLen);
}

void testWebEventStreamPublishStatusNoOpWhenNoClient() {
    WebWiimoteStatusSnapshot snap = {};
    webEventStreamPublishStatus(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_FALSE(gStatusCapture.called);
}

void testWebEventStreamPublishStatusDoesNotCallInputSendFn() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, nullptr);
    webEventStreamConnect(&gStream, WebEventStreamChannel::Status, spyStatusSend, nullptr);
    WebWiimoteStatusSnapshot snap = {};
    webEventStreamPublishStatus(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_TRUE(gStatusCapture.called);
    TEST_ASSERT_FALSE(gInputCapture.called);
}

void testWebEventStreamPublishStatusForwardsUserData() {
    int token = 99;
    webEventStreamConnect(&gStream, WebEventStreamChannel::Status, spyStatusSend, &token);
    WebWiimoteStatusSnapshot snap = {};
    webEventStreamPublishStatus(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_EQUAL_PTR(&token, gStatusCapture.userData);
}

// ===== Router Integration Tests =====

static const WiimoteCredentials kTestCredentials = {
    "admin",
    "password",
    "esp32wiimote_bearer_token_v1",
};
static const char *kValidBearer = "Bearer esp32wiimote_bearer_token_v1";

static WebWiimoteStatusSnapshot gMockStatus;
static WebConfigSnapshot gMockConfig;
static WebCommandQueue gCommandQueue;
static char gRouteBuf[512];

static WebWiimoteStatusSnapshot mockGetStatus(void * /*userData*/) {
    return gMockStatus;
}
static WebConfigSnapshot mockGetConfig(void * /*userData*/) {
    return gMockConfig;
}
static bool mockNoop1(uint8_t /*a*/, void * /*u*/) {
    return true;
}
static bool mockNoop2(uint8_t /*a*/, bool /*b*/, void * /*u*/) {
    return true;
}
static bool mockNoop3(bool /*a*/, void * /*u*/) {
    return true;
}
static bool mockNoop4(void * /*u*/) {
    return true;
}
static void mockNoop5(bool /*a*/, void * /*u*/) {}
static bool mockNoop6(uint8_t /*a*/, void * /*u*/) {
    return true;
}
static void mockNoop7(bool /*a*/, void * /*u*/) {}

static WebApiContext makeCtxWithStream(WebEventStream *stream) {
    WebApiContext ctx;
    ctx.credentials = &kTestCredentials;
    ctx.getWiimoteStatus = mockGetStatus;
    ctx.getConfig = mockGetConfig;
    ctx.setLeds = mockNoop1;
    ctx.setReportingMode = mockNoop2;
    ctx.setAccelEnabled = mockNoop3;
    ctx.requestStatus = mockNoop4;
    ctx.setScanEnabled = mockNoop5;
    ctx.startDiscovery = mockNoop4;
    ctx.stopDiscovery = mockNoop4;
    ctx.disconnect = mockNoop6;
    ctx.setAutoReconnect = mockNoop7;
    ctx.userData = nullptr;
    ctx.eventStream = stream;
    return ctx;
}

static WebApiRouteResult callRoute(const WebApiContext *ctx,
                                   const char *method,
                                   const char *path,
                                   const char *auth) {
    return webApiRoute(ctx, method, path, auth, nullptr, 0U, gRouteBuf, sizeof(gRouteBuf));
}

void testWebApiRouteReturns101ForInputEventPath() {
    WebEventStream stream;
    webEventStreamInit(&stream);
    const WebApiContext kCtx = makeCtxWithStream(&stream);
    const WebApiRouteResult kResult =
        callRoute(&kCtx, "GET", "/api/wiimote/input/events", kValidBearer);
    TEST_ASSERT_EQUAL_INT(101, kResult.httpStatus);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WebEventStreamChannel::Input),
                          static_cast<int>(kResult.upgradeChannel));
}

void testWebApiRouteReturns101ForStatusEventPath() {
    WebEventStream stream;
    webEventStreamInit(&stream);
    const WebApiContext kCtx = makeCtxWithStream(&stream);
    const WebApiRouteResult kResult =
        callRoute(&kCtx, "GET", "/api/wiimote/status/events", kValidBearer);
    TEST_ASSERT_EQUAL_INT(101, kResult.httpStatus);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WebEventStreamChannel::Status),
                          static_cast<int>(kResult.upgradeChannel));
}

void testWebApiRouteIgnoresEventPathsWhenStreamNotConfigured() {
    const WebApiContext kCtx = makeCtxWithStream(nullptr);
    const WebApiRouteResult kResult =
        callRoute(&kCtx, "GET", "/api/wiimote/input/events", kValidBearer);
    TEST_ASSERT_NOT_EQUAL(101, kResult.httpStatus);
}

void testWebApiRouteEventPathRequiresGetMethod() {
    WebEventStream stream;
    webEventStreamInit(&stream);
    const WebApiContext kCtx = makeCtxWithStream(&stream);
    const WebApiRouteResult kResult =
        callRoute(&kCtx, "POST", "/api/wiimote/input/events", kValidBearer);
    TEST_ASSERT_NOT_EQUAL(101, kResult.httpStatus);
}

// ===== Entry Point =====

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testWebEventStreamInitSetsAllChannelsDisconnected);

    RUN_TEST(testWebEventStreamMatchPathInputChannel);
    RUN_TEST(testWebEventStreamMatchPathStatusChannel);
    RUN_TEST(testWebEventStreamMatchPathUnknownReturnsFalse);
    RUN_TEST(testWebEventStreamMatchPathNullPathReturnsFalse);
    RUN_TEST(testWebEventStreamMatchPathNullChannelReturnsFalse);

    RUN_TEST(testWebEventStreamConnectSetsHasClient);
    RUN_TEST(testWebEventStreamConnectStatusChannel);
    RUN_TEST(testWebEventStreamConnectNullSendFnIsNoOp);
    RUN_TEST(testWebEventStreamConnectReplacesExistingClient);

    RUN_TEST(testWebEventStreamDisconnectClearsClient);
    RUN_TEST(testWebEventStreamDisconnectStatusChannel);
    RUN_TEST(testWebEventStreamDisconnectWhenNotConnectedIsNoOp);

    RUN_TEST(testWebEventStreamPublishInputCallsSendFn);
    RUN_TEST(testWebEventStreamPublishInputNoOpWhenNoClient);
    RUN_TEST(testWebEventStreamPublishInputDoesNotCallStatusSendFn);
    RUN_TEST(testWebEventStreamPublishInputForwardsUserData);

    RUN_TEST(testWebEventStreamPublishStatusCallsSendFn);
    RUN_TEST(testWebEventStreamPublishStatusNoOpWhenNoClient);
    RUN_TEST(testWebEventStreamPublishStatusDoesNotCallInputSendFn);
    RUN_TEST(testWebEventStreamPublishStatusForwardsUserData);

    RUN_TEST(testWebApiRouteReturns101ForInputEventPath);
    RUN_TEST(testWebApiRouteReturns101ForStatusEventPath);
    RUN_TEST(testWebApiRouteIgnoresEventPathsWhenStreamNotConfigured);
    RUN_TEST(testWebApiRouteEventPathRequiresGetMethod);

    return UNITY_END();
}
