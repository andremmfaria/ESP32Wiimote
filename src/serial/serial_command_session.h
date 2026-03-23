#ifndef ESP32_WIIMOTE_SERIAL_COMMAND_SESSION_H
#define ESP32_WIIMOTE_SERIAL_COMMAND_SESSION_H

#include <stdint.h>

class SerialCommandSession {
   public:
    SerialCommandSession();

    void lock();
    void unlock(uint32_t durationMs, uint32_t nowMs);
    bool isUnlocked(uint32_t nowMs) const;

   private:
    uint32_t unlockedAtMs_;
    uint32_t unlockDurationMs_;
    bool unlocked_;
};

#endif  // ESP32_WIIMOTE_SERIAL_COMMAND_SESSION_H
