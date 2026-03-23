#include "../../../src/serial/serial_command_dispatcher.h"
#include "../../../src/serial/serial_command_parser.h"

#include <cstring>
#include <unity.h>

// ---------------------------------------------------------------------------
// Mock target
// ---------------------------------------------------------------------------

struct MockTarget : SerialCommandTarget {
    // Recorded calls
    bool setLedsCalledWith{false};
    uint8_t lastLedMask{0};

    bool setReportingModeCalledWith{false};
    uint8_t lastReportMode{0};
    bool lastReportContinuous{false};

    bool setAccelCalledWith{false};
    bool lastAccelEnabled{false};

    bool requestStatusCalled{false};

    bool setScanCalledWith{false};
    bool lastScanEnabled{false};

    bool startDiscoveryCalled{false};
    bool stopDiscoveryCalled{false};

    bool disconnectCalled{false};
    uint8_t lastDisconnectReason{0};

    bool setAutoReconnectCalledWith{false};
    bool lastAutoReconnect{false};

    bool clearReconnectCacheCalled{false};

    // Controllable state
    bool connected{true};
    uint8_t battery{75};

    // Return values for bool-returning methods
    bool retSetLeds{true};
    bool retSetMode{true};
    bool retSetAccel{true};
    bool retRequestStatus{true};
    bool retStartDiscovery{true};
    bool retStopDiscovery{true};
    bool retDisconnect{true};

    bool setLeds(uint8_t ledMask) override {
        setLedsCalledWith = true;
        lastLedMask = ledMask;
        return retSetLeds;
    }
    bool setReportingMode(uint8_t mode, bool continuous) override {
        setReportingModeCalledWith = true;
        lastReportMode = mode;
        lastReportContinuous = continuous;
        return retSetMode;
    }
    bool setAccelerometerEnabled(bool enabled) override {
        setAccelCalledWith = true;
        lastAccelEnabled = enabled;
        return retSetAccel;
    }
    bool requestStatus() override {
        requestStatusCalled = true;
        return retRequestStatus;
    }
    void setScanEnabled(bool enabled) override {
        setScanCalledWith = true;
        lastScanEnabled = enabled;
    }
    bool startDiscovery() override {
        startDiscoveryCalled = true;
        return retStartDiscovery;
    }
    bool stopDiscovery() override {
        stopDiscoveryCalled = true;
        return retStopDiscovery;
    }
    bool disconnectActiveController(uint8_t reason) override {
        disconnectCalled = true;
        lastDisconnectReason = reason;
        return retDisconnect;
    }
    void setAutoReconnectEnabled(bool enabled) override {
        setAutoReconnectCalledWith = true;
        lastAutoReconnect = enabled;
    }
    void clearReconnectCache() override {
        clearReconnectCacheCalled = true;
    }
    bool isConnected() const override { return connected; }
    uint8_t getBatteryLevel() const override { return battery; }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SerialDispatchResult dispatch(const char *line, MockTarget *target) {
    SerialParsedCommand cmd;
    serialCommandParse(line, &cmd);
    return serialCommandDispatch(cmd, target);
}

void setUp() {}
void tearDown() {}

// ---------------------------------------------------------------------------
// serialParseBool
// ---------------------------------------------------------------------------

void testParseBoolOn() {
    bool val = false;
    TEST_ASSERT_TRUE(serialParseBool("on", &val));
    TEST_ASSERT_TRUE(val);
}

void testParseBoolOff() {
    bool val = true;
    TEST_ASSERT_TRUE(serialParseBool("off", &val));
    TEST_ASSERT_FALSE(val);
}

void testParseBoolTrue() {
    bool val = false;
    TEST_ASSERT_TRUE(serialParseBool("true", &val));
    TEST_ASSERT_TRUE(val);
}

void testParseBoolFalse() {
    bool val = true;
    TEST_ASSERT_TRUE(serialParseBool("false", &val));
    TEST_ASSERT_FALSE(val);
}

void testParseBool1() {
    bool val = false;
    TEST_ASSERT_TRUE(serialParseBool("1", &val));
    TEST_ASSERT_TRUE(val);
}

void testParseBool0() {
    bool val = true;
    TEST_ASSERT_TRUE(serialParseBool("0", &val));
    TEST_ASSERT_FALSE(val);
}

void testParseBoolInvalid() {
    bool val = false;
    TEST_ASSERT_FALSE(serialParseBool("yes", &val));
    TEST_ASSERT_FALSE(serialParseBool("", &val));
    TEST_ASSERT_FALSE(serialParseBool("2", &val));
}

// ---------------------------------------------------------------------------
// serialParseUint8
// ---------------------------------------------------------------------------

void testParseUint8Decimal() {
    uint8_t val = 0;
    TEST_ASSERT_TRUE(serialParseUint8("42", &val));
    TEST_ASSERT_EQUAL_UINT8(42, val);
}

void testParseUint8Hex() {
    uint8_t val = 0;
    TEST_ASSERT_TRUE(serialParseUint8("0x1F", &val));
    TEST_ASSERT_EQUAL_UINT8(0x1F, val);
}

void testParseUint8MaxValue() {
    uint8_t val = 0;
    TEST_ASSERT_TRUE(serialParseUint8("255", &val));
    TEST_ASSERT_EQUAL_UINT8(255, val);
}

void testParseUint8Overflow() {
    uint8_t val = 0;
    TEST_ASSERT_FALSE(serialParseUint8("256", &val));
}

void testParseUint8Invalid() {
    uint8_t val = 0;
    TEST_ASSERT_FALSE(serialParseUint8("abc", &val));
    TEST_ASSERT_FALSE(serialParseUint8("", &val));
}

// ---------------------------------------------------------------------------
// serialParseUint16
// ---------------------------------------------------------------------------

void testParseUint16Decimal() {
    uint16_t val = 0;
    TEST_ASSERT_TRUE(serialParseUint16("1000", &val));
    TEST_ASSERT_EQUAL_UINT16(1000, val);
}

void testParseUint16Hex() {
    uint16_t val = 0;
    TEST_ASSERT_TRUE(serialParseUint16("0x0400", &val));
    TEST_ASSERT_EQUAL_UINT16(0x0400, val);
}

void testParseUint16Overflow() {
    uint16_t val = 0;
    TEST_ASSERT_FALSE(serialParseUint16("65536", &val));
}

// ---------------------------------------------------------------------------
// Unknown command
// ---------------------------------------------------------------------------

void testUnknownVerbReturnsUnknownCommand() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::UnknownCommand, dispatch("wm bogus", &t));
}

