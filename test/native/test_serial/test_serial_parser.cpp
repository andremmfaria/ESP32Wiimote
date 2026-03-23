#include "../../../src/serial/serial_command_parser.h"

#include <cstring>
#include <unity.h>

void setUp() {}
void tearDown() {}

// ---------------------------------------------------------------------------
// EmptyLine
// ---------------------------------------------------------------------------

void testEmptyStringIsEmptyLine() {
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::EmptyLine, serialCommandParse("", &cmd));
}

void testWhitespaceOnlyIsEmptyLine() {
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::EmptyLine, serialCommandParse("   \t  ", &cmd));
}

void testSingleTabIsEmptyLine() {
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::EmptyLine, serialCommandParse("\t", &cmd));
}

// ---------------------------------------------------------------------------
// NotACommand
// ---------------------------------------------------------------------------

void testUnknownPrefixIsNotACommand() {
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::NotACommand, serialCommandParse("hello world", &cmd));
}

void testWmPrefixCaseInsensitiveIsNotACommand() {
    SerialParsedCommand cmd;
    // Protocol requires lowercase "wm" exactly
    TEST_ASSERT_EQUAL(SerialParseResult::NotACommand, serialCommandParse("WM led 1", &cmd));
}

void testSingleWordNonWmIsNotACommand() {
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::NotACommand, serialCommandParse("status", &cmd));
}

void testWmSubstringPrefixIsNotACommand() {
    SerialParsedCommand cmd;
    // "wm2" is not the same as "wm"
    TEST_ASSERT_EQUAL(SerialParseResult::NotACommand, serialCommandParse("wm2 led", &cmd));
}

// ---------------------------------------------------------------------------
// LineTooLong
// ---------------------------------------------------------------------------

void testExactMaxLengthIsOk() {
    // Build a "wm " + padding of (128 - 3) 'x' characters = exactly 128 bytes
    char line[kSerialMaxLineLength + 1];
    memset(line, 'x', kSerialMaxLineLength);
    line[0] = 'w';
    line[1] = 'm';
    line[2] = ' ';
    line[kSerialMaxLineLength] = '\0';
    SerialParsedCommand cmd;
    // It starts with "wm " and is exactly 128 bytes — valid
    TEST_ASSERT_EQUAL(SerialParseResult::Ok, serialCommandParse(line, &cmd));
}

void testOneBeyondMaxLengthIsLineTooLong() {
    // 129 printable characters
    char line[kSerialMaxLineLength + 2];
    memset(line, 'x', kSerialMaxLineLength + 1);
    line[kSerialMaxLineLength + 1] = '\0';
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::LineTooLong, serialCommandParse(line, &cmd));
}

// ---------------------------------------------------------------------------
// TooManyTokens
// ---------------------------------------------------------------------------

void testExactlyMaxTokensIsOk() {
    // "wm t1 t2 t3 t4 t5 t6 t7 t8 t9" = 10 tokens
    SerialParsedCommand cmd;
    SerialParseResult r = serialCommandParse("wm t1 t2 t3 t4 t5 t6 t7 t8 t9", &cmd);
    TEST_ASSERT_EQUAL(SerialParseResult::Ok, r);
    TEST_ASSERT_EQUAL_UINT8(kSerialMaxTokens, cmd.tokenCount);
}

void testOneOverMaxTokensIsTooManyTokens() {
    // "wm t1 t2 t3 t4 t5 t6 t7 t8 t9 t10" = 11 tokens
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::TooManyTokens,
                      serialCommandParse("wm t1 t2 t3 t4 t5 t6 t7 t8 t9 t10", &cmd));
}

// ---------------------------------------------------------------------------
// Ok — successful parse
// ---------------------------------------------------------------------------

void testWmAloneIsOk() {
    SerialParsedCommand cmd;
    SerialParseResult r = serialCommandParse("wm", &cmd);
    TEST_ASSERT_EQUAL(SerialParseResult::Ok, r);
    TEST_ASSERT_EQUAL_UINT8(1, cmd.tokenCount);
    TEST_ASSERT_EQUAL_STRING("wm", cmd.tokens[0]);
}

void testWmWithSingleArgIsOk() {
    SerialParsedCommand cmd;
    SerialParseResult r = serialCommandParse("wm status", &cmd);
    TEST_ASSERT_EQUAL(SerialParseResult::Ok, r);
    TEST_ASSERT_EQUAL_UINT8(2, cmd.tokenCount);
    TEST_ASSERT_EQUAL_STRING("wm", cmd.tokens[0]);
    TEST_ASSERT_EQUAL_STRING("status", cmd.tokens[1]);
}

