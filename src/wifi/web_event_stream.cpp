#include "web_event_stream.h"

#include "web_response_serializer.h"

#include <cstring>

namespace {

const char kPathInputEvents[] = "/api/wiimote/input/events";
const char kPathStatusEvents[] = "/api/wiimote/status/events";

}  // namespace

void webEventStreamInit(WebEventStream *stream) {
    if (stream == nullptr) {
        return;
    }

    for (auto &ch : stream->channels) {
        ch.connected = false;
        ch.sendFn = nullptr;
        ch.userData = nullptr;
    }
}

bool webEventStreamMatchPath(const char *path, WebEventStreamChannel *channel) {
    if (path == nullptr || channel == nullptr) {
        return false;
    }

    if (std::strcmp(path, kPathInputEvents) == 0) {
        *channel = WebEventStreamChannel::Input;
        return true;
    }

    if (std::strcmp(path, kPathStatusEvents) == 0) {
        *channel = WebEventStreamChannel::Status;
        return true;
    }

    return false;
}

void webEventStreamConnect(WebEventStream *stream,
                           WebEventStreamChannel channel,
                           web_event_stream_send_fn sendFn,
                           void *userData) {
    if (stream == nullptr || sendFn == nullptr) {
        return;
    }

    const auto kIdx = static_cast<size_t>(channel);
    if (kIdx >= kWebEventStreamChannelCount) {
        return;
    }

    stream->channels[kIdx].connected = true;
    stream->channels[kIdx].sendFn = sendFn;
    stream->channels[kIdx].userData = userData;
}

void webEventStreamDisconnect(WebEventStream *stream, WebEventStreamChannel channel) {
    if (stream == nullptr) {
        return;
    }

    const auto kIdx = static_cast<size_t>(channel);
    if (kIdx >= kWebEventStreamChannelCount) {
        return;
    }

    stream->channels[kIdx].connected = false;
    stream->channels[kIdx].sendFn = nullptr;
    stream->channels[kIdx].userData = nullptr;
}

bool webEventStreamHasClient(const WebEventStream *stream, WebEventStreamChannel channel) {
    if (stream == nullptr) {
        return false;
    }

    const auto kIdx = static_cast<size_t>(channel);
    if (kIdx >= kWebEventStreamChannelCount) {
        return false;
    }

    return stream->channels[kIdx].connected;
}

void webEventStreamPublishInput(WebEventStream *stream,
                                const WebWiimoteInputSnapshot &snapshot,
                                char *buf,
                                size_t bufSize) {
    if (stream == nullptr || buf == nullptr || bufSize == 0U) {
        return;
    }

    const auto kIdx = static_cast<size_t>(WebEventStreamChannel::Input);
    if (!stream->channels[kIdx].connected || stream->channels[kIdx].sendFn == nullptr) {
        return;
    }

    if (serializeWiimoteInput(buf, bufSize, snapshot) != WebSerializeResult::Ok) {
        return;
    }

    stream->channels[kIdx].sendFn(buf, std::strlen(buf), stream->channels[kIdx].userData);
}

void webEventStreamPublishStatus(WebEventStream *stream,
                                 const WebWiimoteStatusSnapshot &snapshot,
                                 char *buf,
                                 size_t bufSize) {
    if (stream == nullptr || buf == nullptr || bufSize == 0U) {
        return;
    }

    const auto kIdx = static_cast<size_t>(WebEventStreamChannel::Status);
    if (!stream->channels[kIdx].connected || stream->channels[kIdx].sendFn == nullptr) {
        return;
    }

    if (serializeWiimoteStatus(buf, bufSize, snapshot) != WebSerializeResult::Ok) {
        return;
    }

    stream->channels[kIdx].sendFn(buf, std::strlen(buf), stream->channels[kIdx].userData);
}
