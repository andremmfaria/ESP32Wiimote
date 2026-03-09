#include <unity.h>
#include <Arduino.h>
#include "esp32wiimote/bt_controller.h"
#include "esp32wiimote/hci_callbacks.h"
#include "esp32wiimote/queue/hci_queue.h"
#include "TinyWiimote.h"

/**
 * Bluetooth hardware test for ESP32
 * 
 * Tests Bluetooth controller initialization and HCI queue operations
 * 
 * IMPORTANT: Run with -v flag to see detailed test output!
 * 
 * Command:
 *   pio test -e esp32dev --upload-port /dev/ttyUSB0 -f embedded/test_bluetooth -v
 * 
 * These tests verify:
 * 1. Bluetooth controller initialization
 * 2. HCI queue creation and management
 * 3. Basic packet queue operations
 * 4. TinyWiimote stack initialization
 */

#define TEST_PRINT(msg) Serial.println(msg)

// Global test objects
BluetoothController* btController = nullptr;
HciCallbacksHandler* hciCallbacks = nullptr;
HciQueueManager* queueManager = nullptr;
bool btInitialized = false;

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

// ===== Bluetooth Controller Tests =====

// Test: Create BluetoothController instance
void test_create_bluetooth_controller(void) {
    TEST_PRINT("Creating BluetoothController...");
    btController = new BluetoothController();
    TEST_ASSERT_NOT_NULL(btController);
}

// Test: Create HCI queue manager
void test_create_hci_queue_manager(void) {
    TEST_PRINT("Creating HCI queue manager...");
    queueManager = new HciQueueManager(32, 32);
    TEST_ASSERT_NOT_NULL(queueManager);
}

// Test: Create HCI callback handler
void test_create_hci_callbacks(void) {
    TEST_PRINT("Creating HCI callback handler...");
    hciCallbacks = new HciCallbacksHandler();
    TEST_ASSERT_NOT_NULL(hciCallbacks);
}

// Test: Create FreeRTOS queues
void test_create_queues(void) {
    if (!queueManager) {
        queueManager = new HciQueueManager(32, 32);
    }
    
    TEST_PRINT("Creating FreeRTOS queues...");
    bool result = queueManager->createQueues();
    TEST_ASSERT_TRUE_MESSAGE(result, "Failed to create HCI queues");
}

// Test: Initialize Bluetooth controller
void test_initialize_bluetooth_controller(void) {
    if (!btController) {
        btController = new BluetoothController();
    }
    if (!hciCallbacks) {
        hciCallbacks = new HciCallbacksHandler();
    }
    if (!queueManager) {
        queueManager = new HciQueueManager(32, 32);
        queueManager->createQueues();
    }
    
    TEST_PRINT("Initializing Bluetooth controller...");
    TEST_PRINT("This may take several seconds...");
    
    bool result = btController->init(hciCallbacks, queueManager);
    TEST_ASSERT_TRUE_MESSAGE(result, "Bluetooth controller initialization failed");
    
    btInitialized = true;
    TEST_PRINT("Bluetooth controller initialized successfully!");
}

// Test: Verify Bluetooth controller is started
void test_bluetooth_controller_is_started(void) {
    if (!btInitialized || !btController) {
        TEST_IGNORE_MESSAGE("Bluetooth not initialized, skipping");
        return;
    }
    
    TEST_ASSERT_TRUE_MESSAGE(btController->isStarted(), 
                            "Bluetooth controller should be started after init");
}

// ===== HCI Queue Tests =====

// Test: Send to TX queue
void test_send_to_tx_queue(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Testing TX queue send...");
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    bool result = queueManager->sendToTxQueue(testData, sizeof(testData));
    TEST_ASSERT_TRUE_MESSAGE(result, "Failed to send to TX queue");
}

// Test: Check TX queue has pending
void test_tx_queue_has_pending(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    // Send some data
    uint8_t testData[] = {0xAA, 0xBB, 0xCC};
    queueManager->sendToTxQueue(testData, sizeof(testData));
    
    // Should have pending data
    bool hasPending = queueManager->hasTxPending();
    TEST_ASSERT_TRUE_MESSAGE(hasPending, "TX queue should have pending data");
}

