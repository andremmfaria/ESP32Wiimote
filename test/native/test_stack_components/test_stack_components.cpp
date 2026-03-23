#include "../../../src/esp32wiimote/bt_controller.h"
#include "../../../src/esp32wiimote/hci_callbacks.h"
#include "../../../src/esp32wiimote/queue/hci_queue.h"
#include "../../../src/tinywiimote/l2cap/l2cap_signaling.h"
#include "../../../src/tinywiimote/protocol/wiimote_reports.h"
#include "../../../src/tinywiimote/protocol/wiimote_state.h"
#include "../../../src/tinywiimote/utils/hci_utils.h"
#include "../../mocks/test_mocks.h"

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

    size_t copyLen = len;
    copyLen = std::min<unsigned long>(copyLen, sizeof(gRawPacket));

    memcpy(gRawPacket, data, copyLen);
}

static const uint8_t *l2capPayloadPtr() {
    return gRawPacket + kHciH4AclPreambleSize + kL2CapHeaderLen;
}

void setUp(void) {
    gRawPacketLen = 0;
    gSendCount = 0;
    memset(gRawPacket, 0, sizeof(gRawPacket));

    mockDeviceIsInited = false;
    mockResetDeviceCallCount = 0;
    mockHandleHciDataCallCount = 0;
    mockTinyWiimoteInitCallCount = 0;
    mockBtStartResult = true;
    mockBtStarted = false;
    mockQueueCreateCallCount = 0;
    mockQueueCreateFailOnCall = 0;
    mockVhciRegisterResult = ESP_OK;
    mockVhciRegisterCallCount = 0;
    mockLastVhciCallback = nullptr;
    gMockVhciSendAvailable = true;
    gMockVhciSendCount = 0;
    gMockVhciSentLen = 0;
    memset(gMockVhciSentData, 0, sizeof(gMockVhciSentData));
    HciCallbacksHandler::setQueueManager(nullptr);
}

void tearDown(void) {}

void testWiimoteReportsPutReadAndBounds() {
    WiimoteReports reports;
    TEST_ASSERT_EQUAL(0, reports.available());

    uint8_t payload[] = {1, 2, 3, 4, 5};
    reports.put(0x30, payload, sizeof(payload));
    TEST_ASSERT_EQUAL(1, reports.available());

    TinyWiimoteData out = reports.read();
    TEST_ASSERT_EQUAL_UINT8(0x30, out.number);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), out.len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, out.data, sizeof(payload));
    TEST_ASSERT_EQUAL(0, reports.available());

    TinyWiimoteData empty = reports.read();
    TEST_ASSERT_EQUAL_UINT8(0, empty.number);
    TEST_ASSERT_EQUAL_UINT8(0, empty.len);

    uint8_t large[RECEIVED_DATA_MAX_LEN + 8];
    memset(large, 0xAB, sizeof(large));
    reports.put(0x31, large, sizeof(large));
    out = reports.read();
    TEST_ASSERT_EQUAL_UINT8(RECEIVED_DATA_MAX_LEN, out.len);

    for (int i = 0; i < RECEIVED_DATA_MAX_NUM + 2; i++) {
        reports.put(0x40 + i, payload, sizeof(payload));
    }
    TEST_ASSERT_EQUAL(RECEIVED_DATA_MAX_NUM, reports.available());
}

void testWiimoteStateTransitionsAndBatteryMapping() {
    WiimoteState state;

    TEST_ASSERT_FALSE(state.isConnected());
    TEST_ASSERT_FALSE(state.isNunchukConnected());
    TEST_ASSERT_TRUE(state.getUseAccelerometer());

    state.setConnected(true);
    state.setNunchukConnected(true);
    state.setUseAccelerometer(false);

    TEST_ASSERT_TRUE(state.isConnected());
    TEST_ASSERT_TRUE(state.isNunchukConnected());
    TEST_ASSERT_FALSE(state.getUseAccelerometer());

    state.setBatteryLevel(0);
    TEST_ASSERT_EQUAL_UINT8(0, state.getBatteryLevel());

    state.setBatteryLevel(104);
    TEST_ASSERT_EQUAL_UINT8(50, state.getBatteryLevel());

    state.setBatteryLevel(208);
    TEST_ASSERT_EQUAL_UINT8(100, state.getBatteryLevel());

    state.setBatteryLevel(255);
    TEST_ASSERT_EQUAL_UINT8(100, state.getBatteryLevel());

    state.reset();
    TEST_ASSERT_FALSE(state.isConnected());
    TEST_ASSERT_FALSE(state.isNunchukConnected());
    TEST_ASSERT_EQUAL_UINT8(0, state.getBatteryLevel());
    TEST_ASSERT_FALSE(state.getUseAccelerometer());
}