void testWmWithMultipleArgsTokensAreCorrect() {
    SerialParsedCommand cmd;
    SerialParseResult r = serialCommandParse("wm led 0x01", &cmd);
    TEST_ASSERT_EQUAL(SerialParseResult::Ok, r);
    TEST_ASSERT_EQUAL_UINT8(3, cmd.tokenCount);
    TEST_ASSERT_EQUAL_STRING("wm", cmd.tokens[0]);
    TEST_ASSERT_EQUAL_STRING("led", cmd.tokens[1]);
    TEST_ASSERT_EQUAL_STRING("0x01", cmd.tokens[2]);
}

void testLeadingAndTrailingWhitespaceIsTrimmed() {
    SerialParsedCommand cmd;
    SerialParseResult r = serialCommandParse("  wm  scan  on  ", &cmd);
    TEST_ASSERT_EQUAL(SerialParseResult::Ok, r);
    TEST_ASSERT_EQUAL_UINT8(3, cmd.tokenCount);
    TEST_ASSERT_EQUAL_STRING("wm", cmd.tokens[0]);
    TEST_ASSERT_EQUAL_STRING("scan", cmd.tokens[1]);
    TEST_ASSERT_EQUAL_STRING("on", cmd.tokens[2]);
}

void testTokensPointIntoBufNotOriginalLine() {
    SerialParsedCommand cmd;
    const char *line = "wm disconnect";
    serialCommandParse(line, &cmd);
    // tokens[0] must live inside cmd.buf, not in line
    TEST_ASSERT_TRUE(cmd.tokens[0] >= cmd.buf && cmd.tokens[0] < cmd.buf + sizeof(cmd.buf));
}

void testMixedTabAndSpaceSeparationIsOk() {
    SerialParsedCommand cmd;
    SerialParseResult r = serialCommandParse("wm\tmode\t0x31", &cmd);
    TEST_ASSERT_EQUAL(SerialParseResult::Ok, r);
    TEST_ASSERT_EQUAL_UINT8(3, cmd.tokenCount);
    TEST_ASSERT_EQUAL_STRING("wm", cmd.tokens[0]);
    TEST_ASSERT_EQUAL_STRING("mode", cmd.tokens[1]);
    TEST_ASSERT_EQUAL_STRING("0x31", cmd.tokens[2]);
}

// ---------------------------------------------------------------------------
// Priority: LineTooLong before NotACommand
// ---------------------------------------------------------------------------

void testLineTooLongBeforeNotACommand() {
    char line[kSerialMaxLineLength + 2];
    memset(line, 'a', kSerialMaxLineLength + 1);
    line[kSerialMaxLineLength + 1] = '\0';
    // The line is long AND doesn't start with "wm", LineTooLong wins
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::LineTooLong, serialCommandParse(line, &cmd));
}

// ---------------------------------------------------------------------------
// Priority: NotACommand before TooManyTokens
// ---------------------------------------------------------------------------

void testNotACommandBeforeTooManyTokens() {
    // 11 tokens, first is not "wm"
    SerialParsedCommand cmd;
    TEST_ASSERT_EQUAL(SerialParseResult::NotACommand,
                      serialCommandParse("xx t1 t2 t3 t4 t5 t6 t7 t8 t9 t10", &cmd));
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testEmptyStringIsEmptyLine);
    RUN_TEST(testWhitespaceOnlyIsEmptyLine);
    RUN_TEST(testSingleTabIsEmptyLine);
    RUN_TEST(testUnknownPrefixIsNotACommand);
    RUN_TEST(testWmPrefixCaseInsensitiveIsNotACommand);
    RUN_TEST(testSingleWordNonWmIsNotACommand);
    RUN_TEST(testWmSubstringPrefixIsNotACommand);
    RUN_TEST(testExactMaxLengthIsOk);
    RUN_TEST(testOneBeyondMaxLengthIsLineTooLong);
    RUN_TEST(testExactlyMaxTokensIsOk);
    RUN_TEST(testOneOverMaxTokensIsTooManyTokens);
    RUN_TEST(testWmAloneIsOk);
    RUN_TEST(testWmWithSingleArgIsOk);
    RUN_TEST(testWmWithMultipleArgsTokensAreCorrect);
    RUN_TEST(testLeadingAndTrailingWhitespaceIsTrimmed);
    RUN_TEST(testTokensPointIntoBufNotOriginalLine);
    RUN_TEST(testMixedTabAndSpaceSeparationIsOk);
    RUN_TEST(testLineTooLongBeforeNotACommand);
    RUN_TEST(testNotACommandBeforeTooManyTokens);

    return UNITY_END();
}
