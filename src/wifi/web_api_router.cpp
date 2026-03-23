#include "web_api_router.h"

#include "web_auth.h"
#include "web_request_parser.h"
#include "web_response_serializer.h"

#include <cstdlib>
#include <cstring>

namespace {

// ===== Route Handler Type =====

using route_handler_fn = WebApiRouteResult (*)(const WebApiContext *ctx,
                                               const WebParsedCommand *cmd,
                                               char *buf,
                                               size_t bufSize);

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

WebApiRouteResult errorResponse(char *buf, size_t size, int status, const char *msg) {
    WebApiRouteResult result;
    result.httpStatus = status;
    serializeError(buf, size, msg);
    return result;
}

// ===== GET Handlers =====

WebApiRouteResult handleGetStatus(const WebApiContext *ctx,
                                  const WebParsedCommand * /*cmd*/,
                                  char *buf,
                                  size_t size) {
    WebWiimoteStatusSnapshot st = ctx->getWiimoteStatus(ctx->userData);
    if (serializeWiimoteStatus(buf, size, st) != WebSerializeResult::Ok) {
        return errorResponse(buf, size, 500, "internal error");
    }
    WebApiRouteResult result;
    result.httpStatus = 200;
    return result;
}

WebApiRouteResult handleGetConfig(const WebApiContext *ctx,
                                  const WebParsedCommand * /*cmd*/,
                                  char *buf,
                                  size_t size) {
    WebConfigSnapshot cfg = ctx->getConfig(ctx->userData);
    if (serializeConfig(buf, size, cfg) != WebSerializeResult::Ok) {
        return errorResponse(buf, size, 500, "internal error");
    }
    WebApiRouteResult result;
    result.httpStatus = 200;
    return result;
}

// ===== POST Handlers =====

WebApiRouteResult handlePostLeds(const WebApiContext *ctx,
                                 const WebParsedCommand *cmd,
                                 char *buf,
                                 size_t size) {
    const char *maskVal = findField(*cmd, "mask");
    if (maskVal == nullptr) {
        return errorResponse(buf, size, 400, "missing field: mask");
    }
    uint8_t mask = 0;
    if (!parseU8(maskVal, &mask)) {
        return errorResponse(buf, size, 400, "invalid field: mask");
    }
    if (!ctx->setLeds(mask, ctx->userData)) {
        return errorResponse(buf, size, 409, "command rejected");
    }
    serializeOk(buf, size);
    WebApiRouteResult result;
    result.httpStatus = 200;
    return result;
}

WebApiRouteResult handlePostReportingMode(const WebApiContext *ctx,
                                          const WebParsedCommand *cmd,
                                          char *buf,
                                          size_t size) {
    const char *modeVal = findField(*cmd, "mode");
    if (modeVal == nullptr) {
        return errorResponse(buf, size, 400, "missing field: mode");
    }
    uint8_t mode = 0;
    if (!parseU8(modeVal, &mode)) {
        return errorResponse(buf, size, 400, "invalid field: mode");
    }
    const char *contVal = findField(*cmd, "continuous");
    bool continuous = (contVal != nullptr) && parseBool(contVal);
    if (!ctx->setReportingMode(mode, continuous, ctx->userData)) {
        return errorResponse(buf, size, 409, "command rejected");
    }
    serializeOk(buf, size);
    WebApiRouteResult result;
    result.httpStatus = 200;
    return result;
}

WebApiRouteResult handlePostAccelerometer(const WebApiContext *ctx,
                                          const WebParsedCommand *cmd,
                                          char *buf,
                                          size_t size) {
    const char *enabledVal = findField(*cmd, "enabled");
    if (enabledVal == nullptr) {
        return errorResponse(buf, size, 400, "missing field: enabled");
    }
    bool enabled = parseBool(enabledVal);
    if (!ctx->setAccelEnabled(enabled, ctx->userData)) {
        return errorResponse(buf, size, 409, "command rejected");
    }
    serializeOk(buf, size);
    WebApiRouteResult result;
    result.httpStatus = 200;
    return result;
}

WebApiRouteResult handlePostRequestStatus(const WebApiContext *ctx,
                                          const WebParsedCommand * /*cmd*/,
                                          char *buf,
                                          size_t size) {
    if (!ctx->requestStatus(ctx->userData)) {
        return errorResponse(buf, size, 409, "command rejected");
    }
    serializeOk(buf, size);
    WebApiRouteResult result;
    result.httpStatus = 200;
    return result;
}

WebApiRouteResult handlePostScan(const WebApiContext *ctx,
                                 const WebParsedCommand *cmd,
                                 char *buf,
                                 size_t size) {
    if (std::strcmp(cmd->verb, "scan_start") == 0) {
        ctx->setScanEnabled(true, ctx->userData);
        serializeOk(buf, size);
        WebApiRouteResult result;
        result.httpStatus = 200;
        return result;
    }
    if (std::strcmp(cmd->verb, "scan_stop") == 0) {
        ctx->setScanEnabled(false, ctx->userData);
        serializeOk(buf, size);
        WebApiRouteResult result;
        result.httpStatus = 200;
        return result;
    }
    return errorResponse(buf, size, 400, "unknown command verb");
}

WebApiRouteResult handlePostDiscovery(const WebApiContext *ctx,
                                      const WebParsedCommand *cmd,
                                      char *buf,
                                      size_t size) {
    if (std::strcmp(cmd->verb, "discovery_start") == 0) {
        if (!ctx->startDiscovery(ctx->userData)) {
            return errorResponse(buf, size, 409, "command rejected");
        }
        serializeOk(buf, size);
        WebApiRouteResult result;
        result.httpStatus = 200;
        return result;
    }
    if (std::strcmp(cmd->verb, "discovery_stop") == 0) {
        if (!ctx->stopDiscovery(ctx->userData)) {
            return errorResponse(buf, size, 409, "command rejected");
        }
        serializeOk(buf, size);
        WebApiRouteResult result;
        result.httpStatus = 200;
        return result;
    }
    return errorResponse(buf, size, 400, "unknown command verb");
}

WebApiRouteResult handlePostDisconnect(const WebApiContext *ctx,
                                       const WebParsedCommand *cmd,
                                       char *buf,
                                       size_t size) {
    const char *reasonVal = findField(*cmd, "reason");
    uint8_t reason = 0x16U;  // Default: LocalHostTerminated
    if (reasonVal != nullptr) {
        if (!parseU8(reasonVal, &reason)) {
            return errorResponse(buf, size, 400, "invalid field: reason");
        }
    }
    if (!ctx->disconnect(reason, ctx->userData)) {
        return errorResponse(buf, size, 409, "command rejected");
    }
    serializeOk(buf, size);
    WebApiRouteResult result;
    result.httpStatus = 200;
    return result;
}

WebApiRouteResult handlePostReconnectPolicy(const WebApiContext *ctx,
                                            const WebParsedCommand *cmd,
                                            char *buf,
                                            size_t size) {
    const char *enabledVal = findField(*cmd, "enabled");
    if (enabledVal == nullptr) {
        return errorResponse(buf, size, 400, "missing field: enabled");
    }
    bool enabled = parseBool(enabledVal);
    ctx->setAutoReconnect(enabled, ctx->userData);
    serializeOk(buf, size);
    WebApiRouteResult result;
    result.httpStatus = 200;
    return result;
}

// ===== Route Table =====

struct RouteEntry {
    const char *method;
    const char *path;
    route_handler_fn handler;
    bool requiresBody;

