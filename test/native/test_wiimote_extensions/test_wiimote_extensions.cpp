#include "../../../src/tinywiimote/l2cap/l2cap_connection.h"
#include "../../../src/tinywiimote/l2cap/l2cap_packets.h"
#include "../../../src/tinywiimote/protocol/wiimote_extensions.h"
#include "../../../src/tinywiimote/protocol/wiimote_state.h"
#include "../../../src/tinywiimote/utils/hci_utils.h"

#include <algorithm>
#include <cstring>
#include <unity.h>

static uint8_t gRawPacket[512];
static size_t gRawPacketLen;
static int gSendCount;

static void captureRawPacket(uint8_t *data, size_t len) {
    gSendCount++;
    gRawPacketLen = len;

    if (data == nullptr) {
        return;
    }

    size_t copyLen = std::min<unsigned long>(len, sizeof(gRawPacket));
    memcpy(gRawPacket, data, copyLen);
}

static const uint8_t *l2capPayloadPtr() {
    return gRawPacket + HCI_H4_ACL_PREAMBLE_SIZE + L2CAP_HEADER_LEN;
}

static void addConnection(L2capConnectionTable *connections, uint16_t ch, uint16_t cid) {
    L2capConnection::Endpoint endpoint = {ch, cid};
    TEST_ASSERT_GREATER_THAN(0, connections->addConnection(L2capConnection(endpoint)));
}

