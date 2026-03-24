#include "../../../src/wifi/web_api_router.h"
#include "../../../src/wifi/web_command_queue.h"

#include <cstring>
#include <unity.h>

// ===== Valid Auth Token =====

static const char *kValidBearer = "Bearer esp32wiimote_bearer_token_v1";
static const char *kTestWifiToken = "esp32wiimote_bearer_token_v1";

// ===== Mock State =====

static WebWiimoteStatusSnapshot gMockStatus;
static WebControllerStatusSnapshot gMockControllerStatus;
static WebConfigSnapshot gMockConfig;

static bool gSetLedsResult;
static uint8_t gLastLedsMask;

static bool gSetReportingModeResult;
static uint8_t gLastReportingMode;
static bool gLastReportingContinuous;

static bool gSetAccelResult;
static bool gLastAccelEnabled;

static bool gRequestStatusResult;

static bool gLastScanEnabled;

static bool gStartDiscoveryResult;
static bool gStopDiscoveryResult;

static bool gDisconnectResult;
static uint8_t gLastDisconnectReason;

static bool gLastAutoReconnect;
static WebWifiControlStateSnapshot gMockWifiControlState;
static bool gSetWifiControlResult;
static bool gLastWifiControlEnabled;
static bool gSetWifiModeResult;
static bool gLastWifiModeRestAndWebSocket;
static bool gSetWifiNetworkResult;
static const char *gLastWifiSsid;
static const char *gLastWifiPassword;
static bool gRestartWifiResult;
static bool gSetWifiTokenResult;
static const char *gLastWifiToken;
static WebCommandQueue gCommandQueue;

static char gBuf[8192];

// ===== Mock Callbacks =====

static WebWiimoteStatusSnapshot mockGetStatus(void * /*userData*/) {
    return gMockStatus;
}
static WebControllerStatusSnapshot mockGetControllerStatus(void * /*userData*/) {
    return gMockControllerStatus;
}
static WebConfigSnapshot mockGetConfig(void * /*userData*/) {
    return gMockConfig;
}
static bool mockSetLeds(uint8_t mask, void * /*userData*/) {
    gLastLedsMask = mask;
    return gSetLedsResult;
}
static bool mockSetReportingMode(uint8_t mode, bool cont, void * /*userData*/) {
    gLastReportingMode = mode;
    gLastReportingContinuous = cont;
    return gSetReportingModeResult;
}
static bool mockSetAccelEnabled(bool enabled, void * /*userData*/) {
    gLastAccelEnabled = enabled;
    return gSetAccelResult;
}
static bool mockRequestStatus(void * /*userData*/) {
    return gRequestStatusResult;
}
static void mockSetScanEnabled(bool enabled, void * /*userData*/) {
    gLastScanEnabled = enabled;
}
static bool mockStartDiscovery(void * /*userData*/) {
    return gStartDiscoveryResult;
}
static bool mockStopDiscovery(void * /*userData*/) {
    return gStopDiscoveryResult;
}
static bool mockDisconnect(uint8_t reason, void * /*userData*/) {
    gLastDisconnectReason = reason;
    return gDisconnectResult;
}
static void mockSetAutoReconnect(bool enabled, void * /*userData*/) {
    gLastAutoReconnect = enabled;
}
static WebWifiControlStateSnapshot mockGetWifiControlState(void * /*userData*/) {
    return gMockWifiControlState;
}
static bool mockSetWifiControl(bool enabled, void * /*userData*/) {
    gLastWifiControlEnabled = enabled;
    return gSetWifiControlResult;
}
static bool mockSetWifiMode(bool restAndWebSocket, void * /*userData*/) {
    gLastWifiModeRestAndWebSocket = restAndWebSocket;
    return gSetWifiModeResult;
}
static bool mockSetWifiNetwork(const char *ssid, const char *password, void * /*userData*/) {
    gLastWifiSsid = ssid;
    gLastWifiPassword = password;
    return gSetWifiNetworkResult;
}
static bool mockRestartWifi(void * /*userData*/) {
    return gRestartWifiResult;
}
static bool mockSetWifiToken(const char *token, void * /*userData*/) {
    gLastWifiToken = token;
    return gSetWifiTokenResult;
}