void testL2capPacketBuilderAndSender() {
    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t packet[64] = {0};

    uint16_t l2capLen = makeL2capPacket(packet, 0x0041, payload, sizeof(payload));
    TEST_ASSERT_EQUAL_UINT16(kL2CapHeaderLen + sizeof(payload), l2capLen);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), packet[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, packet[1]);
    TEST_ASSERT_EQUAL_UINT8(0x41, packet[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, packet[3]);

    AclPacketControl control = {0b10, 0b00};
    uint16_t aclLen =
        makeAclL2capPacket(packet, 0x0040, control, 0x0001, payload, (uint8_t)sizeof(payload));
    TEST_ASSERT_EQUAL_UINT16(kHciH4AclPreambleSize + kL2CapHeaderLen + sizeof(payload), aclLen);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(H4PacketType::Acl), packet[0]);

    L2capPacketSender sender;
    sender.sendAclL2capPacket(0x0040, 0x0001, payload, sizeof(payload));
    TEST_ASSERT_EQUAL(0, gSendCount);  // no callback set

    sender.setSendCallback(captureRawPacket);
    sender.sendAclL2capPacket(0x0040, 0x0001, payload, sizeof(payload));
    TEST_ASSERT_EQUAL(1, gSendCount);

    uint8_t tooLarge[300];
    sender.sendAclL2capPacket(0x0040, 0x0001, tooLarge, sizeof(tooLarge));
    TEST_ASSERT_EQUAL(1, gSendCount);  // should be ignored
}

void testL2capSignalingFlows() {
    L2capConnectionTable connections;
    L2capPacketSender sender;
    sender.setSendCallback(captureRawPacket);

    L2capSignaling signaling;
    signaling.init(&connections, &sender);

    signaling.sendConnectionRequest(0x0040, 0x0011, 0x0042);
    TEST_ASSERT_EQUAL(1, gSendCount);

    const uint8_t *payload = l2capPayloadPtr();
    TEST_ASSERT_EQUAL_UINT8(0x02, payload[0]);  // ConnectionRequest
    TEST_ASSERT_EQUAL_UINT8(0x01, payload[1]);  // Identifier

    gSendCount = 0;
    uint8_t responseOk[12] = {0};
    responseOk[4] = 0x42;
    responseOk[5] = 0x00;  // dstCID = 0x0042
    responseOk[8] = 0x00;
    responseOk[9] = 0x00;  // SUCCESS

    signaling.handleConnectionResponse(0x0040, responseOk, sizeof(responseOk));
    TEST_ASSERT_EQUAL(1, gSendCount);  // sends configuration request

    uint16_t remoteCID = 0;
    TEST_ASSERT_EQUAL(0, connections.getRemoteCid(0x0040, &remoteCID));
    TEST_ASSERT_EQUAL_UINT16(0x0042, remoteCID);

    payload = l2capPayloadPtr();
    TEST_ASSERT_EQUAL_UINT8(0x04, payload[0]);  // ConfigurationRequest

    gSendCount = 0;
    uint8_t responseFail[12] = {0};
    responseFail[4] = 0x43;
    responseFail[8] = 0x04;
    responseFail[9] = 0x00;  // NoResources
    signaling.handleConnectionResponse(0x0050, responseFail, sizeof(responseFail));
    TEST_ASSERT_EQUAL(0, gSendCount);

    // Valid configuration request should produce configuration response.
    L2capConnection::Endpoint endpoint = {0x0060, 0x0061};
    connections.addConnection(L2capConnection(endpoint));

    gSendCount = 0;
    uint8_t configReq[12] = {0};
    configReq[1] = 0x77;   // Identifier
    configReq[2] = 0x08;
    configReq[3] = 0x00;   // dataLen=8
    configReq[6] = 0x00;
    configReq[7] = 0x00;   // flags=0
    configReq[8] = 0x01;
    configReq[9] = 0x02;   // MTU option
    configReq[10] = 0x40;
    configReq[11] = 0x00;  // mtu=64

    signaling.handleConfigurationRequest(0x0060, configReq, sizeof(configReq));
    TEST_ASSERT_EQUAL(1, gSendCount);
    payload = l2capPayloadPtr();
    TEST_ASSERT_EQUAL_UINT8(0x05, payload[0]);  // ConfigurationResponse
    TEST_ASSERT_EQUAL_UINT8(0x77, payload[1]);

    // Invalid request must not respond.
    gSendCount = 0;
    configReq[6] = 0x01;  // invalid flags
    signaling.handleConfigurationRequest(0x0060, configReq, sizeof(configReq));
    TEST_ASSERT_EQUAL(0, gSendCount);
}

