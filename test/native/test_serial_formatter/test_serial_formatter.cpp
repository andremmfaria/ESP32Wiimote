#include "../../../src/serial/serial_response_formatter.h"

#include <cstring>
#include <unity.h>

void setUp() {}
void tearDown() {}

void testFormatOk() {
    char out[32];
    const size_t kWritten = serialFormatOk(out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("@wm: ok", out);
    TEST_ASSERT_EQUAL(strlen("@wm: ok"), kWritten);
}

void testFormatOkQueued() {
    char out[32];
    const size_t kWritten = serialFormatOkQueued(out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("@wm: ok queued", out);
    TEST_ASSERT_EQUAL(strlen("@wm: ok queued"), kWritten);
}

void testFormatError() {
    char out[64];
    const size_t kWritten = serialFormatError(out, sizeof(out), "bad_argument");
    TEST_ASSERT_EQUAL_STRING("@wm: error bad_argument", out);
    TEST_ASSERT_EQUAL(strlen("@wm: error bad_argument"), kWritten);
}

void testFormatDispatchResultMappings() {
    char out[64];

    serialFormatDispatchResult(out, sizeof(out), SerialDispatchResult::Ok);
    TEST_ASSERT_EQUAL_STRING("@wm: ok", out);

    serialFormatDispatchResult(out, sizeof(out), SerialDispatchResult::NotConnected);
    TEST_ASSERT_EQUAL_STRING("@wm: error not_connected", out);

    serialFormatDispatchResult(out, sizeof(out), SerialDispatchResult::UnknownCommand);
    TEST_ASSERT_EQUAL_STRING("@wm: error unknown_command", out);

    serialFormatDispatchResult(out, sizeof(out), SerialDispatchResult::BadArgument);
    TEST_ASSERT_EQUAL_STRING("@wm: error bad_argument", out);

    serialFormatDispatchResult(out, sizeof(out), SerialDispatchResult::MissingArgument);
    TEST_ASSERT_EQUAL_STRING("@wm: error missing_argument", out);

    serialFormatDispatchResult(out, sizeof(out), SerialDispatchResult::Rejected);
    TEST_ASSERT_EQUAL_STRING("@wm: error rejected", out);
}

void testFormatParseResultMappings() {
    char out[64];

    serialFormatParseResult(out, sizeof(out), SerialParseResult::Ok);
    TEST_ASSERT_EQUAL_STRING("@wm: ok", out);

    serialFormatParseResult(out, sizeof(out), SerialParseResult::EmptyLine);
    TEST_ASSERT_EQUAL_STRING("@wm: error empty_line", out);

    serialFormatParseResult(out, sizeof(out), SerialParseResult::NotACommand);
    TEST_ASSERT_EQUAL_STRING("@wm: error not_a_command", out);

    serialFormatParseResult(out, sizeof(out), SerialParseResult::LineTooLong);
    TEST_ASSERT_EQUAL_STRING("@wm: error line_too_long", out);

    serialFormatParseResult(out, sizeof(out), SerialParseResult::TooManyTokens);
    TEST_ASSERT_EQUAL_STRING("@wm: error too_many_tokens", out);
}

void testFormatStatus() {
    char out[128];
    const size_t kWritten = serialFormatStatus(out, sizeof(out), true, false, 0x31, true, 87);
    TEST_ASSERT_EQUAL_STRING("@wm: status connected=1 scan=0 mode=0x31 accel=1 battery=87", out);
    TEST_ASSERT_EQUAL(strlen("@wm: status connected=1 scan=0 mode=0x31 accel=1 battery=87"),
                      kWritten);
}

void testFormatConfig() {
    char out[160];
    const size_t kWritten = serialFormatConfig(out, sizeof(out), true, 180000UL, false);
    TEST_ASSERT_EQUAL_STRING(
        "@wm: cfg auto_reconnect=1 fast_reconnect_ttl_ms=180000 serial_control=0", out);
    TEST_ASSERT_EQUAL(
        strlen("@wm: cfg auto_reconnect=1 fast_reconnect_ttl_ms=180000 serial_control=0"),
        kWritten);
}

void testBoundedWriteKeepsSentinelIntact() {
    struct GuardedBuf {
        char out[12];
        char guard;
    } buf;

    memset(buf.out, 'X', sizeof(buf.out));
    buf.guard = 'Z';

    const size_t kWritten = serialFormatError(buf.out, sizeof(buf.out), "this_is_long");

    TEST_ASSERT_EQUAL('Z', buf.guard);
    TEST_ASSERT_TRUE(kWritten >= sizeof(buf.out) - 1U);
    TEST_ASSERT_EQUAL('\0', buf.out[sizeof(buf.out) - 1]);
}

void testOutSizeOneAlwaysNullTerminated() {
    char out[1];
    out[0] = 'A';
    serialFormatStatus(out, sizeof(out), true, true, 0x30, false, 99);
    TEST_ASSERT_EQUAL('\0', out[0]);
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testFormatOk);
    RUN_TEST(testFormatOkQueued);
    RUN_TEST(testFormatError);
    RUN_TEST(testFormatDispatchResultMappings);
    RUN_TEST(testFormatParseResultMappings);
    RUN_TEST(testFormatStatus);
    RUN_TEST(testFormatConfig);
    RUN_TEST(testBoundedWriteKeepsSentinelIntact);
    RUN_TEST(testOutSizeOneAlwaysNullTerminated);

    return UNITY_END();
}