// ===== Context Factory =====

static WebApiContext makeCtx() {
    WebApiContext ctx;
    ctx.wifiApiToken = kTestWifiToken;
    ctx.getWiimoteStatus = mockGetStatus;
    ctx.getControllerStatus = mockGetControllerStatus;
    ctx.getConfig = mockGetConfig;
    ctx.getWifiControlState = mockGetWifiControlState;
    ctx.setLeds = mockSetLeds;
    ctx.setReportingMode = mockSetReportingMode;
    ctx.setAccelEnabled = mockSetAccelEnabled;
    ctx.requestStatus = mockRequestStatus;
    ctx.setScanEnabled = mockSetScanEnabled;
    ctx.startDiscovery = mockStartDiscovery;
    ctx.stopDiscovery = mockStopDiscovery;
    ctx.disconnect = mockDisconnect;
    ctx.setAutoReconnect = mockSetAutoReconnect;
    ctx.setWifiControlEnabled = mockSetWifiControl;
    ctx.setWifiDeliveryMode = mockSetWifiMode;
    ctx.setWifiNetwork = mockSetWifiNetwork;
    ctx.restartWifiControl = mockRestartWifi;
    ctx.setWifiApiToken = mockSetWifiToken;
    ctx.allowWifiApiTokenMutation = false;
    ctx.userData = nullptr;
    return ctx;
}

// ===== Call Helper =====

static WebApiRouteResult callRoute(const WebApiContext *ctx,
                                   const char *method,
                                   const char *path,
                                   const char *auth,
                                   const char *body) {
    size_t bodyLen = (body != nullptr) ? std::strlen(body) : 0U;
    return webApiRoute(ctx, method, path, auth, body, bodyLen, gBuf, sizeof(gBuf));
}

// ===== Unity Hooks =====

void setUp() {
    gMockStatus = {};
    gMockControllerStatus = {};
    gMockConfig = {};
    gSetLedsResult = true;
    gLastLedsMask = 0;
    gSetReportingModeResult = true;
    gLastReportingMode = 0;
    gLastReportingContinuous = false;
    gSetAccelResult = true;
    gLastAccelEnabled = false;
    gRequestStatusResult = true;
    gLastScanEnabled = false;
    gStartDiscoveryResult = true;
    gStopDiscoveryResult = true;
    gDisconnectResult = true;
    gLastDisconnectReason = 0;
    gLastAutoReconnect = false;
    gMockWifiControlState = {};
    gSetWifiControlResult = true;
    gLastWifiControlEnabled = false;
    gSetWifiModeResult = true;
    gLastWifiModeRestAndWebSocket = false;
    gSetWifiNetworkResult = true;
    gLastWifiSsid = nullptr;
    gLastWifiPassword = nullptr;
    gRestartWifiResult = true;
    gSetWifiTokenResult = true;
    gLastWifiToken = nullptr;
    webCommandQueueInit(&gCommandQueue);
    std::memset(gBuf, 0, sizeof(gBuf));
}

void tearDown() {}

// ===== Auth Tests =====

void testNullCtxReturns400() {
    WebApiRouteResult r = webApiRoute(nullptr, "GET", "/api/wiimote/status", kValidBearer, nullptr,
                                      0U, gBuf, sizeof(gBuf));
    TEST_ASSERT_EQUAL(400, r.httpStatus);
}

void testMissingAuthReturns401() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/api/wiimote/status", nullptr, nullptr);
    TEST_ASSERT_EQUAL(401, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "unauthorized"));
}

void testInvalidAuthReturns401() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r =
        callRoute(&ctx, "GET", "/api/wiimote/status", "Bearer wrong_token", nullptr);
    TEST_ASSERT_EQUAL(401, r.httpStatus);
}

void testValidBearerAuthPasses() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/api/wiimote/status", kValidBearer, nullptr);
    TEST_ASSERT_NOT_EQUAL(401, r.httpStatus);
}

