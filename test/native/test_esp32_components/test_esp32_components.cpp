// Native tests for ESP32 hardware abstraction components:
//   HciQueueManager  (src/esp32wiimote/queue/hci_queue.cpp)
//   HciCallbacksHandler (src/esp32wiimote/hci_callbacks.cpp)
//
// FreeRTOS queues and ESP32 VHCI APIs are provided by mocks under test/mocks/.

#include "../../../src/esp32wiimote/hci_callbacks.h"
#include "../../../src/esp32wiimote/queue/hci_queue.h"
#include "../../mocks/test_mocks.h"

#include <cstring>
#include <unity.h>

// ---- Test fixture helpers ----

void setUp(void) {
    mockDeviceIsInited = false;
    mockResetDeviceCallCount = 0;
    mockHandleHciDataCallCount = 0;
    gMockVhciSendAvailable = true;
    gMockVhciSendCount = 0;
    gMockVhciSentLen = 0;
    memset(gMockVhciSentData, 0, sizeof(gMockVhciSentData));
    HciCallbacksHandler::setQueueManager(nullptr);
}

void tearDown(void) {}

// ===========================================================================
// HciQueueManager tests
// ===========================================================================

void testHciQueueManagerCreateSucceeds() {
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());
}

void testHciQueueManagerNullQueueGuards() {
    // Before createQueues(), TX and RX handles are null.
    HciQueueManager mgr(4, 4);

    uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_FALSE(mgr.sendToTxQueue(payload, sizeof(payload)));
    TEST_ASSERT_FALSE(mgr.sendToRxQueue(payload, sizeof(payload)));
    TEST_ASSERT_FALSE(mgr.hasTxPending());
    TEST_ASSERT_FALSE(mgr.hasRxPending());

    // processTxQueue / processRxQueue with null queues must not crash.
    mgr.processTxQueue();
    mgr.processRxQueue();
}

void testHciQueueManagerSendEmptyDataAccepted() {
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());

    // Sending nullptr / zero length is a no-op that returns true.
    TEST_ASSERT_TRUE(mgr.sendToTxQueue(nullptr, 0));
    TEST_ASSERT_TRUE(mgr.sendToRxQueue(nullptr, 0));
    TEST_ASSERT_FALSE(mgr.hasTxPending());
    TEST_ASSERT_FALSE(mgr.hasRxPending());
}

void testHciQueueManagerTxQueueSendAndProcess() {
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());

    uint8_t payload[] = {0xAA, 0xBB, 0xCC};
    TEST_ASSERT_TRUE(mgr.sendToTxQueue(payload, sizeof(payload)));
    TEST_ASSERT_TRUE(mgr.hasTxPending());

    // processTxQueue calls esp_vhci_host_check_send_available + esp_vhci_host_send_packet.
    gMockVhciSendAvailable = true;
    mgr.processTxQueue();

    TEST_ASSERT_EQUAL(1, gMockVhciSendCount);
    TEST_ASSERT_EQUAL(sizeof(payload), gMockVhciSentLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, gMockVhciSentData, sizeof(payload));
    TEST_ASSERT_FALSE(mgr.hasTxPending());
}

void testHciQueueManagerTxQueueNotProcessedWhenUnavailable() {
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());

    uint8_t payload[] = {0x11, 0x22};
    mgr.sendToTxQueue(payload, sizeof(payload));

    gMockVhciSendAvailable = false;
    mgr.processTxQueue();

    // Host not available → packet stays in queue, not forwarded.
    TEST_ASSERT_EQUAL(0, gMockVhciSendCount);
    TEST_ASSERT_TRUE(mgr.hasTxPending());
}

void testHciQueueManagerRxQueueSendAndProcess() {
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());

    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    TEST_ASSERT_TRUE(mgr.sendToRxQueue(payload, sizeof(payload)));
    TEST_ASSERT_TRUE(mgr.hasRxPending());

    mockHandleHciDataCallCount = 0;
    mgr.processRxQueue();

    TEST_ASSERT_EQUAL(1, mockHandleHciDataCallCount);
    TEST_ASSERT_FALSE(mgr.hasRxPending());
}

void testHciQueueManagerMultiplePacketsOrdered() {
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());

    uint8_t pkt1[] = {0x01};
    uint8_t pkt2[] = {0x02};
    mgr.sendToTxQueue(pkt1, sizeof(pkt1));
    mgr.sendToTxQueue(pkt2, sizeof(pkt2));

    // First process → pkt1 sent.
    mgr.processTxQueue();
    TEST_ASSERT_EQUAL(1, gMockVhciSendCount);
    TEST_ASSERT_EQUAL_UINT8(0x01, gMockVhciSentData[0]);

    // Second process → pkt2 sent.
    mgr.processTxQueue();
    TEST_ASSERT_EQUAL(2, gMockVhciSendCount);
    TEST_ASSERT_EQUAL_UINT8(0x02, gMockVhciSentData[0]);
}

// ===========================================================================
// HciCallbacksHandler tests
// ===========================================================================

void testHciCallbacksConstructionAndInterface() {
    HciCallbacksHandler handler;
    const struct TwHciInterface *iface = handler.getHciInterface();
    TEST_ASSERT_NOT_NULL(iface);
    TEST_ASSERT_NOT_NULL(iface->hciSendPacket);
}

void testHciCallbacksGetVhciCallbackPopulates() {
    HciCallbacksHandler handler;
    esp_vhci_host_callback_t *cb = handler.getVhciCallback();
    TEST_ASSERT_NOT_NULL(cb);
    TEST_ASSERT_NOT_NULL(cb->notify_host_recv);
    TEST_ASSERT_NOT_NULL(cb->notify_host_send_available);
}