void testHciCallbacksSendAvailableResetsOnlyWhenDeviceNotInited() {
    mockDeviceIsInited = false;
    HciCallbacksHandler::notifyHostSendAvailable();
    TEST_ASSERT_EQUAL(1, mockResetDeviceCallCount);

    mockDeviceIsInited = true;
    HciCallbacksHandler::notifyHostSendAvailable();
    TEST_ASSERT_EQUAL(1, mockResetDeviceCallCount);
}

void testHciCallbacksRecvAndHostSendPacketPaths() {
    uint8_t data[] = {0x01, 0x02, 0x03};

    HciCallbacksHandler::setQueueManager(nullptr);
    TEST_ASSERT_EQUAL(ESP_FAIL, HciCallbacksHandler::notifyHostRecv(data, sizeof(data)));
    HciCallbacksHandler::hciHostSendPacket(data, sizeof(data));

    HciQueueManager queueManager(8, 8);
    TEST_ASSERT_TRUE(queueManager.createQueues());
    HciCallbacksHandler::setQueueManager(&queueManager);

    TEST_ASSERT_EQUAL(ESP_OK, HciCallbacksHandler::notifyHostRecv(data, sizeof(data)));
    TEST_ASSERT_TRUE(queueManager.hasRxPending());

    HciCallbacksHandler::hciHostSendPacket(data, sizeof(data));
    TEST_ASSERT_TRUE(queueManager.hasTxPending());

    queueManager.processRxQueue();
    TEST_ASSERT_EQUAL(1, mockHandleHciDataCallCount);

    queueManager.processTxQueue();
    TEST_ASSERT_EQUAL(1, gMockVhciSendCount);
}

void testHciQueueManagerFailureAndPendingBranches() {
    HciQueueManager uninitializedManager;
    uint8_t payload[] = {0xAA, 0xBB};

    TEST_ASSERT_FALSE(uninitializedManager.sendToTxQueue(payload, sizeof(payload)));
    TEST_ASSERT_FALSE(uninitializedManager.sendToRxQueue(payload, sizeof(payload)));
    TEST_ASSERT_FALSE(uninitializedManager.hasTxPending());
    TEST_ASSERT_FALSE(uninitializedManager.hasRxPending());

    mockQueueCreateCallCount = 0;
    mockQueueCreateFailOnCall = 1;
    HciQueueManager txFailManager;
    TEST_ASSERT_FALSE(txFailManager.createQueues());

    mockQueueCreateCallCount = 0;
    mockQueueCreateFailOnCall = 2;
    HciQueueManager rxFailManager;
    TEST_ASSERT_FALSE(rxFailManager.createQueues());

    mockQueueCreateCallCount = 0;
    mockQueueCreateFailOnCall = 0;
    HciQueueManager queueManager;
    TEST_ASSERT_TRUE(queueManager.createQueues());

    TEST_ASSERT_TRUE(queueManager.sendToTxQueue(nullptr, 0U));
    TEST_ASSERT_TRUE(queueManager.sendToRxQueue(nullptr, 0U));
    TEST_ASSERT_FALSE(queueManager.hasTxPending());
    TEST_ASSERT_FALSE(queueManager.hasRxPending());

    TEST_ASSERT_TRUE(queueManager.sendToTxQueue(payload, sizeof(payload)));
    TEST_ASSERT_TRUE(queueManager.sendToRxQueue(payload, sizeof(payload)));
    TEST_ASSERT_TRUE(queueManager.hasTxPending());
    TEST_ASSERT_TRUE(queueManager.hasRxPending());

    gMockVhciSendAvailable = false;
    queueManager.processTxQueue();
    TEST_ASSERT_TRUE(queueManager.hasTxPending());
    TEST_ASSERT_EQUAL(0, gMockVhciSendCount);

    gMockVhciSendAvailable = true;
    queueManager.processTxQueue();
    TEST_ASSERT_FALSE(queueManager.hasTxPending());
    TEST_ASSERT_EQUAL(1, gMockVhciSendCount);

    queueManager.processRxQueue();
    TEST_ASSERT_FALSE(queueManager.hasRxPending());
    TEST_ASSERT_EQUAL(1, mockHandleHciDataCallCount);
}

