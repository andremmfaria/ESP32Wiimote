#include "../../../src/tinywiimote/utils/hci_utils.h"
#include "../../../src/utils/protocol_codes.h"

#include <algorithm>
#include <cstring>
#include <unity.h>

// Compile TinyWiimote core into this test binary with renamed public symbols
// to avoid collisions with existing native mocks used by other test suites.
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteInit twReal_tinyWiimoteInit
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteAvailable twReal_tinyWiimoteAvailable
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteRead twReal_tinyWiimoteRead
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteResetDevice twReal_tinyWiimoteResetDevice
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteDeviceIsInited twReal_tinyWiimoteDeviceIsInited
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteIsConnected twReal_tinyWiimoteIsConnected
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteGetBatteryLevel twReal_tinyWiimoteGetBatteryLevel
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteRequestBatteryUpdate twReal_tinyWiimoteRequestBatteryUpdate
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteSetLeds twReal_tinyWiimoteSetLeds
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteSetReportingMode twReal_tinyWiimoteSetReportingMode
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteRequestStatus twReal_tinyWiimoteRequestStatus
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteWriteMemory twReal_tinyWiimoteWriteMemory
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteReadMemory twReal_tinyWiimoteReadMemory
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteReqAccelerometer twReal_tinyWiimoteReqAccelerometer
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteSetFastReconnectTtlMs twReal_tinyWiimoteSetFastReconnectTtlMs
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteSetScanEnabled twReal_tinyWiimoteSetScanEnabled
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteStartDiscovery twReal_tinyWiimoteStartDiscovery
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteStopDiscovery twReal_tinyWiimoteStopDiscovery
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteDisconnect twReal_tinyWiimoteDisconnect
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteSetAutoReconnectEnabled twReal_tinyWiimoteSetAutoReconnectEnabled
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteClearReconnectCache twReal_tinyWiimoteClearReconnectCache
// NOLINTNEXTLINE(readability-identifier-naming)
#define tinyWiimoteGetControllerState twReal_tinyWiimoteGetControllerState
// NOLINTNEXTLINE(readability-identifier-naming)
#define handleHciData twReal_handleHciData
#include "../../../src/TinyWiimote.cpp"
#undef tinyWiimoteInit
#undef tinyWiimoteAvailable
#undef tinyWiimoteRead
#undef tinyWiimoteResetDevice
#undef tinyWiimoteDeviceIsInited
#undef tinyWiimoteIsConnected
#undef tinyWiimoteGetBatteryLevel
#undef tinyWiimoteRequestBatteryUpdate
#undef tinyWiimoteSetLeds
#undef tinyWiimoteSetReportingMode
#undef tinyWiimoteRequestStatus
#undef tinyWiimoteWriteMemory
#undef tinyWiimoteReadMemory
#undef tinyWiimoteReqAccelerometer
#undef tinyWiimoteSetFastReconnectTtlMs
#undef tinyWiimoteSetScanEnabled
#undef tinyWiimoteStartDiscovery
#undef tinyWiimoteStopDiscovery
#undef tinyWiimoteDisconnect
#undef tinyWiimoteSetAutoReconnectEnabled
#undef tinyWiimoteClearReconnectCache
#undef tinyWiimoteGetControllerState
#undef handleHciData

static uint8_t gLastTx[512];
static size_t gLastTxLen;
static int gSendCount;

static void captureTx(uint8_t *data, size_t len) {
    gSendCount++;
    gLastTxLen = len;
    if (data == nullptr) {
        return;
    }

    size_t copyLen = len;
    copyLen = std::min<unsigned long>(copyLen, sizeof(gLastTx));
    memcpy(gLastTx, data, copyLen);
}

static void buildAclFrame(uint8_t *out,
                          size_t *outLen,
                          uint16_t ch,
                          uint16_t cid,
                          const uint8_t *l2capPayload,
                          uint16_t l2capPayloadLen) {
    const uint16_t kAclLen = kL2CapHeaderLen + l2capPayloadLen;

    out[0] = static_cast<uint8_t>(H4PacketType::Acl);
    out[1] = (uint8_t)(ch & 0xFF);
    out[2] = (uint8_t)(((ch >> 8) & 0x0F) | (0b10 << 4));  // PB=0b10, BC=0b00
    out[3] = (uint8_t)(kAclLen & 0xFF);
    out[4] = (uint8_t)((kAclLen >> 8) & 0xFF);
    out[5] = (uint8_t)(l2capPayloadLen & 0xFF);
    out[6] = (uint8_t)((l2capPayloadLen >> 8) & 0xFF);
    out[7] = (uint8_t)(cid & 0xFF);
    out[8] = (uint8_t)((cid >> 8) & 0xFF);

    if (l2capPayloadLen > 0 && l2capPayload != nullptr) {
        memcpy(out + 9, l2capPayload, l2capPayloadLen);
    }

    *outLen = (size_t)(9 + l2capPayloadLen);
}

