#include "web_api_router.h"

#include "../utils/command_dispatch_message_formatter.h"
#include "../utils/command_dispatch_reason.h"
#include "web/web_assets.h"
#include "web_auth.h"
#include "web_event_stream.h"
#include "web_request_parser.h"
#include "web_response_serializer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// ===== Route Handler Type =====

using route_handler_fn = WebApiRouteResult (*)(const WebApiContext *ctx,
                                               const WebParsedCommand *cmd,
                                               char *buf,
                                               size_t bufSize);

enum class RouteAuthScheme : uint8_t {
    BearerOnly = 0,
};

struct RouteResponseEntry {
    int code;
    const char *description;
};

struct RouteEntry {
    const char *method;
    const char *path;
    route_handler_fn handler;
    const char *summary;
    const RouteResponseEntry *responses;
    size_t responseCount;
    bool requiresBody;
    RouteAuthScheme auth;
};

struct OpenApiExtraRoute {
    const char *method;
    const char *path;
    const char *summary;
    RouteAuthScheme auth;
    const RouteResponseEntry *responses;
    size_t responseCount;
};

struct JsonWriter {
    char *buffer;
    size_t size;
    size_t written;

    bool appendFormat(const char *text) {
        if (text == nullptr) {
            return false;
        }

        const size_t kOffset = (written < size) ? written : (size - 1U);
        const size_t kAvailable = size - kOffset;
        const int kResult = std::snprintf(buffer + kOffset, kAvailable, "%s", text);

        if (kResult < 0) {
            buffer[size - 1U] = '\0';
            return false;
        }

        written += static_cast<size_t>(kResult);
        buffer[(written < size) ? written : (size - 1U)] = '\0';
        return static_cast<size_t>(kResult) < kAvailable;
    }

    template <typename... Args>
    bool appendFormat(const char *format, Args... args) {
        if (buffer == nullptr || size == 0U || format == nullptr) {
            return false;
        }

        const size_t kOffset = (written < size) ? written : (size - 1U);
        const size_t kAvailable = size - kOffset;
        const int kResult = std::snprintf(buffer + kOffset, kAvailable, format, args...);

        if (kResult < 0) {
            buffer[size - 1U] = '\0';
            return false;
        }

        written += static_cast<size_t>(kResult);
        buffer[(written < size) ? written : (size - 1U)] = '\0';
        return static_cast<size_t>(kResult) < kAvailable;
    }
};

template <typename T, size_t N>
constexpr size_t arraySize(const T (&kItems)[N]) {
    return N;
}

const RouteResponseEntry kReadResponses[] = {
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Ok),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Ok)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::BadArgument),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::BadArgument)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Unauthorized),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Unauthorized)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::PolicyBlocked),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Locked)},
};

const RouteResponseEntry kCommandResponses[] = {
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Ok),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Ok)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::BadArgument),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::BadArgument)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Unauthorized),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Unauthorized)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::PolicyBlocked),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Locked)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Rejected),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Rejected)},
};

const RouteResponseEntry kWifiStateResponses[] = {
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Ok), "Wi-Fi control lifecycle state"},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Unauthorized),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Unauthorized)},
};

const RouteResponseEntry kWifiCommandResponses[] = {
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Ok),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Ok)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::BadArgument),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::BadArgument)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Unauthorized),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Unauthorized)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Rejected),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Rejected)},
};

const RouteResponseEntry kWifiTokenResponses[] = {
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Ok),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Ok)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::BadArgument),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::BadArgument)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Unauthorized),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Unauthorized)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::PolicyBlocked),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::PolicyBlocked)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Rejected),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Rejected)},
};

const RouteResponseEntry kCommandStatusResponses[] = {
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Queued), "Queued command status"},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::BadArgument),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::BadArgument)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::Unauthorized),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Unauthorized)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::PolicyBlocked),
     commandDispatchReasonToOpenApiDescription(CommandDispatchReason::Locked)},
    {commandDispatchReasonToHttpStatus(CommandDispatchReason::NotFound), "Command not found"},
};

const OpenApiExtraRoute kOpenApiExtraRoutes[] = {
    {"GET", "/api/commands/{id}/status", "Get queued command status", RouteAuthScheme::BearerOnly,
     kCommandStatusResponses, arraySize(kCommandStatusResponses)},
};

const char *openApiMethodName(const char *method) {
    if (std::strcmp(method, "GET") == 0) {
        return "get";
    }
    if (std::strcmp(method, "POST") == 0) {
        return "post";
    }
    return method;
}

void appendOpenApiSecurity(JsonWriter &writer, RouteAuthScheme auth) {
    if (auth == RouteAuthScheme::BearerOnly) {
        writer.appendFormat("\"security\":[{\"bearerAuth\":[]}]");
    }
}

