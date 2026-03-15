#include "../../mocks/test_mocks.h"

#include <cstring>
#include <unity.h>

// Test fixtures uses real implementations from src/
L2capConnectionTable *connections;
L2capPacketSender *sender;
WiimoteProtocol *protocol;

static L2capConnection makeConnection(uint16_t ch, uint16_t remoteCID) {
    return L2capConnection({ch, remoteCID});
}

static WiimoteLedCommand ledCommand(uint8_t leds) {
    return WiimoteLedCommand{leds};
}

static WiimoteReportingModeCommand reportingModeCommand(uint8_t mode, bool continuous) {
    return WiimoteReportingModeCommand{mode, continuous};
}

void setUp(void) {
    connections = new L2capConnectionTable();
    sender = new L2capPacketSender();
    protocol = new WiimoteProtocol();

    // Set mock callback to capture packets for test verification
    sender->setSendCallback(mockL2capRawSendCallback);

    protocol->init(connections, sender);

    // Reset mock state
    mockLastPacketLen = 0;
    mockSendCallCount = 0;
    memset(mockLastPacket, 0, sizeof(mockLastPacket));
}

void tearDown(void) {
    delete protocol;
    delete sender;
    delete connections;
}

// ===== Connection Table Tests =====

// Test: Empty connection table
void testEmptyConnectionTable() {
    uint16_t remoteCID = 0;
    int result = connections->getRemoteCid(0x0040, &remoteCID);
    TEST_ASSERT_EQUAL(-1, result);  // Not found
}

// Test: Add and find connection
void testAddAndFindConnection() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    int result = connections->addConnection(conn);
    TEST_ASSERT_EQUAL(1, result);

    uint16_t remoteCID = 0;
    result = connections->getRemoteCid(0x0040, &remoteCID);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_UINT16(0x0041, remoteCID);
}

// Test: Get first connection handle
void testGetFirstConnectionHandle() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    uint16_t ch = 0;
    int result = connections->getFirstConnectionHandle(&ch);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_UINT16(0x0040, ch);
}

// Test: Clear connections
void testClearConnections() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    connections->clear();

    uint16_t remoteCID = 0;
    int result = connections->getRemoteCid(0x0040, &remoteCID);
    TEST_ASSERT_EQUAL(-1, result);  // Not found after clear
}

// ===== LED Control Tests =====

// Test: Set LEDs with no connection
void testSetLedsNoConnection() {
    protocol->setLeds(0x0040, ledCommand(0x0F));

    // Should not send anything
    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// Test: Set LEDs with connection
void testSetLedsWithConnection() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->setLeds(0x0040, ledCommand(0x01));  // LED 1

    // Verify packet was sent
    TEST_ASSERT_EQUAL(1, mockSendCallCount);
    TEST_ASSERT_EQUAL_UINT16(0x0040, mockLastChannelHandle);
    TEST_ASSERT_EQUAL_UINT16(0x0041, mockLastRemoteCID);

    // Verify packet format: A2 11 LL
    TEST_ASSERT_EQUAL(3, mockLastPacketLen);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);  // HID Output Report
    TEST_ASSERT_EQUAL_UINT8(0x11, mockLastPacket[1]);  // Set LEDs command
    TEST_ASSERT_EQUAL_UINT8(0x10, mockLastPacket[2]);  // LED bits (0x01 << 4)
}

// Test: Set all LEDs
void testSetAllLeds() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->setLeds(0x0040, ledCommand(0x0F));  // All LEDs

    TEST_ASSERT_EQUAL(3, mockLastPacketLen);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);
    TEST_ASSERT_EQUAL_UINT8(0x11, mockLastPacket[1]);
    TEST_ASSERT_EQUAL_UINT8(0xF0, mockLastPacket[2]);  // 0x0F << 4
}