static void buildHciEventPacket(uint8_t *out,
                                size_t *outLen,
                                uint8_t eventCode,
                                const uint8_t *params,
                                uint8_t paramLen) {
    out[0] = static_cast<uint8_t>(H4PacketType::Event);
    out[1] = eventCode;
    out[2] = paramLen;
    if (paramLen > 0 && params != nullptr) {
        memcpy(out + 3, params, paramLen);
    }
    *outLen = (size_t)(3 + paramLen);
}

static void establishConnectedWiimote(uint16_t connectionHandle, uint16_t remoteCid) {
    uint8_t connResp[12] = {0};
    connResp[0] = (uint8_t)L2capSignalingCode::ConnectionResponse;
    connResp[1] = 0x01;
    connResp[4] = (uint8_t)(remoteCid & 0xFF);
    connResp[5] = (uint8_t)((remoteCid >> 8) & 0xFF);
    connResp[8] = 0x00;
    connResp[9] = 0x00;

    uint8_t frame[64] = {0};
    size_t frameLen = 0;
    buildAclFrame(frame, &frameLen, connectionHandle, (uint16_t)L2capCid::SIGNALING, connResp,
                  sizeof(connResp));
    twReal_handleHciData(frame, frameLen);

    uint8_t hidReport[4] = {(uint8_t)WiimoteHidPrefix::InputReport,
                            (uint8_t)WiimoteInputReport::CoreButtons, 0x00, 0x08};
    buildAclFrame(frame, &frameLen, connectionHandle, remoteCid, hidReport, sizeof(hidReport));
    twReal_handleHciData(frame, frameLen);
}

static const uint8_t *lastL2capPayload() {
    return gLastTx + kHciH4AclPreambleSize + kL2CapHeaderLen;
}

void setUp(void) {
    memset(gLastTx, 0, sizeof(gLastTx));
    gLastTxLen = 0;
    gSendCount = 0;
}

void tearDown(void) {}

void testTinyWiimoteInitResetAndGuards() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    TEST_ASSERT_FALSE(twReal_tinyWiimoteDeviceIsInited());
    TEST_ASSERT_FALSE(twReal_tinyWiimoteIsConnected());
    TEST_ASSERT_EQUAL_UINT8(0, twReal_tinyWiimoteGetBatteryLevel());

    gSendCount = 0;
    twReal_tinyWiimoteRequestBatteryUpdate();
    TEST_ASSERT_EQUAL(0, gSendCount);

    twReal_tinyWiimoteResetDevice();
    TEST_ASSERT_TRUE(twReal_tinyWiimoteDeviceIsInited());
    TEST_ASSERT_GREATER_THAN(0, gSendCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Command), gLastTx[0]);

    // API guard paths.
    twReal_handleHciData(nullptr, 0);
    uint8_t unknown[2] = {0xFF, 0x00};
    twReal_handleHciData(unknown, sizeof(unknown));
}

void testTinyWiimoteAclFlowConnectAndReadReport() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);
    twReal_tinyWiimoteResetDevice();

    // Feed L2CAP signaling ConnectionResponse SUCCESS to populate connection table.
    uint8_t connResp[12] = {0};
    connResp[0] = (uint8_t)L2capSignalingCode::ConnectionResponse;
    connResp[1] = 0x01;
    connResp[2] = 0x08;
    connResp[3] = 0x00;
    connResp[4] = 0x45;
    connResp[5] = 0x00;  // dst CID
    connResp[8] = 0x00;
    connResp[9] = 0x00;  // SUCCESS

    uint8_t frame[64] = {0};
    size_t frameLen = 0;
    buildAclFrame(frame, &frameLen, 0x0040, (uint16_t)L2capCid::SIGNALING, connResp,
                  sizeof(connResp));

    gSendCount = 0;
    twReal_handleHciData(frame, frameLen);
    TEST_ASSERT_EQUAL(1, gSendCount);  // emits ConfigurationRequest

    // Feed HID input report (A1 30 ...), which marks connection and queues report.
    uint8_t hidReport[4] = {(uint8_t)WiimoteHidPrefix::InputReport,
                            (uint8_t)WiimoteInputReport::CoreButtons, 0x00, 0x08};
    buildAclFrame(frame, &frameLen, 0x0040, 0x0045, hidReport, sizeof(hidReport));

    twReal_handleHciData(frame, frameLen);
    TEST_ASSERT_TRUE(twReal_tinyWiimoteIsConnected());
    TEST_ASSERT_EQUAL(1, twReal_tinyWiimoteAvailable());

    TinyWiimoteData report = twReal_tinyWiimoteRead();
    TEST_ASSERT_EQUAL_UINT8(0, report.number);
    TEST_ASSERT_EQUAL_UINT8(sizeof(hidReport), report.len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(hidReport, report.data, sizeof(hidReport));

    // Connected + known channel now allows explicit battery status request.
    int sendsBeforeBatteryReq = gSendCount;
    twReal_tinyWiimoteRequestBatteryUpdate();
    TEST_ASSERT_EQUAL(sendsBeforeBatteryReq + 1, gSendCount);

    const uint8_t *payload = lastL2capPayload();
    TEST_ASSERT_EQUAL_UINT8((uint8_t)WiimoteHidPrefix::OutputReport, payload[0]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)WiimoteOutputReport::RequestStatus, payload[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, payload[2]);
}

