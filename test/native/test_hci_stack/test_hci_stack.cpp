#include "../../../src/tinywiimote/hci/hci_commands.h"
#include "../../../src/tinywiimote/hci/hci_events.h"
#include "../../../src/tinywiimote/hci/hci_types.h"

#include <algorithm>
#include <cstring>
#include <unity.h>

static uint8_t gLastTx[512];
static size_t gLastTxLen;
static int gSendCount;
static uint16_t gAclConnectedHandle;
static uint16_t gDisconnectedHandle;
static uint8_t gDisconnectedReason;

static void captureTx(uint8_t *data, size_t len, void *userData) {
    (void)userData;
    gSendCount++;
    gLastTxLen = len;
    if (data == nullptr) {
        return;
    }

    size_t copyLen = len;
    copyLen = std::min<unsigned long>(copyLen, sizeof(gLastTx));
    memcpy(gLastTx, data, copyLen);
}

static void onAclConnected(uint16_t connectionHandle, void *userData) {
    (void)userData;
    gAclConnectedHandle = connectionHandle;
}

static void onDisconnected(uint16_t connectionHandle, uint8_t reason, void *userData) {
    (void)userData;
    gDisconnectedHandle = connectionHandle;
    gDisconnectedReason = reason;
}

static uint16_t txOpcode() {
    return (uint16_t)gLastTx[1] | ((uint16_t)gLastTx[2] << 8);
}

static void sendCommandComplete(struct HciEventContext *ctx, uint16_t opcode, uint8_t status) {
    uint8_t data[4] = {1, (uint8_t)(opcode & 0xFF), (uint8_t)((opcode >> 8) & 0xFF), status};
    HciEventPacket packet = {HCI_COMMAND_COMPLETE_EVT, sizeof(data), data};
    hciEventsHandleEvent(ctx, packet);
}

void setUp(void) {
    memset(gLastTx, 0, sizeof(gLastTx));
    gLastTxLen = 0;
    gSendCount = 0;
    gAclConnectedHandle = 0;
    gDisconnectedHandle = 0;
    gDisconnectedReason = 0;
}

void tearDown(void) {}

void testHciCommandBuilders() {
    uint8_t tx[300] = {0};

    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE, makeCmdReset(tx));
    TEST_ASSERT_EQUAL_UINT8(H4TypeCommand, tx[0]);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_RESET, (uint16_t)tx[1] | ((uint16_t)tx[2] << 8));

    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE, makeCmdReadBdAddr(tx));
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_READ_BD_ADDR, (uint16_t)tx[1] | ((uint16_t)tx[2] << 8));

    const uint8_t kName[] = "ESP32";
    const uint16_t kNameLen = makeCmdWriteLocalName(tx, kName, sizeof(kName) - 1);
    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_LOCAL_NAME, kNameLen);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_WRITE_LOCAL_NAME, (uint16_t)tx[1] | ((uint16_t)tx[2] << 8));

    const uint8_t kCod[3] = {0x04, 0x05, 0x00};
    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE,
                             makeCmdWriteClassOfDevice(tx, kCod));

    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_SCAN_ENABLE,
                             makeCmdWriteScanEnable(tx, 3));

    HciInquiryParams inquiry = {0x9E8B33, 0x05, 0x00};
    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQUIRY,
                             makeCmdInquiry(tx, inquiry));

    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQUIRY_CANCEL,
                             makeCmdInquiryCancel(tx));

    HciRemoteNameRequestParams remoteName = {};
    remoteName.bdAddr.addr[0] = 0x01;
    remoteName.pageScanRepetitionMode = 0x02;
    remoteName.clockOffset = 0x1234;
    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_REMOTE_NAME_REQUEST,
                             makeCmdRemoteNameRequest(tx, remoteName));

    HciCreateConnectionParams create = {};
    create.bdAddr.addr[0] = 0xAA;
    create.packetType = 0x0008;
    create.pageScanRepetitionMode = 0x01;
    create.clockOffset = 0x5678;
    create.allowRoleSwitch = 0;
    TEST_ASSERT_EQUAL_UINT16(HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CREATE_CONNECTION,
                             makeCmdCreateConnection(tx, create));
}

void testHciEventsInitResetAndCommandFlow() {
    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);

    hciEventsResetDevice(&ctx);
    TEST_ASSERT_EQUAL(1, gSendCount);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_RESET, txOpcode());

    sendCommandComplete(&ctx, HCI_OPCODE_RESET, 0x00);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_READ_BD_ADDR, txOpcode());

    sendCommandComplete(&ctx, HCI_OPCODE_READ_BD_ADDR, 0x00);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_WRITE_LOCAL_NAME, txOpcode());

    sendCommandComplete(&ctx, HCI_OPCODE_WRITE_LOCAL_NAME, 0x00);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_WRITE_CLASS_OF_DEVICE, txOpcode());

    sendCommandComplete(&ctx, HCI_OPCODE_WRITE_CLASS_OF_DEVICE, 0x00);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_WRITE_SCAN_ENABLE, txOpcode());

    sendCommandComplete(&ctx, HCI_OPCODE_WRITE_SCAN_ENABLE, 0x00);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_INQUIRY, txOpcode());
    TEST_ASSERT_TRUE(ctx.deviceInited);
}

