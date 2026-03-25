#include "../../../src/utils/command_dispatch_message_formatter.h"
#include "../../../src/utils/command_dispatch_reason.h"

#include <unity.h>

void setUp() {}
void tearDown() {}

void testCommandDispatchReasonSerialMappings() {
    struct SerialExpectation {
        CommandDispatchReason reason;
        const char *code;
    };

    const SerialExpectation expectations[] = {
        {CommandDispatchReason::NotConnected, "not_connected"},
        {CommandDispatchReason::Locked, "locked"},
        {CommandDispatchReason::UnknownCommand, "unknown_command"},
        {CommandDispatchReason::BadArgument, "bad_argument"},
        {CommandDispatchReason::BadCredentials, "bad_credentials"},
        {CommandDispatchReason::MissingArgument, "missing_argument"},
        {CommandDispatchReason::Rejected, "rejected"},
        {CommandDispatchReason::PolicyBlocked, "policy_blocked"},
        {CommandDispatchReason::ControllerNotInitialized, "controller_not_initialized"},
        {CommandDispatchReason::DiscoveryNotActive, "discovery_not_active"},
        {CommandDispatchReason::DiscoveryAlreadyActive, "discovery_already_active"},
        {CommandDispatchReason::NoActiveConnection, "no_active_connection"},
        {CommandDispatchReason::InvalidVerb, "invalid_verb"},
        {CommandDispatchReason::WifiControlDisabled, "wifi_control_disabled"},
        {CommandDispatchReason::Unauthorized, "unauthorized"},
        {CommandDispatchReason::NotFound, "not_found"},
        {CommandDispatchReason::QueueFull, "queue_full"},
    };

    for (size_t i = 0; i < sizeof(expectations) / sizeof(expectations[0]); ++i) {
        TEST_ASSERT_EQUAL_STRING(expectations[i].code,
                                 commandDispatchReasonToSerialCode(expectations[i].reason));
    }

    TEST_ASSERT_EQUAL_STRING("internal",
                             commandDispatchReasonToSerialCode(CommandDispatchReason::Ok));
    TEST_ASSERT_EQUAL_STRING("internal",
                             commandDispatchReasonToSerialCode(CommandDispatchReason::Queued));
}

void testCommandDispatchReasonWebMessages() {
    TEST_ASSERT_EQUAL_STRING("ok", commandDispatchReasonToWebMessage(CommandDispatchReason::Ok));
    TEST_ASSERT_EQUAL_STRING("accepted",
                             commandDispatchReasonToWebMessage(CommandDispatchReason::Queued));
    TEST_ASSERT_EQUAL_STRING("wiimote not connected", commandDispatchReasonToWebMessage(
                                                          CommandDispatchReason::NotConnected));
    TEST_ASSERT_EQUAL_STRING("locked",
                             commandDispatchReasonToWebMessage(CommandDispatchReason::Locked));
    TEST_ASSERT_EQUAL_STRING("unknown command", commandDispatchReasonToWebMessage(
                                                    CommandDispatchReason::UnknownCommand));
    TEST_ASSERT_EQUAL_STRING("invalid argument",
                             commandDispatchReasonToWebMessage(CommandDispatchReason::BadArgument));
    TEST_ASSERT_EQUAL_STRING("bad credentials", commandDispatchReasonToWebMessage(
                                                    CommandDispatchReason::BadCredentials));
    TEST_ASSERT_EQUAL_STRING("missing argument", commandDispatchReasonToWebMessage(
                                                     CommandDispatchReason::MissingArgument));
    TEST_ASSERT_EQUAL_STRING("command rejected",
                             commandDispatchReasonToWebMessage(CommandDispatchReason::Rejected));
    TEST_ASSERT_EQUAL_STRING(
        "policy blocked", commandDispatchReasonToWebMessage(CommandDispatchReason::PolicyBlocked));
    TEST_ASSERT_EQUAL_STRING(
        "bluetooth controller not started",
        commandDispatchReasonToWebMessage(CommandDispatchReason::ControllerNotInitialized));
    TEST_ASSERT_EQUAL_STRING(
        "discovery not active",
        commandDispatchReasonToWebMessage(CommandDispatchReason::DiscoveryNotActive));
    TEST_ASSERT_EQUAL_STRING(
        "discovery already active",
        commandDispatchReasonToWebMessage(CommandDispatchReason::DiscoveryAlreadyActive));
    TEST_ASSERT_EQUAL_STRING(
        "missing active connection handle",
        commandDispatchReasonToWebMessage(CommandDispatchReason::NoActiveConnection));
    TEST_ASSERT_EQUAL_STRING("invalid command verb",
                             commandDispatchReasonToWebMessage(CommandDispatchReason::InvalidVerb));
    TEST_ASSERT_EQUAL_STRING(
        "wifi control is disabled",
        commandDispatchReasonToWebMessage(CommandDispatchReason::WifiControlDisabled));
    TEST_ASSERT_EQUAL_STRING(
        "unauthorized", commandDispatchReasonToWebMessage(CommandDispatchReason::Unauthorized));
    TEST_ASSERT_EQUAL_STRING("not found",
                             commandDispatchReasonToWebMessage(CommandDispatchReason::NotFound));
    TEST_ASSERT_EQUAL_STRING("command queue full",
                             commandDispatchReasonToWebMessage(CommandDispatchReason::QueueFull));
    TEST_ASSERT_EQUAL_STRING(
        "internal error", commandDispatchReasonToWebMessage(CommandDispatchReason::InternalError));
}

