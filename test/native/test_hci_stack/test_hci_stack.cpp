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
static uint32_t gFakeNowMs;

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

static uint32_t fakeNowMs(void *userData) {
    (void)userData;
    return gFakeNowMs;
}

static uint16_t txOpcode() {
    return (uint16_t)gLastTx[1] | ((uint16_t)gLastTx[2] << 8);
}

static void sendCommandComplete(struct HciEventContext *ctx, uint16_t opcode, uint8_t status) {
    uint8_t data[4] = {1, (uint8_t)(opcode & 0xFF), (uint8_t)((opcode >> 8) & 0xFF), status};
    HciEventPacket packet = {kHciCommandCompleteEvt, sizeof(data), data};
    hciEventsHandleEvent(ctx, packet);
}

static void sendConnectionComplete(struct HciEventContext *ctx, uint8_t status, uint16_t handle) {
    uint8_t data[4] = {status, (uint8_t)(handle & 0xFF), (uint8_t)((handle >> 8) & 0xFF), 0};
    HciEventPacket packet = {kHciConnectionCompEvt, sizeof(data), data};
    hciEventsHandleEvent(ctx, packet);
}

static void sendCommandStatus(struct HciEventContext *ctx, uint8_t status, uint16_t opcode) {
    uint8_t data[4] = {status, 0x01, (uint8_t)(opcode & 0xFF), (uint8_t)(opcode >> 8)};
    HciEventPacket packet = {kHciCommandStatusEvt, sizeof(data), data};
    hciEventsHandleEvent(ctx, packet);
}

static void connectNamedWiimote(struct HciEventContext *ctx,
                                uint8_t b0,
                                uint8_t b1,
                                uint8_t b2,
                                uint8_t b3,
                                uint8_t b4,
                                uint8_t b5,
                                uint16_t handle) {
    uint8_t inquiryResult[15] = {0};
    inquiryResult[0] = 1;
    inquiryResult[1] = b0;
    inquiryResult[2] = b1;
    inquiryResult[3] = b2;
    inquiryResult[4] = b3;
    inquiryResult[5] = b4;
    inquiryResult[6] = b5;
    inquiryResult[7] = 0x01;
    inquiryResult[10] = 0x04;
    inquiryResult[11] = 0x25;
    inquiryResult[12] = 0x00;
    inquiryResult[13] = 0x12;
    inquiryResult[14] = 0x34;

    HciEventPacket packet = {kHciInquiryResultEvt, sizeof(inquiryResult), inquiryResult};
    hciEventsHandleEvent(ctx, packet);

    uint8_t remoteNameData[64] = {0};
    remoteNameData[1] = b0;
    remoteNameData[2] = b1;
    remoteNameData[3] = b2;
    remoteNameData[4] = b3;
    remoteNameData[5] = b4;
    remoteNameData[6] = b5;
    const char *name = "Nintendo RVL-CNT-01";
    memcpy(remoteNameData + 7, name, strlen(name) + 1);

    packet.eventCode = kHciRmtNameRequestCompEvt;
    packet.data = remoteNameData;
    packet.len = sizeof(remoteNameData);
    hciEventsHandleEvent(ctx, packet);

    sendConnectionComplete(ctx, 0x00, handle);
}

static void seedKnownWiimoteAndConnectedState(struct HciEventContext *ctx) {
    connectNamedWiimote(ctx, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x0040);
}

void setUp(void) {
    memset(gLastTx, 0, sizeof(gLastTx));
    gLastTxLen = 0;
    gSendCount = 0;
    gAclConnectedHandle = 0;
    gDisconnectedHandle = 0;
    gDisconnectedReason = 0;
    gFakeNowMs = 0;
}

void tearDown(void) {}

void testHciCommandBuilders() {
    uint8_t tx[300] = {0};

    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize, makeCmdReset(tx));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Command), tx[0]);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeReset, (uint16_t)tx[1] | ((uint16_t)tx[2] << 8));

    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize, makeCmdReadBdAddr(tx));
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeReadBdAddr, (uint16_t)tx[1] | ((uint16_t)tx[2] << 8));

    const uint8_t kName[] = "ESP32";
    const uint16_t kNameLen = makeCmdWriteLocalName(tx, kName, sizeof(kName) - 1);
    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize + kHcicParamSizeWriteLocalName, kNameLen);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeWriteLocalName, (uint16_t)tx[1] | ((uint16_t)tx[2] << 8));

    const uint8_t kCod[3] = {0x04, 0x05, 0x00};
    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize + kHcicParamSizeWriteClassOfDevice,
                             makeCmdWriteClassOfDevice(tx, kCod));

    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize + kHcicParamSizeWriteScanEnable,
                             makeCmdWriteScanEnable(tx, 3));

    HciInquiryParams inquiry = {0x9E8B33, 0x05, 0x00};
    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize + kHcicParamSizeWriteInquiry,
                             makeCmdInquiry(tx, inquiry));

    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize + kHcicParamSizeWriteInquiryCancel,
                             makeCmdInquiryCancel(tx));

    HciRemoteNameRequestParams remoteName = {};
    remoteName.bdAddr.addr[0] = 0x01;
    remoteName.pageScanRepetitionMode = 0x02;
    remoteName.clockOffset = 0x1234;
    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize + kHcicParamSizeRemoteNameRequest,
                             makeCmdRemoteNameRequest(tx, remoteName));

    HciCreateConnectionParams create = {};
    create.bdAddr.addr[0] = 0xAA;
    create.packetType = 0x0008;
    create.pageScanRepetitionMode = 0x01;
    create.clockOffset = 0x5678;
    create.allowRoleSwitch = 0;
    TEST_ASSERT_EQUAL_UINT16(kHciH4CmdPreambleSize + kHcicParamSizeCreateConnection,
                             makeCmdCreateConnection(tx, create));
}