// ---------------------------------------------------------------------------
// HCI ConnectionComplete → onAclConnected → sendConnectionRequest
// HCI DisconnectionComplete → onDisconnected → reset state
// ---------------------------------------------------------------------------
void testTinyWiimoteHciConnectionAndDisconnect() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);
    twReal_tinyWiimoteResetDevice();

    // Send a HCI ConnectionComplete event (H4 type 0x04, event code 0x03).
    // handleHciData parses: data[1]=eventCode, data[2]=paramLen, data[3..]=params.
    // handleConnectionComplete reads: params[0]=status, params[1..2]=handle.
    uint8_t connPkt[] = {
        static_cast<uint8_t>(H4PacketType::Event),
        0x03,
        4,     // param length
        0x00,  // status = success
        0x40,
        0x00,  // connection handle = 0x0040
        0x01   // link type (padding)
    };
    gSendCount = 0;
    twReal_handleHciData(connPkt, sizeof(connPkt));
    // onAclConnected fires → sendConnectionRequest → packet emitted.
    TEST_ASSERT_GREATER_THAN(0, gSendCount);

    // Send a HCI DisconnectionComplete event (event code 0x05).
    // handleDisconnectionComplete reads: params[1..2]=handle, params[3]=reason.
    uint8_t discPkt[] = {
        static_cast<uint8_t>(H4PacketType::Event),
        0x05,
        4,     // param length
        0x00,  // padding (not used by handler)
        0x40,
        0x00,  // connection handle
        0x16   // reason: connection terminated by local host
    };
    gSendCount = 0;
    twReal_handleHciData(discPkt, sizeof(discPkt));
    // onDisconnected → resetDeviceInternal → hciEventsResetDevice → HCI Reset cmd.
    TEST_ASSERT_GREATER_THAN(0, gSendCount);
    TEST_ASSERT_FALSE(twReal_tinyWiimoteIsConnected());
}

// ---------------------------------------------------------------------------
// L2CAP ConfigurationRequest and ConfigurationResponse paths in handleL2capData
// ---------------------------------------------------------------------------
void testTinyWiimoteL2capConfigRequestAndResponse() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);
    twReal_tinyWiimoteResetDevice();

    // Establish a connection in the L2CAP table via ConnectionResponse SUCCESS.
    uint8_t connResp[12] = {0};
    connResp[0] = (uint8_t)L2capSignalingCode::ConnectionResponse;
    connResp[1] = 0x01;
    connResp[4] = 0x45;
    connResp[5] = 0x00;  // dstCID = 0x0045
    connResp[8] = 0x00;
    connResp[9] = 0x00;  // result = SUCCESS
    uint8_t frame[64] = {0};
    size_t frameLen = 0;
    buildAclFrame(frame, &frameLen, 0x0040, (uint16_t)L2capCid::SIGNALING, connResp,
                  sizeof(connResp));
    twReal_handleHciData(frame, frameLen);

    // Send a ConfigurationRequest for the established channel.
    uint8_t configReq[12] = {0};
    configReq[0] = (uint8_t)L2capSignalingCode::ConfigurationRequest;
    configReq[1] = 0x03;   // identifier
    configReq[2] = 0x08;
    configReq[3] = 0x00;   // dataLen = 8
    configReq[6] = 0x00;
    configReq[7] = 0x00;   // flags = 0
    configReq[8] = 0x01;
    configReq[9] = 0x02;   // MTU option type/length
    configReq[10] = 0x40;
    configReq[11] = 0x00;  // MTU value = 64
    buildAclFrame(frame, &frameLen, 0x0040, (uint16_t)L2capCid::SIGNALING, configReq,
                  sizeof(configReq));
    gSendCount = 0;
    twReal_handleHciData(frame, frameLen);
    TEST_ASSERT_GREATER_THAN(0, gSendCount);  // ConfigurationResponse emitted

    // Send a ConfigurationResponse (our side acknowledges peer config; no reply needed).
    uint8_t configResp[12] = {0};
    configResp[0] = (uint8_t)L2capSignalingCode::ConfigurationResponse;
    buildAclFrame(frame, &frameLen, 0x0040, (uint16_t)L2capCid::SIGNALING, configResp,
                  sizeof(configResp));
    gSendCount = 0;
    twReal_handleHciData(frame, frameLen);
    // handleConfigurationResponse is a no-op; no follow-up packet expected.
}

