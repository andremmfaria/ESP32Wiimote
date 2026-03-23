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

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testSessionStartsLocked);
    RUN_TEST(testUnlockWithDurationEnablesWindow);
    RUN_TEST(testLockClearsWindow);
    RUN_TEST(testUnlockZeroDurationKeepsLocked);
    RUN_TEST(testWrapAroundElapsedCheckWorks);

    return UNITY_END();
}
