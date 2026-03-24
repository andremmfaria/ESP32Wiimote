#include "http_server.h"

#ifdef ARDUINO_ARCH_ESP32
#include <WebServer.h>
#endif

namespace {

#ifdef ARDUINO_ARCH_ESP32
constexpr size_t kCollectedHeaderCount = 1U;
const char *kCollectedHeaders[kCollectedHeaderCount] = {"Authorization"};
constexpr size_t kResponseBufSize = 4096U;

WifiHttpMethod mapMethod(const int kMethod) {
    switch (kMethod) {
        case HTTP_GET:
            return WifiHttpMethod::Get;
        case HTTP_POST:
            return WifiHttpMethod::Post;
        default:
            return WifiHttpMethod::Unsupported;
    }
}
#endif

}  // namespace

#ifdef ARDUINO_ARCH_ESP32

WifiHttpServer::WifiHttpServer() = default;

WifiHttpServer::~WifiHttpServer() {
    end();
}

void WifiHttpServer::setHandler(wifi_http_request_handler_fn handler, void *userData) {
    handler_ = handler;
    userData_ = userData;
}

bool WifiHttpServer::begin(uint16_t port) {
    if (started_) {
        return true;
    }

    if (handler_ == nullptr) {
        return false;
    }

    WebServer *server = new WebServer(port);
    server->collectHeaders(kCollectedHeaders, kCollectedHeaderCount);
    impl_ = server;

    server->onNotFound([this]() {
        WebServer *currentServer = static_cast<WebServer *>(impl_);
        if (currentServer == nullptr || handler_ == nullptr) {
            return;
        }

        char responseBuf[kResponseBufSize] = {0};
        WifiHttpResponse response = {};
        const String kUri = currentServer->uri();
        const String kAuthHeader = currentServer->header("Authorization");
        const String kBody = currentServer->arg("plain");
        const WifiHttpRequest kRequest = {
            mapMethod(static_cast<int>(currentServer->method())),
            kUri.c_str(),
            kAuthHeader.length() > 0 ? kAuthHeader.c_str() : nullptr,
            kBody.length() > 0 ? kBody.c_str() : nullptr,
            static_cast<size_t>(kBody.length()),
        };

        handler_(&kRequest, responseBuf, sizeof(responseBuf), &response, userData_);
        currentServer->send(
            response.status,
            response.contentType != nullptr ? response.contentType : "application/json",
            responseBuf);
    });

    server->begin();

    started_ = true;
    port_ = port;
    return true;
}

void WifiHttpServer::end() {
    if (!started_) {
        return;
    }

    if (impl_ != nullptr) {
        WebServer *server = static_cast<WebServer *>(impl_);
        server->stop();
        delete server;
        impl_ = nullptr;
    }

    started_ = false;
    port_ = 0U;
}

void WifiHttpServer::poll() const {
    if (!started_ || impl_ == nullptr) {
        return;
    }

    static_cast<WebServer *>(impl_)->handleClient();
}

bool WifiHttpServer::isStarted() const {
    return started_;
}

#else

WifiHttpServer::WifiHttpServer() = default;

WifiHttpServer::~WifiHttpServer() = default;

void WifiHttpServer::setHandler(wifi_http_request_handler_fn handler, void *userData) {
    handler_ = handler;
    userData_ = userData;
}

bool WifiHttpServer::begin(uint16_t /*port*/) {
    started_ = false;
    port_ = 0U;
    return false;
}

void WifiHttpServer::end() {
    started_ = false;
    port_ = 0U;
}

void WifiHttpServer::poll() const {}

bool WifiHttpServer::isStarted() const {
    return false;
}

#endif