// ---------------------------------------------------------------------------
// handleAclData edge cases: too-short frame, bad PBF flag, truncated L2CAP len
// ---------------------------------------------------------------------------
void testTinyWiimoteAclDataEdgeCases() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    // 1. H4 Acl prefix but only 3 bytes → handleAclData sees len=2 < 8 → guard.
    uint8_t shortAcl[] = {static_cast<uint8_t>(H4PacketType::Acl), 0x40, 0x20};
    gSendCount = 0;
    twReal_handleHciData(shortAcl, sizeof(shortAcl));
    TEST_ASSERT_EQUAL(0, gSendCount);

    // 2. ACL frame with PBF != 0b10 (bit 4-5 of byte[1] after H4 header).
    // Build a valid-length ACL frame but set PBF=0b11.
    uint8_t badPbf[13] = {static_cast<uint8_t>(H4PacketType::Acl),
                          0x40,
                          0x30,  // handle low, (PBF=0b11 | BC=0b00) shifted → byte = 0x30
                          0x04,
                          0x00,  // ACL length = 4
                          0x00,
                          0x00,  // L2CAP length = 0
                          0x05,
                          0x00,  // CID = SIGNALING
                          0x00,
                          0x00,
                          0x00,
                          0x00};
    gSendCount = 0;
    twReal_handleHciData(badPbf, sizeof(badPbf));
    TEST_ASSERT_EQUAL(0, gSendCount);

    // 3. Valid ACL header but L2CAP length field claims more data than available.
    uint8_t truncated[10] = {static_cast<uint8_t>(H4PacketType::Acl),
                             0x40,
                             0x20,  // handle, PBF=0b10 (valid), BC=0b00
                             0x05,
                             0x00,  // ACL length = 5
                             0xFF,
                             0x00,  // L2CAP length = 255 (far exceeds remaining data)
                             0x05,
                             0x00,  // CID
                             0x00};
    gSendCount = 0;
    twReal_handleHciData(truncated, sizeof(truncated));
    TEST_ASSERT_EQUAL(0, gSendCount);
}

// ---------------------------------------------------------------------------
// handleHciData: unknown H4 packet type → default branch
// ---------------------------------------------------------------------------
void testTinyWiimoteHandleHciDataUnknownType() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    uint8_t unknown[] = {0xAA, 0x01, 0x02};
    gSendCount = 0;
    twReal_handleHciData(unknown, sizeof(unknown));
    TEST_ASSERT_EQUAL(0, gSendCount);
}

void testTinyWiimoteL2capUnhandledAndOutputBridges() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    // Unhandled L2CAP code should hit default branch without emitting TX.
    uint8_t unhandledPayload[3] = {0xAA, 0x00, 0x00};
    uint8_t frame[32] = {0};
    size_t frameLen = 0;
    buildAclFrame(frame, &frameLen, 0x0040, (uint16_t)L2capCid::SIGNALING, unhandledPayload,
                  sizeof(unhandledPayload));
    gSendCount = 0;
    twReal_handleHciData(frame, frameLen);
    TEST_ASSERT_EQUAL(0, gSendCount);

    // Output bridge methods should reject when disconnected.
    TEST_ASSERT_FALSE(twReal_tinyWiimoteSetLeds(0x0F));
    TEST_ASSERT_FALSE(
        twReal_tinyWiimoteSetReportingMode((uint8_t)WiimoteInputReport::CoreButtons, false));
    TEST_ASSERT_FALSE(twReal_tinyWiimoteRequestStatus());
    uint8_t writeBuf[2] = {0x12, 0x34};
    TEST_ASSERT_FALSE(twReal_tinyWiimoteWriteMemory((uint8_t)WiimoteAddressSpace::EEPROM, 0x0010,
                                                    writeBuf, sizeof(writeBuf)));
    TEST_ASSERT_FALSE(
        twReal_tinyWiimoteReadMemory((uint8_t)WiimoteAddressSpace::EEPROM, 0x0010, 0x0020));

    // Connected but without an L2CAP handle should also reject.
    gRuntime.wiimoteState.setConnected(true);
    TEST_ASSERT_FALSE(twReal_tinyWiimoteSetLeds(0x0F));
    TEST_ASSERT_FALSE(
        twReal_tinyWiimoteSetReportingMode((uint8_t)WiimoteInputReport::CoreButtons, false));
    TEST_ASSERT_FALSE(twReal_tinyWiimoteRequestStatus());
    TEST_ASSERT_FALSE(twReal_tinyWiimoteWriteMemory((uint8_t)WiimoteAddressSpace::EEPROM, 0x0010,
                                                    writeBuf, sizeof(writeBuf)));
    TEST_ASSERT_FALSE(
        twReal_tinyWiimoteReadMemory((uint8_t)WiimoteAddressSpace::EEPROM, 0x0010, 0x0020));

    // Add a connection so bridge calls can route through wiimote protocol.
    L2capConnection::Endpoint endpoint = {0x0042, 0x0013};
    TEST_ASSERT_GREATER_THAN_INT(
        0, gRuntime.l2capConnections.addConnection(L2capConnection(endpoint)));

    gSendCount = 0;
    TEST_ASSERT_TRUE(twReal_tinyWiimoteSetLeds(0x0F));
    TEST_ASSERT_EQUAL(1, gSendCount);

    gSendCount = 0;
    TEST_ASSERT_TRUE(
        twReal_tinyWiimoteSetReportingMode((uint8_t)WiimoteInputReport::CoreButtonsAccel, true));
    TEST_ASSERT_EQUAL(1, gSendCount);

    gSendCount = 0;
    TEST_ASSERT_TRUE(twReal_tinyWiimoteRequestStatus());
    TEST_ASSERT_EQUAL(1, gSendCount);

    gSendCount = 0;
    TEST_ASSERT_TRUE(twReal_tinyWiimoteWriteMemory((uint8_t)WiimoteAddressSpace::EEPROM, 0x0010,
                                                   writeBuf, sizeof(writeBuf)));
    TEST_ASSERT_EQUAL(1, gSendCount);

    gSendCount = 0;
    TEST_ASSERT_TRUE(
        twReal_tinyWiimoteReadMemory((uint8_t)WiimoteAddressSpace::EEPROM, 0x0010, 0x0010));
    TEST_ASSERT_EQUAL(1, gSendCount);
}

