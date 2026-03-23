#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t byte;
typedef bool boolean;

extern size_t mockEspFreeHeap;
extern unsigned long mockMillis;
inline void mockSetMillis(unsigned long nowMs) {
    mockMillis = nowMs;
}

static constexpr size_t kMockSerialInputBufferSize = 512U;
static constexpr size_t kMockSerialOutputBufferSize = 4096U;

extern char mockSerialInputBuffer[kMockSerialInputBufferSize];
extern size_t mockSerialInputLen;
extern size_t mockSerialInputPos;

extern char mockSerialOutputBuffer[kMockSerialOutputBufferSize];
extern size_t mockSerialOutputLen;

inline void mockSerialClearInput() {
    mockSerialInputLen = 0U;
    mockSerialInputPos = 0U;
    mockSerialInputBuffer[0] = '\0';
}

inline void mockSerialSetInput(const char *input) {
    mockSerialClearInput();
    if (input == nullptr) {
        return;
    }

    const size_t kInputLen = strlen(input);
    const size_t kCopyLen = (kInputLen < (kMockSerialInputBufferSize - 1U))
                                ? kInputLen
                                : (kMockSerialInputBufferSize - 1U);
    memcpy(mockSerialInputBuffer, input, kCopyLen);
    mockSerialInputBuffer[kCopyLen] = '\0';
    mockSerialInputLen = kCopyLen;
}

inline void mockSerialClearOutput() {
    mockSerialOutputLen = 0U;
    mockSerialOutputBuffer[0] = '\0';
}

inline const char *mockSerialGetOutput() {
    if (mockSerialOutputLen >= kMockSerialOutputBufferSize) {
        mockSerialOutputBuffer[kMockSerialOutputBufferSize - 1U] = '\0';
    } else {
        mockSerialOutputBuffer[mockSerialOutputLen] = '\0';
    }
    return mockSerialOutputBuffer;
}

inline void mockSerialAppend(const char *str) {
    if (str == nullptr) {
        return;
    }

    const size_t kLen = strlen(str);
    const size_t kRemaining = (mockSerialOutputLen < (kMockSerialOutputBufferSize - 1U))
                                  ? (kMockSerialOutputBufferSize - 1U - mockSerialOutputLen)
                                  : 0U;
    const size_t kCopyLen = (kLen < kRemaining) ? kLen : kRemaining;
    if (kCopyLen > 0U) {
        memcpy(mockSerialOutputBuffer + mockSerialOutputLen, str, kCopyLen);
        mockSerialOutputLen += kCopyLen;
        mockSerialOutputBuffer[mockSerialOutputLen] = '\0';
    }
}

class MockSerial {
   public:
    inline void begin(unsigned long baud) { (void)baud; }

    inline int available() const {
        if (mockSerialInputPos >= mockSerialInputLen) {
            return 0;
        }
        return static_cast<int>(mockSerialInputLen - mockSerialInputPos);
    }

    inline int read() {
        if (mockSerialInputPos >= mockSerialInputLen) {
            return -1;
        }
        return static_cast<unsigned char>(mockSerialInputBuffer[mockSerialInputPos++]);
    }

    inline void print(const char *str) {
        mockSerialAppend(str);
        if (str != nullptr) {
            fputs(str, stdout);
        }
    }

    inline void println(const char *str) {
        mockSerialAppend(str);
        mockSerialAppend("\n");
        if (str != nullptr) {
            fputs(str, stdout);
        }
        fputc('\n', stdout);
    }

    inline int printf(const char *format, ...) {
        va_list args;
        va_start(args, format);
        const int kWritten = vfprintf(stdout, format, args);
        va_end(args);
        return kWritten;
    }
};

static MockSerial Serial;

inline void delay(unsigned long ms) {
    (void)ms;
}

inline unsigned long millis() {
    return mockMillis;
}

class MockEspClass {
   public:
    size_t getFreeHeap() const { return mockEspFreeHeap; }
};

extern MockEspClass ESP;

#endif  // ARDUINO_H
