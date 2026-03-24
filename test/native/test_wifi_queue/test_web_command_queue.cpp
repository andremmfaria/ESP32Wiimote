#include "../../../src/wifi/web_command_queue.h"

#include <cstring>
#include <unity.h>

static WebCommandQueue gQueue;

void setUp() {
    webCommandQueueInit(&gQueue);
}

void tearDown() {}

void testQueueInitStartsEmpty() {
    TEST_ASSERT_EQUAL_UINT32(1U, gQueue.nextId);
    TEST_ASSERT_EQUAL_UINT32(0U, webCommandQueueCount(&gQueue));
}

void testEnqueueStoresEntryWithQueuedPendingState() {
    uint32_t commandId = 0U;
    bool enqueued =
        webCommandQueueEnqueue(&gQueue, "/api/wiimote/commands/leds", "set_leds", &commandId);

    TEST_ASSERT_TRUE(enqueued);
    TEST_ASSERT_NOT_EQUAL(0U, commandId);

    WebCommandQueueEntry entry = {};
    TEST_ASSERT_TRUE(webCommandQueueGet(&gQueue, commandId, &entry));
    TEST_ASSERT_TRUE(entry.inUse);
    TEST_ASSERT_EQUAL(WebCommandQueueStatus::Queued, entry.status);
    TEST_ASSERT_EQUAL(WebCommandQueueResult::Pending, entry.result);
    TEST_ASSERT_EQUAL_STRING("/api/wiimote/commands/leds", entry.path);
    TEST_ASSERT_EQUAL_STRING("set_leds", entry.verb);
}

void testEnqueueIncrementsCommandId() {
    uint32_t firstId = 0U;
    uint32_t secondId = 0U;

    TEST_ASSERT_TRUE(
        webCommandQueueEnqueue(&gQueue, "/api/wiimote/commands/scan", "scan_start", &firstId));
    TEST_ASSERT_TRUE(
        webCommandQueueEnqueue(&gQueue, "/api/wiimote/commands/scan", "scan_stop", &secondId));

    TEST_ASSERT_EQUAL_UINT32(firstId + 1U, secondId);
    TEST_ASSERT_EQUAL_UINT32(2U, webCommandQueueCount(&gQueue));
}

void testQueueFullReturnsFalse() {
    for (size_t i = 0U; i < kWebCommandQueueCapacity; ++i) {
        uint32_t commandId = 0U;
        TEST_ASSERT_TRUE(webCommandQueueEnqueue(&gQueue, "/api/wiimote/commands/request-status",
                                                "request_status", &commandId));
    }

    uint32_t overflowId = 0U;
    TEST_ASSERT_FALSE(webCommandQueueEnqueue(&gQueue, "/api/wiimote/commands/request-status",
                                             "request_status", &overflowId));
}

void testUpdateChangesStatusAndResult() {
    uint32_t commandId = 0U;
    TEST_ASSERT_TRUE(webCommandQueueEnqueue(&gQueue, "/api/wiimote/commands/disconnect",
                                            "disconnect", &commandId));

    TEST_ASSERT_TRUE(webCommandQueueUpdate(&gQueue, commandId, WebCommandQueueStatus::Completed,
                                           WebCommandQueueResult::Accepted));

    WebCommandQueueEntry entry = {};
    TEST_ASSERT_TRUE(webCommandQueueGet(&gQueue, commandId, &entry));
    TEST_ASSERT_EQUAL(WebCommandQueueStatus::Completed, entry.status);
    TEST_ASSERT_EQUAL(WebCommandQueueResult::Accepted, entry.result);
}

void testInitNullIsNoOp() {
    webCommandQueueInit(nullptr);
}

void testEnqueueNullQueueReturnsFalse() {
    uint32_t commandId = 0U;
    TEST_ASSERT_FALSE(
        webCommandQueueEnqueue(nullptr, "/api/wiimote/commands/leds", "set_leds", &commandId));
}

void testEnqueueOversizedPathReturnsFalse() {
    // kWebCommandQueuePathMaxLen == 64; a 64-char path causes copyBounded to fail
    static const char kOversizedPath[] =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    uint32_t commandId = 0U;
    TEST_ASSERT_FALSE(webCommandQueueEnqueue(&gQueue, kOversizedPath, "verb", &commandId));
    TEST_ASSERT_EQUAL_UINT32(0U, webCommandQueueCount(&gQueue));
}

void testGetNullQueueReturnsFalse() {
    WebCommandQueueEntry entry = {};
    TEST_ASSERT_FALSE(webCommandQueueGet(nullptr, 1U, &entry));
}

void testGetUnknownIdReturnsFalse() {
    WebCommandQueueEntry entry = {};
    TEST_ASSERT_FALSE(webCommandQueueGet(&gQueue, 9999U, &entry));
}

void testUpdateNullQueueReturnsFalse() {
    TEST_ASSERT_FALSE(webCommandQueueUpdate(nullptr, 1U, WebCommandQueueStatus::Completed,
                                            WebCommandQueueResult::Accepted));
}

void testUpdateUnknownIdReturnsFalse() {
    TEST_ASSERT_FALSE(webCommandQueueUpdate(&gQueue, 9999U, WebCommandQueueStatus::Completed,
                                            WebCommandQueueResult::Accepted));
}

void testCountNullQueueReturnsZero() {
    TEST_ASSERT_EQUAL_UINT32(0U, webCommandQueueCount(nullptr));
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testQueueInitStartsEmpty);
    RUN_TEST(testEnqueueStoresEntryWithQueuedPendingState);
    RUN_TEST(testEnqueueIncrementsCommandId);
    RUN_TEST(testQueueFullReturnsFalse);
    RUN_TEST(testUpdateChangesStatusAndResult);

    RUN_TEST(testInitNullIsNoOp);
    RUN_TEST(testEnqueueNullQueueReturnsFalse);
    RUN_TEST(testEnqueueOversizedPathReturnsFalse);
    RUN_TEST(testGetNullQueueReturnsFalse);
    RUN_TEST(testGetUnknownIdReturnsFalse);
    RUN_TEST(testUpdateNullQueueReturnsFalse);
    RUN_TEST(testUpdateUnknownIdReturnsFalse);
    RUN_TEST(testCountNullQueueReturnsZero);

    return UNITY_END();
}