void testWmAloneReturnsUnknownCommand() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::UnknownCommand, dispatch("wm", &t));
}

// ---------------------------------------------------------------------------
// wm status
// ---------------------------------------------------------------------------

void testStatusReturnsOk() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok, dispatch("wm status", &t));
}

// ---------------------------------------------------------------------------
// wm led
// ---------------------------------------------------------------------------

void testLedCallsSetLedsOnConnect() {
    MockTarget t;
    t.connected = true;
    SerialDispatchResult r = dispatch("wm led 0x05", &t);
    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok, r);
    TEST_ASSERT_TRUE(t.setLedsCalledWith);
    TEST_ASSERT_EQUAL_UINT8(0x05, t.lastLedMask);
}

void testLedMissingArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::MissingArgument, dispatch("wm led", &t));
}

void testLedBadArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::BadArgument, dispatch("wm led xyz", &t));
}

void testLedNotConnected() {
    MockTarget t;
    t.connected = false;
    TEST_ASSERT_EQUAL(SerialDispatchResult::NotConnected, dispatch("wm led 0x01", &t));
}

void testLedRejectedByImpl() {
    MockTarget t;
    t.connected = true;
    t.retSetLeds = false;
    TEST_ASSERT_EQUAL(SerialDispatchResult::Rejected, dispatch("wm led 0x01", &t));
}

// ---------------------------------------------------------------------------
// wm mode
// ---------------------------------------------------------------------------

void testModeCallsSetReportingMode() {
    MockTarget t;
    t.connected = true;
    SerialDispatchResult r = dispatch("wm mode 0x31 on", &t);
    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok, r);
    TEST_ASSERT_TRUE(t.setReportingModeCalledWith);
    TEST_ASSERT_EQUAL_UINT8(0x31, t.lastReportMode);
    TEST_ASSERT_TRUE(t.lastReportContinuous);
}

