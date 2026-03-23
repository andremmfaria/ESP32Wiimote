#include "../../../src/wifi/web_request_parser.h"

#include <cstdio>
#include <cstring>
#include <unity.h>

// ===== Valid JSON Tests =====

void testParseSimpleCommand() {
    const char *body = "{\"command\": \"scan_start\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_STRING("scan_start", cmd.verb);
    TEST_ASSERT_EQUAL_UINT(0, cmd.fieldCount);
}

void testParseCommandWithSingleField() {
    const char *body = "{\"command\": \"set_led\", \"led\": \"1\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_STRING("set_led", cmd.verb);
    TEST_ASSERT_EQUAL_UINT(1, cmd.fieldCount);
    TEST_ASSERT_EQUAL_STRING("led", cmd.fields[0].key);
    TEST_ASSERT_EQUAL_STRING("1", cmd.fields[0].value);
}

void testParseCommandWithMultipleFields() {
    const char *body =
        "{\"command\": \"set_leds\", \"led1\": \"on\", \"led2\": \"off\", \"led3\": \"flash\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_STRING("set_leds", cmd.verb);
    TEST_ASSERT_EQUAL_UINT(3, cmd.fieldCount);
    TEST_ASSERT_EQUAL_STRING("led1", cmd.fields[0].key);
    TEST_ASSERT_EQUAL_STRING("on", cmd.fields[0].value);
    TEST_ASSERT_EQUAL_STRING("led2", cmd.fields[1].key);
    TEST_ASSERT_EQUAL_STRING("off", cmd.fields[1].value);
}

void testParseCommandWithWhitespace() {
    const char *body = "  {  \"command\"  :  \"discover_start\"  ,  \"timeout\"  :  \"30\"  }  ";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_STRING("discover_start", cmd.verb);
    TEST_ASSERT_EQUAL_UINT(1, cmd.fieldCount);
    TEST_ASSERT_EQUAL_STRING("timeout", cmd.fields[0].key);
    TEST_ASSERT_EQUAL_STRING("30", cmd.fields[0].value);
}

void testParseCommandWithEscapedCharacters() {
    const char *body = "{\"command\": \"test\", \"message\": \"Hello\\nWorld\\t!\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_STRING("test", cmd.verb);
    TEST_ASSERT_EQUAL_UINT(1, cmd.fieldCount);
}

// ===== Missing Field Tests =====

void testParseMissingCommand() {
    const char *body = "{\"action\": \"scan_start\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::MissingRequiredField, result);
}

void testParseEmptyObject() {
    const char *body = "{}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::MissingRequiredField, result);
}

// ===== Malformed JSON Tests =====

void testParseMissingOpeningBrace() {
    const char *body = "\"command\": \"scan_start\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::MalformedJson, result);
}

void testParseMissingClosingBrace() {
    const char *body = "{\"command\": \"scan_start\"";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::MalformedJson, result);
}

void testParseMissingColon() {
    const char *body = "{\"command\" \"scan_start\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::MalformedJson, result);
}

void testParseMissingComma() {
    const char *body = "{\"command\": \"scan_start\" \"timeout\": \"5\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::MalformedJson, result);
}

void testParseUnterminatedString() {
    const char *body = "{\"command\": \"scan_start}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    // Unterminated string causes parser error when looking for closing quote
    TEST_ASSERT_EQUAL(WebRequestParseResult::InvalidFieldType, result);
}

void testParseInvalidToken() {
    const char *body = "{\"command\": 123}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::InvalidFieldType, result);
}

// ===== Size Limit Tests =====

void testParseMaxBodySize() {
    // Create a body just under the limit with realistic content
    char body[kWebRequestMaxBodySize];
    std::snprintf(body, sizeof(body),
                  "{\"command\": \"test\", \"field1\": \"val1\", \"field2\": \"val2\"}");

    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
}

void testParseBodyTooLarge() {
    const char *body = "{}";
    WebParsedCommand cmd;
    // Try to parse with size exceeding limit
    WebRequestParseResult result = webRequestParse(body, kWebRequestMaxBodySize + 1, &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::BodyTooLarge, result);
}

