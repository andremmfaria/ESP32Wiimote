#include <unity.h>
#ifdef NATIVE_TEST
// For native tests, delay/millis are not needed
#define delay(x) ((void)0)
#define millis() (0UL)
#else
#include <Arduino.h>
#endif
#include "esp32wiimote/state/sensor_state.h"

// Test fixtures
SensorStateManager *sensorState;

void setUp(void) {
    sensorState = new SensorStateManager(1);  // threshold = 1
}

void tearDown(void) {
    delete sensorState;
}

// ===== Accelerometer Tests =====

// Test: Initial accelerometer state should be zero
void testInitialAccelState() {
    AccelState accel = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(0, accel.xAxis);
    TEST_ASSERT_EQUAL_UINT8(0, accel.yAxis);
    TEST_ASSERT_EQUAL_UINT8(0, accel.zAxis);
    TEST_ASSERT_FALSE(sensorState->accelHasChanged());
}

// Test: Update accelerometer changes state
void testUpdateAccelChangesState() {
    AccelState newAccel = {100, 120, 140};
    sensorState->updateAccel(newAccel);

    AccelState current = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(100, current.xAxis);
    TEST_ASSERT_EQUAL_UINT8(120, current.yAxis);
    TEST_ASSERT_EQUAL_UINT8(140, current.zAxis);
    TEST_ASSERT_TRUE(sensorState->accelHasChanged());
}

// Test: Reset accelerometer clears state
void testResetAccelClearsState() {
    AccelState newAccel = {100, 120, 140};
    sensorState->updateAccel(newAccel);
    sensorState->resetAccel();

    AccelState current = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(0, current.xAxis);
    TEST_ASSERT_EQUAL_UINT8(0, current.yAxis);
    TEST_ASSERT_EQUAL_UINT8(0, current.zAxis);
}

// Test: Accelerometer preserves previous state
void testAccelPreservesPreviousState() {
    AccelState accel1 = {50, 60, 70};
    sensorState->updateAccel(accel1);
    sensorState->resetChangeFlags();

    AccelState accel2 = {80, 90, 100};
    sensorState->updateAccel(accel2);

    AccelState previous = sensorState->getPreviousAccel();
    TEST_ASSERT_EQUAL_UINT8(50, previous.xAxis);
    TEST_ASSERT_EQUAL_UINT8(60, previous.yAxis);
    TEST_ASSERT_EQUAL_UINT8(70, previous.zAxis);
}

// Test: Accelerometer max values (255)
void testAccelMaxValues() {
    AccelState maxAccel = {255, 255, 255};
    sensorState->updateAccel(maxAccel);

    AccelState current = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(255, current.xAxis);
    TEST_ASSERT_EQUAL_UINT8(255, current.yAxis);
    TEST_ASSERT_EQUAL_UINT8(255, current.zAxis);
}

// ===== Nunchuk Tests =====

// Test: Initial nunchuk state should be zero
void testInitialNunchukState() {
    NunchukState nunchuk = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(0, nunchuk.xStick);
    TEST_ASSERT_EQUAL_UINT8(0, nunchuk.yStick);
    TEST_ASSERT_EQUAL_UINT8(0, nunchuk.xAxis);
    TEST_ASSERT_EQUAL_UINT8(0, nunchuk.yAxis);
    TEST_ASSERT_EQUAL_UINT8(0, nunchuk.zAxis);
    TEST_ASSERT_FALSE(sensorState->nunchukStickHasChanged());
}

// Test: Update nunchuk changes state
void testUpdateNunchukChangesState() {
    NunchukState newNunchuk = {128, 130, 100, 110, 120};
    sensorState->updateNunchuk(newNunchuk);

    NunchukState current = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(128, current.xStick);
    TEST_ASSERT_EQUAL_UINT8(130, current.yStick);
    TEST_ASSERT_EQUAL_UINT8(100, current.xAxis);
    TEST_ASSERT_EQUAL_UINT8(110, current.yAxis);
    TEST_ASSERT_EQUAL_UINT8(120, current.zAxis);
}

// Test: Reset nunchuk clears state
void testResetNunchukClearsState() {
    NunchukState newNunchuk = {128, 130, 100, 110, 120};
    sensorState->updateNunchuk(newNunchuk);
    sensorState->resetNunchuk();

    NunchukState current = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(0, current.xStick);
    TEST_ASSERT_EQUAL_UINT8(0, current.yStick);
    TEST_ASSERT_EQUAL_UINT8(0, current.xAxis);
    TEST_ASSERT_EQUAL_UINT8(0, current.yAxis);
    TEST_ASSERT_EQUAL_UINT8(0, current.zAxis);
}

// Test: Nunchuk preserves previous state
void testNunchukPreservesPreviousState() {
    NunchukState nunchuk1 = {100, 110, 50, 60, 70};
    sensorState->updateNunchuk(nunchuk1);
    sensorState->resetChangeFlags();

    NunchukState nunchuk2 = {120, 130, 80, 90, 100};
    sensorState->updateNunchuk(nunchuk2);

    NunchukState previous = sensorState->getPreviousNunchuk();
    TEST_ASSERT_EQUAL_UINT8(100, previous.xStick);
    TEST_ASSERT_EQUAL_UINT8(110, previous.yStick);
}

