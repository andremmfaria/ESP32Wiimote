#include "web_event_stream.h"

#include "web_response_serializer.h"

#include <cstdio>
#include <cstring>

namespace {

const char kPathInputEvents[] = "/api/wiimote/input/events";
const char kPathStatusEvents[] = "/api/wiimote/status/events";
const char kInputEventType[] = "input";
const char kStatusEventType[] = "status";

bool isValidChannelIndex(size_t idx) {
    return idx < kWebEventStreamChannelCount;
}

bool snprintfFits(int written, size_t size) {
    return written > 0 && static_cast<size_t>(written) < size;
}

bool buildEventFrame(char *out,
                     size_t outSize,
                     uint32_t seq,
                     const char *eventType,
                     const char *payloadJson,
                     size_t *outLen) {
    if (out == nullptr || outSize == 0U || eventType == nullptr || payloadJson == nullptr ||
        outLen == nullptr) {
        return false;
    }

    int n = std::snprintf(out, outSize,
                          "{"
                          "\"seq\":%lu,"
                          "\"event\":\"%s\","
                          "\"payload\":%s"
                          "}",
                          static_cast<unsigned long>(seq), eventType, payloadJson);
    if (!snprintfFits(n, outSize)) {
        return false;
    }

    *outLen = static_cast<size_t>(n);
    return true;
}

const WebEventStreamFrame *channelFrameAt(const WebEventStreamChannelState &state, size_t offset) {
    const size_t kIdx = (state.head + offset) % kWebEventStreamBufferCapacity;
    return &state.buffer[kIdx];
}

void resetChannel(WebEventStreamChannelState *state) {
    if (state == nullptr) {
        return;
    }

    state->client.connected = false;
    state->client.sendFn = nullptr;
    state->client.userData = nullptr;
    state->nextSeq = 1U;
    state->head = 0U;
    state->count = 0U;

    for (auto &entry : state->buffer) {
        entry.used = false;
        entry.seq = 0U;
        entry.frame[0] = '\0';
        entry.frameLen = 0U;
    }
}

bool appendFrame(WebEventStreamChannelState *state,
                 const char *frame,
                 size_t frameLen,
                 uint32_t *seqOut) {
    if (state == nullptr || frame == nullptr || seqOut == nullptr || frameLen == 0U ||
        frameLen >= kWebEventStreamMaxFrameLen) {
        return false;
    }

    const uint32_t kSeq = state->nextSeq;
    state->nextSeq += 1U;

    if (state->count == kWebEventStreamBufferCapacity) {
        state->head = (state->head + 1U) % kWebEventStreamBufferCapacity;
        state->count -= 1U;
    }

    const size_t kInsertIdx = (state->head + state->count) % kWebEventStreamBufferCapacity;
    WebEventStreamFrame *entry = &state->buffer[kInsertIdx];
    entry->used = true;
    entry->seq = kSeq;
    std::memcpy(entry->frame, frame, frameLen);
    entry->frame[frameLen] = '\0';
    entry->frameLen = frameLen;
    state->count += 1U;

    *seqOut = kSeq;
    return true;
}

uint32_t latestSeqFromChannel(const WebEventStreamChannelState &state) {
    if (state.count == 0U) {
        return 0U;
    }
    const WebEventStreamFrame *latest = channelFrameAt(state, state.count - 1U);
    return latest->seq;
}

uint32_t oldestSeqFromChannel(const WebEventStreamChannelState &state) {
    if (state.count == 0U) {
        return 0U;
    }
    return channelFrameAt(state, 0U)->seq;
}

void publishEvent(WebEventStream *stream,
                  WebEventStreamChannel channel,
                  const char *eventType,
                  const char *payloadJson,
                  char *frameBuf,
                  size_t frameBufSize) {
    if (stream == nullptr || payloadJson == nullptr || frameBuf == nullptr || frameBufSize == 0U ||
        eventType == nullptr) {
        return;
    }

    const size_t kIdx = static_cast<size_t>(channel);
    if (!isValidChannelIndex(kIdx)) {
        return;
    }

    WebEventStreamChannelState *state = &stream->channels[kIdx];

    uint32_t seq = state->nextSeq;
    size_t eventLen = 0U;
    if (!buildEventFrame(frameBuf, frameBufSize, seq, eventType, payloadJson, &eventLen)) {
        return;
    }

    uint32_t appendedSeq = 0U;
    if (!appendFrame(state, frameBuf, eventLen, &appendedSeq)) {
        return;
    }
    if (appendedSeq != seq) {
        return;
    }

    if (state->client.connected && state->client.sendFn != nullptr) {
        state->client.sendFn(frameBuf, eventLen, state->client.userData);
    }
}

}  // namespace