void testHciDisconnectCommandBuilderBytes() {
    uint8_t tx[16] = {0};

    const uint16_t len = makeCmdDisconnect(tx, 0x0040, 0x16);
    TEST_ASSERT_EQUAL_UINT16(7, len);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Command), tx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x06, tx[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, tx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x03, tx[3]);
    TEST_ASSERT_EQUAL_UINT8(0x40, tx[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, tx[5]);
    TEST_ASSERT_EQUAL_UINT8(0x16, tx[6]);
}

void testHciEventsInitResetAndCommandFlow() {
    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);

    hciEventsResetDevice(&ctx);
    TEST_ASSERT_EQUAL(1, gSendCount);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeReset, txOpcode());

    sendCommandComplete(&ctx, kHciOpcodeReset, 0x00);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeReadBdAddr, txOpcode());

    sendCommandComplete(&ctx, kHciOpcodeReadBdAddr, 0x00);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeWriteLocalName, txOpcode());

    sendCommandComplete(&ctx, kHciOpcodeWriteLocalName, 0x00);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeWriteClassOfDevice, txOpcode());

    sendCommandComplete(&ctx, kHciOpcodeWriteClassOfDevice, 0x00);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeWriteScanEnable, txOpcode());

    sendCommandComplete(&ctx, kHciOpcodeWriteScanEnable, 0x00);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeInquiry, txOpcode());
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

    HciEventPacket packet = {kHciInquiryResultEvt, sizeof(inquiryResult), inquiryResult};
    hciEventsHandleEvent(&ctx, packet);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeRemoteNameRequest, txOpcode());

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

    packet.eventCode = kHciRmtNameRequestCompEvt;
    packet.data = remoteNameData;
    packet.len = sizeof(remoteNameData);

    int sendCountBefore = gSendCount;
    hciEventsHandleEvent(&ctx, packet);
    TEST_ASSERT_TRUE(gSendCount >= sendCountBefore + 2);  // inquiry cancel + create connection
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeCreateConnection, txOpcode());
}

void testHciEventsCallbacksAndNullGuards() {
    // Null guard paths should not crash.
    hciEventsInit(nullptr, captureTx, nullptr);
    hciEventsSetCallbacks(nullptr, onAclConnected, onDisconnected);
    hciEventsResetDevice(nullptr);

    HciEventPacket nullPacket = {kHciCommandCompleteEvt, 0, nullptr};
    hciEventsHandleEvent(nullptr, nullPacket);

    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);
    hciEventsSetCallbacks(&ctx, onAclConnected, onDisconnected);

    uint8_t connectionData[4] = {0, 0x40, 0x00, 0};
    HciEventPacket packet = {kHciConnectionCompEvt, sizeof(connectionData), connectionData};
    hciEventsHandleEvent(&ctx, packet);
    TEST_ASSERT_EQUAL_UINT16(0x0040, gAclConnectedHandle);

    uint8_t disconnectionData[4] = {0, 0x40, 0x00, 0x16};
    packet.eventCode = kHciDisconnectionCompEvt;
    packet.data = disconnectionData;
    packet.len = sizeof(disconnectionData);
    hciEventsHandleEvent(&ctx, packet);
    TEST_ASSERT_EQUAL_UINT16(0x0040, gDisconnectedHandle);
    TEST_ASSERT_EQUAL_UINT8(0x16, gDisconnectedReason);

    // Unknown event + command status paths.
    uint8_t statusData[4] = {0x01, 0x01, (uint8_t)(kHciOpcodeReset & 0xFF),
                             (uint8_t)(kHciOpcodeReset >> 8)};
    packet.eventCode = kHciCommandStatusEvt;
    packet.data = statusData;
    packet.len = sizeof(statusData);
    hciEventsHandleEvent(&ctx, packet);

    uint8_t dummy = 0;
    packet.eventCode = 0xFE;
    packet.data = &dummy;
    packet.len = 1;
    hciEventsHandleEvent(&ctx, packet);
}

void testFastReconnectWithinTtlSkipsInquiry() {
    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);
    hciEventsSetTimeProvider(&ctx, fakeNowMs);

    gFakeNowMs = 1000;
    seedKnownWiimoteAndConnectedState(&ctx);

    hciEventsResetDevice(&ctx);
    sendCommandComplete(&ctx, kHciOpcodeReset, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeReadBdAddr, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteLocalName, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteClassOfDevice, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteScanEnable, 0x00);

    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeCreateConnection, txOpcode());
}

