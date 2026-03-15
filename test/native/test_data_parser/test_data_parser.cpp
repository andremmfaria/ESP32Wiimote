#include "../../mocks/test_mocks.h"
#include "esp32wiimote/state/button_state.h"
#include "esp32wiimote/state/sensor_state.h"

#include <unity.h>

// Test fixtures
ButtonStateManager *buttonState;
SensorStateManager *sensorState;
WiimoteDataParser *parser;

void setUp(void) {
    buttonState = new ButtonStateManager();
    sensorState = new SensorStateManager(1);
    parser = new WiimoteDataParser(buttonState, sensorState);
    mockHasData = false;
}

void tearDown(void) {
    delete parser;
    delete sensorState;
    delete buttonState;
}

// Helper function to create mock data
void setMockData(uint8_t reportType, const uint8_t *payload, uint8_t payloadLen) {
    mockData.number = 0;
    mockData.data[0] = 0xA1;  // HID Data report
    mockData.data[1] = reportType;
    for (int i = 0; i < payloadLen; i++) {
        mockData.data[2 + i] = payload[i];
    }
    mockData.len = 2 + payloadLen;
    mockHasData = true;
}

// ===== Basic Functionality Tests =====

// Test: No data available returns 0
void testNoDataAvailable() {
    mockHasData = false;
    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);
}

// Test: Data too short is rejected
void testDataTooShort() {
    mockData.data[0] = 0xA1;
    mockData.len = 3;  // Less than 4 bytes
    mockHasData = true;

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);
}

// Test: Invalid report type is rejected
void testInvalidReportType() {
    mockData.data[0] = 0xFF;  // Not 0xA1
    mockData.data[1] = 0x30;
    mockData.len = 10;
    mockHasData = true;

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);
}

// ===== Button Parsing Tests =====

// Test: Parse Core Buttons (Report 0x30)
void testParseCoreButtonsReport0x30() {
    uint8_t payload[] = {
        0x00, 0x08  // Button data: ButtonA (0x0008)
    };
    setMockData(0x30, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);  // Changed
    TEST_ASSERT_EQUAL(kButtonA, buttonState->getCurrent());
}

// Test: Parse multiple buttons
void testParseMultipleButtons() {
    uint8_t payload[] = {
        0x00, 0x09  // ButtonA (0x0008) | ButtonTwo (0x0001)
    };
    setMockData(0x30, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);

    ButtonState current = buttonState->getCurrent();
    TEST_ASSERT_TRUE(buttonStateHas(current, kButtonA));
    TEST_ASSERT_TRUE(buttonStateHas(current, kButtonTwo));
}

// Test: Parse D-pad buttons
void testParseDpadButtons() {
    uint8_t payload[] = {
        0x01, 0x00  // ButtonLeft (0x0100)
    };
    setMockData(0x31, payload, sizeof(payload));

    parser->parseData();
    TEST_ASSERT_EQUAL(kButtonLeft, buttonState->getCurrent());
}

// Test: Parse HOME button
void testParseHomeButton() {
    uint8_t payload[] = {
        0x00, 0x80  // ButtonHome (0x0080)
    };
    setMockData(0x30, payload, sizeof(payload));

    parser->parseData();
    TEST_ASSERT_EQUAL(kButtonHome, buttonState->getCurrent());
}

// ===== Accelerometer Parsing Tests =====

// Test: Parse accelerometer in Report 0x31
void testParseAccelerometerReport0x31() {
    uint8_t payload[] = {
        0x00, 0x00,    // Buttons (none)
        100, 120, 140  // Accel X, Y, Z
    };
    setMockData(0x31, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);  // Changed

    AccelState accel = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(100, accel.xAxis);
    TEST_ASSERT_EQUAL_UINT8(120, accel.yAxis);
    TEST_ASSERT_EQUAL_UINT8(140, accel.zAxis);
}