void appendOpenApiResponses(JsonWriter &writer,
                            const RouteResponseEntry *responses,
                            size_t responseCount) {
    writer.appendFormat("\"responses\":{");
    for (size_t i = 0U; i < responseCount; ++i) {
        if (i != 0U) {
            writer.appendFormat(",");
        }
        writer.appendFormat("\"%d\":{\"description\":\"%s\"}", responses[i].code,
                            responses[i].description);
    }
    writer.appendFormat("}");
}

void appendOpenApiOperation(JsonWriter &writer,
                            const char *method,
                            const char *summary,
                            RouteAuthScheme auth,
                            const RouteResponseEntry *responses,
                            size_t responseCount) {
    writer.appendFormat("\"%s\":{", openApiMethodName(method));
    writer.appendFormat("\"summary\":\"%s\",", summary);
    appendOpenApiSecurity(writer, auth);
    writer.appendFormat(",");
    appendOpenApiResponses(writer, responses, responseCount);
    writer.appendFormat("}");
}

void appendOpenApiPaths(JsonWriter &writer,
                        const RouteEntry *routes,
                        size_t routeCount,
                        bool *firstPath) {
    for (size_t i = 0U; i < routeCount; ++i) {
        bool duplicatePath = false;
        for (size_t j = 0U; j < i; ++j) {
            if (std::strcmp(routes[i].path, routes[j].path) == 0) {
                duplicatePath = true;
                break;
            }
        }
        if (duplicatePath) {
            continue;
        }

        if (!*firstPath) {
            writer.appendFormat(",");
        }
        *firstPath = false;
        writer.appendFormat("\"%s\":{", routes[i].path);

        bool firstMethod = true;
        for (size_t j = i; j < routeCount; ++j) {
            if (std::strcmp(routes[i].path, routes[j].path) != 0) {
                continue;
            }
            if (!firstMethod) {
                writer.appendFormat(",");
            }
            firstMethod = false;
            appendOpenApiOperation(writer, routes[j].method, routes[j].summary, routes[j].auth,
                                   routes[j].responses, routes[j].responseCount);
        }

        writer.appendFormat("}");
    }
}

void appendOpenApiPaths(JsonWriter &writer,
                        const OpenApiExtraRoute *routes,
                        size_t routeCount,
                        bool *firstPath) {
    for (size_t i = 0U; i < routeCount; ++i) {
        if (!*firstPath) {
            writer.appendFormat(",");
        }
        *firstPath = false;
        writer.appendFormat("\"%s\":{", routes[i].path);
        appendOpenApiOperation(writer, routes[i].method, routes[i].summary, routes[i].auth,
                               routes[i].responses, routes[i].responseCount);
        writer.appendFormat("}");
    }
}

size_t buildOpenApiJson(char *buf, size_t size);
WebApiRouteResult reasonResponse(char *buf,
                                 size_t size,
                                 CommandDispatchReason reason,
                                 const char *messageOverride = nullptr);

// ===== Internal Helpers =====

const char *findField(const WebParsedCommand &cmd, const char *key) {
    for (size_t i = 0; i < cmd.fieldCount; ++i) {
        if (std::strcmp(cmd.fields[i].key, key) == 0) {
            return cmd.fields[i].value;
        }
    }
    return nullptr;
}

bool parseBool(const char *val) {
    return std::strcmp(val, "true") == 0 || std::strcmp(val, "1") == 0;
}

bool parseU8(const char *val, uint8_t *out) {
    char *end = nullptr;
    unsigned long v = std::strtoul(val, &end, 10);
    if (end == val || *end != '\0' || v > 255UL) {
        return false;
    }
    *out = static_cast<uint8_t>(v);
    return true;
}

WebApiRouteResult makeResult(int status, const char *contentType) {
    WebApiRouteResult result;
    result.httpStatus = status;
    result.contentType = contentType;
    return result;
}

WebApiRouteResult writeStaticResponse(const char *body,
                                      size_t bodyLen,
                                      const char *contentType,
                                      char *buf,
                                      size_t size) {
    if (body == nullptr || buf == nullptr || size == 0U) {
        return makeResult(500, "text/plain");
    }

    if (bodyLen + 1U > size) {
        return reasonResponse(
            buf, size, CommandDispatchReason::InternalError,
            commandDispatchMessageText(CommandDispatchMessageTemplate::ResponseBufferTooSmall));
    }

    std::memcpy(buf, body, bodyLen);
    buf[bodyLen] = '\0';

    return makeResult(200, contentType);
}