void testUnsupportedAuthSchemeReturns401() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/api/wiimote/status", "Digest abc123", nullptr);
    TEST_ASSERT_EQUAL(401, r.httpStatus);
}

void testNullCredentialsInCtxReturns401WithValidToken() {
    WebApiContext ctx = makeCtx();
    ctx.wifiApiToken = nullptr;
    WebApiRouteResult r = callRoute(&ctx, "GET", "/api/wiimote/status", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(401, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "unauthorized"));
}

// ===== Routing Tests =====

void testUnknownPathReturns404() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/api/wiimote/unknown", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(404, r.httpStatus);
}

void testMethodMismatchReturns404() {
    WebApiContext ctx = makeCtx();
    // GET on a POST-only route
    WebApiRouteResult r =
        callRoute(&ctx, "GET", "/api/wiimote/commands/leds", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(404, r.httpStatus);
}

void testStaticIndexRouteReturns200WithoutAuth() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/", nullptr, nullptr);
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_STRING("text/html", r.contentType);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "ESP32 Wiimote Control"));
}

void testStaticAppJsRouteReturns200WithoutAuth() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/app.js", nullptr, nullptr);
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_STRING("application/javascript", r.contentType);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "fetchWithAuth"));
}

void testStaticStylesRouteReturns200WithoutAuth() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/styles.css", nullptr, nullptr);
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_STRING("text/css", r.contentType);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, ":root"));
}

void testOpenApiRouteReturns200WithoutAuth() {
    WebApiContext ctx = makeCtx();
    static char kOpenApiBuf[8192];
    std::memset(kOpenApiBuf, 0, sizeof(kOpenApiBuf));
    WebApiRouteResult r = webApiRoute(&ctx, "GET", "/openapi.json", nullptr, nullptr, 0U,
                                      kOpenApiBuf, sizeof(kOpenApiBuf));
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_STRING("application/json", r.contentType);
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"openapi\":\"3.0.3\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"bearerAuth\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/control\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/delivery-mode\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/network\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/restart\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/token\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/commands/{id}/status\""));
}

void testOpenApiRouteContainsAllRegisteredPaths() {
    WebApiContext ctx = makeCtx();
    static char kOpenApiBuf[8192];
    std::memset(kOpenApiBuf, 0, sizeof(kOpenApiBuf));

    WebApiRouteResult r = webApiRoute(&ctx, "GET", "/openapi.json", nullptr, nullptr, 0U,
                                      kOpenApiBuf, sizeof(kOpenApiBuf));
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/status\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/config\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/commands/leds\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/commands/reporting-mode\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/commands/accelerometer\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/commands/request-status\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/commands/scan\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/commands/discovery\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/commands/disconnect\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wiimote/commands/reconnect-policy\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/control\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/delivery-mode\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/network\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/restart\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/wifi/token\""));
    TEST_ASSERT_NOT_NULL(std::strstr(kOpenApiBuf, "\"/api/commands/{id}/status\""));
}

void testOpenApiRouteTruncatesSafelyInSmallBuffer() {
    WebApiContext ctx = makeCtx();
    char smallBuf[32] = {0};

    WebApiRouteResult r =
        webApiRoute(&ctx, "GET", "/openapi.json", nullptr, nullptr, 0U, smallBuf, sizeof(smallBuf));
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_STRING("application/json", r.contentType);
    TEST_ASSERT_EQUAL(0, smallBuf[sizeof(smallBuf) - 1U]);
    TEST_ASSERT_NOT_EQUAL('\0', smallBuf[0]);
}

// ===== GET /api/wiimote/status =====

void testGetStatusReturns200() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/api/wiimote/status", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_STRING("application/json", r.contentType);
}

void testGetStatusJsonShape() {
    gMockStatus.connected = true;
    gMockStatus.batteryLevel = 75;
    WebApiContext ctx = makeCtx();
    callRoute(&ctx, "GET", "/api/wiimote/status", kValidBearer, nullptr);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"connected\":true"));
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"batteryLevel\":75"));
}

