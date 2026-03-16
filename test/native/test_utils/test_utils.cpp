#include "../../../src/tinywiimote/utils/hci_utils.h"
#include "../../../src/utils/protocol_codes.h"
#include "../../../src/utils/serial_logging.h"

#include <cstring>
#include <unity.h>

// Implemented in hci_utils.cpp but not exposed in the public header.
extern char *format2Hex(uint8_t *data, uint16_t len);

void setUp(void) {
    wiimoteSetLogLevel(kWiimoteLogWarning);
}

void tearDown(void) {}

void testL2capSignalCodeToString() {
    TEST_ASSERT_EQUAL_STRING(
        "CONNECTION_REQUEST",
        l2capSignalCodeToString((uint8_t)L2capSignalingCode::ConnectionRequest));
    TEST_ASSERT_EQUAL_STRING(
        "CONNECTION_RESPONSE",
        l2capSignalCodeToString((uint8_t)L2capSignalingCode::ConnectionResponse));
    TEST_ASSERT_EQUAL_STRING(
        "CONFIGURATION_REQUEST",
        l2capSignalCodeToString((uint8_t)L2capSignalingCode::ConfigurationRequest));
    TEST_ASSERT_EQUAL_STRING(
        "CONFIGURATION_RESPONSE",
        l2capSignalCodeToString((uint8_t)L2capSignalingCode::ConfigurationResponse));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_SIGNAL", l2capSignalCodeToString(0xFF));
}

void testL2capSignalingResultToString() {
    TEST_ASSERT_EQUAL_STRING("SUCCESS",
                             l2capSignalingResultToString((uint16_t)L2capSignalingResult::SUCCESS));
    TEST_ASSERT_EQUAL_STRING("PENDING",
                             l2capSignalingResultToString((uint16_t)L2capSignalingResult::PENDING));
    TEST_ASSERT_EQUAL_STRING(
        "PSM_NOT_SUPPORTED",
        l2capSignalingResultToString((uint16_t)L2capSignalingResult::PsmNotSupported));
    TEST_ASSERT_EQUAL_STRING("SECURITY_BLOCK", l2capSignalingResultToString(
                                                   (uint16_t)L2capSignalingResult::SecurityBlock));
    TEST_ASSERT_EQUAL_STRING(
        "NO_RESOURCES", l2capSignalingResultToString((uint16_t)L2capSignalingResult::NoResources));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_RESULT", l2capSignalingResultToString(0xFFFF));
}

void testWiimoteCodeToStringHelpers() {
    TEST_ASSERT_EQUAL_STRING("INPUT_REPORT",
                             wiimoteHidPrefixToString((uint8_t)WiimoteHidPrefix::InputReport));
    TEST_ASSERT_EQUAL_STRING("OUTPUT_REPORT",
                             wiimoteHidPrefixToString((uint8_t)WiimoteHidPrefix::OutputReport));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_HID_PREFIX", wiimoteHidPrefixToString(0x00));

    TEST_ASSERT_EQUAL_STRING(
        "STATUS_INFORMATION",
        wiimoteInputReportToString((uint8_t)WiimoteInputReport::StatusInformation));
    TEST_ASSERT_EQUAL_STRING("READ_MEMORY_DATA", wiimoteInputReportToString(
                                                     (uint8_t)WiimoteInputReport::ReadMemoryData));
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS",
                             wiimoteInputReportToString((uint8_t)WiimoteInputReport::CoreButtons));
    TEST_ASSERT_EQUAL_STRING(
        "CORE_BUTTONS_ACCEL",
        wiimoteInputReportToString((uint8_t)WiimoteInputReport::CoreButtonsAccel));
    TEST_ASSERT_EQUAL_STRING(
        "CORE_BUTTONS_EXT8",
        wiimoteInputReportToString((uint8_t)WiimoteInputReport::CoreButtonsExT8));
    TEST_ASSERT_EQUAL_STRING(
        "CORE_BUTTONS_ACCEL_EXT16",
        wiimoteInputReportToString((uint8_t)WiimoteInputReport::CoreButtonsAccelExT16));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_INPUT_REPORT", wiimoteInputReportToString(0xFF));

    TEST_ASSERT_EQUAL_STRING("SET_LEDS",
                             wiimoteOutputReportToString((uint8_t)WiimoteOutputReport::SetLeds));
    TEST_ASSERT_EQUAL_STRING(
        "SET_REPORTING_MODE",
        wiimoteOutputReportToString((uint8_t)WiimoteOutputReport::SetReportingMode));
    TEST_ASSERT_EQUAL_STRING(
        "REQUEST_STATUS", wiimoteOutputReportToString((uint8_t)WiimoteOutputReport::RequestStatus));
    TEST_ASSERT_EQUAL_STRING(
        "WRITE_MEMORY", wiimoteOutputReportToString((uint8_t)WiimoteOutputReport::WriteMemory));
    TEST_ASSERT_EQUAL_STRING("READ_MEMORY",
                             wiimoteOutputReportToString((uint8_t)WiimoteOutputReport::ReadMemory));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_OUTPUT_REPORT", wiimoteOutputReportToString(0xFF));

    TEST_ASSERT_EQUAL_STRING("EEPROM",
                             wiimoteAddressSpaceToString((uint8_t)WiimoteAddressSpace::EEPROM));
    TEST_ASSERT_EQUAL_STRING("ControlRegister", wiimoteAddressSpaceToString(
                                                    (uint8_t)WiimoteAddressSpace::ControlRegister));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_ADDRESS_SPACE", wiimoteAddressSpaceToString(0xFF));

    TEST_ASSERT_EQUAL_STRING("IDLE",
                             btControllerStatusToString((uint8_t)BtControllerStatusCode::IDLE));
    TEST_ASSERT_EQUAL_STRING("INITED",
                             btControllerStatusToString((uint8_t)BtControllerStatusCode::INITED));
    TEST_ASSERT_EQUAL_STRING("ENABLED",
                             btControllerStatusToString((uint8_t)BtControllerStatusCode::ENABLED));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_STATUS", btControllerStatusToString(0xFF));
}

