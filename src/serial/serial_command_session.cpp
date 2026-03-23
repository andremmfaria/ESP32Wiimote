#include "serial_command_session.h"

#include <cstring>

SerialCommandSession::SerialCommandSession() = default;

void SerialCommandSession::setCredentials(const WiimoteCredentials *credentials) {
    credentials_ = credentials;
}

bool SerialCommandSession::validateCredentials(const char *username, const char *password) const {
    if (credentials_ == nullptr) {
        return true;
    }
    if (username == nullptr || password == nullptr || credentials_->username == nullptr ||
        credentials_->password == nullptr) {
        return false;
    }

    return std::strcmp(username, credentials_->username) == 0 &&
           std::strcmp(password, credentials_->password) == 0;
}

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