// ===== GET /api/wiimote/config =====

void testGetConfigReturns200() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/api/wiimote/config", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(200, r.httpStatus);
}

void testGetConfigJsonShape() {
    gMockConfig.nunchukStickThreshold = 3;
    gMockConfig.txQueueSize = 16;
    gMockConfig.rxQueueSize = 16;
    gMockConfig.fastReconnectTtlMs = 60000UL;
    WebApiContext ctx = makeCtx();
    callRoute(&ctx, "GET", "/api/wiimote/config", kValidBearer, nullptr);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"nunchukStickThreshold\":3"));
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"txQueueSize\":16"));
}

// ===== POST /api/wiimote/commands/leds =====

void testPostLedsSuccess() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                    "{\"command\":\"set_leds\",\"mask\":\"15\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_UINT8(15, gLastLedsMask);
}

void testPostLedsMissingMaskReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                    "{\"command\":\"set_leds\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "missing field"));
}

void testPostLedsInvalidMaskReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                    "{\"command\":\"set_leds\",\"mask\":\"300\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "invalid field"));
}

void testPostLedsRejectedReturns409() {
    gSetLedsResult = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                    "{\"command\":\"set_leds\",\"mask\":\"3\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
}

void testPostLedsDisconnectedReturnsHelpful409() {
    gSetLedsResult = false;
    gMockStatus.connected = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                    "{\"command\":\"set_leds\",\"mask\":\"3\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "wiimote not connected"));
}

void testPostLedsQueuedWhenQueueConfiguredReturns202() {
    WebApiContext ctx = makeCtx();
    ctx.commandQueue = &gCommandQueue;
    gSetLedsResult = false;

    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                    "{\"command\":\"set_leds\",\"mask\":\"15\"}");
    TEST_ASSERT_EQUAL(202, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"status\":\"accepted\""));
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"commandId\":"));
    TEST_ASSERT_EQUAL_UINT32(1U, webCommandQueueCount(&gCommandQueue));
    TEST_ASSERT_EQUAL_UINT8(0U, gLastLedsMask);
}

void testPostLedsQueueFullReturns503() {
    WebApiContext ctx = makeCtx();
    ctx.commandQueue = &gCommandQueue;

    for (size_t i = 0U; i < kWebCommandQueueCapacity; ++i) {
        uint32_t commandId = 0U;
        TEST_ASSERT_TRUE(webCommandQueueEnqueue(&gCommandQueue, "/api/wiimote/commands/scan",
                                                "scan_start", &commandId));
    }

    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                    "{\"command\":\"set_leds\",\"mask\":\"7\"}");
    TEST_ASSERT_EQUAL(503, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "command queue full"));
}

// ===== POST /api/wiimote/commands/reporting-mode =====

void testPostReportingModeSuccess() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/reporting-mode",
                                    kValidBearer, "{\"command\":\"set_mode\",\"mode\":\"49\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_UINT8(49, gLastReportingMode);
    TEST_ASSERT_FALSE(gLastReportingContinuous);
}

void testPostReportingModeWithContinuous() {
    WebApiContext ctx = makeCtx();
    callRoute(&ctx, "POST", "/api/wiimote/commands/reporting-mode", kValidBearer,
              "{\"command\":\"set_mode\",\"mode\":\"49\",\"continuous\":\"true\"}");
    TEST_ASSERT_TRUE(gLastReportingContinuous);
}

void testPostReportingModeMissingModeReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/reporting-mode",
                                    kValidBearer, "{\"command\":\"set_mode\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
}

void testPostReportingModeRejectedReturns409() {
    gSetReportingModeResult = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/reporting-mode",
                                    kValidBearer, "{\"command\":\"set_mode\",\"mode\":\"49\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
}

// ===== POST /api/wiimote/commands/accelerometer =====

void testPostAccelerometerEnable() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r =
        callRoute(&ctx, "POST", "/api/wiimote/commands/accelerometer", kValidBearer,
                  "{\"command\":\"set_accel\",\"enabled\":\"true\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_TRUE(gLastAccelEnabled);
}

