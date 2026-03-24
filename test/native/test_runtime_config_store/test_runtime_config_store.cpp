#include "../../../src/config/runtime_config_store.h"
#include "nvs.h"

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

void testSaveBeforeInitReturnsFalse() {
    RuntimeConfigStore store;
    const RuntimeConfigSnapshot kSnap = {};
    TEST_ASSERT_FALSE(store.save(kSnap));
}

void testLoadBeforeInitReturnsFalse() {
    RuntimeConfigStore store;
    RuntimeConfigSnapshot snap = {};
    TEST_ASSERT_FALSE(store.load(&snap));
}

void testLoadNullSnapshotReturnsFalse() {
    RuntimeConfigStore store;
    TEST_ASSERT_TRUE(store.init());
    TEST_ASSERT_FALSE(store.load(nullptr));
}

void testClearBeforeInitReturnsFalse() {
    RuntimeConfigStore store;
    TEST_ASSERT_FALSE(store.clear());
}

void testPersistenceSurvivesSimulatedReboot() {
    {
        RuntimeConfigStore store;
        TEST_ASSERT_TRUE(store.init());
        RuntimeConfigSnapshot snap = {};
        snap.autoReconnectEnabled = true;
        snap.fastReconnectTtlMs = 200000U;
        snap.ledMask = 0x05U;
        snap.reportingMode = 0x31U;
        snap.reportingContinuous = false;
        TEST_ASSERT_TRUE(store.save(snap));
    }
    {
        RuntimeConfigStore store;
        TEST_ASSERT_TRUE(store.init());
        RuntimeConfigSnapshot loaded = {};
        TEST_ASSERT_TRUE(store.load(&loaded));
        TEST_ASSERT_TRUE(loaded.autoReconnectEnabled);
        TEST_ASSERT_EQUAL_UINT32(200000U, loaded.fastReconnectTtlMs);
        TEST_ASSERT_EQUAL_UINT8(0x05U, loaded.ledMask);
        TEST_ASSERT_EQUAL_UINT8(0x31U, loaded.reportingMode);
        TEST_ASSERT_FALSE(loaded.reportingContinuous);
    }
}

void testSaveOverwritesPreviousValue() {
    RuntimeConfigStore store;
    TEST_ASSERT_TRUE(store.init());

    RuntimeConfigSnapshot first = {};
    first.autoReconnectEnabled = true;
    first.fastReconnectTtlMs = 100000U;
    first.ledMask = 0x01U;
    first.reportingMode = 0x30U;
    first.reportingContinuous = false;
    TEST_ASSERT_TRUE(store.save(first));

    RuntimeConfigSnapshot second = {};
    second.autoReconnectEnabled = false;
    second.fastReconnectTtlMs = 250000U;
    second.ledMask = 0x0FU;
    second.reportingMode = 0x33U;
    second.reportingContinuous = true;
    TEST_ASSERT_TRUE(store.save(second));

    RuntimeConfigSnapshot loaded = {};
    TEST_ASSERT_TRUE(store.load(&loaded));
    TEST_ASSERT_FALSE(loaded.autoReconnectEnabled);
    TEST_ASSERT_EQUAL_UINT32(250000U, loaded.fastReconnectTtlMs);
    TEST_ASSERT_EQUAL_UINT8(0x0FU, loaded.ledMask);
    TEST_ASSERT_EQUAL_UINT8(0x33U, loaded.reportingMode);
    TEST_ASSERT_TRUE(loaded.reportingContinuous);
}

void testSaveFailsWhenNvsOpenFails() {
    RuntimeConfigStore store;
    TEST_ASSERT_TRUE(store.init());
    gMockNvsOpenFail = true;
    const RuntimeConfigSnapshot kSnap = {};
    const bool kResult = store.save(kSnap);
    gMockNvsOpenFail = false;
    TEST_ASSERT_FALSE(kResult);
}

void testLoadFailsWhenNvsOpenFails() {
    RuntimeConfigStore store;
    TEST_ASSERT_TRUE(store.init());
    gMockNvsOpenFail = true;
    RuntimeConfigSnapshot snap = {};
    const bool kResult = store.load(&snap);
    gMockNvsOpenFail = false;
    TEST_ASSERT_FALSE(kResult);
}

void testClearFailsWhenNvsOpenFails() {
    RuntimeConfigStore store;
    TEST_ASSERT_TRUE(store.init());
    gMockNvsOpenFail = true;
    const bool kResult = store.clear();
    gMockNvsOpenFail = false;
    TEST_ASSERT_FALSE(kResult);
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testInitThenLoadWithoutSaveReturnsFalse);
    RUN_TEST(testSaveThenLoadRestoresSnapshot);
    RUN_TEST(testClearRemovesStoredSnapshot);

    RUN_TEST(testSaveBeforeInitReturnsFalse);
    RUN_TEST(testLoadBeforeInitReturnsFalse);
    RUN_TEST(testLoadNullSnapshotReturnsFalse);
    RUN_TEST(testClearBeforeInitReturnsFalse);
    RUN_TEST(testPersistenceSurvivesSimulatedReboot);
    RUN_TEST(testSaveOverwritesPreviousValue);
    RUN_TEST(testSaveFailsWhenNvsOpenFails);
    RUN_TEST(testLoadFailsWhenNvsOpenFails);
    RUN_TEST(testClearFailsWhenNvsOpenFails);

    return UNITY_END();
}