void testTinyWiimoteSetScanEnabledSendsExpectedModes() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    // Not started: setScanEnabled should be rejected (no HCI command sent).
    gSendCount = 0;
    twReal_tinyWiimoteSetScanEnabled(true);
    TEST_ASSERT_EQUAL(0, gSendCount);

    twReal_tinyWiimoteResetDevice();

    gSendCount = 0;
    twReal_tinyWiimoteSetScanEnabled(true);
    TEST_ASSERT_EQUAL(1, gSendCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Command), gLastTx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x1A, gLastTx[1]);
    TEST_ASSERT_EQUAL_UINT8(0x0C, gLastTx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x01, gLastTx[3]);
    TEST_ASSERT_EQUAL_UINT8(0x02, gLastTx[4]);

    // Re-applying the same state is rejected while disconnected.
    gSendCount = 0;
    twReal_tinyWiimoteSetScanEnabled(true);
    TEST_ASSERT_EQUAL(0, gSendCount);

    gSendCount = 0;
    twReal_tinyWiimoteSetScanEnabled(false);
    TEST_ASSERT_EQUAL(1, gSendCount);
    TEST_ASSERT_EQUAL_UINT8(0x00, gLastTx[4]);

    // Re-applying disabled state is rejected while disconnected.
    gSendCount = 0;
    twReal_tinyWiimoteSetScanEnabled(false);
    TEST_ASSERT_EQUAL(0, gSendCount);
}

void testTinyWiimoteDiscoveryStartStopGuards() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    // Not started: both operations should be rejected.
    gSendCount = 0;
    TEST_ASSERT_FALSE(twReal_tinyWiimoteStartDiscovery());
    TEST_ASSERT_EQUAL(0, gSendCount);
    TEST_ASSERT_FALSE(twReal_tinyWiimoteStopDiscovery());
    TEST_ASSERT_EQUAL(0, gSendCount);

    twReal_tinyWiimoteResetDevice();

    // Stop while not scanning should be rejected.
    gSendCount = 0;
    TEST_ASSERT_FALSE(twReal_tinyWiimoteStopDiscovery());
    TEST_ASSERT_EQUAL(0, gSendCount);

    // Start should issue INQUIRY command and succeed.
    gSendCount = 0;
    TEST_ASSERT_TRUE(twReal_tinyWiimoteStartDiscovery());
    TEST_ASSERT_EQUAL(1, gSendCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Command), gLastTx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, gLastTx[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, gLastTx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x05, gLastTx[3]);
    TEST_ASSERT_EQUAL_UINT8(0x33, gLastTx[4]);
    TEST_ASSERT_EQUAL_UINT8(0x8B, gLastTx[5]);
    TEST_ASSERT_EQUAL_UINT8(0x9E, gLastTx[6]);
    TEST_ASSERT_EQUAL_UINT8(0x08, gLastTx[7]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, gLastTx[8]);

    // Second start while scanning should be rejected.
    gSendCount = 0;
    TEST_ASSERT_FALSE(twReal_tinyWiimoteStartDiscovery());
    TEST_ASSERT_EQUAL(0, gSendCount);

    // Stop while scanning should issue INQUIRY_CANCEL and succeed.
    gSendCount = 0;
    TEST_ASSERT_TRUE(twReal_tinyWiimoteStopDiscovery());
    TEST_ASSERT_EQUAL(1, gSendCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Command), gLastTx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, gLastTx[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, gLastTx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, gLastTx[3]);
}

