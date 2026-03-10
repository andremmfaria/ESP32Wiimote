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
void test_no_data_available() {
    mockHasData = false;
    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);
}

// Test: Data too short is rejected
void test_data_too_short() {
    mockData.data[0] = 0xA1;
    mockData.len = 3;  // Less than 4 bytes
    mockHasData = true;

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);
}

// Test: Invalid report type is rejected
void test_invalid_report_type() {
    mockData.data[0] = 0xFF;  // Not 0xA1
    mockData.data[1] = 0x30;
    mockData.len = 10;
    mockHasData = true;

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);
}

// ===== Button Parsing Tests =====

// Test: Parse Core Buttons (Report 0x30)
void test_parse_core_buttons_report_0x30() {
    uint8_t payload[] = {
        0x00, 0x08  // Button data: BUTTON_A (0x0008)
    };
    setMockData(0x30, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);  // Changed
    TEST_ASSERT_EQUAL(BUTTON_A, buttonState->getCurrent());
}

// Test: Parse multiple buttons
void test_parse_multiple_buttons() {
    uint8_t payload[] = {
        0x00, 0x09  // BUTTON_A (0x0008) | BUTTON_TWO (0x0001)
    };
    setMockData(0x30, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);

    ButtonState current = buttonState->getCurrent();
    TEST_ASSERT_TRUE(current & BUTTON_A);
    TEST_ASSERT_TRUE(current & BUTTON_TWO);
}

// Test: Parse D-pad buttons
void test_parse_dpad_buttons() {
    uint8_t payload[] = {
        0x01, 0x00  // BUTTON_LEFT (0x0100)
    };
    setMockData(0x31, payload, sizeof(payload));

    parser->parseData();
    TEST_ASSERT_EQUAL(BUTTON_LEFT, buttonState->getCurrent());
}

// Test: Parse HOME button
void test_parse_home_button() {
    uint8_t payload[] = {
        0x00, 0x80  // BUTTON_HOME (0x0080)
    };
    setMockData(0x30, payload, sizeof(payload));

    parser->parseData();
    TEST_ASSERT_EQUAL(BUTTON_HOME, buttonState->getCurrent());
}

// ===== Accelerometer Parsing Tests =====