WebApiRouteResult tryServeStatic(const char *method,
                                 const char *path,
                                 char *responseBuf,
                                 size_t responseBufSize) {
    if (std::strcmp(method, "GET") != 0) {
        return makeResult(0, "application/json");
    }

    if (std::strcmp(path, "/") == 0 || std::strcmp(path, "/index.html") == 0) {
        return writeStaticResponse(web_assets::kIndexHtml, web_assets::kIndexHtmlLen, "text/html",
                                   responseBuf, responseBufSize);
    }

    if (std::strcmp(path, "/app.js") == 0) {
        return writeStaticResponse(web_assets::kAppJs, web_assets::kAppJsLen,
                                   "application/javascript", responseBuf, responseBufSize);
    }

    if (std::strcmp(path, "/styles.css") == 0) {
        return writeStaticResponse(web_assets::kStylesCss, web_assets::kStylesCssLen, "text/css",
                                   responseBuf, responseBufSize);
    }

    if (std::strcmp(path, "/openapi.json") == 0) {
        buildOpenApiJson(responseBuf, responseBufSize);
        return makeResult(200, "application/json");
    }

    return makeResult(0, "application/json");
}

WebApiRouteResult errorResponse(char *buf, size_t size, int status, const char *msg) {
    serializeError(buf, size, msg);
    return makeResult(status, "application/json");
}

WebApiRouteResult reasonResponse(char *buf,
                                 size_t size,
                                 CommandDispatchReason reason,
                                 const char *messageOverride) {
    const char *kMessage =
        (messageOverride != nullptr) ? messageOverride : commandDispatchReasonToWebMessage(reason);
    return errorResponse(buf, size, commandDispatchReasonToHttpStatus(reason), kMessage);
}

WebApiRouteResult missingFieldResponse(char *buf, size_t size, const char *field) {
    char message[64] = {0};
    if (!commandDispatchFormatMessage(CommandDispatchMessageTemplate::MissingField, field, message,
                                      sizeof(message))) {
        return reasonResponse(
            buf, size, CommandDispatchReason::MissingArgument,
            commandDispatchMessageText(CommandDispatchMessageTemplate::MissingRequiredField));
    }
    return reasonResponse(buf, size, CommandDispatchReason::MissingArgument, message);
}

WebApiRouteResult invalidFieldResponse(char *buf, size_t size, const char *field) {
    char message[64] = {0};
    if (!commandDispatchFormatMessage(CommandDispatchMessageTemplate::InvalidField, field, message,
                                      sizeof(message))) {
        return reasonResponse(
            buf, size, CommandDispatchReason::BadArgument,
            commandDispatchMessageText(CommandDispatchMessageTemplate::InvalidRequestField));
    }
    return reasonResponse(buf, size, CommandDispatchReason::BadArgument, message);
}

WebApiRouteResult emptyFieldResponse(char *buf, size_t size, const char *field) {
    char message[96] = {0};
    if (!commandDispatchFormatMessage(CommandDispatchMessageTemplate::EmptyField, field, message,
                                      sizeof(message))) {
        return reasonResponse(
            buf, size, CommandDispatchReason::BadArgument,
            commandDispatchMessageText(CommandDispatchMessageTemplate::FieldMustNotBeEmpty));
    }
    return reasonResponse(buf, size, CommandDispatchReason::BadArgument, message);
}

WebApiRouteResult internalSerializeFailureResponse(char *buf, size_t size, const char *message) {
    return reasonResponse(buf, size, CommandDispatchReason::InternalError, message);
}

WebApiRouteResult invalidVerbResponse(char *buf, size_t size, const char *message) {
    return reasonResponse(buf, size, CommandDispatchReason::InvalidVerb, message);
}

WebApiRouteResult acceptedResponse(char *buf, size_t size, uint32_t commandId) {
    int written = std::snprintf(buf, size, "{\"status\":\"accepted\",\"commandId\":%lu}",
                                static_cast<unsigned long>(commandId));
    if (written < 0 || static_cast<size_t>(written) >= size) {
        return reasonResponse(
            buf, size, CommandDispatchReason::InternalError,
            commandDispatchMessageText(CommandDispatchMessageTemplate::ResponseBufferTooSmall));
    }
    return makeResult(commandDispatchReasonToHttpStatus(CommandDispatchReason::Queued),
                      "application/json");
}

WebApiRouteResult scanCommandResponse(char *buf,
                                      size_t size,
                                      const char *command,
                                      bool scanEnabled) {
    const int written =
        std::snprintf(buf, size, "{\"status\":\"ok\",\"command\":\"%s\",\"scanEnabled\":%s}",
                      command, scanEnabled ? "true" : "false");
    if (written < 0 || static_cast<size_t>(written) >= size) {
        return reasonResponse(
            buf, size, CommandDispatchReason::InternalError,
            commandDispatchMessageText(CommandDispatchMessageTemplate::ResponseBufferTooSmall));
    }
    return makeResult(200, "application/json");
}

bool enqueueCommandIfConfigured(const WebApiContext *ctx,
                                const char *path,
                                const WebParsedCommand *cmd,
                                char *buf,
                                size_t size,
                                WebApiRouteResult *result) {
    if (ctx->commandQueue == nullptr) {
        return false;
    }

    uint32_t commandId = 0U;
    const char *verb = (cmd != nullptr) ? cmd->verb : "";
    if (!webCommandQueueEnqueue(ctx->commandQueue, path, verb, &commandId)) {
        *result = reasonResponse(buf, size, CommandDispatchReason::QueueFull);
        return true;
    }

    *result = acceptedResponse(buf, size, commandId);
    return true;
}