void testTinyWiimoteScanningLifecycleFromCommandsAndConnection() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);
    twReal_tinyWiimoteResetDevice();

    TEST_ASSERT_FALSE(gRuntime.hciEventContext.scanningEnabled);

    // Command submission path should toggle scanning state immediately.
    twReal_tinyWiimoteSetScanEnabled(true);
    TEST_ASSERT_TRUE(gRuntime.hciEventContext.scanningEnabled);

    twReal_tinyWiimoteSetScanEnabled(false);
    TEST_ASSERT_FALSE(gRuntime.hciEventContext.scanningEnabled);

    TEST_ASSERT_TRUE(twReal_tinyWiimoteStartDiscovery());
    TEST_ASSERT_TRUE(gRuntime.hciEventContext.scanningEnabled);

    establishConnectedWiimote(0x0040, 0x0045);

    // Connection establishment should force scanning state to false.
    TEST_ASSERT_TRUE(twReal_tinyWiimoteIsConnected());
    TEST_ASSERT_FALSE(gRuntime.hciEventContext.scanningEnabled);
}

void testTinyWiimoteDiscoveryCommandChurnRemainsDeterministic() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);
    twReal_tinyWiimoteResetDevice();

    for (int i = 0; i < 10; i++) {
        gSendCount = 0;
        TEST_ASSERT_TRUE(twReal_tinyWiimoteStartDiscovery());
        TEST_ASSERT_EQUAL(1, gSendCount);
        TEST_ASSERT_TRUE(gRuntime.hciEventContext.scanningEnabled);

        gSendCount = 0;
        TEST_ASSERT_FALSE(twReal_tinyWiimoteStartDiscovery());
        TEST_ASSERT_EQUAL(0, gSendCount);
        TEST_ASSERT_TRUE(gRuntime.hciEventContext.scanningEnabled);

        gSendCount = 0;
        TEST_ASSERT_TRUE(twReal_tinyWiimoteStopDiscovery());
        TEST_ASSERT_EQUAL(1, gSendCount);
        TEST_ASSERT_FALSE(gRuntime.hciEventContext.scanningEnabled);

        gSendCount = 0;
        TEST_ASSERT_FALSE(twReal_tinyWiimoteStopDiscovery());
        TEST_ASSERT_EQUAL(0, gSendCount);
        TEST_ASSERT_FALSE(gRuntime.hciEventContext.scanningEnabled);
    }
}

void testTinyWiimoteDisconnectIdleChurnAlwaysRejected() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);
    twReal_tinyWiimoteResetDevice();

    for (int i = 0; i < 20; i++) {
        gSendCount = 0;
        TEST_ASSERT_FALSE(twReal_tinyWiimoteDisconnect(0x16));
        TEST_ASSERT_EQUAL(0, gSendCount);
    }
}

void testTinyWiimoteReconnectCacheClearChurnRemainsStable() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);
    twReal_tinyWiimoteResetDevice();

    for (int i = 0; i < 16; i++) {
        gRuntime.hciEventContext.pendingFastReconnect = true;
        gRuntime.hciEventContext.hasLastWiimote = true;
        gRuntime.hciEventContext.lastWiimote.bdAddr.addr[0] = 0xAA;

        twReal_tinyWiimoteClearReconnectCache();
        TEST_ASSERT_FALSE(gRuntime.hciEventContext.hasLastWiimote);
        TEST_ASSERT_FALSE(gRuntime.hciEventContext.pendingFastReconnect);
        TEST_ASSERT_EQUAL_UINT8(0x00, gRuntime.hciEventContext.lastWiimote.bdAddr.addr[0]);
    }
}

void testTinyWiimoteAutoReconnectDisabledSuppressesFastReconnectFlow() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    // Set up a valid fast-reconnect cache, then disable policy.
    gRuntime.hciEventContext.hasLastWiimote = true;
    gRuntime.hciEventContext.lastWiimoteSeenMs = 0;
    gRuntime.hciEventContext.lastWiimote.bdAddr.addr[0] = 0x12;
    gRuntime.hciEventContext.lastWiimote.bdAddr.addr[1] = 0x34;
    gRuntime.hciEventContext.lastWiimote.bdAddr.addr[2] = 0x56;
    gRuntime.hciEventContext.lastWiimote.bdAddr.addr[3] = 0x78;
    gRuntime.hciEventContext.lastWiimote.bdAddr.addr[4] = 0x9A;
    gRuntime.hciEventContext.lastWiimote.bdAddr.addr[5] = 0xBC;
    gRuntime.hciEventContext.deviceInited = false;
    gRuntime.hciEventContext.autoReconnectEnabled = false;

    // CommandComplete(WriteScanEnable, status=0) should start inquiry when policy is disabled.
    uint8_t params[4] = {0x01, 0x1A, 0x0C, 0x00};
    uint8_t eventPacket[16] = {0};
    size_t eventLen = 0;
    buildHciEventPacket(eventPacket, &eventLen, 0x0E, params, sizeof(params));

    gSendCount = 0;
    twReal_handleHciData(eventPacket, eventLen);

    TEST_ASSERT_EQUAL(1, gSendCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Command), gLastTx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, gLastTx[1]);  // Inquiry opcode LSB (0x0401)
    TEST_ASSERT_EQUAL_UINT8(0x04, gLastTx[2]);  // Inquiry opcode MSB
    TEST_ASSERT_FALSE(gRuntime.hciEventContext.pendingFastReconnect);
}

