#include "../../../src/serial/serial_command_session.h"

#include <unity.h>

void setUp() {}
void tearDown() {}

void testSessionStartsLocked() {
    SerialCommandSession session;
    TEST_ASSERT_FALSE(session.isUnlocked(0U));
    TEST_ASSERT_FALSE(session.isUnlocked(100U));
}

void testUnlockWithDurationEnablesWindow() {
    SerialCommandSession session;
    session.unlock(30000U, 1000U);

    TEST_ASSERT_TRUE(session.isUnlocked(1000U));
    TEST_ASSERT_TRUE(session.isUnlocked(30999U));
    TEST_ASSERT_FALSE(session.isUnlocked(31000U));
}

void testLockClearsWindow() {
    SerialCommandSession session;
    session.unlock(5000U, 50U);
    TEST_ASSERT_TRUE(session.isUnlocked(60U));

    session.lock();
    TEST_ASSERT_FALSE(session.isUnlocked(60U));
    TEST_ASSERT_FALSE(session.isUnlocked(1000U));
}

void testUnlockZeroDurationKeepsLocked() {
    SerialCommandSession session;
    session.unlock(0U, 500U);
    TEST_ASSERT_FALSE(session.isUnlocked(500U));
}

void testWrapAroundElapsedCheckWorks() {
    SerialCommandSession session;
    const uint32_t kNearWrap = 0xFFFFFFF0U;
    session.unlock(32U, kNearWrap);

    TEST_ASSERT_TRUE(session.isUnlocked(kNearWrap + 10U));
    TEST_ASSERT_FALSE(session.isUnlocked(kNearWrap + 40U));
}

void testSetCredentialsAndValidateMatching() {
    SerialCommandSession session;
    const WiimoteCredentials kCredentials = {"admin", "password", "token"};

    session.setCredentials(&kCredentials);

    TEST_ASSERT_TRUE(session.validateCredentials("admin", "password"));
}

void testSetCredentialsAndValidateWrongPassword() {
    SerialCommandSession session;
    const WiimoteCredentials kCredentials = {"admin", "password", "token"};

    session.setCredentials(&kCredentials);

    TEST_ASSERT_FALSE(session.validateCredentials("admin", "wrong"));
}

void testSetCredentialsAndValidateWrongUsername() {
    SerialCommandSession session;
    const WiimoteCredentials kCredentials = {"admin", "password", "token"};

    session.setCredentials(&kCredentials);

    TEST_ASSERT_FALSE(session.validateCredentials("wrong", "password"));
}

void testValidateNullInputWhenCredentialsSet() {
    SerialCommandSession session;
    const WiimoteCredentials kCredentials = {"admin", "password", "token"};

    session.setCredentials(&kCredentials);

    TEST_ASSERT_FALSE(session.validateCredentials(nullptr, "password"));
    TEST_ASSERT_FALSE(session.validateCredentials("admin", nullptr));
}

void testValidateWithNullCredentialsAllowsNoAuthMode() {
    SerialCommandSession session;

    TEST_ASSERT_TRUE(session.validateCredentials("anything", "anything"));
    TEST_ASSERT_TRUE(session.validateCredentials(nullptr, nullptr));
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testSessionStartsLocked);
    RUN_TEST(testUnlockWithDurationEnablesWindow);
    RUN_TEST(testLockClearsWindow);
    RUN_TEST(testUnlockZeroDurationKeepsLocked);
    RUN_TEST(testWrapAroundElapsedCheckWorks);
    RUN_TEST(testSetCredentialsAndValidateMatching);
    RUN_TEST(testSetCredentialsAndValidateWrongPassword);
    RUN_TEST(testSetCredentialsAndValidateWrongUsername);
    RUN_TEST(testValidateNullInputWhenCredentialsSet);
    RUN_TEST(testValidateWithNullCredentialsAllowsNoAuthMode);

    return UNITY_END();
}