const char *queueStatusToString(WebCommandQueueStatus status) {
    switch (status) {
        case WebCommandQueueStatus::Queued:
            return "queued";
        case WebCommandQueueStatus::Running:
            return "running";
        case WebCommandQueueStatus::Completed:
            return "completed";
        default:
            return "unknown";
    }
}

const char *queueResultToString(WebCommandQueueResult result) {
    switch (result) {
        case WebCommandQueueResult::Pending:
            return "pending";
        case WebCommandQueueResult::Accepted:
            return "accepted";
        case WebCommandQueueResult::Rejected:
            return "rejected";
        default:
            return "unknown";
    }
}

bool parseCommandStatusPath(const char *path, uint32_t *commandId, bool *matchedRoute) {
    static const char kPrefix[] = "/api/commands/";
    static const char kSuffix[] = "/status";

    if (path == nullptr || commandId == nullptr || matchedRoute == nullptr) {
        return false;
    }

    *matchedRoute = false;

    const size_t kPathLen = std::strlen(path);
    const size_t kPrefixLen = sizeof(kPrefix) - 1U;
    const size_t kSuffixLen = sizeof(kSuffix) - 1U;

    if (kPathLen < (kPrefixLen + kSuffixLen + 1U)) {
        return false;
    }

    if (std::strncmp(path, kPrefix, kPrefixLen) != 0) {
        return false;
    }

    if (std::strcmp(path + (kPathLen - kSuffixLen), kSuffix) != 0) {
        return false;
    }

    *matchedRoute = true;

    const size_t kIdLen = kPathLen - kPrefixLen - kSuffixLen;
    if (kIdLen == 0U || kIdLen > 10U) {
        return false;
    }

    char idBuf[16] = {0};
    std::memcpy(idBuf, path + kPrefixLen, kIdLen);
    idBuf[kIdLen] = '\0';

    for (size_t i = 0U; i < kIdLen; ++i) {
        if (idBuf[i] < '0' || idBuf[i] > '9') {
            return false;
        }
    }

    char *end = nullptr;
    const unsigned long kParsed = std::strtoul(idBuf, &end, 10);
    if (end == idBuf || *end != '\0' || kParsed == 0UL || kParsed > 0xFFFFFFFFUL) {
        return false;
    }

    *commandId = static_cast<uint32_t>(kParsed);
    return true;
}

WebApiRouteResult handleGetCommandStatus(const WebApiContext *ctx,
                                         const char *path,
                                         char *buf,
                                         size_t size) {
    bool matchedRoute = false;
    uint32_t commandId = 0U;
    const bool kParsed = parseCommandStatusPath(path, &commandId, &matchedRoute);
    if (!matchedRoute) {
        return makeResult(0, "application/json");
    }

    if (!kParsed) {
        return reasonResponse(
            buf, size, CommandDispatchReason::BadArgument,
            commandDispatchMessageText(CommandDispatchMessageTemplate::InvalidCommandId));
    }

    if (ctx->commandQueue == nullptr) {
        return reasonResponse(buf, size, CommandDispatchReason::NotFound);
    }

    WebCommandQueueEntry entry = {};
    if (!webCommandQueueGet(ctx->commandQueue, commandId, &entry)) {
        return reasonResponse(
            buf, size, CommandDispatchReason::NotFound,
            commandDispatchMessageText(CommandDispatchMessageTemplate::CommandNotFound));
    }

    const int kWritten =
        std::snprintf(buf, size, "{\"commandId\":%lu,\"status\":\"%s\",\"result\":\"%s\"}",
                      static_cast<unsigned long>(entry.id), queueStatusToString(entry.status),
                      queueResultToString(entry.result));
    if (kWritten < 0 || static_cast<size_t>(kWritten) >= size) {
        return reasonResponse(
            buf, size, CommandDispatchReason::InternalError,
            commandDispatchMessageText(CommandDispatchMessageTemplate::ResponseBufferTooSmall));
    }

    return makeResult(200, "application/json");
}

CommandDispatchReason wiimoteCommandRejectReason(const WebApiContext *ctx) {
    const WebWiimoteStatusSnapshot kWiimoteStatus = ctx->getWiimoteStatus(ctx->userData);
    if (!kWiimoteStatus.connected) {
        return CommandDispatchReason::NotConnected;
    }

    const WebControllerStatusSnapshot kControllerStatus = ctx->getControllerStatus(ctx->userData);
    if (kControllerStatus.activeConnectionHandle == 0U) {
        return CommandDispatchReason::NoActiveConnection;
    }

    return CommandDispatchReason::Rejected;
}

