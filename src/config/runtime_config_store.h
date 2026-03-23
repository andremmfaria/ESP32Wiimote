#ifndef RUNTIME_CONFIG_STORE_H
#define RUNTIME_CONFIG_STORE_H

#include <stdint.h>

struct RuntimeConfigSnapshot {
    bool autoReconnectEnabled;
    uint32_t fastReconnectTtlMs;
    uint8_t ledMask;
    uint8_t reportingMode;
    bool reportingContinuous;
};

class RuntimeConfigStore {
   public:
    RuntimeConfigStore();

    bool init();
    bool save(const RuntimeConfigSnapshot &snapshot) const;
    bool load(RuntimeConfigSnapshot *snapshot) const;
    bool clear() const;

   private:
    bool initialized_;
};

#endif  // RUNTIME_CONFIG_STORE_H
