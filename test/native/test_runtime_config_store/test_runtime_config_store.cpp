#include "../../../src/config/runtime_config_store.h"

#include <unity.h>

void setUp() {
    RuntimeConfigStore store;
    store.init();
    store.clear();
}

void tearDown() {}

void testInitThenLoadWithoutSaveReturnsFalse() {
    RuntimeConfigStore store;
    RuntimeConfigSnapshot snapshot = {};

    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_FALSE(store.load(&snapshot));
}

void testSaveThenLoadRestoresSnapshot() {
    RuntimeConfigStore store;
    RuntimeConfigSnapshot written = {};
    written.autoReconnectEnabled = true;
    written.fastReconnectTtlMs = 180000U;
    written.ledMask = 0x0FU;
    written.reportingMode = 0x31U;
    written.reportingContinuous = true;

    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_TRUE(store.save(written));

    RuntimeConfigSnapshot loaded = {};
    TEST_ASSERT_TRUE(store.load(&loaded));
    TEST_ASSERT_TRUE(loaded.autoReconnectEnabled);
    TEST_ASSERT_EQUAL_UINT32(180000U, loaded.fastReconnectTtlMs);
    TEST_ASSERT_EQUAL_UINT8(0x0FU, loaded.ledMask);
    TEST_ASSERT_EQUAL_UINT8(0x31U, loaded.reportingMode);
    TEST_ASSERT_TRUE(loaded.reportingContinuous);
}

void testClearRemovesStoredSnapshot() {
    RuntimeConfigStore store;
    RuntimeConfigSnapshot written = {};
    written.autoReconnectEnabled = true;
    written.fastReconnectTtlMs = 120000U;
    written.ledMask = 0x03U;
    written.reportingMode = 0x35U;
    written.reportingContinuous = false;

    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_TRUE(store.save(written));
    TEST_ASSERT_TRUE(store.clear());

    RuntimeConfigSnapshot loaded = {};
    TEST_ASSERT_FALSE(store.load(&loaded));
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testInitThenLoadWithoutSaveReturnsFalse);
    RUN_TEST(testSaveThenLoadRestoresSnapshot);
    RUN_TEST(testClearRemovesStoredSnapshot);

    return UNITY_END();
}
