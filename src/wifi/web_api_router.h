#ifndef WEB_API_ROUTER_H
#define WEB_API_ROUTER_H

#include "../wiimote_config.h"
#include "web_response_serializer.h"

#include <cstddef>
#include <stdint.h>

/**
 * Web API Router
 *
 * Enforces authentication, routes HTTP requests to command handlers, and
 * writes bounded JSON responses. No dynamic allocation.
 *
 * Callers supply a WebApiContext with function pointers for each operation
 * so the router can be unit-tested without any ESP32/Arduino dependency.
 */

// ===== API Context =====

/**
 * Callback context.
 * All callbacks default to nullptr; populate every member before calling webApiRoute.
 */
struct WebApiContext {
    const WiimoteCredentials *credentials = nullptr;

    // Read callbacks
    WebWiimoteStatusSnapshot (*getWiimoteStatus)(void *userData) = nullptr;
    WebConfigSnapshot (*getConfig)(void *userData) = nullptr;

    // Command callbacks — return true on acceptance, false if driver rejects
    bool (*setLeds)(uint8_t ledMask, void *userData) = nullptr;
    bool (*setReportingMode)(uint8_t mode, bool continuous, void *userData) = nullptr;
    bool (*setAccelEnabled)(bool enabled, void *userData) = nullptr;
    bool (*requestStatus)(void *userData) = nullptr;
    void (*setScanEnabled)(bool enabled, void *userData) = nullptr;  // always succeeds
    bool (*startDiscovery)(void *userData) = nullptr;
    bool (*stopDiscovery)(void *userData) = nullptr;
    bool (*disconnect)(uint8_t reason, void *userData) = nullptr;
    void (*setAutoReconnect)(bool enabled, void *userData) = nullptr;  // always succeeds

    void *userData = nullptr;
};

// ===== Route Result =====

struct WebApiRouteResult {
    int httpStatus{200};
    const char *contentType{"application/json"};
};

// ===== Router Interface =====

/**
 * Route an HTTP request through the Wiimote REST API.
 *
 * Enforces authentication first (401 on failure).
 * For POST routes, parses the JSON body (400 on malformed/missing body).
 * Returns 409 when a command is rejected by the underlying driver.
 * Returns 404 for unknown paths.
 *
 * @param ctx             Populated context (must not be null)
 * @param method          HTTP method string ("GET" or "POST")
 * @param path            Request path (e.g., "/api/wiimote/status")
 * @param authHeader      Authorization header value (null if absent)
 * @param body            Request body bytes (null for GET)
 * @param bodyLen         Body length in bytes
 * @param responseBuf     Output buffer for JSON response
 * @param responseBufSize Size of responseBuf
 * @return WebApiRouteResult with HTTP status code
 */
WebApiRouteResult webApiRoute(const WebApiContext *ctx,
                              const char *method,
                              const char *path,
                              const char *authHeader,
                              const char *body,
                              size_t bodyLen,
                              char *responseBuf,
                              size_t responseBufSize);

#endif  // WEB_API_ROUTER_H