void testPostAccelerometerDisable() {
    WebApiContext ctx = makeCtx();
    callRoute(&ctx, "POST", "/api/wiimote/commands/accelerometer", kValidBearer,
              "{\"command\":\"set_accel\",\"enabled\":\"false\"}");
    TEST_ASSERT_FALSE(gLastAccelEnabled);
}

void testPostAccelerometerMissingEnabledReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/accelerometer",
                                    kValidBearer, "{\"command\":\"set_accel\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
}

// ===== POST /api/wiimote/commands/request-status =====

void testPostRequestStatusSuccess() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/request-status",
                                    kValidBearer, "{\"command\":\"request_status\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
}

void testPostRequestStatusRejectedReturns409() {
    gRequestStatusResult = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/request-status",
                                    kValidBearer, "{\"command\":\"request_status\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
}

void testPostRequestStatusDisconnectedReturnsHelpful409() {
    gMockStatus.connected = false;
    gRequestStatusResult = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/request-status",
                                    kValidBearer, "{\"command\":\"request_status\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "wiimote not connected"));
}

// ===== POST /api/wiimote/commands/scan =====

void testPostScanStart() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/scan", kValidBearer,
                                    "{\"command\":\"scan_start\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_TRUE(gLastScanEnabled);
}

void testPostScanStop() {
    gLastScanEnabled = true;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/scan", kValidBearer,
                                    "{\"command\":\"scan_stop\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_FALSE(gLastScanEnabled);
}

void testPostScanUnknownVerbReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/scan", kValidBearer,
                                    "{\"command\":\"scan_unknown\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "expected scan_start or scan_stop"));
}

// ===== POST /api/wiimote/commands/discovery =====

void testPostDiscoveryStartSuccess() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/discovery", kValidBearer,
                                    "{\"command\":\"discovery_start\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
}

void testPostDiscoveryStopSuccess() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/discovery", kValidBearer,
                                    "{\"command\":\"discovery_stop\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
}

void testPostDiscoveryStartRejectedReturns409() {
    gStartDiscoveryResult = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/discovery", kValidBearer,
                                    "{\"command\":\"discovery_start\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
}

void testPostDiscoveryStartNotStartedReturnsHelpful409() {
    gStartDiscoveryResult = false;
    gMockControllerStatus.started = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/discovery", kValidBearer,
                                    "{\"command\":\"discovery_start\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "bluetooth controller not started"));
}

void testPostDiscoveryStopInactiveReturnsHelpful409() {
    gStopDiscoveryResult = false;
    gMockControllerStatus.started = true;
    gMockControllerStatus.connected = false;
    gMockControllerStatus.scanning = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/discovery", kValidBearer,
                                    "{\"command\":\"discovery_stop\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "discovery not active"));
}

void testPostDiscoveryUnknownVerbReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/discovery", kValidBearer,
                                    "{\"command\":\"discovery_bad\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "expected discovery_start or discovery_stop"));
}

// ===== POST /api/wiimote/commands/disconnect =====

void testPostDisconnectDefaultReason() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/disconnect", kValidBearer,
                                    "{\"command\":\"disconnect\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_UINT8(0x16U, gLastDisconnectReason);
}

void testPostDisconnectCustomReason() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/disconnect", kValidBearer,
                                    "{\"command\":\"disconnect\",\"reason\":\"19\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_UINT8(19, gLastDisconnectReason);
}

void testPostDisconnectInvalidReasonReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/disconnect", kValidBearer,
                                    "{\"command\":\"disconnect\",\"reason\":\"300\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
}

void testPostDisconnectRejectedReturns409() {
    gDisconnectResult = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/disconnect", kValidBearer,
                                    "{\"command\":\"disconnect\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
}

void testPostDisconnectNotConnectedReturnsHelpful409() {
    gDisconnectResult = false;
    gMockControllerStatus.started = true;
    gMockControllerStatus.connected = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/disconnect", kValidBearer,
                                    "{\"command\":\"disconnect\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "wiimote not connected"));
}