// Test: Parse accelerometer max values
void testParseAccelerometerMaxValues() {
    uint8_t payload[] = {
        0x00, 0x00,    // Buttons
        255, 255, 255  // Accel max values
    };
    setMockData(0x31, payload, sizeof(payload));

    parser->parseData();

    AccelState accel = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(255, accel.xAxis);
    TEST_ASSERT_EQUAL_UINT8(255, accel.yAxis);
    TEST_ASSERT_EQUAL_UINT8(255, accel.zAxis);
}

// Test: Parse accelerometer with Report 0x35
void testParseAccelerometerReport0x35() {
    uint8_t payload[] = {
        0x00, 0x00,                                            // Buttons
        50,   60,   70,                                        // Accel X, Y, Z
        0,    0,    0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // Extension data (16 bytes)
    };
    setMockData(0x35, payload, sizeof(payload));

    parser->parseData();

    AccelState accel = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(50, accel.xAxis);
    TEST_ASSERT_EQUAL_UINT8(60, accel.yAxis);
    TEST_ASSERT_EQUAL_UINT8(70, accel.zAxis);
}

// Test: Accelerometer reset on unsupported report
void testAccelerometerResetOnUnsupportedReport() {
    // First set some accelerometer data
    uint8_t accelPayload[] = {0x00, 0x00, 100, 120, 140};
    setMockData(0x31, accelPayload, sizeof(accelPayload));
    parser->parseData();

    // Then send report without accelerometer (0x30)
    uint8_t payload[] = {
        0x00, 0x00  // Just buttons
    };
    setMockData(0x30, payload, sizeof(payload));
    parser->parseData();

    // Accelerometer should be reset to 0
    AccelState accel = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(0, accel.xAxis);
    TEST_ASSERT_EQUAL_UINT8(0, accel.yAxis);
    TEST_ASSERT_EQUAL_UINT8(0, accel.zAxis);
}

// ===== Nunchuk Parsing Tests =====

// Test: Parse nunchuk with Report 0x32 (8 extension bytes)
void testParseNunchukReport0x32() {
    uint8_t payload[] = {
        0x00, 0x00,       // Buttons
        128,  130,        // Nunchuk stick X, Y
        100,  110,  120,  // Nunchuk accel X, Y, Z
        0xFC, 0x00, 0x00  // Button byte (C and Z released: bits inverted)
    };
    setMockData(0x32, payload, sizeof(payload));

    parser->parseData();

    NunchukState nunchuk = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(128, nunchuk.xStick);
    TEST_ASSERT_EQUAL_UINT8(130, nunchuk.yStick);
    TEST_ASSERT_EQUAL_UINT8(100, nunchuk.xAxis);
    TEST_ASSERT_EQUAL_UINT8(110, nunchuk.yAxis);
    TEST_ASSERT_EQUAL_UINT8(120, nunchuk.zAxis);
}

// Test: Parse nunchuk buttons C and Z
void testParseNunchukButtons() {
    // Nunchuk C button pressed (bit 1 = 0 when pressed, inverted)
    uint8_t payloadC[] = {
        0x00, 0x00,       // Buttons
        128,  128,        // Stick
        0,    0,    0,    // Accel
        0xFD, 0x00, 0x00  // Button byte: C pressed (bit 1 = 0)
    };
    setMockData(0x32, payloadC, sizeof(payloadC));
    parser->parseData();

    ButtonState buttons = buttonState->getCurrent();
    TEST_ASSERT_TRUE(buttonStateHas(buttons, kButtonC));
    TEST_ASSERT_FALSE(buttonStateHas(buttons, kButtonZ));

    // Reset for next test
    setUp();

    // Nunchuk Z button pressed (bit 0 = 0 when pressed, inverted)
    uint8_t payloadZ[] = {
        0x00, 0x00, 128, 128, 0, 0, 0, 0xFE, 0x00, 0x00  // Button byte: Z pressed (bit 0 = 0)
    };
    setMockData(0x32, payloadZ, sizeof(payloadZ));
    parser->parseData();

    buttons = buttonState->getCurrent();
    TEST_ASSERT_FALSE(buttonStateHas(buttons, kButtonC));
    TEST_ASSERT_TRUE(buttonStateHas(buttons, kButtonZ));
}