void testHciEventsInquiryAndRemoteNameFlow() {
    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);

    // One inquiry result with Wiimote class-of-device [04 25 00].
    uint8_t inquiryResult[15] = {0};
    inquiryResult[0] = 1;  // num devices
    inquiryResult[1] = 0x11;
    inquiryResult[2] = 0x22;
    inquiryResult[3] = 0x33;
    inquiryResult[4] = 0x44;
    inquiryResult[5] = 0x55;
    inquiryResult[6] = 0x66;
    inquiryResult[7] = 0x01;   // psrm
    inquiryResult[10] = 0x04;  // class-of-device
    inquiryResult[11] = 0x25;
    inquiryResult[12] = 0x00;
    inquiryResult[13] = 0x12;  // clkofs high-low parts
    inquiryResult[14] = 0x34;

    HciEventPacket packet = {HCI_INQUIRY_RESULT_EVT, sizeof(inquiryResult), inquiryResult};
    hciEventsHandleEvent(&ctx, packet);
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_REMOTE_NAME_REQUEST, txOpcode());

    // Remote name complete for same device and matching Nintendo name.
    uint8_t remoteNameData[64] = {0};
    remoteNameData[1] = 0x11;
    remoteNameData[2] = 0x22;
    remoteNameData[3] = 0x33;
    remoteNameData[4] = 0x44;
    remoteNameData[5] = 0x55;
    remoteNameData[6] = 0x66;
    const char *name = "Nintendo RVL-CNT-01";
    memcpy(remoteNameData + 7, name, strlen(name) + 1);

    packet.eventCode = HCI_RMT_NAME_REQUEST_COMP_EVT;
    packet.data = remoteNameData;
    packet.len = sizeof(remoteNameData);

    int sendCountBefore = gSendCount;
    hciEventsHandleEvent(&ctx, packet);
    TEST_ASSERT_TRUE(gSendCount >= sendCountBefore + 2);  // inquiry cancel + create connection
    TEST_ASSERT_EQUAL_UINT16(HCI_OPCODE_CREATE_CONNECTION, txOpcode());
}

void testHciEventsCallbacksAndNullGuards() {
    // Null guard paths should not crash.
    hciEventsInit(nullptr, captureTx, nullptr);
    hciEventsSetCallbacks(nullptr, onAclConnected, onDisconnected);
    hciEventsResetDevice(nullptr);

    HciEventPacket nullPacket = {HCI_COMMAND_COMPLETE_EVT, 0, nullptr};
    hciEventsHandleEvent(nullptr, nullPacket);

    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);
    hciEventsSetCallbacks(&ctx, onAclConnected, onDisconnected);

    uint8_t connectionData[4] = {0, 0x40, 0x00, 0};
    HciEventPacket packet = {HCI_CONNECTION_COMP_EVT, sizeof(connectionData), connectionData};
    hciEventsHandleEvent(&ctx, packet);
    TEST_ASSERT_EQUAL_UINT16(0x0040, gAclConnectedHandle);

    uint8_t disconnectionData[4] = {0, 0x40, 0x00, 0x16};
    packet.eventCode = HCI_DISCONNECTION_COMP_EVT;
    packet.data = disconnectionData;
    packet.len = sizeof(disconnectionData);
    hciEventsHandleEvent(&ctx, packet);
    TEST_ASSERT_EQUAL_UINT16(0x0040, gDisconnectedHandle);
    TEST_ASSERT_EQUAL_UINT8(0x16, gDisconnectedReason);

    // Unknown event + command status paths.
    uint8_t statusData[4] = {0x01, 0x01, (uint8_t)(HCI_OPCODE_RESET & 0xFF),
                             (uint8_t)(HCI_OPCODE_RESET >> 8)};
    packet.eventCode = HCI_COMMAND_STATUS_EVT;
    packet.data = statusData;
    packet.len = sizeof(statusData);
    hciEventsHandleEvent(&ctx, packet);

    uint8_t dummy = 0;
    packet.eventCode = 0xFE;
    packet.data = &dummy;
    packet.len = 1;
    hciEventsHandleEvent(&ctx, packet);
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testHciCommandBuilders);
    RUN_TEST(testHciEventsInitResetAndCommandFlow);
    RUN_TEST(testHciEventsInquiryAndRemoteNameFlow);
    RUN_TEST(testHciEventsCallbacksAndNullGuards);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testHciCommandBuilders);
    RUN_TEST(testHciEventsInitResetAndCommandFlow);
    RUN_TEST(testHciEventsInquiryAndRemoteNameFlow);
    RUN_TEST(testHciEventsCallbacksAndNullGuards);

    UNITY_END();
}

void loop() {}
#endif