// Test: Set individual LEDs
void testSetIndividualLeds() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    // LED 1
    protocol->setLeds(0x0040, ledCommand(0x01));
    TEST_ASSERT_EQUAL_UINT8(0x10, mockLastPacket[2]);

    // LED 2
    protocol->setLeds(0x0040, ledCommand(0x02));
    TEST_ASSERT_EQUAL_UINT8(0x20, mockLastPacket[2]);

    // LED 3
    protocol->setLeds(0x0040, ledCommand(0x04));
    TEST_ASSERT_EQUAL_UINT8(0x40, mockLastPacket[2]);

    // LED 4
    protocol->setLeds(0x0040, ledCommand(0x08));
    TEST_ASSERT_EQUAL_UINT8(0x80, mockLastPacket[2]);
}

// ===== Reporting Mode Tests =====

// Test: Set reporting mode with no connection
void testSetReportingModeNoConnection() {
    protocol->setReportingMode(0x0040, reportingModeCommand(0x30, false));

    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// Test: Set reporting mode continuous
void testSetReportingModeContinuous() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->setReportingMode(0x0040, reportingModeCommand(0x31, true));

    // Verify packet format: A2 12 TT MM
    TEST_ASSERT_EQUAL(4, mockLastPacketLen);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);  // HID Output Report
    TEST_ASSERT_EQUAL_UINT8(0x12, mockLastPacket[1]);  // Set Reporting Mode
    TEST_ASSERT_EQUAL_UINT8(0x04, mockLastPacket[2]);  // Continuous flag
    TEST_ASSERT_EQUAL_UINT8(0x31, mockLastPacket[3]);  // Mode 0x31
}

// Test: Set reporting mode non-continuous
void testSetReportingModeNonContinuous() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->setReportingMode(0x0040, reportingModeCommand(0x35, false));

    TEST_ASSERT_EQUAL(4, mockLastPacketLen);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, mockLastPacket[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastPacket[2]);  // Non-continuous
    TEST_ASSERT_EQUAL_UINT8(0x35, mockLastPacket[3]);  // Mode 0x35
}

// Test: Different reporting modes
void testDifferentReportingModes() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    uint8_t modes[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};

    for (uint8_t mode : modes) {
        protocol->setReportingMode(0x0040, reportingModeCommand(mode, false));
        TEST_ASSERT_EQUAL_UINT8(mode, mockLastPacket[3]);
    }
}

// ===== Status Request Tests =====

// Test: Request status with no connection
void testRequestStatusNoConnection() {
    protocol->requestStatus(0x0040);

    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// Test: Request status with connection
void testRequestStatusWithConnection() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->requestStatus(0x0040);

    // Verify packet format: A2 15 00
    TEST_ASSERT_EQUAL(3, mockLastPacketLen);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);  // HID Output Report
    TEST_ASSERT_EQUAL_UINT8(0x15, mockLastPacket[1]);  // Request Status
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastPacket[2]);  // Parameter
}

// ===== Memory Operations Tests =====

// Test: Write memory EEPROM
void testWriteMemoryEeprom() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    protocol->writeMemory(0x0040, WiimoteAddressSpace::EEPROM, 0x00000010, data, sizeof(data));

    // Verify packet sent
    TEST_ASSERT_EQUAL(1, mockSendCallCount);
    TEST_ASSERT_GREATER_THAN(3, mockLastPacketLen);

    // Verify packet header: A2 16
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);  // HID Output Report
    TEST_ASSERT_EQUAL_UINT8(0x16, mockLastPacket[1]);  // Write Memory
}

// Test: Write memory control register
void testWriteMemoryControlRegister() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    uint8_t data[] = {0xAA, 0xBB};
    protocol->writeMemory(0x0040, WiimoteAddressSpace::ControlRegister, 0x00A400F0, data,
                          sizeof(data));

    TEST_ASSERT_EQUAL(1, mockSendCallCount);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);
    TEST_ASSERT_EQUAL_UINT8(0x16, mockLastPacket[1]);
}