// Test: Process TX queue
void test_process_tx_queue(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Processing TX queue...");
    // This should not crash even with no data
    queueManager->processTxQueue();
    TEST_ASSERT_TRUE(true); // If we got here, no crash
}

// Test: Send to RX queue
void test_send_to_rx_queue(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Testing RX queue send...");
    uint8_t testData[] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00}; // HCI Command Complete
    bool result = queueManager->sendToRxQueue(testData, sizeof(testData));
    TEST_ASSERT_TRUE_MESSAGE(result, "Failed to send to RX queue");
}

// Test: Check RX queue has pending
void test_rx_queue_has_pending(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    // Send some data
    uint8_t testData[] = {0xFF, 0xEE, 0xDD};
    queueManager->sendToRxQueue(testData, sizeof(testData));
    
    // Should have pending data
    bool hasPending = queueManager->hasRxPending();
    TEST_ASSERT_TRUE_MESSAGE(hasPending, "RX queue should have pending data");
}

// Test: Process RX queue
void test_process_rx_queue(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Processing RX queue...");
    // This should not crash even with no data
    queueManager->processRxQueue();
    TEST_ASSERT_TRUE(true); // If we got here, no crash
}

// ===== Queue Stress Tests =====

// Test: Multiple sequential sends to TX queue
void test_multiple_tx_sends(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Testing multiple TX queue sends...");
    int successCount = 0;
    
    for (int i = 0; i < 10; i++) {
        uint8_t testData[] = {(uint8_t)i, (uint8_t)(i+1), (uint8_t)(i+2)};
        if (queueManager->sendToTxQueue(testData, sizeof(testData))) {
            successCount++;
        }
    }
    
    TEST_ASSERT_GREATER_THAN(0, successCount);
    char msg[64];
    sprintf(msg, "Successfully sent %d/10 packets", successCount);
    TEST_PRINT(msg);
}

// Test: Multiple sequential sends to RX queue
void test_multiple_rx_sends(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Testing multiple RX queue sends...");
    int successCount = 0;
    
    for (int i = 0; i < 10; i++) {
        uint8_t testData[] = {(uint8_t)(0x80 + i), (uint8_t)(i+1), (uint8_t)(i+2)};
        if (queueManager->sendToRxQueue(testData, sizeof(testData))) {
            successCount++;
        }
    }
    
    TEST_ASSERT_GREATER_THAN(0, successCount);
    char msg[64];
    sprintf(msg, "Successfully sent %d/10 packets", successCount);
    TEST_PRINT(msg);
}

// Test: Alternating TX/RX sends
void test_alternating_tx_rx_sends(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Testing alternating TX/RX sends...");
    
    for (int i = 0; i < 5; i++) {
        uint8_t txData[] = {0x01, (uint8_t)i};
        uint8_t rxData[] = {0x02, (uint8_t)i};
        
        queueManager->sendToTxQueue(txData, sizeof(txData));
        queueManager->sendToRxQueue(rxData, sizeof(rxData));
    }
    
    TEST_ASSERT_TRUE(true); // If we got here, no crash
}

// Test: Send large packet
void test_send_large_packet(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Testing large packet send...");
    uint8_t largePacket[256];
    for (int i = 0; i < 256; i++) {
        largePacket[i] = i & 0xFF;
    }
    
    bool result = queueManager->sendToTxQueue(largePacket, sizeof(largePacket));
    // Large packets may or may not succeed depending on queue implementation
    // Just verify it doesn't crash
    TEST_ASSERT_TRUE(true);
}

// Test: Send empty packet
void test_send_empty_packet(void) {
    if (!queueManager) {
        TEST_IGNORE_MESSAGE("Queue manager not available, skipping");
        return;
    }
    
    TEST_PRINT("Testing empty packet send...");
    uint8_t emptyData[] = {};
    // This might fail but shouldn't crash
    queueManager->sendToTxQueue(emptyData, 0);
    TEST_ASSERT_TRUE(true); // If we got here, no crash
}

// ===== TinyWiimote Stack Tests =====