void testTinyWiimoteConnectedDiscoveryGuardsRejectTransitions() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);
    twReal_tinyWiimoteResetDevice();
    establishConnectedWiimote(0x0040, 0x0045);

    TEST_ASSERT_TRUE(twReal_tinyWiimoteIsConnected());

    gSendCount = 0;
    TEST_ASSERT_FALSE(twReal_tinyWiimoteStartDiscovery());
    TEST_ASSERT_EQUAL(0, gSendCount);

    gSendCount = 0;
    TEST_ASSERT_FALSE(twReal_tinyWiimoteStopDiscovery());
    TEST_ASSERT_EQUAL(0, gSendCount);
}

void testTinyWiimoteDisconnectGuardAndCommand() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    // Not started => reject.
    gSendCount = 0;
    TEST_ASSERT_FALSE(twReal_tinyWiimoteDisconnect(0x16));
    TEST_ASSERT_EQUAL(0, gSendCount);

    twReal_tinyWiimoteResetDevice();

    // Not connected => reject.
    gSendCount = 0;
    TEST_ASSERT_FALSE(twReal_tinyWiimoteDisconnect(0x16));
    TEST_ASSERT_EQUAL(0, gSendCount);

    // Build connection table + connected state.
    establishConnectedWiimote(0x0040, 0x0045);

    gSendCount = 0;
    TEST_ASSERT_TRUE(twReal_tinyWiimoteDisconnect(0x16));
    TEST_ASSERT_EQUAL(1, gSendCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Command), gLastTx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x06, gLastTx[1]);
    TEST_ASSERT_EQUAL_UINT8(0x04, gLastTx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x03, gLastTx[3]);
    TEST_ASSERT_EQUAL_UINT8(0x40, gLastTx[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, gLastTx[5]);
    TEST_ASSERT_EQUAL_UINT8(0x16, gLastTx[6]);
}

void testTinyWiimoteDisconnectMissingHandleAndConfigSetters() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    // Reconnect-policy and cache updates should be rejected before controller start.
    TEST_ASSERT_TRUE(gRuntime.hciEventContext.autoReconnectEnabled);
    twReal_tinyWiimoteSetAutoReconnectEnabled(false);
    TEST_ASSERT_TRUE(gRuntime.hciEventContext.autoReconnectEnabled);

    gRuntime.hciEventContext.pendingFastReconnect = true;
    gRuntime.hciEventContext.hasLastWiimote = true;
    gRuntime.hciEventContext.lastWiimote.bdAddr.addr[0] = 0xAA;
    twReal_tinyWiimoteClearReconnectCache();
    TEST_ASSERT_TRUE(gRuntime.hciEventContext.hasLastWiimote);
    TEST_ASSERT_TRUE(gRuntime.hciEventContext.pendingFastReconnect);
    TEST_ASSERT_EQUAL_UINT8(0xAA, gRuntime.hciEventContext.lastWiimote.bdAddr.addr[0]);

    twReal_tinyWiimoteResetDevice();

    // Connected with no L2CAP handle should hit disconnect guard.
    gRuntime.wiimoteState.setConnected(true);
    gSendCount = 0;
    TEST_ASSERT_FALSE(twReal_tinyWiimoteDisconnect(0x16));
    TEST_ASSERT_EQUAL(0, gSendCount);

    // Direct setter APIs should update runtime configuration fields.
    TEST_ASSERT_TRUE(gRuntime.wiimoteState.getUseAccelerometer());
    twReal_tinyWiimoteReqAccelerometer(false);
    TEST_ASSERT_FALSE(gRuntime.wiimoteState.getUseAccelerometer());
    twReal_tinyWiimoteReqAccelerometer(true);
    TEST_ASSERT_TRUE(gRuntime.wiimoteState.getUseAccelerometer());

    const uint32_t kNewTtlMs = 4242;
    twReal_tinyWiimoteSetFastReconnectTtlMs(kNewTtlMs);
    TEST_ASSERT_EQUAL_UINT32(kNewTtlMs, gRuntime.hciEventContext.fastReconnectTtlMs);
}