// Test: Write memory packet layout and zero padding
void testWriteMemoryPacketLayoutAndPadding() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    uint8_t data[] = {0xDE, 0xAD, 0xBE};
    protocol->writeMemory(0x0040, WiimoteAddressSpace::ControlRegister, 0x00A400F0, data,
                          sizeof(data));

    // Packet format: A2 16 MM O1 O2 O3 LL [16-byte payload, zero-padded]
    TEST_ASSERT_EQUAL(23, mockLastPacketLen);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);
    TEST_ASSERT_EQUAL_UINT8(0x16, mockLastPacket[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, mockLastPacket[2]);
    TEST_ASSERT_EQUAL_UINT8(0xA4, mockLastPacket[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastPacket[4]);
    TEST_ASSERT_EQUAL_UINT8(0xF0, mockLastPacket[5]);
    TEST_ASSERT_EQUAL_UINT8(sizeof(data), mockLastPacket[6]);
    TEST_ASSERT_EQUAL_UINT8(0xDE, mockLastPacket[7]);
    TEST_ASSERT_EQUAL_UINT8(0xAD, mockLastPacket[8]);
    TEST_ASSERT_EQUAL_UINT8(0xBE, mockLastPacket[9]);

    for (int i = 10; i < 23; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x00, mockLastPacket[i]);
    }
}

// Test: Write memory size guard blocks oversized payload
void testWriteMemoryOversizedPayloadIsRejected() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    uint8_t data[17] = {0};
    protocol->writeMemory(0x0040, WiimoteAddressSpace::EEPROM, 0x00000010, data, sizeof(data));

    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// Test: Write memory with no connection
void testWriteMemoryNoConnection() {
    uint8_t data[] = {0x01};
    protocol->writeMemory(0x0040, WiimoteAddressSpace::EEPROM, 0x00000010, data, sizeof(data));

    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// Test: Read memory EEPROM
void testReadMemoryEeprom() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->readMemory(0x0040, WiimoteAddressSpace::EEPROM, 0x00000020, 16);

    // Verify packet sent
    TEST_ASSERT_EQUAL(1, mockSendCallCount);
    TEST_ASSERT_GREATER_THAN(3, mockLastPacketLen);

    // Verify packet header: A2 17
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);  // HID Output Report
    TEST_ASSERT_EQUAL_UINT8(0x17, mockLastPacket[1]);  // Read Memory
}

// Test: Read memory control register
void testReadMemoryControlRegister() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->readMemory(0x0040, WiimoteAddressSpace::ControlRegister, 0x00A400FA, 6);

    TEST_ASSERT_EQUAL(1, mockSendCallCount);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);
    TEST_ASSERT_EQUAL_UINT8(0x17, mockLastPacket[1]);
}

// Test: Read memory packet layout
void testReadMemoryPacketLayout() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->readMemory(0x0040, WiimoteAddressSpace::ControlRegister, 0x00A400FA, 6);

    // Packet format: A2 17 MM O1 O2 O3 S1 S2
    TEST_ASSERT_EQUAL(8, mockLastPacketLen);
    TEST_ASSERT_EQUAL_UINT8(0xA2, mockLastPacket[0]);
    TEST_ASSERT_EQUAL_UINT8(0x17, mockLastPacket[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, mockLastPacket[2]);
    TEST_ASSERT_EQUAL_UINT8(0xA4, mockLastPacket[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastPacket[4]);
    TEST_ASSERT_EQUAL_UINT8(0xFA, mockLastPacket[5]);
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastPacket[6]);
    TEST_ASSERT_EQUAL_UINT8(0x06, mockLastPacket[7]);
}

// Test: Read memory size guard blocks oversized size
void testReadMemoryOversizedSizeIsRejected() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    protocol->readMemory(0x0040, WiimoteAddressSpace::EEPROM, 0x00000020, 17);

    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// Test: Read memory with no connection
