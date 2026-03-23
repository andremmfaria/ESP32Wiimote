#ifndef WEB_COMMAND_QUEUE_H
#define WEB_COMMAND_QUEUE_H

#include <cstddef>
#include <cstdint>

static constexpr size_t kWebCommandQueueCapacity = 8U;
static constexpr size_t kWebCommandQueuePathMaxLen = 64U;
static constexpr size_t kWebCommandQueueVerbMaxLen = 64U;

enum class WebCommandQueueStatus : uint8_t {
    Queued = 0U,
    Running = 1U,
    Completed = 2U,
};

enum class WebCommandQueueResult : uint8_t {
    Pending = 0U,
    Accepted = 1U,
    Rejected = 2U,
};

struct WebCommandQueueEntry {
    bool inUse;
    uint32_t id;
    WebCommandQueueStatus status;
    WebCommandQueueResult result;
    char path[kWebCommandQueuePathMaxLen];
    char verb[kWebCommandQueueVerbMaxLen];
};

struct WebCommandQueue {
    WebCommandQueueEntry entries[kWebCommandQueueCapacity];
    uint32_t nextId;
    size_t count;

    WebCommandQueue() : nextId(1U), count(0U) {}
};

void webCommandQueueInit(WebCommandQueue *queue);
bool webCommandQueueEnqueue(WebCommandQueue *queue,
                            const char *path,
                            const char *verb,
                            uint32_t *outCommandId);
bool webCommandQueueGet(const WebCommandQueue *queue,
                        uint32_t commandId,
                        WebCommandQueueEntry *outEntry);
bool webCommandQueueUpdate(WebCommandQueue *queue,
                           uint32_t commandId,
                           WebCommandQueueStatus status,
                           WebCommandQueueResult result);
size_t webCommandQueueCount(const WebCommandQueue *queue);

#endif  // WEB_COMMAND_QUEUE_H