void testParseEmptyBody() {
    const char *body = "";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, 0, &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::BodyTooLarge, result);
}

// ===== Field Overflow Tests =====

void testParseMaxFieldCount() {
    // Build JSON with many fields (up to limit)
    char body[512] = "{\"command\": \"test\"";
    for (int i = 0; i < kWebRequestMaxFieldCount; ++i) {
        std::strcat(body, ", \"field");
        char num[4];
        std::snprintf(num, sizeof(num), "%d", i);
        std::strcat(body, num);
        std::strcat(body, "\": \"value\"");
    }
    std::strcat(body, "}");

    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_UINT(kWebRequestMaxFieldCount, cmd.fieldCount);
}

void testParseTooManyFields() {
    // Build JSON with more fields than allowed
    char body[512] = "{\"command\": \"test\"";
    for (int i = 0; i < kWebRequestMaxFieldCount + 2; ++i) {
        std::strcat(body, ", \"field");
        char num[4];
        std::snprintf(num, sizeof(num), "%d", i);
        std::strcat(body, num);
        std::strcat(body, "\": \"value\"");
    }
    std::strcat(body, "}");

    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::BodyTooLarge, result);
}

// ===== Null Pointer Tests =====

void testParseNullBody() {
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(nullptr, 0, &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::MalformedJson, result);
}

void testParseNullCommand() {
    const char *body = "{\"command\": \"test\"}";
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), nullptr);

    TEST_ASSERT_EQUAL(WebRequestParseResult::MalformedJson, result);
}

// ===== Edge Cases =====

void testParseCommandFieldAtEnd() {
    // "command" field is at the end
    const char *body = "{\"timeout\": \"30\", \"command\": \"delayed_scan\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_STRING("delayed_scan", cmd.verb);
    TEST_ASSERT_EQUAL_UINT(1, cmd.fieldCount);
}

void testParseEmptyStringValue() {
    const char *body = "{\"command\": \"test\", \"field\": \"\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_STRING("", cmd.fields[0].value);
}

void testParseSpecialCharactersInValue() {
    const char *body = "{\"command\": \"test\", \"mac\": \"00:11:22:33:44:55\"}";
    WebParsedCommand cmd;
    WebRequestParseResult result = webRequestParse(body, std::strlen(body), &cmd);

    TEST_ASSERT_EQUAL(WebRequestParseResult::Ok, result);
    TEST_ASSERT_EQUAL_STRING("00:11:22:33:44:55", cmd.fields[0].value);
}

// ===== Main =====

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    // Valid JSON tests
    RUN_TEST(testParseSimpleCommand);
    RUN_TEST(testParseCommandWithSingleField);
    RUN_TEST(testParseCommandWithMultipleFields);
    RUN_TEST(testParseCommandWithWhitespace);
    RUN_TEST(testParseCommandWithEscapedCharacters);

    // Missing field tests
    RUN_TEST(testParseMissingCommand);
    RUN_TEST(testParseEmptyObject);

    // Malformed JSON tests
    RUN_TEST(testParseMissingOpeningBrace);
    RUN_TEST(testParseMissingClosingBrace);
    RUN_TEST(testParseMissingColon);
    RUN_TEST(testParseMissingComma);
    RUN_TEST(testParseUnterminatedString);
    RUN_TEST(testParseInvalidToken);

    // Size limit tests
    RUN_TEST(testParseMaxBodySize);
    RUN_TEST(testParseBodyTooLarge);
    RUN_TEST(testParseEmptyBody);

    // Field overflow tests
    RUN_TEST(testParseMaxFieldCount);
    RUN_TEST(testParseTooManyFields);

    // Null pointer tests
    RUN_TEST(testParseNullBody);
    RUN_TEST(testParseNullCommand);

    // Edge cases
    RUN_TEST(testParseCommandFieldAtEnd);
    RUN_TEST(testParseEmptyStringValue);
    RUN_TEST(testParseSpecialCharactersInValue);

    return UNITY_END();
}