// Test: Nunchuk stick threshold detection
void testNunchukThresholdDetection() {
    delete sensorState;
    sensorState = new SensorStateManager(9);  // threshold = 9 (3*3)

    NunchukState nunchuk1 = {128, 128, 0, 0, 0};
    sensorState->updateNunchuk(nunchuk1);
    sensorState->resetChangeFlags();

    // Change by 2 (2*2=4, below threshold of 9)
    NunchukState nunchuk2 = {130, 128, 0, 0, 0};
    sensorState->updateNunchuk(nunchuk2);
    TEST_ASSERT_FALSE(sensorState->nunchukStickHasChanged());

    // Reset and test again from baseline
    NunchukState nunchukBaseline = {128, 128, 0, 0, 0};
    sensorState->updateNunchuk(nunchukBaseline);
    sensorState->resetChangeFlags();

    // Change by 3 (3*3=9, equals threshold, should trigger)
    NunchukState nunchuk3 = {131, 128, 0, 0, 0};
    sensorState->updateNunchuk(nunchuk3);
    TEST_ASSERT_TRUE(sensorState->nunchukStickHasChanged());
}

// Test: Nunchuk stick threshold on Y axis
void testNunchukThresholdYAxis() {
    delete sensorState;
    sensorState = new SensorStateManager(100);  // threshold = 100 (10*10)

    NunchukState nunchuk1 = {128, 128, 0, 0, 0};
    sensorState->updateNunchuk(nunchuk1);
    sensorState->resetChangeFlags();

    // Change Y by 10 (10*10=100, equals threshold, should trigger)
    NunchukState nunchuk2 = {128, 138, 0, 0, 0};
    sensorState->updateNunchuk(nunchuk2);
    TEST_ASSERT_TRUE(sensorState->nunchukStickHasChanged());
}

// ===== Reset and Change Flags Tests =====

// Test: Reset change flags
void testResetChangeFlags() {
    AccelState newAccel = {100, 120, 140};
    sensorState->updateAccel(newAccel);

    NunchukState newNunchuk = {128, 130, 100, 110, 120};
    sensorState->updateNunchuk(newNunchuk);

    TEST_ASSERT_TRUE(sensorState->accelHasChanged());
    TEST_ASSERT_TRUE(sensorState->nunchukStickHasChanged());

    sensorState->resetChangeFlags();

    TEST_ASSERT_FALSE(sensorState->accelHasChanged());
    TEST_ASSERT_FALSE(sensorState->nunchukStickHasChanged());
}

// Test: Multiple rapid updates
void testRapidSensorUpdates() {
    for (int i = 0; i < 10; i++) {
        AccelState accel = {(uint8_t)(i * 10), (uint8_t)(i * 11), (uint8_t)(i * 12)};
        sensorState->updateAccel(accel);
        sensorState->resetChangeFlags();
    }

    AccelState final = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(90, final.xAxis);
    TEST_ASSERT_EQUAL_UINT8(99, final.yAxis);
    TEST_ASSERT_EQUAL_UINT8(108, final.zAxis);
}

// Test: Nunchuk center position (stick at 128, 128)
void testNunchukCenterPosition() {
    NunchukState center = {128, 128, 0, 0, 0};
    sensorState->updateNunchuk(center);

    NunchukState current = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(128, current.xStick);
    TEST_ASSERT_EQUAL_UINT8(128, current.yStick);
}

// Test: Nunchuk extreme positions
void testNunchukExtremePositions() {
    // Max right/up
    NunchukState maxPos = {255, 255, 0, 0, 0};
    sensorState->updateNunchuk(maxPos);
    NunchukState current = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(255, current.xStick);
    TEST_ASSERT_EQUAL_UINT8(255, current.yStick);

    sensorState->resetChangeFlags();

    // Min left/down
    NunchukState minPos = {0, 0, 0, 0, 0};
    sensorState->updateNunchuk(minPos);
    current = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(0, current.xStick);
    TEST_ASSERT_EQUAL_UINT8(0, current.yStick);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Accelerometer tests
    RUN_TEST(testInitialAccelState);
    RUN_TEST(testUpdateAccelChangesState);
    RUN_TEST(testResetAccelClearsState);
    RUN_TEST(testAccelPreservesPreviousState);
    RUN_TEST(testAccelMaxValues);

    // Nunchuk tests
    RUN_TEST(testInitialNunchukState);
    RUN_TEST(testUpdateNunchukChangesState);
    RUN_TEST(testResetNunchukClearsState);
    RUN_TEST(testNunchukPreservesPreviousState);
    RUN_TEST(testNunchukThresholdDetection);
    RUN_TEST(testNunchukThresholdYAxis);

    // Combined tests
    RUN_TEST(testResetChangeFlags);
    RUN_TEST(testRapidSensorUpdates);
    RUN_TEST(testNunchukCenterPosition);
    RUN_TEST(testNunchukExtremePositions);

    return UNITY_END();
}

#ifdef ARDUINO
void setup() {
    delay(2000);  // Wait for serial
    UNITY_BEGIN();

    // Accelerometer tests
    RUN_TEST(testInitialAccelState);
    RUN_TEST(testUpdateAccelChangesState);
    RUN_TEST(testResetAccelClearsState);
    RUN_TEST(testAccelPreservesPreviousState);
    RUN_TEST(testAccelMaxValues);

    // Nunchuk tests
    RUN_TEST(testInitialNunchukState);
    RUN_TEST(testUpdateNunchukChangesState);
    RUN_TEST(testResetNunchukClearsState);
    RUN_TEST(testNunchukPreservesPreviousState);
    RUN_TEST(testNunchukThresholdDetection);
    RUN_TEST(testNunchukThresholdYAxis);

    // Combined tests
    RUN_TEST(testResetChangeFlags);
    RUN_TEST(testRapidSensorUpdates);
    RUN_TEST(testNunchukCenterPosition);
    RUN_TEST(testNunchukExtremePositions);

    UNITY_END();
}

void loop() {
    // Nothing here
}
#endif
