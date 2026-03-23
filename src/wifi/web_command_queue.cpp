#include "web_command_queue.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {

bool copyBounded(char *dst, size_t dstSize, const char *src) {
    if (dst == nullptr || dstSize == 0U || src == nullptr) {
        return false;
    }

    int written = std::snprintf(dst, dstSize, "%s", src);
    return written >= 0 && static_cast<size_t>(written) < dstSize;
}

}  // namespace

void webCommandQueueInit(WebCommandQueue *queue) {
    if (queue == nullptr) {
        return;
    }

    std::memset(queue->entries, 0, sizeof(queue->entries));
    queue->nextId = 1U;
    queue->count = 0U;
}

bool webCommandQueueEnqueue(WebCommandQueue *queue,
                            const char *path,
                            const char *verb,
                            uint32_t *outCommandId) {
    if (queue == nullptr || path == nullptr || verb == nullptr || outCommandId == nullptr) {
        return false;
    }

    if (queue->count >= kWebCommandQueueCapacity) {
        return false;
    }

    WebCommandQueueEntry *begin = queue->entries;
    WebCommandQueueEntry *end = queue->entries + kWebCommandQueueCapacity;
    WebCommandQueueEntry *slot =
        std::find_if(begin, end, [](const WebCommandQueueEntry &entry) { return !entry.inUse; });
    if (slot == end) {
        return false;
    }

    if (!copyBounded(slot->path, sizeof(slot->path), path) ||
        !copyBounded(slot->verb, sizeof(slot->verb), verb)) {
        return false;
    }

    slot->inUse = true;
    slot->id = queue->nextId;
    slot->status = WebCommandQueueStatus::Queued;
    slot->result = WebCommandQueueResult::Pending;

    *outCommandId = slot->id;
    queue->nextId++;
    queue->count++;
    return true;
}

bool webCommandQueueGet(const WebCommandQueue *queue,
                        uint32_t commandId,
                        WebCommandQueueEntry *outEntry) {
    if (queue == nullptr || outEntry == nullptr || commandId == 0U) {
        return false;
    }

    const WebCommandQueueEntry *begin = queue->entries;
    const WebCommandQueueEntry *end = queue->entries + kWebCommandQueueCapacity;
    const WebCommandQueueEntry *kEntry =
        std::find_if(begin, end, [commandId](const WebCommandQueueEntry &value) {
            return value.inUse && value.id == commandId;
        });
    if (kEntry == end) {
        return false;
    }

    *outEntry = *kEntry;
    return true;
}

bool webCommandQueueUpdate(WebCommandQueue *queue,
                           uint32_t commandId,
                           WebCommandQueueStatus status,
                           WebCommandQueueResult result) {
    if (queue == nullptr || commandId == 0U) {
        return false;
    }

    WebCommandQueueEntry *begin = queue->entries;
    WebCommandQueueEntry *end = queue->entries + kWebCommandQueueCapacity;
    WebCommandQueueEntry *entry =
        std::find_if(begin, end, [commandId](const WebCommandQueueEntry &value) {
            return value.inUse && value.id == commandId;
        });
    if (entry == end) {
        return false;
    }

    entry->status = status;
    entry->result = result;
    return true;
}

size_t webCommandQueueCount(const WebCommandQueue *queue) {
    if (queue == nullptr) {
        return 0U;
    }
    return queue->count;
}