void webEventStreamInit(WebEventStream *stream) {
    if (stream == nullptr) {
        return;
    }

    for (auto &channel : stream->channels) {
        resetChannel(&channel);
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

    stream->channels[kIdx].client.connected = true;
    stream->channels[kIdx].client.sendFn = sendFn;
    stream->channels[kIdx].client.userData = userData;
}

void webEventStreamDisconnect(WebEventStream *stream, WebEventStreamChannel channel) {
    if (stream == nullptr) {
        return;
    }

    const auto kIdx = static_cast<size_t>(channel);
    if (kIdx >= kWebEventStreamChannelCount) {
        return;
    }

    stream->channels[kIdx].client.connected = false;
    stream->channels[kIdx].client.sendFn = nullptr;
    stream->channels[kIdx].client.userData = nullptr;
}

bool webEventStreamHasClient(const WebEventStream *stream, WebEventStreamChannel channel) {
    if (stream == nullptr) {
        return false;
    }

    const auto kIdx = static_cast<size_t>(channel);
    if (kIdx >= kWebEventStreamChannelCount) {
        return false;
    }

    return stream->channels[kIdx].client.connected;
}

uint32_t webEventStreamLatestSeq(const WebEventStream *stream, WebEventStreamChannel channel) {
    if (stream == nullptr) {
        return 0U;
    }

    const size_t kIdx = static_cast<size_t>(channel);
    if (!isValidChannelIndex(kIdx)) {
        return 0U;
    }

    return latestSeqFromChannel(stream->channels[kIdx]);
}

void webEventStreamPublishInput(WebEventStream *stream,
                                const WebWiimoteInputSnapshot &snapshot,
                                char *buf,
                                size_t bufSize) {
    if (stream == nullptr || buf == nullptr || bufSize == 0U) {
        return;
    }

    if (serializeWiimoteInput(buf, bufSize, snapshot) != WebSerializeResult::Ok) {
        return;
    }

    publishEvent(stream, WebEventStreamChannel::Input, kInputEventType, buf, buf, bufSize);
}

void webEventStreamPublishStatus(WebEventStream *stream,
                                 const WebWiimoteStatusSnapshot &snapshot,
                                 char *buf,
                                 size_t bufSize) {
    if (stream == nullptr || buf == nullptr || bufSize == 0U) {
        return;
    }

    if (serializeWiimoteStatus(buf, bufSize, snapshot) != WebSerializeResult::Ok) {
        return;
    }

    publishEvent(stream, WebEventStreamChannel::Status, kStatusEventType, buf, buf, bufSize);
}

bool webEventStreamReplaySince(const WebEventStream *stream,
                               WebEventStreamChannel channel,
                               uint32_t afterSeq,
                               web_event_stream_send_fn sendFn,
                               void *userData,
                               WebEventStreamReplayResult *result) {
    if (stream == nullptr || sendFn == nullptr || result == nullptr) {
        return false;
    }

    const size_t kIdx = static_cast<size_t>(channel);
    if (!isValidChannelIndex(kIdx)) {
        return false;
    }

    const WebEventStreamChannelState &state = stream->channels[kIdx];
    result->replayedCount = 0U;
    result->latestSeq = latestSeqFromChannel(state);
    result->nextSeq = state.nextSeq;
    result->requiresSnapshotRecovery = false;

    if (state.count == 0U) {
        return true;
    }

    const uint32_t kOldestSeq = oldestSeqFromChannel(state);
    if ((afterSeq + 1U) < kOldestSeq) {
        result->requiresSnapshotRecovery = true;
        return true;
    }

    for (size_t i = 0U; i < state.count; ++i) {
        const WebEventStreamFrame *entry = channelFrameAt(state, i);
        if (!entry->used || entry->seq <= afterSeq) {
            continue;
        }

        sendFn(entry->frame, entry->frameLen, userData);
        result->replayedCount += 1U;
    }

    return true;
}
