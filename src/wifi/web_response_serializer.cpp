#include "web_response_serializer.h"

#include <cstdio>
#include <cstring>

// ===== Internal Helper =====

namespace {

/** Returns true when snprintf produced a complete (non-truncated) string. */
bool snprintfFits(int written, size_t size) {
    return written > 0 && static_cast<size_t>(written) < size;
}

const char *boolStr(bool v) {
    return v ? "true" : "false";
}

}  // namespace

// ===== Public Functions =====

WebSerializeResult serializeOk(char *buf, size_t size) {
    if (buf == nullptr || size == 0U) {
        return WebSerializeResult::BufferTooSmall;
    }
    int n = std::snprintf(buf, size, "{\"status\":\"ok\"}");
    if (!snprintfFits(n, size)) {
        return WebSerializeResult::BufferTooSmall;
    }
    return WebSerializeResult::Ok;
}

WebSerializeResult serializeError(char *buf, size_t size, const char *message) {
    if (buf == nullptr || size == 0U || message == nullptr) {
        return WebSerializeResult::BufferTooSmall;
    }
    int n = std::snprintf(buf, size, "{\"status\":\"error\",\"message\":\"%s\"}", message);
    if (!snprintfFits(n, size)) {
        return WebSerializeResult::BufferTooSmall;
    }
    return WebSerializeResult::Ok;
}

WebSerializeResult serializeWiimoteInput(char *buf,
                                         size_t size,
                                         const WebWiimoteInputSnapshot &in) {
    if (buf == nullptr || size == 0U) {
        return WebSerializeResult::BufferTooSmall;
    }
    int n = std::snprintf(
        buf, size,
        "{"
        "\"buttons\":\"%08lx\","
        "\"accelX\":%u,"
        "\"accelY\":%u,"
        "\"accelZ\":%u,"
        "\"nunchukX\":%u,"
        "\"nunchukY\":%u"
        "}",
        static_cast<unsigned long>(in.buttonMask), static_cast<unsigned int>(in.accelX),
        static_cast<unsigned int>(in.accelY), static_cast<unsigned int>(in.accelZ),
        static_cast<unsigned int>(in.nunchukX), static_cast<unsigned int>(in.nunchukY));
    if (!snprintfFits(n, size)) {
        return WebSerializeResult::BufferTooSmall;
    }
    return WebSerializeResult::Ok;
}

WebSerializeResult serializeWiimoteStatus(char *buf,
                                          size_t size,
                                          const WebWiimoteStatusSnapshot &st) {
    if (buf == nullptr || size == 0U) {
        return WebSerializeResult::BufferTooSmall;
    }
    int n = std::snprintf(buf, size, "{\"connected\":%s,\"batteryLevel\":%u}",
                          boolStr(st.connected), static_cast<unsigned int>(st.batteryLevel));
    if (!snprintfFits(n, size)) {
        return WebSerializeResult::BufferTooSmall;
    }
    return WebSerializeResult::Ok;
}

WebSerializeResult serializeControllerStatus(char *buf,
                                             size_t size,
                                             const WebControllerStatusSnapshot &st) {
    if (buf == nullptr || size == 0U) {
        return WebSerializeResult::BufferTooSmall;
    }
    int n =
        std::snprintf(buf, size,
                      "{"
                      "\"initialized\":%s,"
                      "\"started\":%s,"
                      "\"scanning\":%s,"
                      "\"connected\":%s,"
                      "\"activeConnectionHandle\":%u,"
                      "\"fastReconnectActive\":%s,"
                      "\"autoReconnectEnabled\":%s"
                      "}",
                      boolStr(st.initialized), boolStr(st.started), boolStr(st.scanning),
                      boolStr(st.connected), static_cast<unsigned int>(st.activeConnectionHandle),
                      boolStr(st.fastReconnectActive), boolStr(st.autoReconnectEnabled));
    if (!snprintfFits(n, size)) {
        return WebSerializeResult::BufferTooSmall;
    }
    return WebSerializeResult::Ok;
}

WebSerializeResult serializeConfig(char *buf, size_t size, const WebConfigSnapshot &cfg) {
    if (buf == nullptr || size == 0U) {
        return WebSerializeResult::BufferTooSmall;
    }
    int n = std::snprintf(buf, size,
                          "{"
                          "\"nunchukStickThreshold\":%d,"
                          "\"txQueueSize\":%d,"
                          "\"rxQueueSize\":%d,"
                          "\"fastReconnectTtlMs\":%lu"
                          "}",
                          cfg.nunchukStickThreshold, cfg.txQueueSize, cfg.rxQueueSize,
                          static_cast<unsigned long>(cfg.fastReconnectTtlMs));
    if (!snprintfFits(n, size)) {
        return WebSerializeResult::BufferTooSmall;
    }
    return WebSerializeResult::Ok;
}

WebSerializeResult serializeWifiControlState(char *buf,
                                             size_t size,
                                             const WebWifiControlStateSnapshot &state) {
    if (buf == nullptr || size == 0U) {
        return WebSerializeResult::BufferTooSmall;
    }

    int n = std::snprintf(buf, size,
                          "{"
                          "\"enabled\":%s,"
                          "\"ready\":%s,"
                          "\"networkCredentialsConfigured\":%s,"
                          "\"networkConnectAttempted\":%s,"
                          "\"networkConnected\":%s,"
                          "\"networkConnectFailed\":%s,"
                          "\"serverStarted\":%s,"
                          "\"serverBindFailed\":%s,"
                          "\"deliveryMode\":\"%s\","
                          "\"hasToken\":%s"
                          "}",
                          boolStr(state.enabled), boolStr(state.ready),
                          boolStr(state.networkCredentialsConfigured),
                          boolStr(state.networkConnectAttempted), boolStr(state.networkConnected),
                          boolStr(state.networkConnectFailed), boolStr(state.serverStarted),
                          boolStr(state.serverBindFailed),
                          state.restAndWebSocket ? "rest-ws" : "rest", boolStr(state.hasToken));
    if (!snprintfFits(n, size)) {
        return WebSerializeResult::BufferTooSmall;
    }
    return WebSerializeResult::Ok;
}