void testCommandDispatchReasonOpenApiDescriptionsAndStatuses() {
    TEST_ASSERT_EQUAL_STRING("OK",
                             commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Ok));
    TEST_ASSERT_EQUAL_STRING(
        "Accepted", commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Queued));
    TEST_ASSERT_EQUAL_STRING("Wiimote not connected", commandDispatchReasonToOpenApiDescription(
                                                          CommandDispatchReason::NotConnected));
    TEST_ASSERT_EQUAL_STRING(
        "Forbidden", commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Locked));
    TEST_ASSERT_EQUAL_STRING("Bad request", commandDispatchReasonToOpenApiDescription(
                                                CommandDispatchReason::UnknownCommand));
    TEST_ASSERT_EQUAL_STRING("Bad request", commandDispatchReasonToOpenApiDescription(
                                                CommandDispatchReason::InvalidVerb));
    TEST_ASSERT_EQUAL_STRING("Bad request", commandDispatchReasonToOpenApiDescription(
                                                CommandDispatchReason::BadArgument));
    TEST_ASSERT_EQUAL_STRING("Bad request", commandDispatchReasonToOpenApiDescription(
                                                CommandDispatchReason::MissingArgument));
    TEST_ASSERT_EQUAL_STRING("Unauthorized", commandDispatchReasonToOpenApiDescription(
                                                 CommandDispatchReason::BadCredentials));
    TEST_ASSERT_EQUAL_STRING("Unauthorized", commandDispatchReasonToOpenApiDescription(
                                                 CommandDispatchReason::Unauthorized));
    TEST_ASSERT_EQUAL_STRING("Command rejected", commandDispatchReasonToOpenApiDescription(
                                                     CommandDispatchReason::Rejected));
    TEST_ASSERT_EQUAL_STRING("Policy blocked", commandDispatchReasonToOpenApiDescription(
                                                   CommandDispatchReason::PolicyBlocked));
    TEST_ASSERT_EQUAL_STRING(
        "Bluetooth controller not started",
        commandDispatchReasonToOpenApiDescription(CommandDispatchReason::ControllerNotInitialized));
    TEST_ASSERT_EQUAL_STRING(
        "Discovery not active",
        commandDispatchReasonToOpenApiDescription(CommandDispatchReason::DiscoveryNotActive));
    TEST_ASSERT_EQUAL_STRING(
        "Discovery already active",
        commandDispatchReasonToOpenApiDescription(CommandDispatchReason::DiscoveryAlreadyActive));
    TEST_ASSERT_EQUAL_STRING(
        "Missing active connection handle",
        commandDispatchReasonToOpenApiDescription(CommandDispatchReason::NoActiveConnection));
    TEST_ASSERT_EQUAL_STRING(
        "Wi-Fi control is disabled",
        commandDispatchReasonToOpenApiDescription(CommandDispatchReason::WifiControlDisabled));
    TEST_ASSERT_EQUAL_STRING(
        "Not found", commandDispatchReasonToOpenApiDescription(CommandDispatchReason::NotFound));
    TEST_ASSERT_EQUAL_STRING("Command queue full", commandDispatchReasonToOpenApiDescription(
                                                       CommandDispatchReason::QueueFull));
    TEST_ASSERT_EQUAL_STRING("Internal error", commandDispatchReasonToOpenApiDescription(
                                                   CommandDispatchReason::InternalError));

    TEST_ASSERT_EQUAL(200, commandDispatchReasonToHttpStatus(CommandDispatchReason::Ok));
    TEST_ASSERT_EQUAL(202, commandDispatchReasonToHttpStatus(CommandDispatchReason::Queued));
    TEST_ASSERT_EQUAL(401,
                      commandDispatchReasonToHttpStatus(CommandDispatchReason::BadCredentials));
    TEST_ASSERT_EQUAL(401, commandDispatchReasonToHttpStatus(CommandDispatchReason::Unauthorized));
    TEST_ASSERT_EQUAL(403, commandDispatchReasonToHttpStatus(CommandDispatchReason::Locked));
    TEST_ASSERT_EQUAL(403, commandDispatchReasonToHttpStatus(CommandDispatchReason::PolicyBlocked));
    TEST_ASSERT_EQUAL(400,
                      commandDispatchReasonToHttpStatus(CommandDispatchReason::UnknownCommand));
    TEST_ASSERT_EQUAL(400, commandDispatchReasonToHttpStatus(CommandDispatchReason::InvalidVerb));
    TEST_ASSERT_EQUAL(400, commandDispatchReasonToHttpStatus(CommandDispatchReason::BadArgument));
    TEST_ASSERT_EQUAL(400,
                      commandDispatchReasonToHttpStatus(CommandDispatchReason::MissingArgument));
    TEST_ASSERT_EQUAL(404, commandDispatchReasonToHttpStatus(CommandDispatchReason::NotFound));
    TEST_ASSERT_EQUAL(503, commandDispatchReasonToHttpStatus(CommandDispatchReason::QueueFull));
    TEST_ASSERT_EQUAL(500, commandDispatchReasonToHttpStatus(CommandDispatchReason::InternalError));
    TEST_ASSERT_EQUAL(409, commandDispatchReasonToHttpStatus(CommandDispatchReason::NotConnected));
    TEST_ASSERT_EQUAL(409, commandDispatchReasonToHttpStatus(CommandDispatchReason::Rejected));
    TEST_ASSERT_EQUAL(
        409, commandDispatchReasonToHttpStatus(CommandDispatchReason::ControllerNotInitialized));
    TEST_ASSERT_EQUAL(409,
                      commandDispatchReasonToHttpStatus(CommandDispatchReason::DiscoveryNotActive));
    TEST_ASSERT_EQUAL(
        409, commandDispatchReasonToHttpStatus(CommandDispatchReason::DiscoveryAlreadyActive));
    TEST_ASSERT_EQUAL(409,
                      commandDispatchReasonToHttpStatus(CommandDispatchReason::NoActiveConnection));
    TEST_ASSERT_EQUAL(
        409, commandDispatchReasonToHttpStatus(CommandDispatchReason::WifiControlDisabled));

    TEST_ASSERT_TRUE(commandDispatchReasonIsSuccess(CommandDispatchReason::Ok));
    TEST_ASSERT_TRUE(commandDispatchReasonIsSuccess(CommandDispatchReason::Queued));
    TEST_ASSERT_FALSE(commandDispatchReasonIsSuccess(CommandDispatchReason::Rejected));
}

