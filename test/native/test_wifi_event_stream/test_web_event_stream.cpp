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
    size_t callCount;
    char frame[256];
    size_t frameLen;
    void *userData;
    char frames[32][256];
    size_t frameLens[32];
};

static SendCapture gInputCapture;
static SendCapture gStatusCapture;

static void recordCapture(SendCapture *capture,
                          const char *frame,
                          size_t frameLen,
                          void *userData) {
    if (capture == nullptr || frame == nullptr) {
        return;
    }

    capture->called = true;
    capture->frameLen = frameLen;
    capture->userData = userData;
    capture->callCount += 1U;

    std::strncpy(capture->frame, frame, sizeof(capture->frame) - 1U);
    capture->frame[sizeof(capture->frame) - 1U] = '\0';

    if (capture->callCount <= 32U) {
        const size_t kIdx = capture->callCount - 1U;
        std::strncpy(capture->frames[kIdx], frame, sizeof(capture->frames[kIdx]) - 1U);
        capture->frames[kIdx][sizeof(capture->frames[kIdx]) - 1U] = '\0';
        capture->frameLens[kIdx] = frameLen;
    }
}

static void spyInputSend(const char *frame, size_t frameLen, void *userData) {
    recordCapture(&gInputCapture, frame, frameLen, userData);
}

static void spyStatusSend(const char *frame, size_t frameLen, void *userData) {
    recordCapture(&gStatusCapture, frame, frameLen, userData);
}

static void spyReplaySend(const char *frame, size_t frameLen, void *userData) {
    SendCapture *capture = static_cast<SendCapture *>(userData);
    recordCapture(capture, frame, frameLen, nullptr);
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
    TEST_ASSERT_EQUAL_UINT32(1U, gInputCapture.callCount);
    TEST_ASSERT_GREATER_THAN(0U, gInputCapture.frameLen);
}

void testWebEventStreamPublishInputFrameIncludesSeqAndType() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Input, spyInputSend, nullptr);
    WebWiimoteInputSnapshot snap = {};
    webEventStreamPublishInput(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_NOT_NULL(std::strstr(gInputCapture.frame, "\"seq\":1"));
    TEST_ASSERT_NOT_NULL(std::strstr(gInputCapture.frame, "\"event\":\"input\""));
    TEST_ASSERT_NOT_NULL(std::strstr(gInputCapture.frame, "\"payload\":"));
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
    TEST_ASSERT_EQUAL_UINT32(1U, gStatusCapture.callCount);
    TEST_ASSERT_GREATER_THAN(0U, gStatusCapture.frameLen);
}

void testWebEventStreamPublishStatusFrameIncludesSeqAndType() {
    webEventStreamConnect(&gStream, WebEventStreamChannel::Status, spyStatusSend, nullptr);
    WebWiimoteStatusSnapshot snap = {};
    webEventStreamPublishStatus(&gStream, snap, gBuf, sizeof(gBuf));
    TEST_ASSERT_NOT_NULL(std::strstr(gStatusCapture.frame, "\"seq\":1"));
    TEST_ASSERT_NOT_NULL(std::strstr(gStatusCapture.frame, "\"event\":\"status\""));
    TEST_ASSERT_NOT_NULL(std::strstr(gStatusCapture.frame, "\"payload\":"));
}

// ===== Buffer / Replay / Seq Tests =====

void testWebEventStreamLatestSeqTracksPerChannel() {
    WebWiimoteInputSnapshot input = {};
    WebWiimoteStatusSnapshot status = {};

    webEventStreamPublishInput(&gStream, input, gBuf, sizeof(gBuf));
    webEventStreamPublishInput(&gStream, input, gBuf, sizeof(gBuf));
    webEventStreamPublishStatus(&gStream, status, gBuf, sizeof(gBuf));

    TEST_ASSERT_EQUAL_UINT32(2U, webEventStreamLatestSeq(&gStream, WebEventStreamChannel::Input));
    TEST_ASSERT_EQUAL_UINT32(1U, webEventStreamLatestSeq(&gStream, WebEventStreamChannel::Status));
}

void testWebEventStreamReplaySinceReplaysBufferedEvents() {
    WebWiimoteInputSnapshot input = {};
    webEventStreamPublishInput(&gStream, input, gBuf, sizeof(gBuf));
    webEventStreamPublishInput(&gStream, input, gBuf, sizeof(gBuf));
    webEventStreamPublishInput(&gStream, input, gBuf, sizeof(gBuf));

    SendCapture replayCapture = {};
    WebEventStreamReplayResult replay = {};
    TEST_ASSERT_TRUE(webEventStreamReplaySince(&gStream, WebEventStreamChannel::Input, 1U,
                                               spyReplaySend, &replayCapture, &replay));
    TEST_ASSERT_EQUAL_UINT32(2U, replay.replayedCount);
    TEST_ASSERT_EQUAL_UINT32(3U, replay.latestSeq);
    TEST_ASSERT_EQUAL_UINT32(4U, replay.nextSeq);
    TEST_ASSERT_FALSE(replay.requiresSnapshotRecovery);
    TEST_ASSERT_NOT_NULL(std::strstr(replayCapture.frames[0], "\"seq\":2"));
    TEST_ASSERT_NOT_NULL(std::strstr(replayCapture.frames[1], "\"seq\":3"));
}