// ===== POST /api/wiimote/commands/reconnect-policy =====

void testPostReconnectPolicyEnable() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r =
        callRoute(&ctx, "POST", "/api/wiimote/commands/reconnect-policy", kValidBearer,
                  "{\"command\":\"set_reconnect\",\"enabled\":\"true\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_TRUE(gLastAutoReconnect);
}

void testPostReconnectPolicyDisable() {
    WebApiContext ctx = makeCtx();
    callRoute(&ctx, "POST", "/api/wiimote/commands/reconnect-policy", kValidBearer,
              "{\"command\":\"set_reconnect\",\"enabled\":\"false\"}");
    TEST_ASSERT_FALSE(gLastAutoReconnect);
}

void testPostReconnectPolicyMissingEnabledReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wiimote/commands/reconnect-policy",
                                    kValidBearer, "{\"command\":\"set_reconnect\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
}

// ===== GET /api/wifi/control =====

void testGetWifiControlReturns200() {
    gMockWifiControlState.enabled = true;
    gMockWifiControlState.restAndWebSocket = true;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "GET", "/api/wifi/control", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"enabled\":true"));
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"serverStarted\":false"));
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"deliveryMode\":\"rest-ws\""));
}

// ===== POST /api/wifi/control =====

void testPostWifiControlEnable() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wifi/control", kValidBearer,
                                    "{\"command\":\"wifi_control\",\"enabled\":\"true\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_TRUE(gLastWifiControlEnabled);
}

void testPostWifiModeRestWs() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wifi/delivery-mode", kValidBearer,
                                    "{\"command\":\"wifi_mode\",\"mode\":\"rest-ws\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_TRUE(gLastWifiModeRestAndWebSocket);
}

void testPostWifiNetworkUpdate() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r =
        callRoute(&ctx, "POST", "/api/wifi/network", kValidBearer,
                  "{\"command\":\"wifi_network\",\"ssid\":\"myssid\",\"password\":\"mypass\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_STRING("myssid", gLastWifiSsid);
    TEST_ASSERT_EQUAL_STRING("mypass", gLastWifiPassword);
}

void testPostWifiRestart() {
    gMockWifiControlState.enabled = true;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wifi/restart", kValidBearer,
                                    "{\"command\":\"wifi_restart\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
}

void testPostWifiRestartDisabledReturnsHelpful409() {
    gRestartWifiResult = false;
    gMockWifiControlState.enabled = false;
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wifi/restart", kValidBearer,
                                    "{\"command\":\"wifi_restart\"}");
    TEST_ASSERT_EQUAL(409, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "wifi control is disabled"));
}

void testPostWifiTokenPolicyBlockedReturns403() {
    WebApiContext ctx = makeCtx();
    ctx.allowWifiApiTokenMutation = false;
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wifi/token", kValidBearer,
                                    "{\"command\":\"wifi_token\",\"token\":\"new_token\"}");
    TEST_ASSERT_EQUAL(403, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "wifi API token mutation is disabled by policy"));
}

void testPostWifiTokenEmptyReturns400() {
    WebApiContext ctx = makeCtx();
    ctx.allowWifiApiTokenMutation = true;
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wifi/token", kValidBearer,
                                    "{\"command\":\"wifi_token\",\"token\":\"\"}");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "token must not be empty"));
}

void testPostWifiTokenAllowedReturns200() {
    WebApiContext ctx = makeCtx();
    ctx.allowWifiApiTokenMutation = true;
    WebApiRouteResult r = callRoute(&ctx, "POST", "/api/wifi/token", kValidBearer,
                                    "{\"command\":\"wifi_token\",\"token\":\"new_token\"}");
    TEST_ASSERT_EQUAL(200, r.httpStatus);
    TEST_ASSERT_EQUAL_STRING("new_token", gLastWifiToken);
}

// ===== GET /api/commands/<id>/status =====

