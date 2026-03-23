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

const char kStaticOpenApiJson[] =
    "{"
    "\"openapi\":\"3.0.3\","
    "\"info\":{"
    "\"title\":\"ESP32Wiimote Wi-Fi API\","
    "\"version\":\"0.4.0\""
    "},"
    "\"paths\":{"
    "\"/api/wiimote/status\":{\"get\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"Status snapshot\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"}"
    "}}},"
    "\"/api/wiimote/config\":{\"get\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"Config snapshot\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"}"
    "}}},"
    "\"/api/wiimote/commands/leds\":{\"post\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"OK\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"},"
    "\"409\":{\"description\":\"Command rejected\"}"
    "}}},"
    "\"/api/wiimote/commands/reporting-mode\":{\"post\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"OK\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"},"
    "\"409\":{\"description\":\"Command rejected\"}"
    "}}},"
    "\"/api/wiimote/commands/accelerometer\":{\"post\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"OK\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"},"
    "\"409\":{\"description\":\"Command rejected\"}"
    "}}},"
    "\"/api/wiimote/commands/request-status\":{\"post\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"OK\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"},"
    "\"409\":{\"description\":\"Command rejected\"}"
    "}}},"
    "\"/api/wiimote/commands/scan\":{\"post\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"OK\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"},"
    "\"409\":{\"description\":\"Command rejected\"}"
    "}}},"
    "\"/api/wiimote/commands/discovery\":{\"post\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"OK\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"},"
    "\"409\":{\"description\":\"Command rejected\"}"
    "}}},"
    "\"/api/wiimote/commands/disconnect\":{\"post\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"OK\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"},"
    "\"409\":{\"description\":\"Command rejected\"}"
    "}}},"
    "\"/api/wiimote/commands/reconnect-policy\":{\"post\":{"
    "\"security\":[{\"bearerAuth\":[]},{\"basicAuth\":[]}],"
    "\"responses\":{"
    "\"200\":{\"description\":\"OK\"},"
    "\"400\":{\"description\":\"Bad request\"},"
    "\"401\":{\"description\":\"Unauthorized\"},"
    "\"403\":{\"description\":\"Forbidden\"},"
    "\"409\":{\"description\":\"Command rejected\"}"
    "}}}"
    "},"
    "\"components\":{"
    "\"securitySchemes\":{"
    "\"bearerAuth\":{\"type\":\"http\",\"scheme\":\"bearer\"},"
    "\"basicAuth\":{\"type\":\"http\",\"scheme\":\"basic\"}"
    "}"
    "}"
    "}";

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

    if (std::strcmp(path, "/openapi.json") == 0) {
        return writeStaticResponse(kStaticOpenApiJson, "application/json", responseBuf,
                                   responseBufSize);
    }

    return makeResult(0, "application/json");
}

WebApiRouteResult errorResponse(char *buf, size_t size, int status, const char *msg) {
    serializeError(buf, size, msg);
    return makeResult(status, "application/json");
}

WebApiRouteResult acceptedResponse(char *buf, size_t size, uint32_t commandId) {
    int written = std::snprintf(buf, size, "{\"status\":\"accepted\",\"commandId\":%lu}",
                                static_cast<unsigned long>(commandId));
    if (written < 0 || static_cast<size_t>(written) >= size) {
        return errorResponse(buf, size, 500, "response buffer too small");
    }
    return makeResult(202, "application/json");
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
        *result = errorResponse(buf, size, 503, "command queue full");
        return true;
    }

    *result = acceptedResponse(buf, size, commandId);
    return true;
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

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/leds", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
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

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/reporting-mode", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

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

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/accelerometer", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

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
    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/request-status", nullptr, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

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
    WebApiRouteResult queueResult;

    if (std::strcmp(cmd->verb, "scan_start") == 0) {
        if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/scan", cmd, buf, size,
                                       &queueResult)) {
            return queueResult;
        }
        ctx->setScanEnabled(true, ctx->userData);
        serializeOk(buf, size);
        return makeResult(200, "application/json");
    }
    if (std::strcmp(cmd->verb, "scan_stop") == 0) {
        if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/scan", cmd, buf, size,
                                       &queueResult)) {
            return queueResult;
        }
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
    WebApiRouteResult queueResult;

    if (std::strcmp(cmd->verb, "discovery_start") == 0) {
        if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/discovery", cmd, buf, size,
                                       &queueResult)) {
            return queueResult;
        }
        if (!ctx->startDiscovery(ctx->userData)) {
            return errorResponse(buf, size, 409, "command rejected");
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

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/disconnect", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
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

    WebApiRouteResult queueResult;
    if (enqueueCommandIfConfigured(ctx, "/api/wiimote/commands/reconnect-policy", cmd, buf, size,
                                   &queueResult)) {
        return queueResult;
    }

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
    if (webAuthValidate(authHeader, ctx->credentials) != WebAuthResult::Ok) {
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