void testHciQueueManagerQueueFullReturnsFalse() {
    HciQueueManager queueManager;
    uint8_t payload = 0x11;
    TEST_ASSERT_TRUE(queueManager.createQueues());

    bool enqueueResult = true;
    for (int i = 0; i < 64; ++i) {
        TEST_ASSERT_TRUE(queueManager.sendToTxQueue(&payload, sizeof(payload)));
    }

    enqueueResult = queueManager.sendToTxQueue(&payload, sizeof(payload));
    TEST_ASSERT_FALSE(enqueueResult);
}

void testBluetoothControllerInitErrorPathsAndStartedState() {
    HciCallbacksHandler callbacks;
    HciQueueManager queueManager(8, 8);

    TEST_ASSERT_FALSE(BluetoothController::init(nullptr, &queueManager));
    TEST_ASSERT_FALSE(BluetoothController::init(&callbacks, nullptr));

    mockBtStartResult = false;
    TEST_ASSERT_FALSE(BluetoothController::init(&callbacks, &queueManager));

    mockBtStartResult = true;
    mockQueueCreateCallCount = 0;
    mockQueueCreateFailOnCall = 2;
    TEST_ASSERT_FALSE(BluetoothController::init(&callbacks, &queueManager));

    mockQueueCreateCallCount = 0;
    mockQueueCreateFailOnCall = 0;
    mockVhciRegisterResult = ESP_FAIL;
    TEST_ASSERT_FALSE(BluetoothController::init(&callbacks, &queueManager));

    mockVhciRegisterResult = ESP_OK;
    TEST_ASSERT_TRUE(BluetoothController::init(&callbacks, &queueManager));
    TEST_ASSERT_GREATER_THAN(0, mockTinyWiimoteInitCallCount);

    mockBtStarted = false;
    TEST_ASSERT_FALSE(BluetoothController::isStarted());

    mockBtStarted = true;
    TEST_ASSERT_TRUE(BluetoothController::isStarted());
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testWiimoteReportsPutReadAndBounds);
    RUN_TEST(testWiimoteStateTransitionsAndBatteryMapping);
    RUN_TEST(testL2capPacketBuilderAndSender);
    RUN_TEST(testL2capSignalingFlows);
    RUN_TEST(testHciCallbacksSendAvailableResetsOnlyWhenDeviceNotInited);
    RUN_TEST(testHciCallbacksRecvAndHostSendPacketPaths);
    RUN_TEST(testHciQueueManagerFailureAndPendingBranches);
    RUN_TEST(testHciQueueManagerQueueFullReturnsFalse);
    RUN_TEST(testBluetoothControllerInitErrorPathsAndStartedState);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testWiimoteReportsPutReadAndBounds);
    RUN_TEST(testWiimoteStateTransitionsAndBatteryMapping);
    RUN_TEST(testL2capPacketBuilderAndSender);
    RUN_TEST(testL2capSignalingFlows);
    RUN_TEST(testHciCallbacksSendAvailableResetsOnlyWhenDeviceNotInited);
    RUN_TEST(testHciCallbacksRecvAndHostSendPacketPaths);
    RUN_TEST(testHciQueueManagerFailureAndPendingBranches);
    RUN_TEST(testHciQueueManagerQueueFullReturnsFalse);
    RUN_TEST(testBluetoothControllerInitErrorPathsAndStartedState);

    UNITY_END();
}

void loop() {}
#endif