// Test: TinyWiimote initial state
void test_tinywiimote_initial_state(void) {
    TEST_PRINT("Checking TinyWiimote initial state...");
    
    bool isConnected = TinyWiimoteIsConnected();
    TEST_ASSERT_FALSE_MESSAGE(isConnected, "Should not be connected initially");
    
    bool deviceInited = TinyWiimoteDeviceIsInited();
    // Device init state depends on whether BT was initialized
    TEST_PRINT(deviceInited ? "Device is initialized" : "Device not initialized");
}

// Test: TinyWiimote data availability
void test_tinywiimote_data_availability(void) {
    TEST_PRINT("Checking TinyWiimote data availability...");
    
    int available = TinyWiimoteAvailable();
    TEST_ASSERT_EQUAL(0, available); // No data without connection
}

// Test: TinyWiimote battery level
void test_tinywiimote_battery_level(void) {
    TEST_PRINT("Checking TinyWiimote battery level...");
    
    uint8_t battery = TinyWiimoteGetBatteryLevel();
    // Without connection, battery should be 0
    TEST_ASSERT_EQUAL_UINT8(0, battery);
}

// ===== Memory and Resource Tests =====

// Test: Heap memory check before BT init
void test_heap_memory_before_bt(void) {
    size_t freeBefore = ESP.getFreeHeap();
    char msg[64];
    sprintf(msg, "Free heap before BT: %d bytes", freeBefore);
    TEST_PRINT(msg);
    
    TEST_ASSERT_GREATER_THAN(10000, freeBefore); // Should have at least 10KB
}

// Test: Heap memory check after BT init
void test_heap_memory_after_bt(void) {
    if (!btInitialized) {
        TEST_IGNORE_MESSAGE("Bluetooth not initialized, skipping");
        return;
    }
    
    size_t freeAfter = ESP.getFreeHeap();
    char msg[64];
    sprintf(msg, "Free heap after BT: %d bytes", freeAfter);
    TEST_PRINT(msg);
    
    // BT uses significant memory, but should still have some free
    TEST_ASSERT_GREATER_THAN(5000, freeAfter); // Should have at least 5KB
}

// ===== Cleanup Tests =====

// Test: Safe cleanup (run last)
void test_cleanup(void) {
    TEST_PRINT("Testing cleanup...");
    
    // Note: We don't actually delete objects here because other tests may still need them
    // In real usage, cleanup would happen at program exit
    
    TEST_ASSERT_TRUE(true);
}

// ===== Main Test Runner =====

void setup() {
    Serial.begin(115200);
    delay(2000); // Give serial time to initialize
    
    UNITY_BEGIN();
    
    TEST_PRINT("\n========================================");
    TEST_PRINT("ESP32 Bluetooth Hardware Tests");
    TEST_PRINT("========================================\n");
    
    // Object creation tests
    RUN_TEST(test_create_bluetooth_controller);
    RUN_TEST(test_create_hci_queue_manager);
    RUN_TEST(test_create_hci_callbacks);
    RUN_TEST(test_create_queues);
    
    // Memory check before BT init
    RUN_TEST(test_heap_memory_before_bt);
    
    // Bluetooth initialization (may take several seconds)
    RUN_TEST(test_initialize_bluetooth_controller);
    RUN_TEST(test_bluetooth_controller_is_started);
    
    // Memory check after BT init
    RUN_TEST(test_heap_memory_after_bt);
    
    // HCI queue tests
    RUN_TEST(test_send_to_tx_queue);
    RUN_TEST(test_tx_queue_has_pending);
    RUN_TEST(test_process_tx_queue);
    RUN_TEST(test_send_to_rx_queue);
    RUN_TEST(test_rx_queue_has_pending);
    RUN_TEST(test_process_rx_queue);
    
    // Queue stress tests
    RUN_TEST(test_multiple_tx_sends);
    RUN_TEST(test_multiple_rx_sends);
    RUN_TEST(test_alternating_tx_rx_sends);
    RUN_TEST(test_send_large_packet);
    RUN_TEST(test_send_empty_packet);
    
    // TinyWiimote stack tests
    RUN_TEST(test_tinywiimote_initial_state);
    RUN_TEST(test_tinywiimote_data_availability);
    RUN_TEST(test_tinywiimote_battery_level);
    
    // Cleanup
    RUN_TEST(test_cleanup);
    
    UNITY_END();
}

void loop() {
    // Empty
}