    RouteEntry(const char *m, const char *p, route_handler_fn h, bool rb)
        : method(m), path(p), handler(h), requiresBody(rb) {}
};

const RouteEntry kRoutes[] = {
    {"GET", "/api/wiimote/status", handleGetStatus, false},
    {"GET", "/api/wiimote/config", handleGetConfig, false},
    {"POST", "/api/wiimote/commands/leds", handlePostLeds, true},
    {"POST", "/api/wiimote/commands/reporting-mode", handlePostReportingMode, true},
    {"POST", "/api/wiimote/commands/accelerometer", handlePostAccelerometer, true},
    {"POST", "/api/wiimote/commands/request-status", handlePostRequestStatus, true},
    {"POST", "/api/wiimote/commands/scan", handlePostScan, true},
    {"POST", "/api/wiimote/commands/discovery", handlePostDiscovery, true},
    {"POST", "/api/wiimote/commands/disconnect", handlePostDisconnect, true},
    {"POST", "/api/wiimote/commands/reconnect-policy", handlePostReconnectPolicy, true},
};

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
        WebApiRouteResult result;
        result.httpStatus = 400;
        return result;
    }

    // Step 1: Authenticate
    if (webAuthValidate(authHeader) != WebAuthResult::Ok) {
        serializeError(responseBuf, responseBufSize, "unauthorized");
        WebApiRouteResult result;
        result.httpStatus = 401;
        return result;
    }

    // Step 2: Find and dispatch route
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
                serializeError(responseBuf, responseBufSize, "missing request body");
                WebApiRouteResult result;
                result.httpStatus = 400;
                return result;
            }
            WebParsedCommand cmd;
            WebRequestParseResult pr = webRequestParse(body, bodyLen, &cmd);
            if (pr == WebRequestParseResult::BodyTooLarge) {
                serializeError(responseBuf, responseBufSize, "request body too large");
                WebApiRouteResult result;
                result.httpStatus = 400;
                return result;
            }
            if (pr != WebRequestParseResult::Ok) {
                serializeError(responseBuf, responseBufSize, "malformed request body");
                WebApiRouteResult result;
                result.httpStatus = 400;
                return result;
            }
            return route.handler(ctx, &cmd, responseBuf, responseBufSize);
        }

        return route.handler(ctx, nullptr, responseBuf, responseBufSize);
    }

    // Not found
    serializeError(responseBuf, responseBufSize, "not found");
    WebApiRouteResult result;
    result.httpStatus = 404;
    return result;
}