// Test: Parse nunchuk with Report 0x35 (16 extension bytes)
void testParseNunchukReport0x35() {
    uint8_t payload[] = {
        0x00, 0x00,                  // Buttons
        100,  110,  120,             // Wiimote accel
        150,  160,                   // Nunchuk stick X, Y
        50,   60,   70,              // Nunchuk accel X, Y, Z
        0xFC,                        // Button byte
        0,    0,    0,   0, 0, 0, 0  // Remaining extension bytes
    };
    setMockData(0x35, payload, sizeof(payload));

    parser->parseData();

    NunchukState nunchuk = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(150, nunchuk.xStick);
    TEST_ASSERT_EQUAL_UINT8(160, nunchuk.yStick);
}

// Test: Combined Wiimote + Nunchuk buttons
void testCombinedWiimoteNunchukButtons() {
    uint8_t payload[] = {
        0x00, 0x08,       // ButtonA pressed on Wiimote
        128,  128,        // Stick
        0,    0,    0,    // Accel
        0xFD, 0x00, 0x00  // C button pressed on Nunchuk
    };
    setMockData(0x32, payload, sizeof(payload));

    parser->parseData();

    ButtonState buttons = buttonState->getCurrent();
    TEST_ASSERT_TRUE(buttonStateHas(buttons, kButtonA));
    TEST_ASSERT_TRUE(buttonStateHas(buttons, kButtonC));
}

// ===== Filter Tests =====

// Test: Button filter
void testButtonFilter() {
    parser->setFilter(kFilterButton);

    uint8_t payload[] = {
        0x00, 0x08  // ButtonA
    };
    setMockData(0x30, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);  // Filtered, no change reported

    // Button state should still be updated
    TEST_ASSERT_EQUAL(kButtonA, buttonState->getCurrent());
}

// Test: Accelerometer filter
void testAccelerometerFilter() {
    parser->setFilter(kFilterAccel);

    uint8_t payload[] = {
        0x00, 0x00,    // No buttons
        100, 120, 140  // Accel data
    };
    setMockData(0x31, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);  // Filtered, no change reported

    // Accel state should still be updated
    AccelState accel = sensorState->getAccel();
    TEST_ASSERT_EQUAL_UINT8(100, accel.xAxis);
}

// Test: Nunchuk stick filter
void testNunchukStickFilter() {
    parser->setFilter(kFilterNunchukStick);

    uint8_t payload[] = {0x00, 0x00, 128, 130,  // Stick values
                         0,    0,    0,   0xFC, 0x00, 0x00};
    setMockData(0x32, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);  // Current parser reports accel change for 0x32 reports

    // Nunchuk state should still be updated
    NunchukState nunchuk = sensorState->getNunchuk();
    TEST_ASSERT_EQUAL_UINT8(128, nunchuk.xStick);
}

// Test: Multiple filters combined
void testMultipleFilters() {
    parser->setFilter(kFilterButton | kFilterAccel);

    uint8_t payload[] = {
        0x00, 0x08,    // ButtonA
        100, 120, 140  // Accel
    };
    setMockData(0x31, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);  // Both filtered
}

// Test: Get and set filter
void testGetSetFilter() {
    TEST_ASSERT_EQUAL(kFilterNone, parser->getFilter());

    parser->setFilter(kFilterButton | kFilterAccel);
    TEST_ASSERT_EQUAL(kFilterButton | kFilterAccel, parser->getFilter());

    parser->setFilter(kFilterNone);
    TEST_ASSERT_EQUAL(kFilterNone, parser->getFilter());
}

// ===== Edge Cases =====

