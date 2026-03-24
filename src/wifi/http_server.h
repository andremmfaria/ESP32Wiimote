#ifndef WIFI_HTTP_SERVER_H
#define WIFI_HTTP_SERVER_H

#include <cstddef>
#include <stdint.h>

enum class WifiHttpMethod : uint8_t {
    Get = 0,
    Post = 1,
    Unsupported = 2,
};

struct WifiHttpRequest {
    WifiHttpMethod method;
    const char *path;
    const char *authHeader;
    const char *body;
    size_t bodyLen;
};

struct WifiHttpResponse {
    int status{200};
    const char *contentType{"application/json"};
};

using wifi_http_request_handler_fn = void (*)(const WifiHttpRequest *request,
                                              char *responseBuf,
                                              size_t responseBufSize,
                                              WifiHttpResponse *response,
                                              void *userData);

class WifiHttpServer {
   public:
    WifiHttpServer();
    ~WifiHttpServer();

    void setHandler(wifi_http_request_handler_fn handler, void *userData);
    bool begin(uint16_t port);
    void end();
    void poll() const;
    bool isStarted() const;

   private:
    wifi_http_request_handler_fn handler_ = nullptr;
    void *userData_ = nullptr;
    bool started_ = false;
    uint16_t port_ = 0U;
    void *impl_ = nullptr;
};

#endif  // WIFI_HTTP_SERVER_H