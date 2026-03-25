#include "../../../src/wifi/http_server.h"
#include "../../../src/wifi/web/web_assets.h"

#include <cstring>
#include <unity.h>

void setUp() {}
void tearDown() {}

void testWifiHttpServerNativeBackendUnavailable() {
    WifiHttpServer server;

    TEST_ASSERT_FALSE(server.isStarted());
    TEST_ASSERT_FALSE(server.begin(80));
    TEST_ASSERT_EQUAL(static_cast<int>(WifiHttpServerStartError::BackendUnavailable),
                      static_cast<int>(server.lastStartError()));
    TEST_ASSERT_FALSE(server.isStarted());

    server.poll();
    server.end();
    TEST_ASSERT_FALSE(server.isStarted());
}

void noopHandler(const WifiHttpRequest * /*request*/,
                 char *responseBuf,
                 size_t responseBufSize,
                 WifiHttpResponse *response,
                 void * /*userData*/) {
    if (responseBuf != nullptr && responseBufSize > 0U) {
        responseBuf[0] = '\0';
    }
    if (response != nullptr) {
        response->status = 204;
        response->contentType = "text/plain";
    }
}

void testWifiHttpServerNativeBackendUnavailableEvenWithHandler() {
    WifiHttpServer server;
    server.setHandler(noopHandler, nullptr);

    TEST_ASSERT_FALSE(server.begin(8080));
    TEST_ASSERT_EQUAL(static_cast<int>(WifiHttpServerStartError::BackendUnavailable),
                      static_cast<int>(server.lastStartError()));
}

void testWebAssetsAccessorsReturnExpectedContent() {
    TEST_ASSERT_EQUAL_UINT32(std::strlen(web_assets::indexHtml()), web_assets::indexHtmlLen());
    TEST_ASSERT_EQUAL_UINT32(std::strlen(web_assets::appJs()), web_assets::appJsLen());
    TEST_ASSERT_EQUAL_UINT32(std::strlen(web_assets::stylesCss()), web_assets::stylesCssLen());

    TEST_ASSERT_NOT_NULL(std::strstr(web_assets::indexHtml(), "ESP32 Wiimote Control"));
    TEST_ASSERT_NOT_NULL(std::strstr(web_assets::appJs(), "fetchWithAuth"));
    TEST_ASSERT_NOT_NULL(std::strstr(web_assets::stylesCss(), "--accent"));

    TEST_ASSERT_EQUAL(web_assets::kIndexHtml, web_assets::indexHtml());
    TEST_ASSERT_EQUAL(web_assets::kAppJs, web_assets::appJs());
    TEST_ASSERT_EQUAL(web_assets::kStylesCss, web_assets::stylesCss());
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(testWifiHttpServerNativeBackendUnavailable);
    RUN_TEST(testWifiHttpServerNativeBackendUnavailableEvenWithHandler);
    RUN_TEST(testWebAssetsAccessorsReturnExpectedContent);

    return UNITY_END();
}