void testHciCallbacksNotifySendAvailableNotInited() {
    // When device is NOT inited, notifyHostSendAvailable must call tinyWiimoteResetDevice.
    mockDeviceIsInited = false;
    mockResetDeviceCallCount = 0;

    HciCallbacksHandler::notifyHostSendAvailable();

    TEST_ASSERT_EQUAL(1, mockResetDeviceCallCount);
}

void testHciCallbacksNotifySendAvailableIsInited() {
    // When device IS already inited, notifyHostSendAvailable must NOT reset.
    mockDeviceIsInited = true;
    mockResetDeviceCallCount = 0;

    HciCallbacksHandler::notifyHostSendAvailable();

    TEST_ASSERT_EQUAL(0, mockResetDeviceCallCount);
}

void testHciCallbacksNotifyHostRecvNoManager() {
    // No queue manager set → returns ESP_FAIL.
    HciCallbacksHandler::setQueueManager(nullptr);

    uint8_t data[] = {0x04, 0x0E, 0x04};
    int result = HciCallbacksHandler::notifyHostRecv(data, sizeof(data));

    TEST_ASSERT_EQUAL(ESP_FAIL, result);
}

void testHciCallbacksNotifyHostRecvWithManager() {
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());
    HciCallbacksHandler::setQueueManager(&mgr);

    uint8_t data[] = {0x04, 0x0E, 0x04};
    int result = HciCallbacksHandler::notifyHostRecv(data, sizeof(data));

    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(mgr.hasRxPending());
}

void testHciCallbacksHciHostSendPacketNoManager() {
    // No queue manager → must not crash.
    HciCallbacksHandler::setQueueManager(nullptr);

    uint8_t pkt[] = {0x01, 0x03, 0x00};
    HciCallbacksHandler::hciHostSendPacket(pkt, sizeof(pkt));
    // No assertion needed; the guard must not crash.
}

void testHciCallbacksHciHostSendPacketWithManager() {
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());
    HciCallbacksHandler::setQueueManager(&mgr);

    uint8_t pkt[] = {0x01, 0x03, 0x00};
    HciCallbacksHandler::hciHostSendPacket(pkt, sizeof(pkt));

    TEST_ASSERT_TRUE(mgr.hasTxPending());
}

void testHciCallbacksInterfaceRoutesPacketsViaSendPacket() {
    // Verify that the hciSendPacket function pointer (from getHciInterface)
    // routes data into the TX queue through the static callback chain.
    HciQueueManager mgr(4, 4);
    TEST_ASSERT_TRUE(mgr.createQueues());
    HciCallbacksHandler::setQueueManager(&mgr);

    HciCallbacksHandler handler;
    const struct TwHciInterface *iface = handler.getHciInterface();

    uint8_t cmd[] = {0x01, 0x03, 0x0C, 0x00};
    iface->hciSendPacket(cmd, sizeof(cmd));

    TEST_ASSERT_TRUE(mgr.hasTxPending());
}

// ---- Main ----

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testHciQueueManagerCreateSucceeds);
    RUN_TEST(testHciQueueManagerNullQueueGuards);
    RUN_TEST(testHciQueueManagerSendEmptyDataAccepted);
    RUN_TEST(testHciQueueManagerTxQueueSendAndProcess);
    RUN_TEST(testHciQueueManagerTxQueueNotProcessedWhenUnavailable);
    RUN_TEST(testHciQueueManagerRxQueueSendAndProcess);
    RUN_TEST(testHciQueueManagerMultiplePacketsOrdered);

    RUN_TEST(testHciCallbacksConstructionAndInterface);
    RUN_TEST(testHciCallbacksGetVhciCallbackPopulates);
    RUN_TEST(testHciCallbacksNotifySendAvailableNotInited);
    RUN_TEST(testHciCallbacksNotifySendAvailableIsInited);
    RUN_TEST(testHciCallbacksNotifyHostRecvNoManager);
    RUN_TEST(testHciCallbacksNotifyHostRecvWithManager);
    RUN_TEST(testHciCallbacksHciHostSendPacketNoManager);
    RUN_TEST(testHciCallbacksHciHostSendPacketWithManager);
    RUN_TEST(testHciCallbacksInterfaceRoutesPacketsViaSendPacket);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testHciQueueManagerCreateSucceeds);
    RUN_TEST(testHciQueueManagerNullQueueGuards);
    RUN_TEST(testHciQueueManagerSendEmptyDataAccepted);
    RUN_TEST(testHciQueueManagerTxQueueSendAndProcess);
    RUN_TEST(testHciQueueManagerTxQueueNotProcessedWhenUnavailable);
    RUN_TEST(testHciQueueManagerRxQueueSendAndProcess);
    RUN_TEST(testHciQueueManagerMultiplePacketsOrdered);

    RUN_TEST(testHciCallbacksConstructionAndInterface);
    RUN_TEST(testHciCallbacksGetVhciCallbackPopulates);
    RUN_TEST(testHciCallbacksNotifySendAvailableNotInited);
    RUN_TEST(testHciCallbacksNotifySendAvailableIsInited);
    RUN_TEST(testHciCallbacksNotifyHostRecvNoManager);
    RUN_TEST(testHciCallbacksNotifyHostRecvWithManager);
    RUN_TEST(testHciCallbacksHciHostSendPacketNoManager);
    RUN_TEST(testHciCallbacksHciHostSendPacketWithManager);
    RUN_TEST(testHciCallbacksInterfaceRoutesPacketsViaSendPacket);

    UNITY_END();
}
void loop() {}
#endif