void testGetCommandStatusReturnsQueuedForEnqueuedCommand() {
    WebApiContext ctx = makeCtx();
    ctx.commandQueue = &gCommandQueue;

    WebApiRouteResult enqueue = callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                          "{\"command\":\"set_leds\",\"mask\":\"15\"}");
    TEST_ASSERT_EQUAL(202, enqueue.httpStatus);

    WebApiRouteResult status =
        callRoute(&ctx, "GET", "/api/commands/1/status", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(200, status.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"commandId\":1"));
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"status\":\"queued\""));
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"result\":\"pending\""));
}

void testGetCommandStatusReturnsUpdatedState() {
    WebApiContext ctx = makeCtx();
    ctx.commandQueue = &gCommandQueue;

    uint32_t commandId = 0U;
    TEST_ASSERT_TRUE(webCommandQueueEnqueue(&gCommandQueue, "/api/wiimote/commands/scan",
                                            "scan_start", &commandId));
    TEST_ASSERT_TRUE(webCommandQueueUpdate(&gCommandQueue, commandId,
                                           WebCommandQueueStatus::Completed,
                                           WebCommandQueueResult::Accepted));

    WebApiRouteResult status =
        callRoute(&ctx, "GET", "/api/commands/1/status", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(200, status.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"status\":\"completed\""));
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "\"result\":\"accepted\""));
}

void testGetCommandStatusInvalidIdReturns400() {
    WebApiContext ctx = makeCtx();
    ctx.commandQueue = &gCommandQueue;

    WebApiRouteResult status =
        callRoute(&ctx, "GET", "/api/commands/not-a-number/status", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(400, status.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "invalid command id"));
}

void testGetCommandStatusUnknownIdReturns404() {
    WebApiContext ctx = makeCtx();
    ctx.commandQueue = &gCommandQueue;

    WebApiRouteResult status =
        callRoute(&ctx, "GET", "/api/commands/99/status", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(404, status.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "command not found"));
}

void testGetCommandStatusWithoutQueueReturns404() {
    WebApiContext ctx = makeCtx();
    ctx.commandQueue = nullptr;

    WebApiRouteResult status =
        callRoute(&ctx, "GET", "/api/commands/1/status", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(404, status.httpStatus);
}

// ===== Body Error Tests =====

void testPostMissingBodyReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r =
        callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer, nullptr);
    TEST_ASSERT_EQUAL(400, r.httpStatus);
}

void testPostMalformedBodyReturns400() {
    WebApiContext ctx = makeCtx();
    WebApiRouteResult r =
        callRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer, "{malformed");
    TEST_ASSERT_EQUAL(400, r.httpStatus);
}

void testPostBodyTooLargeReturns400() {
    WebApiContext ctx = makeCtx();
    // bodyLen > kWebRequestMaxBodySize (1024) triggers BodyTooLarge
    const char *body = "{\"command\":\"set_leds\",\"mask\":\"1\"}";
    WebApiRouteResult r = webApiRoute(&ctx, "POST", "/api/wiimote/commands/leds", kValidBearer,
                                      body, 1025U, gBuf, sizeof(gBuf));
    TEST_ASSERT_EQUAL(400, r.httpStatus);
    TEST_ASSERT_NOT_NULL(std::strstr(gBuf, "too large"));
}

