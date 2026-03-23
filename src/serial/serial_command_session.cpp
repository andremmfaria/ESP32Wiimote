#include "serial_command_session.h"

SerialCommandSession::SerialCommandSession()
    : unlockedAtMs_(0U), unlockDurationMs_(0U), unlocked_(false) {}

void SerialCommandSession::lock() {
    unlocked_ = false;
    unlockedAtMs_ = 0U;
    unlockDurationMs_ = 0U;
}

void SerialCommandSession::unlock(uint32_t durationMs, uint32_t nowMs) {
    if (durationMs == 0U) {
        lock();
        return;
    }

    unlocked_ = true;
    unlockedAtMs_ = nowMs;
    unlockDurationMs_ = durationMs;
}

bool SerialCommandSession::isUnlocked(uint32_t nowMs) const {
    if (!unlocked_) {
        return false;
    }

    const uint32_t kElapsedMs = nowMs - unlockedAtMs_;
    return kElapsedMs < unlockDurationMs_;
}