void testWebEventStreamReplaySinceRequiresSnapshotRecoveryOnGap() {
    WebWiimoteInputSnapshot input = {};
    for (size_t i = 0U; i < (kWebEventStreamBufferCapacity + 2U); ++i) {
        webEventStreamPublishInput(&gStream, input, gBuf, sizeof(gBuf));
    }

    SendCapture replayCapture = {};
    WebEventStreamReplayResult replay = {};
    TEST_ASSERT_TRUE(webEventStreamReplaySince(&gStream, WebEventStreamChannel::Input, 1U,
                                               spyReplaySend, &replayCapture, &replay));
    TEST_ASSERT_EQUAL_UINT32(0U, replay.replayedCount);
    TEST_ASSERT_EQUAL_UINT32(kWebEventStreamBufferCapacity + 2U, replay.latestSeq);
    TEST_ASSERT_EQUAL_UINT32(kWebEventStreamBufferCapacity + 3U, replay.nextSeq);
    TEST_ASSERT_TRUE(replay.requiresSnapshotRecovery);
    TEST_ASSERT_FALSE(replayCapture.called);
}

void testWebEventStreamReplaySinceWithinBufferReplaysTail() {
    WebWiimoteInputSnapshot input = {};
    for (size_t i = 0U; i < (kWebEventStreamBufferCapacity + 2U); ++i) {
        webEventStreamPublishInput(&gStream, input, gBuf, sizeof(gBuf));
    }

    SendCapture replayCapture = {};
    WebEventStreamReplayResult replay = {};
    TEST_ASSERT_TRUE(webEventStreamReplaySince(&gStream, WebEventStreamChannel::Input, 4U,
                                               spyReplaySend, &replayCapture, &replay));
    TEST_ASSERT_EQUAL_UINT32(kWebEventStreamBufferCapacity - 2U, replay.replayedCount);
    TEST_ASSERT_EQUAL_UINT32(kWebEventStreamBufferCapacity + 2U, replay.latestSeq);
    TEST_ASSERT_FALSE(replay.requiresSnapshotRecovery);
    TEST_ASSERT_NOT_NULL(std::strstr(replayCapture.frames[0], "\"seq\":5"));
}

void testWebEventStreamReplaySinceReturnsNoEventsWhenUpToDate() {
    WebWiimoteStatusSnapshot status = {};
    webEventStreamPublishStatus(&gStream, status, gBuf, sizeof(gBuf));

    SendCapture replayCapture = {};
    WebEventStreamReplayResult replay = {};
    TEST_ASSERT_TRUE(webEventStreamReplaySince(&gStream, WebEventStreamChannel::Status, 1U,
                                               spyReplaySend, &replayCapture, &replay));
    TEST_ASSERT_EQUAL_UINT32(0U, replay.replayedCount);
    TEST_ASSERT_EQUAL_UINT32(1U, replay.latestSeq);
    TEST_ASSERT_EQUAL_UINT32(2U, replay.nextSeq);
    TEST_ASSERT_FALSE(replay.requiresSnapshotRecovery);
}

void testWebEventStreamReplaySinceRejectsInvalidArgs() {
    WebEventStreamReplayResult replay = {};
    TEST_ASSERT_FALSE(webEventStreamReplaySince(nullptr, WebEventStreamChannel::Input, 0U,
                                                spyReplaySend, nullptr, &replay));
    TEST_ASSERT_FALSE(webEventStreamReplaySince(&gStream, WebEventStreamChannel::Input, 0U, nullptr,
                                                nullptr, &replay));
    TEST_ASSERT_FALSE(webEventStreamReplaySince(&gStream, WebEventStreamChannel::Input, 0U,
                                                spyReplaySend, nullptr, nullptr));
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
    RUN_TEST(testWebEventStreamPublishInputFrameIncludesSeqAndType);
    RUN_TEST(testWebEventStreamPublishInputNoOpWhenNoClient);
    RUN_TEST(testWebEventStreamPublishInputDoesNotCallStatusSendFn);
    RUN_TEST(testWebEventStreamPublishInputForwardsUserData);

    RUN_TEST(testWebEventStreamPublishStatusCallsSendFn);
    RUN_TEST(testWebEventStreamPublishStatusFrameIncludesSeqAndType);
    RUN_TEST(testWebEventStreamPublishStatusNoOpWhenNoClient);
    RUN_TEST(testWebEventStreamPublishStatusDoesNotCallInputSendFn);
    RUN_TEST(testWebEventStreamPublishStatusForwardsUserData);

    RUN_TEST(testWebEventStreamLatestSeqTracksPerChannel);
    RUN_TEST(testWebEventStreamReplaySinceReplaysBufferedEvents);
    RUN_TEST(testWebEventStreamReplaySinceRequiresSnapshotRecoveryOnGap);
    RUN_TEST(testWebEventStreamReplaySinceWithinBufferReplaysTail);
    RUN_TEST(testWebEventStreamReplaySinceReturnsNoEventsWhenUpToDate);
    RUN_TEST(testWebEventStreamReplaySinceRejectsInvalidArgs);

    RUN_TEST(testWebApiRouteReturns101ForInputEventPath);
    RUN_TEST(testWebApiRouteReturns101ForStatusEventPath);
    RUN_TEST(testWebApiRouteIgnoresEventPathsWhenStreamNotConfigured);
    RUN_TEST(testWebApiRouteEventPathRequiresGetMethod);

    return UNITY_END();
}
