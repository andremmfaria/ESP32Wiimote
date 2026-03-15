#include "../../../src/utils/hci_codes.h"

#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void testHciStatusCodeToStringCoverage() {
    for (uint16_t code = 0; code <= 0x3F; code++) {
        const char *name = hciStatusCodeToString((uint8_t)code);
        TEST_ASSERT_NOT_NULL(name);
    }

    TEST_ASSERT_EQUAL_STRING("UNKNOWN_STATUS", hciStatusCodeToString(0x2B));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_STATUS", hciStatusCodeToString(0xFF));
}

void testHciOpcodeToStringCoverage() {
    TEST_ASSERT_EQUAL_STRING("INQUIRY", hciOpcodeToString((uint16_t)HciOpcode::INQUIRY));
    TEST_ASSERT_EQUAL_STRING("INQUIRY_CANCEL",
                             hciOpcodeToString((uint16_t)HciOpcode::InquiryCancel));
    TEST_ASSERT_EQUAL_STRING("CREATE_CONNECTION",
                             hciOpcodeToString((uint16_t)HciOpcode::CreateConnection));
    TEST_ASSERT_EQUAL_STRING("REMOTE_NAME_REQUEST",
                             hciOpcodeToString((uint16_t)HciOpcode::RemoteNameRequest));
    TEST_ASSERT_EQUAL_STRING("RESET", hciOpcodeToString((uint16_t)HciOpcode::RESET));
    TEST_ASSERT_EQUAL_STRING("WRITE_LOCAL_NAME",
                             hciOpcodeToString((uint16_t)HciOpcode::WriteLocalName));
    TEST_ASSERT_EQUAL_STRING("WRITE_CLASS_OF_DEVICE",
                             hciOpcodeToString((uint16_t)HciOpcode::WriteClassOfDevice));
    TEST_ASSERT_EQUAL_STRING("WRITE_SCAN_ENABLE",
                             hciOpcodeToString((uint16_t)HciOpcode::WriteScanEnable));
    TEST_ASSERT_EQUAL_STRING("READ_BD_ADDR", hciOpcodeToString((uint16_t)HciOpcode::ReadBdAddr));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_OPCODE", hciOpcodeToString(0xFFFF));
}

void testHciEventCodeToStringCoverage() {
    TEST_ASSERT_EQUAL_STRING("INQUIRY_COMPLETE",
                             hciEventCodeToString((uint8_t)HciEventCode::InquiryComplete));
    TEST_ASSERT_EQUAL_STRING("INQUIRY_RESULT",
                             hciEventCodeToString((uint8_t)HciEventCode::InquiryResult));
    TEST_ASSERT_EQUAL_STRING("CONNECTION_COMPLETE",
                             hciEventCodeToString((uint8_t)HciEventCode::ConnectionComplete));
    TEST_ASSERT_EQUAL_STRING("DISCONNECTION_COMPLETE",
                             hciEventCodeToString((uint8_t)HciEventCode::DisconnectionComplete));
    TEST_ASSERT_EQUAL_STRING(
        "REMOTE_NAME_REQUEST_COMPLETE",
        hciEventCodeToString((uint8_t)HciEventCode::RemoteNameRequestComplete));
    TEST_ASSERT_EQUAL_STRING("COMMAND_COMPLETE",
                             hciEventCodeToString((uint8_t)HciEventCode::CommandComplete));
    TEST_ASSERT_EQUAL_STRING("COMMAND_STATUS",
                             hciEventCodeToString((uint8_t)HciEventCode::CommandStatus));
    TEST_ASSERT_EQUAL_STRING("NUMBER_OF_COMPLETED_PACKETS",
                             hciEventCodeToString((uint8_t)HciEventCode::NumberOfCompletedPackets));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_EVENT", hciEventCodeToString(0xFF));
}

void testHciDisconnectionReasonToStringCoverage() {
    TEST_ASSERT_EQUAL_STRING(
        "AUTHENTICATION_FAILURE",
        hciDisconnectionReasonToString((uint8_t)HciDisconnectionReason::AuthenticationFailure));
    TEST_ASSERT_EQUAL_STRING("REMOTE_USER_TERMINATED_CONNECTION",
                             hciDisconnectionReasonToString(
                                 (uint8_t)HciDisconnectionReason::RemoteUserTerminatedConnection));
    TEST_ASSERT_EQUAL_STRING(
        "REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES",
        hciDisconnectionReasonToString(
            (uint8_t)HciDisconnectionReason::RemoteDeviceTerminatedConnectionLowResources));
    TEST_ASSERT_EQUAL_STRING(
        "REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF",
        hciDisconnectionReasonToString(
            (uint8_t)HciDisconnectionReason::RemoteDeviceTerminatedConnectionPowerOff));
    TEST_ASSERT_EQUAL_STRING("CONNECTION_TERMINATED_BY_LOCAL_HOST",
                             hciDisconnectionReasonToString(
                                 (uint8_t)HciDisconnectionReason::ConnectionTerminatedByLocalHost));
    TEST_ASSERT_EQUAL_STRING(
        "CONNECTION_TIMEOUT",
        hciDisconnectionReasonToString((uint8_t)HciDisconnectionReason::ConnectionTimeout));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN_REASON", hciDisconnectionReasonToString(0xFF));
}

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testHciStatusCodeToStringCoverage);
    RUN_TEST(testHciOpcodeToStringCoverage);
    RUN_TEST(testHciEventCodeToStringCoverage);
    RUN_TEST(testHciDisconnectionReasonToStringCoverage);

    return UNITY_END();
}
#else
void setup() {
    UNITY_BEGIN();

    RUN_TEST(testHciStatusCodeToStringCoverage);
    RUN_TEST(testHciOpcodeToStringCoverage);
    RUN_TEST(testHciEventCodeToStringCoverage);
    RUN_TEST(testHciDisconnectionReasonToStringCoverage);

    UNITY_END();
}

void loop() {}
#endif