void testTinyWiimoteReconnectPolicyAndStateSnapshot() {
    TwHciInterface hci = {captureTx};
    twReal_tinyWiimoteInit(hci);

    // Reconnect policy changes are guarded until controller startup completes.
    TEST_ASSERT_TRUE(gRuntime.hciEventContext.autoReconnectEnabled);
    twReal_tinyWiimoteSetAutoReconnectEnabled(false);
    TEST_ASSERT_TRUE(gRuntime.hciEventContext.autoReconnectEnabled);

    twReal_tinyWiimoteResetDevice();

    twReal_tinyWiimoteSetAutoReconnectEnabled(false);
    TEST_ASSERT_FALSE(gRuntime.hciEventContext.autoReconnectEnabled);

    gRuntime.hciEventContext.pendingFastReconnect = true;
    gRuntime.hciEventContext.hasLastWiimote = true;
    gRuntime.hciEventContext.lastWiimote.bdAddr.addr[0] = 0xAA;

    twReal_tinyWiimoteClearReconnectCache();
    TEST_ASSERT_FALSE(gRuntime.hciEventContext.hasLastWiimote);
    TEST_ASSERT_FALSE(gRuntime.hciEventContext.pendingFastReconnect);
    TEST_ASSERT_EQUAL_UINT8(0x00, gRuntime.hciEventContext.lastWiimote.bdAddr.addr[0]);

    BluetoothControllerState state = twReal_tinyWiimoteGetControllerState();
    TEST_ASSERT_TRUE(state.initialized);
    TEST_ASSERT_FALSE(state.connected);
    TEST_ASSERT_FALSE(state.autoReconnectEnabled);
    TEST_ASSERT_EQUAL_UINT16(0x0000, state.activeConnectionHandle);
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testTinyWiimoteInitResetAndGuards);
    RUN_TEST(testTinyWiimoteAclFlowConnectAndReadReport);
    RUN_TEST(testTinyWiimoteHciConnectionAndDisconnect);
    RUN_TEST(testTinyWiimoteL2capConfigRequestAndResponse);
    RUN_TEST(testTinyWiimoteAclDataEdgeCases);
    RUN_TEST(testTinyWiimoteHandleHciDataUnknownType);
    RUN_TEST(testTinyWiimoteL2capUnhandledAndOutputBridges);
    RUN_TEST(testTinyWiimoteSetScanEnabledSendsExpectedModes);
    RUN_TEST(testTinyWiimoteDiscoveryStartStopGuards);
    RUN_TEST(testTinyWiimoteScanningLifecycleFromCommandsAndConnection);
    RUN_TEST(testTinyWiimoteDiscoveryCommandChurnRemainsDeterministic);
    RUN_TEST(testTinyWiimoteDisconnectIdleChurnAlwaysRejected);
    RUN_TEST(testTinyWiimoteReconnectCacheClearChurnRemainsStable);
    RUN_TEST(testTinyWiimoteAutoReconnectDisabledSuppressesFastReconnectFlow);
    RUN_TEST(testTinyWiimoteConnectedDiscoveryGuardsRejectTransitions);
    RUN_TEST(testTinyWiimoteDisconnectGuardAndCommand);
    RUN_TEST(testTinyWiimoteDisconnectMissingHandleAndConfigSetters);
    RUN_TEST(testTinyWiimoteReconnectPolicyAndStateSnapshot);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testTinyWiimoteInitResetAndGuards);
    RUN_TEST(testTinyWiimoteAclFlowConnectAndReadReport);
    RUN_TEST(testTinyWiimoteHciConnectionAndDisconnect);
    RUN_TEST(testTinyWiimoteL2capConfigRequestAndResponse);
    RUN_TEST(testTinyWiimoteAclDataEdgeCases);
    RUN_TEST(testTinyWiimoteHandleHciDataUnknownType);
    RUN_TEST(testTinyWiimoteL2capUnhandledAndOutputBridges);
    RUN_TEST(testTinyWiimoteSetScanEnabledSendsExpectedModes);
    RUN_TEST(testTinyWiimoteDiscoveryStartStopGuards);
    RUN_TEST(testTinyWiimoteScanningLifecycleFromCommandsAndConnection);
    RUN_TEST(testTinyWiimoteDiscoveryCommandChurnRemainsDeterministic);
    RUN_TEST(testTinyWiimoteDisconnectIdleChurnAlwaysRejected);
    RUN_TEST(testTinyWiimoteReconnectCacheClearChurnRemainsStable);
    RUN_TEST(testTinyWiimoteAutoReconnectDisabledSuppressesFastReconnectFlow);
    RUN_TEST(testTinyWiimoteConnectedDiscoveryGuardsRejectTransitions);
    RUN_TEST(testTinyWiimoteDisconnectGuardAndCommand);
    RUN_TEST(testTinyWiimoteDisconnectMissingHandleAndConfigSetters);
    RUN_TEST(testTinyWiimoteReconnectPolicyAndStateSnapshot);

    UNITY_END();
}

void loop() {}
#endif