void testWiimoteReportingModeToString() {
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS", wiimoteReportingModeToString(0x30));
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS_ACCEL", wiimoteReportingModeToString(0x31));
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS_EXT8", wiimoteReportingModeToString(0x32));
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS_ACCEL_IR12", wiimoteReportingModeToString(0x33));
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS_EXT19", wiimoteReportingModeToString(0x34));
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS_ACCEL_EXT16", wiimoteReportingModeToString(0x35));
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS_IR10_EXT9", wiimoteReportingModeToString(0x36));
    TEST_ASSERT_EQUAL_STRING("CORE_BUTTONS_ACCEL_IR10_EXT6", wiimoteReportingModeToString(0x37));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_REPORTING_MODE", wiimoteReportingModeToString(0xFF));
}

void testFormat2HexShortPayload() {
    uint8_t payload[] = {0x01, 0xAB, 0xCD};
    char *formatted = format2Hex(payload, sizeof(payload));

    TEST_ASSERT_NOT_NULL(formatted);
    TEST_ASSERT_NOT_NULL(strstr(formatted, "01 AB CD "));
}

void testFormat2HexTruncatedPayload() {
    uint8_t payload[40];
    memset(payload, 0xEE, sizeof(payload));

    char *formatted = format2Hex(payload, sizeof(payload));
    TEST_ASSERT_NOT_NULL(formatted);

    // format2Hex truncates beyond 30 bytes and appends ellipsis.
    TEST_ASSERT_NOT_NULL(strstr(formatted, "..."));
}

void testLogLevelSanitization() {
    wiimoteSetLogLevel(0xFF);
    TEST_ASSERT_EQUAL_UINT8(kWiimoteLogDebug, wiimoteGetLogLevel());

    wiimoteSetLogLevel(kWiimoteLogError);
    TEST_ASSERT_EQUAL_UINT8(kWiimoteLogError, wiimoteGetLogLevel());
}

void testWiimoteLogPrintGuardsAndPrintPath() {
    wiimoteSetLogLevel(kWiimoteLogInfo);

    // Guard paths.
    wiimoteLogPrint(kWiimoteLogInfo, "[INFO] ", nullptr);
    wiimoteLogPrint(kWiimoteLogInfo, nullptr, "hello");
    wiimoteLogPrint(kWiimoteLogDebug, "[DEBUG] ", "hidden");
    wiimoteLogPrint(kWiimoteLogInfo, "[INFO] ", "");

    // Executed print path.
    wiimoteLogPrint(kWiimoteLogInfo, "[INFO] ", "value=%d", 42);
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testL2capSignalCodeToString);
    RUN_TEST(testL2capSignalingResultToString);
    RUN_TEST(testWiimoteCodeToStringHelpers);
    RUN_TEST(testWiimoteReportingModeToString);
    RUN_TEST(testFormat2HexShortPayload);
    RUN_TEST(testFormat2HexTruncatedPayload);
    RUN_TEST(testLogLevelSanitization);
    RUN_TEST(testWiimoteLogPrintGuardsAndPrintPath);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testL2capSignalCodeToString);
    RUN_TEST(testL2capSignalingResultToString);
    RUN_TEST(testWiimoteCodeToStringHelpers);
    RUN_TEST(testWiimoteReportingModeToString);
    RUN_TEST(testFormat2HexShortPayload);
    RUN_TEST(testFormat2HexTruncatedPayload);
    RUN_TEST(testLogLevelSanitization);
    RUN_TEST(testWiimoteLogPrintGuardsAndPrintPath);

    UNITY_END();
}

void loop() {}
#endif