void testModeDefaultContinuousFalse() {
    MockTarget t;
    t.connected = true;
    dispatch("wm mode 0x30", &t);
    TEST_ASSERT_FALSE(t.lastReportContinuous);
}

void testModeMissingArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::MissingArgument, dispatch("wm mode", &t));
}

// ---------------------------------------------------------------------------
// wm accel
// ---------------------------------------------------------------------------

void testAccelEnableCallsTarget() {
    MockTarget t;
    SerialDispatchResult r = dispatch("wm accel on", &t);
    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok, r);
    TEST_ASSERT_TRUE(t.setAccelCalledWith);
    TEST_ASSERT_TRUE(t.lastAccelEnabled);
}

void testAccelDisable() {
    MockTarget t;
    dispatch("wm accel off", &t);
    TEST_ASSERT_FALSE(t.lastAccelEnabled);
}

void testAccelMissingArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::MissingArgument, dispatch("wm accel", &t));
}

void testAccelBadArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::BadArgument, dispatch("wm accel maybe", &t));
}

// ---------------------------------------------------------------------------
// wm request-status
// ---------------------------------------------------------------------------

void testRequestStatusCallsTarget() {
    MockTarget t;
    t.connected = true;
    SerialDispatchResult r = dispatch("wm request-status", &t);
    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok, r);
    TEST_ASSERT_TRUE(t.requestStatusCalled);
}

void testRequestStatusNotConnected() {
    MockTarget t;
    t.connected = false;
    TEST_ASSERT_EQUAL(SerialDispatchResult::NotConnected, dispatch("wm request-status", &t));
}

// ---------------------------------------------------------------------------
// wm scan
// ---------------------------------------------------------------------------

void testScanOnCallsTargetWithTrue() {
    MockTarget t;
    dispatch("wm scan on", &t);
    TEST_ASSERT_TRUE(t.setScanCalledWith);
    TEST_ASSERT_TRUE(t.lastScanEnabled);
}

void testScanOffCallsTargetWithFalse() {
    MockTarget t;
    dispatch("wm scan off", &t);
    TEST_ASSERT_FALSE(t.lastScanEnabled);
}

void testScanMissingArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::MissingArgument, dispatch("wm scan", &t));
}

// ---------------------------------------------------------------------------
// wm discover
// ---------------------------------------------------------------------------

void testDiscoverStartCallsStartDiscovery() {
    MockTarget t;
    SerialDispatchResult r = dispatch("wm discover start", &t);
    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok, r);
    TEST_ASSERT_TRUE(t.startDiscoveryCalled);
}

void testDiscoverStopCallsStopDiscovery() {
    MockTarget t;
    SerialDispatchResult r = dispatch("wm discover stop", &t);
    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok, r);
    TEST_ASSERT_TRUE(t.stopDiscoveryCalled);
}

void testDiscoverUnknownSubcommandIsBadArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::BadArgument, dispatch("wm discover pause", &t));
}

void testDiscoverMissingArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::MissingArgument, dispatch("wm discover", &t));
}

void testDiscoverStartRejectedByImpl() {
    MockTarget t;
    t.retStartDiscovery = false;
    TEST_ASSERT_EQUAL(SerialDispatchResult::Rejected, dispatch("wm discover start", &t));
}

// ---------------------------------------------------------------------------
// wm disconnect
// ---------------------------------------------------------------------------

void testDisconnectDefaultReasonIsLocalHost() {
    MockTarget t;
    t.connected = true;
    dispatch("wm disconnect", &t);
    TEST_ASSERT_EQUAL_UINT8(0x16, t.lastDisconnectReason);
}

void testDisconnectCustomReason() {
    MockTarget t;
    t.connected = true;
    dispatch("wm disconnect 0x13", &t);
    TEST_ASSERT_EQUAL_UINT8(0x13, t.lastDisconnectReason);
}

void testDisconnectNotConnected() {
    MockTarget t;
    t.connected = false;
    TEST_ASSERT_EQUAL(SerialDispatchResult::NotConnected, dispatch("wm disconnect", &t));
}