CommandDispatchReason discoveryStartRejectReason(const WebApiContext *ctx) {
    const WebControllerStatusSnapshot kControllerStatus = ctx->getControllerStatus(ctx->userData);
    if (!kControllerStatus.started) {
        return CommandDispatchReason::ControllerNotInitialized;
    }
    if (kControllerStatus.connected) {
        return CommandDispatchReason::Rejected;
    }
    if (kControllerStatus.scanning) {
        return CommandDispatchReason::DiscoveryAlreadyActive;
    }

    return CommandDispatchReason::Rejected;
}

CommandDispatchReason discoveryStopRejectReason(const WebApiContext *ctx) {
    const WebControllerStatusSnapshot kControllerStatus = ctx->getControllerStatus(ctx->userData);
    if (!kControllerStatus.started) {
        return CommandDispatchReason::ControllerNotInitialized;
    }
    if (!kControllerStatus.scanning) {
        return CommandDispatchReason::DiscoveryNotActive;
    }

    return CommandDispatchReason::Rejected;
}

CommandDispatchReason disconnectRejectReason(const WebApiContext *ctx) {
    const WebControllerStatusSnapshot kControllerStatus = ctx->getControllerStatus(ctx->userData);
    if (!kControllerStatus.started) {
        return CommandDispatchReason::ControllerNotInitialized;
    }
    if (!kControllerStatus.connected) {
        return CommandDispatchReason::NotConnected;
    }
    if (kControllerStatus.activeConnectionHandle == 0U) {
        return CommandDispatchReason::NoActiveConnection;
    }

    return CommandDispatchReason::Rejected;
}

// ===== GET Handlers =====

WebApiRouteResult handleGetStatus(const WebApiContext *ctx,
                                  const WebParsedCommand * /*cmd*/,
                                  char *buf,
                                  size_t size) {
    WebWiimoteStatusSnapshot st = ctx->getWiimoteStatus(ctx->userData);
    if (serializeWiimoteStatus(buf, size, st) != WebSerializeResult::Ok) {
        return internalSerializeFailureResponse(
            buf, size,
            commandDispatchMessageText(
                CommandDispatchMessageTemplate::FailedSerializeWiimoteStatus));
    }
    return makeResult(200, "application/json");
}

WebApiRouteResult handleGetConfig(const WebApiContext *ctx,
                                  const WebParsedCommand * /*cmd*/,
                                  char *buf,
                                  size_t size) {
    WebConfigSnapshot cfg = ctx->getConfig(ctx->userData);
    if (serializeConfig(buf, size, cfg) != WebSerializeResult::Ok) {
        return internalSerializeFailureResponse(
            buf, size,
            commandDispatchMessageText(
                CommandDispatchMessageTemplate::FailedSerializeRuntimeConfig));
    }
    return makeResult(200, "application/json");
}

WebApiRouteResult handleGetWifiControl(const WebApiContext *ctx,
                                       const WebParsedCommand * /*cmd*/,
                                       char *buf,
                                       size_t size) {
    const WebWifiControlStateSnapshot kState = ctx->getWifiControlState(ctx->userData);
    if (serializeWifiControlState(buf, size, kState) != WebSerializeResult::Ok) {
        return internalSerializeFailureResponse(
            buf, size,
            commandDispatchMessageText(
                CommandDispatchMessageTemplate::FailedSerializeWifiControlState));
    }
    return makeResult(200, "application/json");
}

// ===== POST Handlers =====