// ===== Main =====

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testNullCtxReturns400);
    RUN_TEST(testMissingAuthReturns401);
    RUN_TEST(testInvalidAuthReturns401);
    RUN_TEST(testValidBearerAuthPasses);
    RUN_TEST(testUnsupportedAuthSchemeReturns401);
    RUN_TEST(testNullCredentialsInCtxReturns401WithValidToken);

    RUN_TEST(testUnknownPathReturns404);
    RUN_TEST(testMethodMismatchReturns404);
    RUN_TEST(testStaticIndexRouteReturns200WithoutAuth);
    RUN_TEST(testStaticAppJsRouteReturns200WithoutAuth);
    RUN_TEST(testStaticStylesRouteReturns200WithoutAuth);
    RUN_TEST(testOpenApiRouteReturns200WithoutAuth);
    RUN_TEST(testOpenApiRouteContainsAllRegisteredPaths);
    RUN_TEST(testOpenApiRouteTruncatesSafelyInSmallBuffer);

    RUN_TEST(testGetStatusReturns200);
    RUN_TEST(testGetStatusJsonShape);
    RUN_TEST(testGetConfigReturns200);
    RUN_TEST(testGetConfigJsonShape);

    RUN_TEST(testPostLedsSuccess);
    RUN_TEST(testPostLedsMissingMaskReturns400);
    RUN_TEST(testPostLedsInvalidMaskReturns400);
    RUN_TEST(testPostLedsRejectedReturns409);
    RUN_TEST(testPostLedsDisconnectedReturnsHelpful409);
    RUN_TEST(testPostLedsQueuedWhenQueueConfiguredReturns202);
    RUN_TEST(testPostLedsQueueFullReturns503);

    RUN_TEST(testPostReportingModeSuccess);
    RUN_TEST(testPostReportingModeWithContinuous);
    RUN_TEST(testPostReportingModeMissingModeReturns400);
    RUN_TEST(testPostReportingModeRejectedReturns409);

    RUN_TEST(testPostAccelerometerEnable);
    RUN_TEST(testPostAccelerometerDisable);
    RUN_TEST(testPostAccelerometerMissingEnabledReturns400);

    RUN_TEST(testPostRequestStatusSuccess);
    RUN_TEST(testPostRequestStatusRejectedReturns409);
    RUN_TEST(testPostRequestStatusDisconnectedReturnsHelpful409);

    RUN_TEST(testPostScanStart);
    RUN_TEST(testPostScanStop);
    RUN_TEST(testPostScanUnknownVerbReturns400);

    RUN_TEST(testPostDiscoveryStartSuccess);
    RUN_TEST(testPostDiscoveryStopSuccess);
    RUN_TEST(testPostDiscoveryStartRejectedReturns409);
    RUN_TEST(testPostDiscoveryStartNotStartedReturnsHelpful409);
    RUN_TEST(testPostDiscoveryStopInactiveReturnsHelpful409);
    RUN_TEST(testPostDiscoveryUnknownVerbReturns400);

    RUN_TEST(testPostDisconnectDefaultReason);
    RUN_TEST(testPostDisconnectCustomReason);
    RUN_TEST(testPostDisconnectInvalidReasonReturns400);
    RUN_TEST(testPostDisconnectRejectedReturns409);
    RUN_TEST(testPostDisconnectNotConnectedReturnsHelpful409);

    RUN_TEST(testPostReconnectPolicyEnable);
    RUN_TEST(testPostReconnectPolicyDisable);
    RUN_TEST(testPostReconnectPolicyMissingEnabledReturns400);

    RUN_TEST(testGetWifiControlReturns200);
    RUN_TEST(testPostWifiControlEnable);
    RUN_TEST(testPostWifiModeRestWs);
    RUN_TEST(testPostWifiNetworkUpdate);
    RUN_TEST(testPostWifiRestart);
    RUN_TEST(testPostWifiRestartDisabledReturnsHelpful409);
    RUN_TEST(testPostWifiTokenPolicyBlockedReturns403);
    RUN_TEST(testPostWifiTokenEmptyReturns400);
    RUN_TEST(testPostWifiTokenAllowedReturns200);

    RUN_TEST(testGetCommandStatusReturnsQueuedForEnqueuedCommand);
    RUN_TEST(testGetCommandStatusReturnsUpdatedState);
    RUN_TEST(testGetCommandStatusInvalidIdReturns400);
    RUN_TEST(testGetCommandStatusUnknownIdReturns404);
    RUN_TEST(testGetCommandStatusWithoutQueueReturns404);

    RUN_TEST(testPostMissingBodyReturns400);
    RUN_TEST(testPostMalformedBodyReturns400);
    RUN_TEST(testPostBodyTooLargeReturns400);

    return UNITY_END();
}