void testReadMemoryNoConnection() {
    protocol->readMemory(0x0040, WiimoteAddressSpace::EEPROM, 0x00000020, 16);

    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// ===== Multiple Connections Tests =====

// Test: Multiple connections
void testMultipleConnections() {
    L2capConnection conn1 = makeConnection(0x0040, 0x0041);
    L2capConnection conn2 = makeConnection(0x0050, 0x0051);

    connections->addConnection(conn1);
    connections->addConnection(conn2);

    // Send to first connection
    protocol->setLeds(0x0040, ledCommand(0x01));
    TEST_ASSERT_EQUAL_UINT16(0x0040, mockLastChannelHandle);
    TEST_ASSERT_EQUAL_UINT16(0x0041, mockLastRemoteCID);

    // Send to second connection
    protocol->setLeds(0x0050, ledCommand(0x02));
    TEST_ASSERT_EQUAL_UINT16(0x0050, mockLastChannelHandle);
    TEST_ASSERT_EQUAL_UINT16(0x0051, mockLastRemoteCID);
}

// ===== Initialization Tests =====

// Test: Uninitialized protocol
void testUninitializedProtocol() {
    WiimoteProtocol uninitProtocol;
    // Don't call init()

    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    // Operations should be safe but do nothing
    uninitProtocol.setLeds(0x0040, ledCommand(0x01));
    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// Test: Re-initialization
void testReinitialization() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    // First use
    protocol->setLeds(0x0040, ledCommand(0x01));
    TEST_ASSERT_EQUAL(1, mockSendCallCount);

    // Re-init with new objects
    L2capConnectionTable *newConnections = new L2capConnectionTable();
    L2capPacketSender *newSender = new L2capPacketSender();
    newSender->setSendCallback(mockL2capRawSendCallback);

    protocol->init(newConnections, newSender);

    mockSendCallCount = 0;

    // Should not work without connection in new table
    protocol->setLeds(0x0040, ledCommand(0x01));
    TEST_ASSERT_EQUAL(0, mockSendCallCount);

    // Add connection and try again
    newConnections->addConnection(conn);
    protocol->setLeds(0x0040, ledCommand(0x01));
    TEST_ASSERT_EQUAL(1, mockSendCallCount);

    delete newSender;
    delete newConnections;
}

// ===== Stress Tests =====

// Test: Rapid sequential operations
void testRapidOperations() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    int operationCount = 0;

    for (int i = 0; i < 10; i++) {
        protocol->setLeds(0x0040, ledCommand(i & 0x0F));
        operationCount++;
    }

    for (int i = 0; i < 5; i++) {
        protocol->setReportingMode(0x0040, reportingModeCommand(0x30 + i, true));
        operationCount++;
    }

    for (int i = 0; i < 3; i++) {
        protocol->requestStatus(0x0040);
        operationCount++;
    }

    TEST_ASSERT_EQUAL(operationCount, mockSendCallCount);
}

// Test: Connection table capacity
void testConnectionTableCapacity() {
    // Add up to max connections
    for (int i = 0; i < L2CAP_CONNECTION_LIST_SIZE; i++) {
        L2capConnection conn = makeConnection(0x0040 + i, 0x0041 + i);
        int result = connections->addConnection(conn);
        TEST_ASSERT_EQUAL(i + 1, result);
    }

    // Verify all connections work
    for (int i = 0; i < L2CAP_CONNECTION_LIST_SIZE; i++) {
        uint16_t remoteCID = 0;
        int result = connections->getRemoteCid(0x0040 + i, &remoteCID);
        TEST_ASSERT_EQUAL(0, result);
        TEST_ASSERT_EQUAL_UINT16(0x0041 + i, remoteCID);
    }
}

// ===== Edge Cases =====

// Test: Invalid channel handle
void testInvalidChannelHandle() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    // Try to send to non-existent channel
    protocol->setLeds(0x9999, ledCommand(0x01));
    TEST_ASSERT_EQUAL(0, mockSendCallCount);
}

// Test: Zero values
void testZeroValues() {
    L2capConnection conn = makeConnection(0x0040, 0x0041);
    connections->addConnection(conn);

    // Zero LEDs
    protocol->setLeds(0x0040, ledCommand(0x00));
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastPacket[2]);

    // Zero reporting mode
    protocol->setReportingMode(0x0040, reportingModeCommand(0x00, false));
    TEST_ASSERT_EQUAL_UINT8(0x00, mockLastPacket[3]);
}