void testDisconnectBadReason() {
    MockTarget t;
    t.connected = true;
    TEST_ASSERT_EQUAL(SerialDispatchResult::BadArgument, dispatch("wm disconnect bad", &t));
}

// ---------------------------------------------------------------------------
// wm reconnect
// ---------------------------------------------------------------------------

void testReconnectOnEnablesAutoReconnect() {
    MockTarget t;
    dispatch("wm reconnect on", &t);
    TEST_ASSERT_TRUE(t.setAutoReconnectCalledWith);
    TEST_ASSERT_TRUE(t.lastAutoReconnect);
}

void testReconnectOffDisablesAutoReconnect() {
    MockTarget t;
    dispatch("wm reconnect off", &t);
    TEST_ASSERT_FALSE(t.lastAutoReconnect);
}

void testReconnectClearCallsClearCache() {
    MockTarget t;
    SerialDispatchResult r = dispatch("wm reconnect clear", &t);
    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok, r);
    TEST_ASSERT_TRUE(t.clearReconnectCacheCalled);
}

void testReconnectMissingArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::MissingArgument, dispatch("wm reconnect", &t));
}

void testReconnectBadArgument() {
    MockTarget t;
    TEST_ASSERT_EQUAL(SerialDispatchResult::BadArgument,
                      dispatch("wm reconnect maybe", &t));
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testParseBoolOn);
    RUN_TEST(testParseBoolOff);
    RUN_TEST(testParseBoolTrue);
    RUN_TEST(testParseBoolFalse);
    RUN_TEST(testParseBool1);
    RUN_TEST(testParseBool0);
    RUN_TEST(testParseBoolInvalid);

    RUN_TEST(testParseUint8Decimal);
    RUN_TEST(testParseUint8Hex);
    RUN_TEST(testParseUint8MaxValue);
    RUN_TEST(testParseUint8Overflow);
    RUN_TEST(testParseUint8Invalid);

    RUN_TEST(testParseUint16Decimal);
    RUN_TEST(testParseUint16Hex);
    RUN_TEST(testParseUint16Overflow);

    RUN_TEST(testUnknownVerbReturnsUnknownCommand);
    RUN_TEST(testWmAloneReturnsUnknownCommand);

    RUN_TEST(testStatusReturnsOk);

    RUN_TEST(testLedCallsSetLedsOnConnect);
    RUN_TEST(testLedMissingArgument);
    RUN_TEST(testLedBadArgument);
    RUN_TEST(testLedNotConnected);
    RUN_TEST(testLedRejectedByImpl);

    RUN_TEST(testModeCallsSetReportingMode);
    RUN_TEST(testModeDefaultContinuousFalse);
    RUN_TEST(testModeMissingArgument);

    RUN_TEST(testAccelEnableCallsTarget);
    RUN_TEST(testAccelDisable);
    RUN_TEST(testAccelMissingArgument);
    RUN_TEST(testAccelBadArgument);

    RUN_TEST(testRequestStatusCallsTarget);
    RUN_TEST(testRequestStatusNotConnected);

    RUN_TEST(testScanOnCallsTargetWithTrue);
    RUN_TEST(testScanOffCallsTargetWithFalse);
    RUN_TEST(testScanMissingArgument);

    RUN_TEST(testDiscoverStartCallsStartDiscovery);
    RUN_TEST(testDiscoverStopCallsStopDiscovery);
    RUN_TEST(testDiscoverUnknownSubcommandIsBadArgument);
    RUN_TEST(testDiscoverMissingArgument);
    RUN_TEST(testDiscoverStartRejectedByImpl);

    RUN_TEST(testDisconnectDefaultReasonIsLocalHost);
    RUN_TEST(testDisconnectCustomReason);
    RUN_TEST(testDisconnectNotConnected);
    RUN_TEST(testDisconnectBadReason);

    RUN_TEST(testReconnectOnEnablesAutoReconnect);
    RUN_TEST(testReconnectOffDisablesAutoReconnect);
    RUN_TEST(testReconnectClearCallsClearCache);
    RUN_TEST(testReconnectMissingArgument);
    RUN_TEST(testReconnectBadArgument);

    return UNITY_END();
}
