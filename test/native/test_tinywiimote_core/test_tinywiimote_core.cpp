#include "../../../src/tinywiimote/utils/hci_utils.h"
#include "../../../src/utils/protocol_codes.h"

#include <algorithm>
#include <cstring>
#include <unity.h>

// Compile TinyWiimote core into this test binary with renamed public symbols
// to avoid collisions with existing native mocks used by other test suites.
#define TINY_WIIMOTE_INIT twReal_tinyWiimoteInit
#define TINY_WIIMOTE_AVAILABLE twReal_tinyWiimoteAvailable
#define TINY_WIIMOTE_READ twReal_tinyWiimoteRead
#define TINY_WIIMOTE_RESET_DEVICE twReal_tinyWiimoteResetDevice
#define TINY_WIIMOTE_DEVICE_IS_INITED twReal_tinyWiimoteDeviceIsInited
#define TINY_WIIMOTE_IS_CONNECTED twReal_tinyWiimoteIsConnected
#define TINY_WIIMOTE_GET_BATTERY_LEVEL twReal_tinyWiimoteGetBatteryLevel
#define TINY_WIIMOTE_REQUEST_BATTERY_UPDATE twReal_tinyWiimoteRequestBatteryUpdate
#define TINY_WIIMOTE_REQ_ACCELEROMETER twReal_tinyWiimoteReqAccelerometer
#define HANDLE_HCI_DATA twReal_handleHciData
#include "../../../src/TinyWiimote.cpp"
#undef tinyWiimoteInit
#undef tinyWiimoteAvailable
#undef tinyWiimoteRead
#undef tinyWiimoteResetDevice
#undef tinyWiimoteDeviceIsInited
#undef tinyWiimoteIsConnected
#undef tinyWiimoteGetBatteryLevel
#undef tinyWiimoteRequestBatteryUpdate
#undef tinyWiimoteReqAccelerometer
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
    const uint16_t kAclLen = L2CAP_HEADER_LEN + l2capPayloadLen;

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

static const uint8_t *lastL2capPayload() {
    return gLastTx + HCI_H4_ACL_PREAMBLE_SIZE + L2CAP_HEADER_LEN;
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

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testTinyWiimoteInitResetAndGuards);
    RUN_TEST(testTinyWiimoteAclFlowConnectAndReadReport);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testTinyWiimoteInitResetAndGuards);
    RUN_TEST(testTinyWiimoteAclFlowConnectAndReadReport);

    UNITY_END();
}

void loop() {}
#endif
