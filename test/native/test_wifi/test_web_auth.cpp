#include "../../../src/wifi/web_auth.h"

#include <unity.h>

static const char *kTestToken = "esp32wiimote_bearer_token_v1";
static const char *kOtherToken = "other_token";

void testBearerValidTokenMatches() {
    WebAuthResult result = webAuthValidateBearer("Bearer esp32wiimote_bearer_token_v1", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerInvalidToken() {
    WebAuthResult result = webAuthValidateBearer("Bearer wrong_token", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerEmptyToken() {
    WebAuthResult result = webAuthValidateBearer("Bearer ", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerMissingSpace() {
    WebAuthResult result = webAuthValidateBearer("Beareresp32wiimote_bearer_token_v1", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testBearerCaseInsensitiveScheme() {
    WebAuthResult result = webAuthValidateBearer("bearer esp32wiimote_bearer_token_v1", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerMixedCaseScheme() {
    WebAuthResult result = webAuthValidateBearer("BEARER esp32wiimote_bearer_token_v1", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerLeadingWhitespace() {
    WebAuthResult result =
        webAuthValidateBearer("  Bearer esp32wiimote_bearer_token_v1", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerLeadingTabWhitespace() {
    WebAuthResult result =
        webAuthValidateBearer("\tBearer esp32wiimote_bearer_token_v1", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerWhitespaceAfterScheme() {
    WebAuthResult result =
        webAuthValidateBearer("Bearer   esp32wiimote_bearer_token_v1", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerNull() {
    WebAuthResult result = webAuthValidateBearer(nullptr, kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testBearerWrongScheme() {
    WebAuthResult result = webAuthValidateBearer("Digest abc123", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testBearerNullTokenReturnsInvalidCredentials() {
    WebAuthResult result = webAuthValidateBearer("Bearer esp32wiimote_bearer_token_v1", nullptr);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerCustomTokenMatch() {
    WebAuthResult result = webAuthValidateBearer("Bearer other_token", kOtherToken);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testCombinedValidBearer() {
    WebAuthResult result = webAuthValidate("Bearer esp32wiimote_bearer_token_v1", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testCombinedInvalidBearer() {
    WebAuthResult result = webAuthValidate("Bearer wrong_token", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testCombinedUnsupportedScheme() {
    WebAuthResult result = webAuthValidate("Digest abc123", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testCombinedNull() {
    WebAuthResult result = webAuthValidate(nullptr, kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testCombinedEmpty() {
    WebAuthResult result = webAuthValidate("", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testCombinedWhitespaceOnly() {
    WebAuthResult result = webAuthValidate(" \t  ", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testCombinedNullTokenReturnsInvalidCredentials() {
    WebAuthResult result = webAuthValidate("Bearer esp32wiimote_bearer_token_v1", nullptr);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerTokenWithLeadingTrailingWhitespace() {
    WebAuthResult result = webAuthValidate("Bearer  esp32wiimote_bearer_token_v1  ", kTestToken);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testBearerValidTokenMatches);
    RUN_TEST(testBearerInvalidToken);
    RUN_TEST(testBearerEmptyToken);
    RUN_TEST(testBearerMissingSpace);
    RUN_TEST(testBearerCaseInsensitiveScheme);
    RUN_TEST(testBearerMixedCaseScheme);
    RUN_TEST(testBearerLeadingWhitespace);
    RUN_TEST(testBearerLeadingTabWhitespace);
    RUN_TEST(testBearerWhitespaceAfterScheme);
    RUN_TEST(testBearerNull);
    RUN_TEST(testBearerWrongScheme);
    RUN_TEST(testBearerNullTokenReturnsInvalidCredentials);
    RUN_TEST(testBearerCustomTokenMatch);

    RUN_TEST(testCombinedValidBearer);
    RUN_TEST(testCombinedInvalidBearer);
    RUN_TEST(testCombinedUnsupportedScheme);
    RUN_TEST(testCombinedNull);
    RUN_TEST(testCombinedEmpty);
    RUN_TEST(testCombinedWhitespaceOnly);
    RUN_TEST(testCombinedNullTokenReturnsInvalidCredentials);

    RUN_TEST(testBearerTokenWithLeadingTrailingWhitespace);

    return UNITY_END();
}