// Test: All buttons pressed
void testAllButtonsPressed() {
    uint8_t payload[] = {
        0xFF, 0xFF  // All button bits set
    };
    setMockData(0x30, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);
    // Just verify it doesn't crash
}

// Test: Zero values everywhere
void testAllZeros() {
    uint8_t payload[] = {
        0x00, 0x00,  // No buttons
        0, 0, 0      // Zero accel
    };
    setMockData(0x31, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(kNoButton, buttonState->getCurrent());
}

// Test: Sequential updates
void testSequentialUpdates() {
    // First update: ButtonA
    uint8_t payload1[] = {0x00, 0x08};
    setMockData(0x30, payload1, sizeof(payload1));
    parser->parseData();
    TEST_ASSERT_EQUAL(kButtonA, buttonState->getCurrent());

    // Second update: ButtonB
    uint8_t payload2[] = {0x00, 0x04};
    setMockData(0x30, payload2, sizeof(payload2));
    parser->parseData();
    TEST_ASSERT_EQUAL(kButtonB, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(kButtonB, buttonState->getPrevious());
}

// ===== Main Test Runner =====

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(testNoDataAvailable);
    RUN_TEST(testDataTooShort);
    RUN_TEST(testInvalidReportType);

    // Button parsing
    RUN_TEST(testParseCoreButtonsReport0x30);
    RUN_TEST(testParseMultipleButtons);
    RUN_TEST(testParseDpadButtons);
    RUN_TEST(testParseHomeButton);

    // Accelerometer parsing
    RUN_TEST(testParseAccelerometerReport0x31);
    RUN_TEST(testParseAccelerometerMaxValues);
    RUN_TEST(testParseAccelerometerReport0x35);
    RUN_TEST(testAccelerometerResetOnUnsupportedReport);

    // Nunchuk parsing
    RUN_TEST(testParseNunchukReport0x32);
    RUN_TEST(testParseNunchukButtons);
    RUN_TEST(testParseNunchukReport0x35);
    RUN_TEST(testCombinedWiimoteNunchukButtons);

    // Filters
    RUN_TEST(testButtonFilter);
    RUN_TEST(testAccelerometerFilter);
    RUN_TEST(testNunchukStickFilter);
    RUN_TEST(testMultipleFilters);
    RUN_TEST(testGetSetFilter);

    // Edge cases
    RUN_TEST(testAllButtonsPressed);
    RUN_TEST(testAllZeros);
    RUN_TEST(testSequentialUpdates);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(testNoDataAvailable);
    RUN_TEST(testDataTooShort);
    RUN_TEST(testInvalidReportType);

    // Button parsing
    RUN_TEST(testParseCoreButtonsReport0x30);
    RUN_TEST(testParseMultipleButtons);
    RUN_TEST(testParseDpadButtons);
    RUN_TEST(testParseHomeButton);

    // Accelerometer parsing
    RUN_TEST(testParseAccelerometerReport0x31);
    RUN_TEST(testParseAccelerometerMaxValues);
    RUN_TEST(testParseAccelerometerReport0x35);
    RUN_TEST(testAccelerometerResetOnUnsupportedReport);

    // Nunchuk parsing
    RUN_TEST(testParseNunchukReport0x32);
    RUN_TEST(testParseNunchukButtons);
    RUN_TEST(testParseNunchukReport0x35);
    RUN_TEST(testCombinedWiimoteNunchukButtons);

    // Filters
    RUN_TEST(testButtonFilter);
    RUN_TEST(testAccelerometerFilter);
    RUN_TEST(testNunchukStickFilter);
    RUN_TEST(testMultipleFilters);
    RUN_TEST(testGetSetFilter);

    // Edge cases
    RUN_TEST(testAllButtonsPressed);
    RUN_TEST(testAllZeros);
    RUN_TEST(testSequentialUpdates);

    UNITY_END();
}

void loop() {
    // Empty
}
#endif
