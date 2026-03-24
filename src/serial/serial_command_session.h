#ifndef ESP32_WIIMOTE_SERIAL_COMMAND_SESSION_H
#define ESP32_WIIMOTE_SERIAL_COMMAND_SESSION_H

#include <stdint.h>

class SerialCommandSession {
   public:
    SerialCommandSession();

    void setToken(const char *token);
    bool validateToken(const char *token) const;
    void lock();
    void unlock(uint32_t durationMs, uint32_t nowMs);
    bool isUnlocked(uint32_t nowMs) const;

   private:
    const char *token_ = nullptr;
    uint32_t unlockedAtMs_ = 0U;
    uint32_t unlockDurationMs_ = 0U;
    bool unlocked_ = false;
};

#endif  // ESP32_WIIMOTE_SERIAL_COMMAND_SESSION_H