void testCommandDispatchMessageFormatterCoversTemplates() {
    char out[96] = {0};

    TEST_ASSERT_TRUE(commandDispatchFormatMessage(CommandDispatchMessageTemplate::MissingField,
                                                  "mask", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("missing field: mask", out);

    TEST_ASSERT_TRUE(commandDispatchFormatMessage(CommandDispatchMessageTemplate::InvalidField,
                                                  "mode", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("invalid field: mode", out);

    TEST_ASSERT_TRUE(commandDispatchFormatMessage(CommandDispatchMessageTemplate::EmptyField,
                                                  "token", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("token must not be empty", out);

    TEST_ASSERT_FALSE(commandDispatchFormatMessage(CommandDispatchMessageTemplate::MissingField,
                                                   nullptr, out, sizeof(out)));
    TEST_ASSERT_FALSE(commandDispatchFormatMessage(CommandDispatchMessageTemplate::MissingField,
                                                   "mask", nullptr, sizeof(out)));
    TEST_ASSERT_FALSE(commandDispatchFormatMessage(CommandDispatchMessageTemplate::MissingField,
                                                   "mask", out, 0U));
    TEST_ASSERT_FALSE(commandDispatchFormatMessage(
        CommandDispatchMessageTemplate::MissingRequiredField, "mask", out, sizeof(out)));

    TEST_ASSERT_EQUAL_STRING(
        "missing required field",
        commandDispatchMessageText(CommandDispatchMessageTemplate::MissingRequiredField));
    TEST_ASSERT_EQUAL_STRING(
        "invalid request field",
        commandDispatchMessageText(CommandDispatchMessageTemplate::InvalidRequestField));
    TEST_ASSERT_EQUAL_STRING(
        "field must not be empty",
        commandDispatchMessageText(CommandDispatchMessageTemplate::FieldMustNotBeEmpty));
    TEST_ASSERT_EQUAL_STRING(
        "response buffer too small",
        commandDispatchMessageText(CommandDispatchMessageTemplate::ResponseBufferTooSmall));
    TEST_ASSERT_EQUAL_STRING(
        "invalid command id",
        commandDispatchMessageText(CommandDispatchMessageTemplate::InvalidCommandId));
    TEST_ASSERT_EQUAL_STRING(
        "command not found",
        commandDispatchMessageText(CommandDispatchMessageTemplate::CommandNotFound));
    TEST_ASSERT_EQUAL_STRING(
        "failed to serialize wiimote status",
        commandDispatchMessageText(CommandDispatchMessageTemplate::FailedSerializeWiimoteStatus));
    TEST_ASSERT_EQUAL_STRING(
        "failed to serialize runtime config",
        commandDispatchMessageText(CommandDispatchMessageTemplate::FailedSerializeRuntimeConfig));
    TEST_ASSERT_EQUAL_STRING("failed to serialize wifi control state",
                             commandDispatchMessageText(
                                 CommandDispatchMessageTemplate::FailedSerializeWifiControlState));
    TEST_ASSERT_EQUAL_STRING(
        "missing request body",
        commandDispatchMessageText(CommandDispatchMessageTemplate::MissingRequestBody));
    TEST_ASSERT_EQUAL_STRING(
        "request body too large",
        commandDispatchMessageText(CommandDispatchMessageTemplate::RequestBodyTooLarge));
    TEST_ASSERT_EQUAL_STRING(
        "malformed request body",
        commandDispatchMessageText(CommandDispatchMessageTemplate::MalformedRequestBody));
    TEST_ASSERT_EQUAL_STRING(
        "unknown command verb: expected scan_start or scan_stop",
        commandDispatchMessageText(CommandDispatchMessageTemplate::UnknownScanVerb));
    TEST_ASSERT_EQUAL_STRING(
        "unknown command verb: expected discovery_start or discovery_stop",
        commandDispatchMessageText(CommandDispatchMessageTemplate::UnknownDiscoveryVerb));
    TEST_ASSERT_EQUAL_STRING(
        "invalid field: mode (expected rest or rest-ws)",
        commandDispatchMessageText(CommandDispatchMessageTemplate::InvalidWifiMode));
    TEST_ASSERT_EQUAL_STRING(
        "wifi API token mutation is disabled by policy",
        commandDispatchMessageText(
            CommandDispatchMessageTemplate::WifiApiTokenMutationDisabledByPolicy));
    TEST_ASSERT_EQUAL_STRING(
        "internal error", commandDispatchMessageText(CommandDispatchMessageTemplate::MissingField));
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testCommandDispatchReasonSerialMappings);
    RUN_TEST(testCommandDispatchReasonWebMessages);
    RUN_TEST(testCommandDispatchReasonOpenApiDescriptionsAndStatuses);
    RUN_TEST(testCommandDispatchMessageFormatterCoversTemplates);

    return UNITY_END();
}