WebApiRouteResult handlePostLeds(const WebApiContext *ctx,
                                 const WebParsedCommand *cmd,
                                 char *buf,
                                 size_t size) {
    const char *maskVal = findField(*cmd, "mask");
    if (maskVal == nullptr) {
        return missingFieldResponse(buf, size, "mask");
    }
    uint8_t mask = 0;
    if (!parseU8(maskVal, &mask)) {
        return invalidFieldResponse(buf, size, "mask");
    }

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/leds", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

    if (!ctx->setLeds(mask, ctx->userData)) {
        return reasonResponse(buf, size, wiimoteCommandRejectReason(ctx));
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostReportingMode(const WebApiContext *ctx,
                                          const WebParsedCommand *cmd,
                                          char *buf,
                                          size_t size) {
    const char *modeVal = findField(*cmd, "mode");
    if (modeVal == nullptr) {
        return missingFieldResponse(buf, size, "mode");
    }
    uint8_t mode = 0;
    if (!parseU8(modeVal, &mode)) {
        return invalidFieldResponse(buf, size, "mode");
    }
    const char *contVal = findField(*cmd, "continuous");
    bool continuous = (contVal != nullptr) && parseBool(contVal);

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/reporting-mode", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

    if (!ctx->setReportingMode(mode, continuous, ctx->userData)) {
        return reasonResponse(buf, size, wiimoteCommandRejectReason(ctx));
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostAccelerometer(const WebApiContext *ctx,
                                          const WebParsedCommand *cmd,
                                          char *buf,
                                          size_t size) {
    const char *enabledVal = findField(*cmd, "enabled");
    if (enabledVal == nullptr) {
        return missingFieldResponse(buf, size, "enabled");
    }
    bool enabled = parseBool(enabledVal);

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/accelerometer", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

    if (!ctx->setAccelEnabled(enabled, ctx->userData)) {
        return reasonResponse(buf, size, wiimoteCommandRejectReason(ctx));
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostRequestStatus(const WebApiContext *ctx,
                                          const WebParsedCommand * /*cmd*/,
                                          char *buf,
                                          size_t size) {
    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/request-status", nullptr, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

    if (!ctx->requestStatus(ctx->userData)) {
        return reasonResponse(buf, size, wiimoteCommandRejectReason(ctx));
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostScan(const WebApiContext *ctx,
                                 const WebParsedCommand *cmd,
                                 char *buf,
                                 size_t size) {
    WebApiRouteResult queueResult;

    if (std::strcmp(cmd->verb, "scan_start") == 0) {
        if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/scan", cmd, buf, size,
                                       &queueResult)) {
            return queueResult;
        }
        ctx->setScanEnabled(true, ctx->userData);
        return scanCommandResponse(buf, size, "scan_start", true);
    }
    if (std::strcmp(cmd->verb, "scan_stop") == 0) {
        if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/scan", cmd, buf, size,
                                       &queueResult)) {
            return queueResult;
        }
        ctx->setScanEnabled(false, ctx->userData);
        return scanCommandResponse(buf, size, "scan_stop", false);
    }
    return invalidVerbResponse(
        buf, size, commandDispatchMessageText(CommandDispatchMessageTemplate::UnknownScanVerb));
}

WebApiRouteResult handlePostDiscovery(const WebApiContext *ctx,
                                      const WebParsedCommand *cmd,
                                      char *buf,
                                      size_t size) {
    WebApiRouteResult queueResult;

    if (std::strcmp(cmd->verb, "discovery_start") == 0) {
        if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/discovery", cmd, buf, size,
                                       &queueResult)) {
            return queueResult;
        }
        if (!ctx->startDiscovery(ctx->userData)) {
            return reasonResponse(buf, size, discoveryStartRejectReason(ctx));
        }
        serializeOk(buf, size);
        return makeResult(200, "application/json");
    }
    if (std::strcmp(cmd->verb, "discovery_stop") == 0) {
        if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/discovery", cmd, buf, size,
                                       &queueResult)) {
            return queueResult;
        }
        if (!ctx->stopDiscovery(ctx->userData)) {
            return reasonResponse(buf, size, discoveryStopRejectReason(ctx));
        }
        serializeOk(buf, size);
        return makeResult(200, "application/json");
    }
    return invalidVerbResponse(
        buf, size,
        commandDispatchMessageText(CommandDispatchMessageTemplate::UnknownDiscoveryVerb));
}

