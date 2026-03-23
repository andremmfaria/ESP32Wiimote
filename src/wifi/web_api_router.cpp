#include "web_api_router.h"

#include "web_auth.h"
#include "web_request_parser.h"
#include "web_response_serializer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

const char kStaticIndexHtml[] =
    "<!doctype html>\n"
    "<html lang=\"en\">\n"
    "<head><meta charset=\"utf-8\"><meta name=\"viewport\" "
    "content=\"width=device-width,initial-scale=1\">"
    "<title>ESP32 Wiimote Control</title><link rel=\"stylesheet\" href=\"/styles.css\"></head>\n"
    "<body><main class=\"shell\"><h1>ESP32 Wiimote Control</h1><p>Open the control "
    "interface.</p></main>"
    "<script src=\"/app.js\"></script></body></html>\n";

const char kStaticAppJs[] =
    "async function fetchWithAuth(path, init = {}) {\n"
    "  const auth = localStorage.getItem('auth') || 'Bearer esp32wiimote_bearer_token_v1';\n"
    "  const headers = { Authorization: auth, ...(init.headers || {}) };\n"
    "  return fetch(path, { ...init, headers });\n"
    "}\n";

const char kStaticStylesCss[] =
    ":root{--bg:#0f1828;--ink:#e8f0ff;--accent:#34d6a3;}\n"
    "body{margin:0;background:var(--bg);color:var(--ink);font-family:'Trebuchet MS','Avenir "
    "Next','Segoe UI',sans-serif;}\n"
    ".shell{width:min(920px,92vw);margin:24px auto;}\n"
    "h1{color:var(--accent);}\n";

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

WebApiRouteResult makeResult(int status, const char *contentType) {
    WebApiRouteResult result;
    result.httpStatus = status;
    result.contentType = contentType;
    return result;
}

WebApiRouteResult writeStaticResponse(const char *body,
                                      const char *contentType,
                                      char *buf,
                                      size_t size) {
    if (body == nullptr || buf == nullptr || size == 0U) {
        return makeResult(500, "text/plain");
    }

    int written = std::snprintf(buf, size, "%s", body);
    if (written < 0 || static_cast<size_t>(written) >= size) {
        serializeError(buf, size, "response buffer too small");
        return makeResult(500, "application/json");
    }

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
        return writeStaticResponse(kStaticIndexHtml, "text/html", responseBuf, responseBufSize);
    }

    if (std::strcmp(path, "/app.js") == 0) {
        return writeStaticResponse(kStaticAppJs, "application/javascript", responseBuf,
                                   responseBufSize);
    }

    if (std::strcmp(path, "/styles.css") == 0) {
        return writeStaticResponse(kStaticStylesCss, "text/css", responseBuf, responseBufSize);
    }

    return makeResult(0, "application/json");
}

WebApiRouteResult errorResponse(char *buf, size_t size, int status, const char *msg) {
    serializeError(buf, size, msg);
    return makeResult(status, "application/json");
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
    return makeResult(200, "application/json");
}

WebApiRouteResult handleGetConfig(const WebApiContext *ctx,
                                  const WebParsedCommand * /*cmd*/,
                                  char *buf,
                                  size_t size) {
    WebConfigSnapshot cfg = ctx->getConfig(ctx->userData);
    if (serializeConfig(buf, size, cfg) != WebSerializeResult::Ok) {
        return errorResponse(buf, size, 500, "internal error");
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
    return makeResult(200, "application/json");
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
    return makeResult(200, "application/json");
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
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostRequestStatus(const WebApiContext *ctx,
                                          const WebParsedCommand * /*cmd*/,
                                          char *buf,
                                          size_t size) {
    if (!ctx->requestStatus(ctx->userData)) {
        return errorResponse(buf, size, 409, "command rejected");
    }
    serializeOk(buf, size);
    return makeResult(200, "application/json");
}

WebApiRouteResult handlePostScan(const WebApiContext *ctx,
                                 const WebParsedCommand *cmd,
                                 char *buf,
                                 size_t size) {
    if (std::strcmp(cmd->verb, "scan_start") == 0) {
        ctx->setScanEnabled(true, ctx->userData);
        serializeOk(buf, size);
        return makeResult(200, "application/json");
    }
    if (std::strcmp(cmd->verb, "scan_stop") == 0) {
        ctx->setScanEnabled(false, ctx->userData);
        serializeOk(buf, size);
        return makeResult(200, "application/json");
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
        return makeResult(200, "application/json");
    }
    if (std::strcmp(cmd->verb, "discovery_stop") == 0) {
        if (!ctx->stopDiscovery(ctx->userData)) {
            return errorResponse(buf, size, 409, "command rejected");
        }
        serializeOk(buf, size);
        return makeResult(200, "application/json");
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
    return makeResult(200, "application/json");
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
    return makeResult(200, "application/json");
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
        return makeResult(400, "application/json");
    }

    WebApiRouteResult staticResult = tryServeStatic(method, path, responseBuf, responseBufSize);
    if (staticResult.httpStatus != 0) {
        return staticResult;
    }

    // Step 1: Authenticate
    if (webAuthValidate(authHeader) != WebAuthResult::Ok) {
        serializeError(responseBuf, responseBufSize, "unauthorized");
        return makeResult(401, "application/json");
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
                return makeResult(400, "application/json");
            }
            WebParsedCommand cmd;
            WebRequestParseResult pr = webRequestParse(body, bodyLen, &cmd);
            if (pr == WebRequestParseResult::BodyTooLarge) {
                serializeError(responseBuf, responseBufSize, "request body too large");
                return makeResult(400, "application/json");
            }
            if (pr != WebRequestParseResult::Ok) {
                serializeError(responseBuf, responseBufSize, "malformed request body");
                return makeResult(400, "application/json");
            }
            return route.handler(ctx, &cmd, responseBuf, responseBufSize);
        }

        return route.handler(ctx, nullptr, responseBuf, responseBufSize);
    }

    // Not found
    serializeError(responseBuf, responseBufSize, "not found");
    return makeResult(404, "application/json");
}
