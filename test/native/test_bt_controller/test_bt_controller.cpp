#include "../../../src/esp32wiimote/bt_controller.h"
#include "../../../src/esp32wiimote/hci_callbacks.h"
#include "../../../src/esp32wiimote/queue/hci_queue.h"
#include "../../mocks/test_mocks.h"

#include <unity.h>

void setUp(void) {
    mockBtStartResult = true;
    mockBtStarted = false;
    mockBtControllerStatus = static_cast<uint8_t>(ESP_BT_CONTROLLER_STATUS_IDLE);
    mockEspFreeHeap = 4096U;
    mockTinyWiimoteInitCallCount = 0;
    mockLastHciInterface = {nullptr};
    mockVhciRegisterResult = ESP_OK;
    mockVhciRegisterCallCount = 0;
    mockLastVhciCallback = nullptr;
    mockQueueCreateCallCount = 0;
    mockQueueCreateFailOnCall = 0;
    HciCallbacksHandler::setQueueManager(nullptr);
}

void tearDown(void) {}

void testBluetoothControllerRejectsNullParameters() {
    HciCallbacksHandler callbacks;
    HciQueueManager queueManager;

    TEST_ASSERT_FALSE(BluetoothController::init(nullptr, &queueManager));
    TEST_ASSERT_FALSE(BluetoothController::init(&callbacks, nullptr));
    TEST_ASSERT_EQUAL(0, mockTinyWiimoteInitCallCount);
    TEST_ASSERT_EQUAL(0, mockVhciRegisterCallCount);
}

void testBluetoothControllerBtStartFailureStopsInitialization() {
    HciCallbacksHandler callbacks;
    HciQueueManager queueManager;

    mockBtStartResult = false;

    TEST_ASSERT_FALSE(BluetoothController::init(&callbacks, &queueManager));
    TEST_ASSERT_EQUAL(0, mockTinyWiimoteInitCallCount);
    TEST_ASSERT_EQUAL(0, mockQueueCreateCallCount);
    TEST_ASSERT_EQUAL(0, mockVhciRegisterCallCount);
    TEST_ASSERT_FALSE(BluetoothController::isStarted());
}

void testBluetoothControllerQueueCreationFailureStopsInitialization() {
    HciCallbacksHandler callbacks;
    HciQueueManager queueManager;

    mockQueueCreateFailOnCall = 2;

    TEST_ASSERT_FALSE(BluetoothController::init(&callbacks, &queueManager));
    TEST_ASSERT_EQUAL(1, mockTinyWiimoteInitCallCount);
    TEST_ASSERT_EQUAL(2, mockQueueCreateCallCount);
    TEST_ASSERT_EQUAL(0, mockVhciRegisterCallCount);
    TEST_ASSERT_FALSE(BluetoothController::isStarted());
}

void testBluetoothControllerVhciRegistrationFailureStopsInitialization() {
    HciCallbacksHandler callbacks;
    HciQueueManager queueManager;

    mockVhciRegisterResult = ESP_FAIL;

    TEST_ASSERT_FALSE(BluetoothController::init(&callbacks, &queueManager));
    TEST_ASSERT_EQUAL(1, mockTinyWiimoteInitCallCount);
    TEST_ASSERT_EQUAL(2, mockQueueCreateCallCount);
    TEST_ASSERT_EQUAL(1, mockVhciRegisterCallCount);
    TEST_ASSERT_NOT_NULL(mockLastVhciCallback);
    TEST_ASSERT_FALSE(BluetoothController::isStarted());
}

void testBluetoothControllerInitSuccessAndStartedState() {
    HciCallbacksHandler callbacks;
    HciQueueManager queueManager;

    mockBtControllerStatus = static_cast<uint8_t>(ESP_BT_CONTROLLER_STATUS_INITED);

    TEST_ASSERT_TRUE(BluetoothController::init(&callbacks, &queueManager));
    TEST_ASSERT_EQUAL(1, mockTinyWiimoteInitCallCount);
    TEST_ASSERT_NOT_NULL(mockLastHciInterface.hciSendPacket);
    TEST_ASSERT_EQUAL(1, mockVhciRegisterCallCount);
    TEST_ASSERT_NOT_NULL(mockLastVhciCallback);
    TEST_ASSERT_TRUE(BluetoothController::isStarted());

    mockBtStarted = false;
    TEST_ASSERT_FALSE(BluetoothController::isStarted());
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testBluetoothControllerRejectsNullParameters);
    RUN_TEST(testBluetoothControllerBtStartFailureStopsInitialization);
    RUN_TEST(testBluetoothControllerQueueCreationFailureStopsInitialization);
    RUN_TEST(testBluetoothControllerVhciRegistrationFailureStopsInitialization);
    RUN_TEST(testBluetoothControllerInitSuccessAndStartedState);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testBluetoothControllerRejectsNullParameters);
    RUN_TEST(testBluetoothControllerBtStartFailureStopsInitialization);
    RUN_TEST(testBluetoothControllerQueueCreationFailureStopsInitialization);
    RUN_TEST(testBluetoothControllerVhciRegistrationFailureStopsInitialization);
    RUN_TEST(testBluetoothControllerInitSuccessAndStartedState);

    UNITY_END();
}

void loop() {}
#endif