WebApiRouteResult handlePostDisconnect(const WebApiContext *ctx,
                                       const WebParsedCommand *cmd,
                                       char *buf,
                                       size_t size) {
    const char *reasonVal = findField(*cmd, "reason");
    uint8_t reason = 0x16U;  // Default: LocalHostTerminated
    if (reasonVal != nullptr) {
        if (!parseU8(reasonVal, &reason)) {
            return invalidFieldResponse(buf, size, "reason");
        }
    }

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/disconnect", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

    if (!ctx->disconnect(reason, ctx->userData)) {
        return reasonResponse(buf, size, disconnectRejectReason(ctx));
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostReconnectPolicy(const WebApiContext *ctx,
                                            const WebParsedCommand *cmd,
                                            char *buf,
                                            size_t size) {
    const char *enabledVal = findField(*cmd, "enabled");
    if (enabledVal == nullptr) {
        return missingFieldResponse(buf, size, "enabled");
    }
    bool enabled = parseBool(enabledVal);

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/reconnect-policy", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

    ctx->setAutoReconnect(enabled, ctx->userData);
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostWifiControl(const WebApiContext *ctx,
                                        const WebParsedCommand *cmd,
                                        char *buf,
                                        size_t size) {
    const char *enabledVal = findField(*cmd, "enabled");
    if (enabledVal == nullptr) {
        return missingFieldResponse(buf, size, "enabled");
    }
    const bool kEnabled = parseBool(enabledVal);

    if (!ctx->setWifiControlEnabled(kEnabled, ctx->userData)) {
        return reasonResponse(buf, size, CommandDispatchReason::Rejected);
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostWifiMode(const WebApiContext *ctx,
                                     const WebParsedCommand *cmd,
                                     char *buf,
                                     size_t size) {
    const char *modeVal = findField(*cmd, "mode");
    if (modeVal == nullptr) {
        return missingFieldResponse(buf, size, "mode");
    }

    bool restAndWebSocket = false;
    if (std::strcmp(modeVal, "rest") == 0) {
        restAndWebSocket = false;
    } else if (std::strcmp(modeVal, "rest-ws") == 0 ||
               std::strcmp(modeVal, "rest-websocket") == 0) {
        restAndWebSocket = true;
    } else {
        return reasonResponse(
            buf, size, CommandDispatchReason::BadArgument,
            commandDispatchMessageText(CommandDispatchMessageTemplate::InvalidWifiMode));
    }

    if (!ctx->setWifiDeliveryMode(restAndWebSocket, ctx->userData)) {
        return reasonResponse(buf, size, CommandDispatchReason::Rejected);
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostWifiNetwork(const WebApiContext *ctx,
                                        const WebParsedCommand *cmd,
                                        char *buf,
                                        size_t size) {
    const char *ssid = findField(*cmd, "ssid");
    const char *password = findField(*cmd, "password");
    if (ssid == nullptr) {
        return missingFieldResponse(buf, size, "ssid");
    }
    if (password == nullptr) {
        return missingFieldResponse(buf, size, "password");
    }

    if (ssid[0] == '\0') {
        return emptyFieldResponse(buf, size, "ssid");
    }
    if (password[0] == '\0') {
        return emptyFieldResponse(buf, size, "password");
    }

    if (!ctx->setWifiNetwork(ssid, password, ctx->userData)) {
        return reasonResponse(buf, size, CommandDispatchReason::Rejected);
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostWifiRestart(const WebApiContext *ctx,
                                        const WebParsedCommand * /*cmd*/,
                                        char *buf,
                                        size_t size) {
    if (!ctx->restartWifiControl(ctx->userData)) {
        return reasonResponse(buf, size, CommandDispatchReason::WifiControlDisabled);
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostWifiToken(const WebApiContext *ctx,
                                      const WebParsedCommand *cmd,
                                      char *buf,
                                      size_t size) {
    if (!ctx->allowWifiApiTokenMutation) {
        return reasonResponse(
            buf, size, CommandDispatchReason::PolicyBlocked,
            commandDispatchMessageText(
                CommandDispatchMessageTemplate::WifiApiTokenMutationDisabledByPolicy));
    }

    const char *token = findField(*cmd, "token");
    if (token == nullptr) {
        return missingFieldResponse(buf, size, "token");
    }

    if (token[0] == '\0') {
        return emptyFieldResponse(buf, size, "token");
    }

    if (!ctx->setWifiApiToken(token, ctx->userData)) {
        return reasonResponse(buf, size, CommandDispatchReason::Rejected);
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

// ===== Route Table =====

const RouteEntry kRoutes[] = {
    {"GET", "/api/wiimote/status", handleGetStatus, "Get Wiimote status snapshot", kReadResponses,
     arraySize(kReadResponses), false, RouteAuthScheme::BearerOnly},
    {"GET", "/api/wiimote/config", handleGetConfig, "Get runtime configuration snapshot",
     kReadResponses, arraySize(kReadResponses), false, RouteAuthScheme::BearerOnly},
    {"GET", "/api/wifi/control", handleGetWifiControl, "Get Wi-Fi control lifecycle state",
     kWifiStateResponses, arraySize(kWifiStateResponses), false, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wiimote/commands/leds", handlePostLeds, "Set LED mask", kCommandResponses,
     arraySize(kCommandResponses), true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wiimote/commands/reporting-mode", handlePostReportingMode, "Set reporting mode",
     kCommandResponses, arraySize(kCommandResponses), true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wiimote/commands/accelerometer", handlePostAccelerometer,
     "Enable or disable accelerometer parsing", kCommandResponses, arraySize(kCommandResponses),
     true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wiimote/commands/request-status", handlePostRequestStatus,
     "Request status report from controller", kCommandResponses, arraySize(kCommandResponses), true,
     RouteAuthScheme::BearerOnly},
    {"POST", "/api/wiimote/commands/scan", handlePostScan, "Start or stop scan", kCommandResponses,
     arraySize(kCommandResponses), true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wiimote/commands/discovery", handlePostDiscovery, "Start or stop discovery",
     kCommandResponses, arraySize(kCommandResponses), true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wiimote/commands/disconnect", handlePostDisconnect,
     "Disconnect active controller", kCommandResponses, arraySize(kCommandResponses), true,
     RouteAuthScheme::BearerOnly},
    {"POST", "/api/wiimote/commands/reconnect-policy", handlePostReconnectPolicy,
     "Set reconnect policy", kCommandResponses, arraySize(kCommandResponses), true,
     RouteAuthScheme::BearerOnly},
    {"POST", "/api/wifi/control", handlePostWifiControl, "Enable or disable Wi-Fi control",
     kWifiCommandResponses, arraySize(kWifiCommandResponses), true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wifi/delivery-mode", handlePostWifiMode, "Set Wi-Fi delivery mode",
     kWifiCommandResponses, arraySize(kWifiCommandResponses), true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wifi/network", handlePostWifiNetwork, "Update Wi-Fi network credentials",
     kWifiCommandResponses, arraySize(kWifiCommandResponses), true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wifi/restart", handlePostWifiRestart, "Restart Wi-Fi control service",
     kWifiCommandResponses, arraySize(kWifiCommandResponses), true, RouteAuthScheme::BearerOnly},
    {"POST", "/api/wifi/token", handlePostWifiToken, "Update Wi-Fi API token", kWifiTokenResponses,
     arraySize(kWifiTokenResponses), true, RouteAuthScheme::BearerOnly},
};

size_t buildOpenApiJson(char *buf, size_t size) {
    JsonWriter writer{buf, size, 0U};
    writer.appendFormat(
        "{\"openapi\":\"3.0.3\","
        "\"info\":{\"title\":\"ESP32Wiimote Wi-Fi API\","
        "\"version\":\"0.4.0\"},"
        "\"paths\":{");

    bool firstPath = true;
    appendOpenApiPaths(writer, kRoutes, arraySize(kRoutes), &firstPath);
    appendOpenApiPaths(writer, kOpenApiExtraRoutes, arraySize(kOpenApiExtraRoutes), &firstPath);

    writer.appendFormat(
        "},"
        "\"components\":{"
        "\"securitySchemes\":{"
        "\"bearerAuth\":{\"type\":\"http\",\"scheme\":\"bearer\"}"
        "}}}");
    return writer.written;
}

}  // namespace

// ===== Public Function =====

WebApiRouteResult webApiRoute(const WebApiContext *ctx,
                              const char *method,
                              const char *path,
                              const char *authHeader,
                              const char *body,
                              size_t bodyLen,
                              char *responseBuf,
                              size_t responseBufSize) {
    if (ctx == nullptr || method == nullptr || path == nullptr || responseBuf == nullptr ||
        responseBufSize == 0U) {
        return makeResult(400, "application/json");
    }

    WebApiRouteResult staticResult = tryServeStatic(method, path, responseBuf, responseBufSize);
    if (staticResult.httpStatus != 0) {
        return staticResult;
    }

    // Step 1: Authenticate
    if (webAuthValidate(authHeader, ctx->wifiApiToken) != WebAuthResult::Ok) {
        return reasonResponse(responseBuf, responseBufSize, CommandDispatchReason::Unauthorized);
    }

    // Step 2: Find and dispatch route
    if (std::strcmp(method, "GET") == 0) {
        const WebApiRouteResult kCommandStatusResult =
            handleGetCommandStatus(ctx, path, responseBuf, responseBufSize);
        if (kCommandStatusResult.httpStatus != 0) {
            return kCommandStatusResult;
        }
    }

    // WebSocket upgrade paths (GET only, eventStream must be configured)
    if (std::strcmp(method, "GET") == 0 && ctx->eventStream != nullptr) {
        WebEventStreamChannel upgradeChannel = WebEventStreamChannel::Input;
        if (webEventStreamMatchPath(path, &upgradeChannel)) {
            WebApiRouteResult wsResult = makeResult(101, "application/json");
            wsResult.upgradeChannel = upgradeChannel;
            return wsResult;
        }
    }

    for (const auto &route : kRoutes) {
        if (std::strcmp(route.method, method) != 0) {
            continue;
        }
        if (std::strcmp(route.path, path) != 0) {
            continue;
        }

        // Step 3: Parse body for POST routes
        if (route.requiresBody) {
            if (body == nullptr || bodyLen == 0U) {
                return reasonResponse(
                    responseBuf, responseBufSize, CommandDispatchReason::MissingArgument,
                    commandDispatchMessageText(CommandDispatchMessageTemplate::MissingRequestBody));
            }
            WebParsedCommand cmd;
            WebRequestParseResult pr = webRequestParse(body, bodyLen, &cmd);
            if (pr == WebRequestParseResult::BodyTooLarge) {
                return reasonResponse(responseBuf, responseBufSize,
                                      CommandDispatchReason::BadArgument,
                                      commandDispatchMessageText(
                                          CommandDispatchMessageTemplate::RequestBodyTooLarge));
            }
            if (pr != WebRequestParseResult::Ok) {
                return reasonResponse(responseBuf, responseBufSize,
                                      CommandDispatchReason::BadArgument,
                                      commandDispatchMessageText(
                                          CommandDispatchMessageTemplate::MalformedRequestBody));
            }
            return route.handler(ctx, &cmd, responseBuf, responseBufSize);
        }

        return route.handler(ctx, nullptr, responseBuf, responseBufSize);
    }

    // Not found
    return reasonResponse(responseBuf, responseBufSize, CommandDispatchReason::NotFound);
}