static void assertSetReportingModePacket(uint8_t expectedMode) {
    const uint8_t *payload = l2capPayloadPtr();
    TEST_ASSERT_EQUAL_UINT8(0xA2, payload[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, payload[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, payload[2]);
    TEST_ASSERT_EQUAL_UINT8(expectedMode, payload[3]);
}

void setUp(void) {
    gRawPacketLen = 0;
    gSendCount = 0;
    memset(gRawPacket, 0, sizeof(gRawPacket));
}

void tearDown(void) {}

void testExtensionsGuardsAndInitNoExtensionPath() {
    WiimoteExtensions extensions;
    WiimoteState state;
    L2capConnectionTable connections;
    L2capPacketSender sender;
    sender.setSendCallback(captureRawPacket);

    uint8_t report[8] = {0};
    report[1] = 0x20;
    report[7] = 104;

    // Guard: handle before init should not emit packets.
    extensions.handleReport(0x0040, report, sizeof(report));
    TEST_ASSERT_EQUAL(0, gSendCount);

    addConnection(&connections, 0x0040, 0x0013);
    extensions.init(&state, &connections, &sender);

    state.setUseAccelerometer(true);
    extensions.handleReport(0x0040, report, sizeof(report));
    TEST_ASSERT_EQUAL_UINT8(50, state.getBatteryLevel());
    TEST_ASSERT_FALSE(state.isNunchukConnected());
    TEST_ASSERT_EQUAL(1, gSendCount);
    assertSetReportingModePacket(0x31);

    gSendCount = 0;
    state.setUseAccelerometer(false);
    report[7] = 208;
    extensions.handleReport(0x0040, report, sizeof(report));
    TEST_ASSERT_EQUAL_UINT8(100, state.getBatteryLevel());
    TEST_ASSERT_EQUAL(1, gSendCount);
    assertSetReportingModePacket(0x30);
}

void testExtensionsDetectNunchukAndSetModeWithoutAccelerometer() {
    WiimoteExtensions extensions;
    WiimoteState state;
    L2capConnectionTable connections;
    L2capPacketSender sender;
    sender.setSendCallback(captureRawPacket);

    addConnection(&connections, 0x0040, 0x0013);
    state.setUseAccelerometer(false);
    extensions.init(&state, &connections, &sender);

    // Step 1: status report indicates extension connected.
    uint8_t statusReport[8] = {0};
    statusReport[1] = 0x20;
    statusReport[4] = 0x02;
    statusReport[7] = 130;
    extensions.handleReport(0x0040, statusReport, sizeof(statusReport));

    TEST_ASSERT_EQUAL(1, gSendCount);
    const uint8_t *payload = l2capPayloadPtr();
    TEST_ASSERT_EQUAL_UINT8(0xA2, payload[0]);
    TEST_ASSERT_EQUAL_UINT8(0x16, payload[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, payload[2]);
    TEST_ASSERT_EQUAL_UINT8(0xA4, payload[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, payload[4]);
    TEST_ASSERT_EQUAL_UINT8(0xF0, payload[5]);
    TEST_ASSERT_EQUAL_UINT8(0x01, payload[6]);
    TEST_ASSERT_EQUAL_UINT8(0x55, payload[7]);

    // Step 2: ack write 0x16 success.
    uint8_t ackWriteEnable[8] = {0};
    ackWriteEnable[1] = 0x22;
    ackWriteEnable[4] = 0x16;
    ackWriteEnable[5] = 0x00;
    extensions.handleReport(0x0040, ackWriteEnable, sizeof(ackWriteEnable));

    TEST_ASSERT_EQUAL(2, gSendCount);
    payload = l2capPayloadPtr();
    TEST_ASSERT_EQUAL_UINT8(0xA2, payload[0]);
    TEST_ASSERT_EQUAL_UINT8(0x16, payload[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, payload[2]);
    TEST_ASSERT_EQUAL_UINT8(0xA4, payload[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, payload[4]);
    TEST_ASSERT_EQUAL_UINT8(0xFB, payload[5]);
    TEST_ASSERT_EQUAL_UINT8(0x01, payload[6]);
    TEST_ASSERT_EQUAL_UINT8(0x00, payload[7]);

    // Step 3: ack write 0x16 success, then request read at 0xA400FA.
    extensions.handleReport(0x0040, ackWriteEnable, sizeof(ackWriteEnable));
    TEST_ASSERT_EQUAL(3, gSendCount);
    payload = l2capPayloadPtr();
    TEST_ASSERT_EQUAL_UINT8(0xA2, payload[0]);
    TEST_ASSERT_EQUAL_UINT8(0x17, payload[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, payload[2]);
    TEST_ASSERT_EQUAL_UINT8(0xA4, payload[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, payload[4]);
    TEST_ASSERT_EQUAL_UINT8(0xFA, payload[5]);
    TEST_ASSERT_EQUAL_UINT8(0x00, payload[6]);
    TEST_ASSERT_EQUAL_UINT8(0x06, payload[7]);

    // Step 4: read response identifies Nunchuk.
    uint8_t readResponse[13] = {0};
    readResponse[1] = 0x21;
    readResponse[5] = 0x00;
    readResponse[6] = 0xFA;
    readResponse[7] = 0x00;
    readResponse[8] = 0x00;
    readResponse[9] = 0xA4;
    readResponse[10] = 0x20;
    readResponse[11] = 0x00;
    readResponse[12] = 0x00;
    extensions.handleReport(0x0040, readResponse, sizeof(readResponse));

    TEST_ASSERT_TRUE(state.isNunchukConnected());
    TEST_ASSERT_EQUAL(4, gSendCount);
    assertSetReportingModePacket(0x32);
}

void testExtensionsDetectNunchukAndSetModeWithAccelerometer() {
    WiimoteExtensions extensions;
    WiimoteState state;
    L2capConnectionTable connections;
    L2capPacketSender sender;
    sender.setSendCallback(captureRawPacket);

    addConnection(&connections, 0x0040, 0x0013);
    state.setUseAccelerometer(true);
    extensions.init(&state, &connections, &sender);

    uint8_t statusReport[8] = {0};
    statusReport[1] = 0x20;
    statusReport[4] = 0x02;
    statusReport[7] = 90;
    extensions.handleReport(0x0040, statusReport, sizeof(statusReport));

    uint8_t ackWriteEnable[8] = {0};
    ackWriteEnable[1] = 0x22;
    ackWriteEnable[4] = 0x16;
    ackWriteEnable[5] = 0x00;
    extensions.handleReport(0x0040, ackWriteEnable, sizeof(ackWriteEnable));
    extensions.handleReport(0x0040, ackWriteEnable, sizeof(ackWriteEnable));

    uint8_t readResponse[13] = {0};
    readResponse[1] = 0x21;
    readResponse[5] = 0x00;
    readResponse[6] = 0xFA;
    readResponse[7] = 0x00;
    readResponse[8] = 0x00;
    readResponse[9] = 0xA4;
    readResponse[10] = 0x20;
    readResponse[11] = 0x00;
    readResponse[12] = 0x00;
    extensions.handleReport(0x0040, readResponse, sizeof(readResponse));

    TEST_ASSERT_TRUE(state.isNunchukConnected());
    assertSetReportingModePacket(0x35);
}

void testExtensionsFailurePathsResetAndRecover() {
    WiimoteExtensions extensions;
    WiimoteState state;
    L2capConnectionTable connections;
    L2capPacketSender sender;
    sender.setSendCallback(captureRawPacket);

    addConnection(&connections, 0x0040, 0x0013);
    state.setUseAccelerometer(false);
    extensions.init(&state, &connections, &sender);

    uint8_t statusReport[8] = {0};
    statusReport[1] = 0x20;
    statusReport[4] = 0x02;
    extensions.handleReport(0x0040, statusReport, sizeof(statusReport));
    TEST_ASSERT_EQUAL(1, gSendCount);

    // Ack with error should reset to init state.
    uint8_t ackError[8] = {0};
    ackError[1] = 0x22;
    ackError[4] = 0x16;
    ackError[5] = 0x01;
    extensions.handleReport(0x0040, ackError, sizeof(ackError));
    TEST_ASSERT_EQUAL(1, gSendCount);

    // A normal status report without extension should now be handled from init state.
    statusReport[4] = 0x00;
    statusReport[7] = 104;
    extensions.handleReport(0x0040, statusReport, sizeof(statusReport));
    TEST_ASSERT_FALSE(state.isNunchukConnected());
    TEST_ASSERT_EQUAL(2, gSendCount);
    assertSetReportingModePacket(0x30);

    // Re-enter extension flow and fail in wait-read-controller-type.
    statusReport[4] = 0x02;
    extensions.handleReport(0x0040, statusReport, sizeof(statusReport));

    uint8_t ackSuccess[8] = {0};
    ackSuccess[1] = 0x22;
    ackSuccess[4] = 0x16;
    ackSuccess[5] = 0x00;
    extensions.handleReport(0x0040, ackSuccess, sizeof(ackSuccess));

    uint8_t ackSecondError[8] = {0};
    ackSecondError[1] = 0x22;
    ackSecondError[4] = 0x16;
    ackSecondError[5] = 0x02;
    int sendsBeforeSecondError = gSendCount;
    extensions.handleReport(0x0040, ackSecondError, sizeof(ackSecondError));
    TEST_ASSERT_EQUAL(sendsBeforeSecondError, gSendCount);

    // Short packet while waiting should be ignored.
    uint8_t shortPacket[5] = {0};
    extensions.handleReport(0x0040, shortPacket, sizeof(shortPacket));
    TEST_ASSERT_EQUAL(sendsBeforeSecondError, gSendCount);
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testExtensionsGuardsAndInitNoExtensionPath);
    RUN_TEST(testExtensionsDetectNunchukAndSetModeWithoutAccelerometer);
    RUN_TEST(testExtensionsDetectNunchukAndSetModeWithAccelerometer);
    RUN_TEST(testExtensionsFailurePathsResetAndRecover);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testExtensionsGuardsAndInitNoExtensionPath);
    RUN_TEST(testExtensionsDetectNunchukAndSetModeWithoutAccelerometer);
    RUN_TEST(testExtensionsDetectNunchukAndSetModeWithAccelerometer);
    RUN_TEST(testExtensionsFailurePathsResetAndRecover);

    UNITY_END();
}

void loop() {}
#endif
