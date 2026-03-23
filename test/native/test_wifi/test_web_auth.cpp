#include "../../../src/wifi/web_auth.h"

#include <unity.h>

// ===== Bearer Token Tests =====

void testBearerValidTokenMatches() {
    WebAuthResult result = webAuthValidateBearer("Bearer esp32wiimote_bearer_token_v1");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerInvalidToken() {
    WebAuthResult result = webAuthValidateBearer("Bearer wrong_token");
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerEmptyToken() {
    WebAuthResult result = webAuthValidateBearer("Bearer ");
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerMissingSpace() {
    WebAuthResult result = webAuthValidateBearer("Beareresp32wiimote_bearer_token_v1");
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testBearerCaseInsensitiveScheme() {
    WebAuthResult result = webAuthValidateBearer("bearer esp32wiimote_bearer_token_v1");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerMixedCaseScheme() {
    WebAuthResult result = webAuthValidateBearer("BEARER esp32wiimote_bearer_token_v1");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerLeadingWhitespace() {
    WebAuthResult result = webAuthValidateBearer("  Bearer esp32wiimote_bearer_token_v1");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerWhitespaceAfterScheme() {
    WebAuthResult result = webAuthValidateBearer("Bearer   esp32wiimote_bearer_token_v1");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerNull() {
    WebAuthResult result = webAuthValidateBearer(nullptr);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testBearerWrongScheme() {
    WebAuthResult result = webAuthValidateBearer("Basic dXNlcjpwYXNzd29yZA==");
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

// ===== Basic Auth Tests (with built-in base64 decoding) =====

void testBasicValidCredentials() {
    // admin:password in base64 is "YWRtaW46cGFzc3dvcmQ="
    WebAuthResult result = webAuthValidateBasic("Basic YWRtaW46cGFzc3dvcmQ=");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicInvalidPassword() {
    // admin:wrongpass
    WebAuthResult result = webAuthValidateBasic("Basic YWRtaW46d3JvbmdwYXNz");
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBasicInvalidUsername() {
    // wronguser:password
    WebAuthResult result = webAuthValidateBasic("Basic d3Jvbmd1c2VyOnBhc3N3b3Jk");
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBasicMissingColon() {
    // adminpassword (no colon)
    // base64: YWRtaW5wYXNzd29yZA==
    WebAuthResult result = webAuthValidateBasic("Basic YWRtaW5wYXNzd29yZA==");
    TEST_ASSERT_EQUAL(WebAuthResult::MalformedHeader, result);
}

void testBasicCaseInsensitiveScheme() {
    WebAuthResult result = webAuthValidateBasic("basic YWRtaW46cGFzc3dvcmQ=");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicMixedCaseScheme() {
    WebAuthResult result = webAuthValidateBasic("BaSiC YWRtaW46cGFzc3dvcmQ=");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicLeadingWhitespace() {
    WebAuthResult result = webAuthValidateBasic("  Basic YWRtaW46cGFzc3dvcmQ=");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicWhitespaceAfterScheme() {
    WebAuthResult result = webAuthValidateBasic("Basic   YWRtaW46cGFzc3dvcmQ=");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicNull() {
    WebAuthResult result = webAuthValidateBasic(nullptr);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testBasicWrongScheme() {
    WebAuthResult result = webAuthValidateBasic("Bearer esp32wiimote_bearer_token_v1");
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testBasicEmptyBase64() {
    WebAuthResult result = webAuthValidateBasic("Basic ");
    TEST_ASSERT_EQUAL(WebAuthResult::MalformedHeader, result);
}

// ===== Combined Auth Tests =====

void testCombinedValidBearer() {
    WebAuthResult result = webAuthValidate("Bearer esp32wiimote_bearer_token_v1");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testCombinedValidBasic() {
    WebAuthResult result = webAuthValidate("Basic YWRtaW46cGFzc3dvcmQ=");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testCombinedInvalidBearer() {
    WebAuthResult result = webAuthValidate("Bearer wrong_token");
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testCombinedInvalidBasic() {
    // admin:wrongpass in base64 is YWRtaW46d3JvbmdwYXNz
    WebAuthResult result = webAuthValidate("Basic YWRtaW46d3JvbmdwYXNz");
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testCombinedUnsupportedScheme() {
    WebAuthResult result = webAuthValidate("Digest abc123");
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testCombinedNull() {
    WebAuthResult result = webAuthValidate(nullptr);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testCombinedEmpty() {
    WebAuthResult result = webAuthValidate("");
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

// ===== Edge Cases =====

void testBearerTokenWithLeadingTrailingWhitespace() {
    // Token itself contains whitespace (invalid)
    WebAuthResult result = webAuthValidate("Bearer  esp32wiimote_bearer_token_v1  ");
    // The extra spaces after token should make it not match
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBasicCredentialsWithLeadingSpaceInBase64() {
    // Space before base64 should be trimmed
    WebAuthResult result = webAuthValidate("Basic   YWRtaW46cGFzc3dvcmQ=");
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

// ===== Main =====

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    // Bearer tests
    RUN_TEST(testBearerValidTokenMatches);
    RUN_TEST(testBearerInvalidToken);
    RUN_TEST(testBearerEmptyToken);
    RUN_TEST(testBearerMissingSpace);
    RUN_TEST(testBearerCaseInsensitiveScheme);
    RUN_TEST(testBearerMixedCaseScheme);
    RUN_TEST(testBearerLeadingWhitespace);
    RUN_TEST(testBearerWhitespaceAfterScheme);
    RUN_TEST(testBearerNull);
    RUN_TEST(testBearerWrongScheme);

    // Basic tests
    RUN_TEST(testBasicValidCredentials);
    RUN_TEST(testBasicInvalidPassword);
    RUN_TEST(testBasicInvalidUsername);
    RUN_TEST(testBasicMissingColon);
    RUN_TEST(testBasicCaseInsensitiveScheme);
    RUN_TEST(testBasicMixedCaseScheme);
    RUN_TEST(testBasicLeadingWhitespace);
    RUN_TEST(testBasicWhitespaceAfterScheme);
    RUN_TEST(testBasicNull);
    RUN_TEST(testBasicWrongScheme);
    RUN_TEST(testBasicEmptyBase64);

    // Combined tests
    RUN_TEST(testCombinedValidBearer);
    RUN_TEST(testCombinedValidBasic);
    RUN_TEST(testCombinedInvalidBearer);
    RUN_TEST(testCombinedInvalidBasic);
    RUN_TEST(testCombinedUnsupportedScheme);
    RUN_TEST(testCombinedNull);
    RUN_TEST(testCombinedEmpty);

    // Edge cases
    RUN_TEST(testBearerTokenWithLeadingTrailingWhitespace);
    RUN_TEST(testBasicCredentialsWithLeadingSpaceInBase64);

    return UNITY_END();
}
