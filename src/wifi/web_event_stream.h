#ifndef WEB_EVENT_STREAM_H
#define WEB_EVENT_STREAM_H

#include "web_response_serializer.h"

#include <cstddef>
#include <stdint.h>

/**
 * WebEventStream
 *
 * Portable abstraction for split WebSocket event streams.
 *
 * Two independent channels are supported:
 *   - Input events:  GET /api/wiimote/input/events
 *   - Status events: GET /api/wiimote/status/events
 *
 * One active client per channel is enforced (one-client policy).
 * Callers supply a send callback for frame delivery so this module
 * can be tested in the native environment without any ESP32 dependency.
 *
 * Lifecycle:
 *   1. Call webEventStreamRoute() from the HTTP/WebSocket upgrade handler.
 *      It returns true and populates the matched channel when the path matches.
 *   2. On successful WebSocket upgrade, call webEventStreamConnect().
 *   3. To deliver an event, call webEventStreamPublishInput() or
 *      webEventStreamPublishStatus().
 *   4. On client disconnect, call webEventStreamDisconnect().
 */

// ===== Channel IDs =====

enum class WebEventStreamChannel : uint8_t {
    Input = 0,   ///< /api/wiimote/input/events
    Status = 1,  ///< /api/wiimote/status/events
};

static const size_t kWebEventStreamChannelCount = 2U;
static const size_t kWebEventStreamBufferCapacity = 16U;
static const size_t kWebEventStreamMaxFrameLen = 224U;

// ===== Send Callback =====

/**
 * Callback invoked by webEventStreamPublish* to deliver a serialized frame.
 *
 * @param frame     Null-terminated JSON string.
 * @param frameLen  Length of frame (excluding the null terminator).
 * @param userData  Caller-supplied context pointer.
 */
using web_event_stream_send_fn = void (*)(const char *frame, size_t frameLen, void *userData);

// ===== Stream State =====

struct WebEventStreamClient {
    bool connected;
    web_event_stream_send_fn sendFn;
    void *userData;
};

struct WebEventStreamFrame {
    bool used;
    uint32_t seq;
    char frame[kWebEventStreamMaxFrameLen];
    size_t frameLen;
};

struct WebEventStreamChannelState {
    WebEventStreamClient client;
    uint32_t nextSeq;
    size_t head;
    size_t count;
    WebEventStreamFrame buffer[kWebEventStreamBufferCapacity];
};

struct WebEventStream {
    WebEventStreamChannelState channels[kWebEventStreamChannelCount];
};

struct WebEventStreamReplayResult {
    size_t replayedCount;
    uint32_t latestSeq;
    uint32_t nextSeq;
    bool requiresSnapshotRecovery;
};

// ===== API =====

/**
 * Initialise all channels to disconnected state.
 */
void webEventStreamInit(WebEventStream *stream);

/**
 * Match path against the two event-stream endpoints.
 *
 * @param path     Request path to match.
 * @param channel  Populated with the matched channel when true is returned.
 * @return true if path matches either endpoint, false otherwise.
 */
bool webEventStreamMatchPath(const char *path, WebEventStreamChannel *channel);

/**
 * Register an active WebSocket client for a channel, replacing any previous one.
 * Implements the one-active-client-per-channel policy by disconnecting the prior
 * client (if any) before registering the new one.
 *
 * @param stream   Target stream.
 * @param channel  Channel to register the client on.
 * @param sendFn   Frame-send callback (must not be null).
 * @param userData Caller context forwarded to sendFn.
 */
void webEventStreamConnect(WebEventStream *stream,
                           WebEventStreamChannel channel,
                           web_event_stream_send_fn sendFn,
                           void *userData);

/**
 * Deregister the active client for a channel.
 */
void webEventStreamDisconnect(WebEventStream *stream, WebEventStreamChannel channel);

/**
 * Returns true when the given channel has an active client.
 */
bool webEventStreamHasClient(const WebEventStream *stream, WebEventStreamChannel channel);

/**
 * Return latest sequence for a channel, or 0 when no event has been published.
 */
uint32_t webEventStreamLatestSeq(const WebEventStream *stream, WebEventStreamChannel channel);

/**
 * Publish an input-event frame to the input channel.
 * No-op when no client is connected.
 *
 * @param stream   Target stream.
 * @param snapshot Input snapshot to serialise and send.
 * @param buf      Scratch buffer for serialisation.
 * @param bufSize  Size of buf.
 */
void webEventStreamPublishInput(WebEventStream *stream,
                                const WebWiimoteInputSnapshot &snapshot,
                                char *buf,
                                size_t bufSize);

/**
 * Publish a status-event frame to the status channel.
 * No-op when no client is connected.
 *
 * @param stream   Target stream.
 * @param snapshot Status snapshot to serialise and send.
 * @param buf      Scratch buffer for serialisation.
 * @param bufSize  Size of buf.
 */
void webEventStreamPublishStatus(WebEventStream *stream,
                                 const WebWiimoteStatusSnapshot &snapshot,
                                 char *buf,
                                 size_t bufSize);

/**
 * Replay events with seq > afterSeq to the provided callback.
 *
 * If afterSeq is older than the oldest buffered event, no frames are replayed
 * and requiresSnapshotRecovery is set true so caller can recover via REST
 * snapshot and then continue from nextSeq.
 */
bool webEventStreamReplaySince(const WebEventStream *stream,
                               WebEventStreamChannel channel,
                               uint32_t afterSeq,
                               web_event_stream_send_fn sendFn,
                               void *userData,
                               WebEventStreamReplayResult *result);

#endif  // WEB_EVENT_STREAM_H