// Test: Parse accelerometer in Report 0x31
void test_parse_accelerometer_report_0x31() {
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
void test_parse_accelerometer_max_values() {
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
void test_parse_accelerometer_report_0x35() {
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
void test_accelerometer_reset_on_unsupported_report() {
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
void test_parse_nunchuk_report_0x32() {
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
void test_parse_nunchuk_buttons() {
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
    TEST_ASSERT_TRUE(buttons & BUTTON_C);
    TEST_ASSERT_FALSE(buttons & BUTTON_Z);

    // Reset for next test
    setUp();

    // Nunchuk Z button pressed (bit 0 = 0 when pressed, inverted)
    uint8_t payloadZ[] = {
        0x00, 0x00, 128, 128, 0, 0, 0, 0xFE, 0x00, 0x00  // Button byte: Z pressed (bit 0 = 0)
    };
    setMockData(0x32, payloadZ, sizeof(payloadZ));
    parser->parseData();

    buttons = buttonState->getCurrent();
    TEST_ASSERT_FALSE(buttons & BUTTON_C);
    TEST_ASSERT_TRUE(buttons & BUTTON_Z);
}

// Test: Parse nunchuk with Report 0x35 (16 extension bytes)
void test_parse_nunchuk_report_0x35() {
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
void test_combined_wiimote_nunchuk_buttons() {
    uint8_t payload[] = {
        0x00, 0x08,       // BUTTON_A pressed on Wiimote
        128,  128,        // Stick
        0,    0,    0,    // Accel
        0xFD, 0x00, 0x00  // C button pressed on Nunchuk
    };
    setMockData(0x32, payload, sizeof(payload));

    parser->parseData();

    ButtonState buttons = buttonState->getCurrent();
    TEST_ASSERT_TRUE(buttons & BUTTON_A);
    TEST_ASSERT_TRUE(buttons & BUTTON_C);
}

// ===== Filter Tests =====

// Test: Button filter
void test_button_filter() {
    parser->setFilter(FILTER_BUTTON);

    uint8_t payload[] = {
        0x00, 0x08  // BUTTON_A
    };
    setMockData(0x30, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);  // Filtered, no change reported

    // Button state should still be updated
    TEST_ASSERT_EQUAL(BUTTON_A, buttonState->getCurrent());
}

// Test: Accelerometer filter
void test_accelerometer_filter() {
    parser->setFilter(FILTER_ACCEL);

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
void test_nunchuk_stick_filter() {
    parser->setFilter(FILTER_NUNCHUK_STICK);

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
void test_multiple_filters() {
    parser->setFilter(FILTER_BUTTON | FILTER_ACCEL);

    uint8_t payload[] = {
        0x00, 0x08,    // BUTTON_A
        100, 120, 140  // Accel
    };
    setMockData(0x31, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(0, result);  // Both filtered
}

// Test: Get and set filter
void test_get_set_filter() {
    TEST_ASSERT_EQUAL(FILTER_NONE, parser->getFilter());

    parser->setFilter(FILTER_BUTTON | FILTER_ACCEL);
    TEST_ASSERT_EQUAL(FILTER_BUTTON | FILTER_ACCEL, parser->getFilter());

    parser->setFilter(FILTER_NONE);
    TEST_ASSERT_EQUAL(FILTER_NONE, parser->getFilter());
}

// ===== Edge Cases =====

// Test: All buttons pressed
void test_all_buttons_pressed() {
    uint8_t payload[] = {
        0xFF, 0xFF  // All button bits set
    };
    setMockData(0x30, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);
    // Just verify it doesn't crash
}

// Test: Zero values everywhere
void test_all_zeros() {
    uint8_t payload[] = {
        0x00, 0x00,  // No buttons
        0, 0, 0      // Zero accel
    };
    setMockData(0x31, payload, sizeof(payload));

    int result = parser->parseData();
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(NO_BUTTON, buttonState->getCurrent());
}

// Test: Sequential updates
void test_sequential_updates() {
    // First update: BUTTON_A
    uint8_t payload1[] = {0x00, 0x08};
    setMockData(0x30, payload1, sizeof(payload1));
    parser->parseData();
    TEST_ASSERT_EQUAL(BUTTON_A, buttonState->getCurrent());

    // Second update: BUTTON_B
    uint8_t payload2[] = {0x00, 0x04};
    setMockData(0x30, payload2, sizeof(payload2));
    parser->parseData();
    TEST_ASSERT_EQUAL(BUTTON_B, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(BUTTON_B, buttonState->getPrevious());
}

// ===== Main Test Runner =====

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(test_no_data_available);
    RUN_TEST(test_data_too_short);
    RUN_TEST(test_invalid_report_type);

    // Button parsing
    RUN_TEST(test_parse_core_buttons_report_0x30);
    RUN_TEST(test_parse_multiple_buttons);
    RUN_TEST(test_parse_dpad_buttons);
    RUN_TEST(test_parse_home_button);

    // Accelerometer parsing
    RUN_TEST(test_parse_accelerometer_report_0x31);
    RUN_TEST(test_parse_accelerometer_max_values);
    RUN_TEST(test_parse_accelerometer_report_0x35);
    RUN_TEST(test_accelerometer_reset_on_unsupported_report);

    // Nunchuk parsing
    RUN_TEST(test_parse_nunchuk_report_0x32);
    RUN_TEST(test_parse_nunchuk_buttons);
    RUN_TEST(test_parse_nunchuk_report_0x35);
    RUN_TEST(test_combined_wiimote_nunchuk_buttons);

    // Filters
    RUN_TEST(test_button_filter);
    RUN_TEST(test_accelerometer_filter);
    RUN_TEST(test_nunchuk_stick_filter);
    RUN_TEST(test_multiple_filters);
    RUN_TEST(test_get_set_filter);

    // Edge cases
    RUN_TEST(test_all_buttons_pressed);
    RUN_TEST(test_all_zeros);
    RUN_TEST(test_sequential_updates);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(test_no_data_available);
    RUN_TEST(test_data_too_short);
    RUN_TEST(test_invalid_report_type);

    // Button parsing
    RUN_TEST(test_parse_core_buttons_report_0x30);
    RUN_TEST(test_parse_multiple_buttons);
    RUN_TEST(test_parse_dpad_buttons);
    RUN_TEST(test_parse_home_button);

    // Accelerometer parsing
    RUN_TEST(test_parse_accelerometer_report_0x31);
    RUN_TEST(test_parse_accelerometer_max_values);
    RUN_TEST(test_parse_accelerometer_report_0x35);
    RUN_TEST(test_accelerometer_reset_on_unsupported_report);

    // Nunchuk parsing
    RUN_TEST(test_parse_nunchuk_report_0x32);
    RUN_TEST(test_parse_nunchuk_buttons);
    RUN_TEST(test_parse_nunchuk_report_0x35);
    RUN_TEST(test_combined_wiimote_nunchuk_buttons);

    // Filters
    RUN_TEST(test_button_filter);
    RUN_TEST(test_accelerometer_filter);
    RUN_TEST(test_nunchuk_stick_filter);
    RUN_TEST(test_multiple_filters);
    RUN_TEST(test_get_set_filter);

    // Edge cases
    RUN_TEST(test_all_buttons_pressed);
    RUN_TEST(test_all_zeros);
    RUN_TEST(test_sequential_updates);

    UNITY_END();
}

void loop() {
    // Empty
}
#endif