void testFastReconnectAfterTtlFallsBackToInquiry() {
    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);
    hciEventsSetTimeProvider(&ctx, fakeNowMs);

    gFakeNowMs = 1000;
    seedKnownWiimoteAndConnectedState(&ctx);

    gFakeNowMs = 181001;
    hciEventsResetDevice(&ctx);
    sendCommandComplete(&ctx, kHciOpcodeReset, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeReadBdAddr, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteLocalName, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteClassOfDevice, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteScanEnable, 0x00);

    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeInquiry, txOpcode());
}

void testFastReconnectFailureFallsBackToInquiry() {
    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);
    hciEventsSetTimeProvider(&ctx, fakeNowMs);

    gFakeNowMs = 1000;
    seedKnownWiimoteAndConnectedState(&ctx);

    hciEventsResetDevice(&ctx);
    sendCommandComplete(&ctx, kHciOpcodeReset, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeReadBdAddr, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteLocalName, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteClassOfDevice, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteScanEnable, 0x00);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeCreateConnection, txOpcode());

    sendCommandStatus(&ctx, 0x01, kHciOpcodeCreateConnection);
    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeInquiry, txOpcode());
}

void testDifferentControllerConnectionReplacesCache() {
    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);
    hciEventsSetTimeProvider(&ctx, fakeNowMs);

    gFakeNowMs = 1000;
    seedKnownWiimoteAndConnectedState(&ctx);
    TEST_ASSERT_TRUE(ctx.hasLastWiimote);

    uint8_t oldCachedAddr[kBdAddrLen] = {0};
    memcpy(oldCachedAddr, ctx.lastWiimote.bdAddr.addr, kBdAddrLen);

    hciEventsResetDevice(&ctx);
    sendCommandComplete(&ctx, kHciOpcodeReset, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeReadBdAddr, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteLocalName, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteClassOfDevice, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteScanEnable, 0x00);
    sendCommandStatus(&ctx, 0x01, kHciOpcodeCreateConnection);

    gFakeNowMs = 2000;
    connectNamedWiimote(&ctx, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x0041);

    TEST_ASSERT_TRUE(ctx.hasLastWiimote);
    TEST_ASSERT_NOT_EQUAL(0, memcmp(oldCachedAddr, ctx.lastWiimote.bdAddr.addr, kBdAddrLen));
    TEST_ASSERT_EQUAL(
        0, memcmp(ctx.lastWiimote.bdAddr.addr, ctx.currentConnectTarget.bdAddr.addr, kBdAddrLen));
    TEST_ASSERT_EQUAL_UINT32(2000, ctx.lastWiimoteSeenMs);
}

void testFastReconnectDisabledWhenTtlIsZero() {
    HciEventContext ctx;
    hciEventsInit(&ctx, captureTx, nullptr);
    hciEventsSetTimeProvider(&ctx, fakeNowMs);

    gFakeNowMs = 1000;
    seedKnownWiimoteAndConnectedState(&ctx);

    hciEventsSetFastReconnectTtlMs(&ctx, 0);

    hciEventsResetDevice(&ctx);
    sendCommandComplete(&ctx, kHciOpcodeReset, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeReadBdAddr, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteLocalName, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteClassOfDevice, 0x00);
    sendCommandComplete(&ctx, kHciOpcodeWriteScanEnable, 0x00);

    TEST_ASSERT_EQUAL_UINT16(kHciOpcodeInquiry, txOpcode());
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testHciCommandBuilders);
    RUN_TEST(testHciDisconnectCommandBuilderBytes);
    RUN_TEST(testHciEventsInitResetAndCommandFlow);
    RUN_TEST(testHciEventsInquiryAndRemoteNameFlow);
    RUN_TEST(testHciEventsCallbacksAndNullGuards);
    RUN_TEST(testFastReconnectWithinTtlSkipsInquiry);
    RUN_TEST(testFastReconnectAfterTtlFallsBackToInquiry);
    RUN_TEST(testFastReconnectFailureFallsBackToInquiry);
    RUN_TEST(testDifferentControllerConnectionReplacesCache);
    RUN_TEST(testFastReconnectDisabledWhenTtlIsZero);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testHciCommandBuilders);
    RUN_TEST(testHciDisconnectCommandBuilderBytes);
    RUN_TEST(testHciEventsInitResetAndCommandFlow);
    RUN_TEST(testHciEventsInquiryAndRemoteNameFlow);
    RUN_TEST(testHciEventsCallbacksAndNullGuards);
    RUN_TEST(testFastReconnectWithinTtlSkipsInquiry);
    RUN_TEST(testFastReconnectAfterTtlFallsBackToInquiry);
    RUN_TEST(testFastReconnectFailureFallsBackToInquiry);
    RUN_TEST(testDifferentControllerConnectionReplacesCache);
    RUN_TEST(testFastReconnectDisabledWhenTtlIsZero);

    UNITY_END();
}

void loop() {}
#endif
