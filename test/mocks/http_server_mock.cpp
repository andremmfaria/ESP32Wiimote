#include "http_server_mock.h"

#include <cstring>

namespace {

bool gWifiHttpServerMockBeginResult = true;
int gWifiHttpServerMockBeginCallCount = 0;
int gWifiHttpServerMockEndCallCount = 0;
int gWifiHttpServerMockPollCallCount = 0;
bool gWifiHttpServerMockStarted = false;
wifi_http_request_handler_fn gWifiHttpServerMockHandler = nullptr;
void *gWifiHttpServerMockUserData = nullptr;

WifiHttpMethod parseMethodString(const char *method) {
    if (method == nullptr) {
        return WifiHttpMethod::Unsupported;
    }
    if (std::strcmp(method, "GET") == 0) {
        return WifiHttpMethod::Get;
    }
    if (std::strcmp(method, "POST") == 0) {
        return WifiHttpMethod::Post;
    }
    return WifiHttpMethod::Unsupported;
}

}  // namespace

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

    ++gWifiHttpServerMockBeginCallCount;
    if (!gWifiHttpServerMockBeginResult) {
        return false;
    }

    gWifiHttpServerMockStarted = true;
    gWifiHttpServerMockHandler = handler_;
    gWifiHttpServerMockUserData = userData_;
    started_ = true;
    port_ = port;
    return true;
}

void WifiHttpServer::end() {
    if (!started_) {
        return;
    }

    ++gWifiHttpServerMockEndCallCount;
    gWifiHttpServerMockStarted = false;
    gWifiHttpServerMockHandler = nullptr;
    gWifiHttpServerMockUserData = nullptr;
    started_ = false;
    port_ = 0U;
}

void WifiHttpServer::poll() const {
    if (!started_) {
        return;
    }

    ++gWifiHttpServerMockPollCallCount;
}

bool WifiHttpServer::isStarted() const {
    return started_;
}

void wifiHttpServerMockReset() {
    gWifiHttpServerMockBeginResult = true;
    gWifiHttpServerMockBeginCallCount = 0;
    gWifiHttpServerMockEndCallCount = 0;
    gWifiHttpServerMockPollCallCount = 0;
    gWifiHttpServerMockStarted = false;
    gWifiHttpServerMockHandler = nullptr;
    gWifiHttpServerMockUserData = nullptr;
}

void wifiHttpServerMockSetBeginResult(bool result) {
    gWifiHttpServerMockBeginResult = result;
}

int wifiHttpServerMockGetBeginCallCount() {
    return gWifiHttpServerMockBeginCallCount;
}

int wifiHttpServerMockGetEndCallCount() {
    return gWifiHttpServerMockEndCallCount;
}

int wifiHttpServerMockGetPollCallCount() {
    return gWifiHttpServerMockPollCallCount;
}

bool wifiHttpServerMockDispatchRequest(const char *method,
                                       const char *path,
                                       const char *authHeader,
                                       const char *body,
                                       int *status,
                                       const char **contentType,
                                       char *responseBuf,
                                       size_t responseBufSize) {
    if (!gWifiHttpServerMockStarted || gWifiHttpServerMockHandler == nullptr ||
        responseBuf == nullptr || responseBufSize == 0U) {
        return false;
    }

    WifiHttpResponse response = {};
    const WifiHttpRequest kRequest = {parseMethodString(method), path, authHeader, body,
                                      body != nullptr ? std::strlen(body) : 0U};
    gWifiHttpServerMockHandler(&kRequest, responseBuf, responseBufSize, &response,
                               gWifiHttpServerMockUserData);

    if (status != nullptr) {
        *status = response.status;
    }
    if (contentType != nullptr) {
        *contentType = response.contentType;
    }

    return true;
}
