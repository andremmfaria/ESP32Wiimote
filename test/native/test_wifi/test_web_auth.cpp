#include "../../../src/wifi/web_auth.h"
#include "../../../src/wiimote_config.h"

#include <unity.h>

static const WiimoteCredentials kTestCreds = {
    "admin",
    "password",
    "esp32wiimote_bearer_token_v1",
};

static const WiimoteCredentials kOtherCreds = {
    "root",
    "secret",
    "other_token",
};

void testBearerValidTokenMatches() {
    WebAuthResult result =
        webAuthValidateBearer("Bearer esp32wiimote_bearer_token_v1", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerInvalidToken() {
    WebAuthResult result = webAuthValidateBearer("Bearer wrong_token", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerEmptyToken() {
    WebAuthResult result = webAuthValidateBearer("Bearer ", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerMissingSpace() {
    WebAuthResult result = webAuthValidateBearer("Beareresp32wiimote_bearer_token_v1", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testBearerCaseInsensitiveScheme() {
    WebAuthResult result =
        webAuthValidateBearer("bearer esp32wiimote_bearer_token_v1", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerMixedCaseScheme() {
    WebAuthResult result =
        webAuthValidateBearer("BEARER esp32wiimote_bearer_token_v1", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerLeadingWhitespace() {
    WebAuthResult result =
        webAuthValidateBearer("  Bearer esp32wiimote_bearer_token_v1", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerWhitespaceAfterScheme() {
    WebAuthResult result =
        webAuthValidateBearer("Bearer   esp32wiimote_bearer_token_v1", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBearerNull() {
    WebAuthResult result = webAuthValidateBearer(nullptr, &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testBearerWrongScheme() {
    WebAuthResult result = webAuthValidateBearer("Basic dXNlcjpwYXNzd29yZA==", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testBearerNullCredentialsReturnsInvalidCredentials() {
    WebAuthResult result = webAuthValidateBearer("Bearer esp32wiimote_bearer_token_v1", nullptr);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerCustomCredentialsMatch() {
    WebAuthResult result = webAuthValidateBearer("Bearer other_token", &kOtherCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicValidCredentials() {
    WebAuthResult result = webAuthValidateBasic("Basic YWRtaW46cGFzc3dvcmQ=", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicInvalidPassword() {
    WebAuthResult result = webAuthValidateBasic("Basic YWRtaW46d3JvbmdwYXNz", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBasicInvalidUsername() {
    WebAuthResult result = webAuthValidateBasic("Basic d3Jvbmd1c2VyOnBhc3N3b3Jk", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBasicMissingColon() {
    WebAuthResult result = webAuthValidateBasic("Basic YWRtaW5wYXNzd29yZA==", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::MalformedHeader, result);
}

void testBasicCaseInsensitiveScheme() {
    WebAuthResult result = webAuthValidateBasic("basic YWRtaW46cGFzc3dvcmQ=", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicMixedCaseScheme() {
    WebAuthResult result = webAuthValidateBasic("BaSiC YWRtaW46cGFzc3dvcmQ=", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicLeadingWhitespace() {
    WebAuthResult result = webAuthValidateBasic("  Basic YWRtaW46cGFzc3dvcmQ=", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicWhitespaceAfterScheme() {
    WebAuthResult result = webAuthValidateBasic("Basic   YWRtaW46cGFzc3dvcmQ=", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testBasicNull() {
    WebAuthResult result = webAuthValidateBasic(nullptr, &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testBasicWrongScheme() {
    WebAuthResult result = webAuthValidateBasic("Bearer esp32wiimote_bearer_token_v1", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testBasicEmptyBase64() {
    WebAuthResult result = webAuthValidateBasic("Basic ", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::MalformedHeader, result);
}

void testBasicNullCredentialsReturnsInvalidCredentials() {
    WebAuthResult result = webAuthValidateBasic("Basic YWRtaW46cGFzc3dvcmQ=", nullptr);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBasicCustomCredentialsMatch() {
    WebAuthResult result = webAuthValidateBasic("Basic cm9vdDpzZWNyZXQ=", &kOtherCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testCombinedValidBearer() {
    WebAuthResult result = webAuthValidate("Bearer esp32wiimote_bearer_token_v1", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testCombinedValidBasic() {
    WebAuthResult result = webAuthValidate("Basic YWRtaW46cGFzc3dvcmQ=", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
}

void testCombinedInvalidBearer() {
    WebAuthResult result = webAuthValidate("Bearer wrong_token", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testCombinedInvalidBasic() {
    WebAuthResult result = webAuthValidate("Basic YWRtaW46d3JvbmdwYXNz", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testCombinedUnsupportedScheme() {
    WebAuthResult result = webAuthValidate("Digest abc123", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::UnsupportedScheme, result);
}

void testCombinedNull() {
    WebAuthResult result = webAuthValidate(nullptr, &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testCombinedEmpty() {
    WebAuthResult result = webAuthValidate("", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::MissingHeader, result);
}

void testCombinedNullCredentialsReturnsInvalidCredentials() {
    WebAuthResult result = webAuthValidate("Bearer esp32wiimote_bearer_token_v1", nullptr);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBearerTokenWithLeadingTrailingWhitespace() {
    WebAuthResult result = webAuthValidate("Bearer  esp32wiimote_bearer_token_v1  ", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::InvalidCredentials, result);
}

void testBasicCredentialsWithLeadingSpaceInBase64() {
    WebAuthResult result = webAuthValidate("Basic   YWRtaW46cGFzc3dvcmQ=", &kTestCreds);
    TEST_ASSERT_EQUAL(WebAuthResult::Ok, result);
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
    RUN_TEST(testBearerWhitespaceAfterScheme);
    RUN_TEST(testBearerNull);
    RUN_TEST(testBearerWrongScheme);
    RUN_TEST(testBearerNullCredentialsReturnsInvalidCredentials);
    RUN_TEST(testBearerCustomCredentialsMatch);

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
    RUN_TEST(testBasicNullCredentialsReturnsInvalidCredentials);
    RUN_TEST(testBasicCustomCredentialsMatch);

    RUN_TEST(testCombinedValidBearer);
    RUN_TEST(testCombinedValidBasic);
    RUN_TEST(testCombinedInvalidBearer);
    RUN_TEST(testCombinedInvalidBasic);
    RUN_TEST(testCombinedUnsupportedScheme);
    RUN_TEST(testCombinedNull);
    RUN_TEST(testCombinedEmpty);
    RUN_TEST(testCombinedNullCredentialsReturnsInvalidCredentials);

    RUN_TEST(testBearerTokenWithLeadingTrailingWhitespace);
    RUN_TEST(testBasicCredentialsWithLeadingSpaceInBase64);

    return UNITY_END();
}
