#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t byte;
typedef bool boolean;

extern size_t mockEspFreeHeap;

class MockSerial {
   public:
    inline void begin(unsigned long baud) { (void)baud; }

    inline void print(const char *str) {
        if (str != nullptr) {
            fputs(str, stdout);
        }
    }

    inline void println(const char *str) {
        if (str != nullptr) {
            fputs(str, stdout);
        }
        fputc('\n', stdout);
    }

    inline int printf(const char *format, ...) {
        va_list args;
        va_start(args, format);
        const int written = vfprintf(stdout, format, args);
        va_end(args);
        return written;
    }
};

static MockSerial Serial;

inline void delay(unsigned long ms) {
    (void)ms;
}

inline unsigned long millis() {
    return 0UL;
}

class MockEspClass {
   public:
    size_t getFreeHeap() const { return mockEspFreeHeap; }
};

extern MockEspClass ESP;

#endif  // ARDUINO_H