// ===== Main Test Runner =====

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Connection table tests
    RUN_TEST(testEmptyConnectionTable);
    RUN_TEST(testAddAndFindConnection);
    RUN_TEST(testGetFirstConnectionHandle);
    RUN_TEST(testClearConnections);

    // LED control tests
    RUN_TEST(testSetLedsNoConnection);
    RUN_TEST(testSetLedsWithConnection);
    RUN_TEST(testSetAllLeds);
    RUN_TEST(testSetIndividualLeds);

    // Reporting mode tests
    RUN_TEST(testSetReportingModeNoConnection);
    RUN_TEST(testSetReportingModeContinuous);
    RUN_TEST(testSetReportingModeNonContinuous);
    RUN_TEST(testDifferentReportingModes);

    // Status request tests
    RUN_TEST(testRequestStatusNoConnection);
    RUN_TEST(testRequestStatusWithConnection);

    // Memory operation tests
    RUN_TEST(testWriteMemoryEeprom);
    RUN_TEST(testWriteMemoryControlRegister);
    RUN_TEST(testWriteMemoryPacketLayoutAndPadding);
    RUN_TEST(testWriteMemoryOversizedPayloadIsRejected);
    RUN_TEST(testWriteMemoryNoConnection);
    RUN_TEST(testReadMemoryEeprom);
    RUN_TEST(testReadMemoryControlRegister);
    RUN_TEST(testReadMemoryPacketLayout);
    RUN_TEST(testReadMemoryOversizedSizeIsRejected);
    RUN_TEST(testReadMemoryNoConnection);

    // Multiple connections tests
    RUN_TEST(testMultipleConnections);

    // Initialization tests
    RUN_TEST(testUninitializedProtocol);
    RUN_TEST(testReinitialization);

    // Stress tests
    RUN_TEST(testRapidOperations);
    RUN_TEST(testConnectionTableCapacity);

    // Edge cases
    RUN_TEST(testInvalidChannelHandle);
    RUN_TEST(testZeroValues);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    // Connection table tests
    RUN_TEST(testEmptyConnectionTable);
    RUN_TEST(testAddAndFindConnection);
    RUN_TEST(testGetFirstConnectionHandle);
    RUN_TEST(testClearConnections);

    // LED control tests
    RUN_TEST(testSetLedsNoConnection);
    RUN_TEST(testSetLedsWithConnection);
    RUN_TEST(testSetAllLeds);
    RUN_TEST(testSetIndividualLeds);

    // Reporting mode tests
    RUN_TEST(testSetReportingModeNoConnection);
    RUN_TEST(testSetReportingModeContinuous);
    RUN_TEST(testSetReportingModeNonContinuous);
    RUN_TEST(testDifferentReportingModes);

    // Status request tests
    RUN_TEST(testRequestStatusNoConnection);
    RUN_TEST(testRequestStatusWithConnection);

    // Memory operation tests
    RUN_TEST(testWriteMemoryEeprom);
    RUN_TEST(testWriteMemoryControlRegister);
    RUN_TEST(testWriteMemoryPacketLayoutAndPadding);
    RUN_TEST(testWriteMemoryOversizedPayloadIsRejected);
    RUN_TEST(testWriteMemoryNoConnection);
    RUN_TEST(testReadMemoryEeprom);
    RUN_TEST(testReadMemoryControlRegister);
    RUN_TEST(testReadMemoryPacketLayout);
    RUN_TEST(testReadMemoryOversizedSizeIsRejected);
    RUN_TEST(testReadMemoryNoConnection);

    // Multiple connections tests
    RUN_TEST(testMultipleConnections);

    // Initialization tests
    RUN_TEST(testUninitializedProtocol);
    RUN_TEST(testReinitialization);

    // Stress tests
    RUN_TEST(testRapidOperations);
    RUN_TEST(testConnectionTableCapacity);

    // Edge cases
    RUN_TEST(testInvalidChannelHandle);
    RUN_TEST(testZeroValues);

    UNITY_END();
}

void loop() {
    // Empty
}
#endif
