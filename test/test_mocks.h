#ifndef __TEST_MOCKS_H__
#define __TEST_MOCKS_H__

#ifdef NATIVE_TEST

#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <cstdarg>

// Mock Arduino types and functions for native testing

// Basic types
typedef bool boolean;
typedef uint8_t byte;

// Mock delay function
inline void delay(unsigned long ms) {
    // No-op for native tests
    (void)ms;
}

// Mock millis function
inline unsigned long millis() {
    // Simple counter for testing
    static unsigned long counter = 0;
    return counter += 100;
}

// Mock Serial (simplified)
class MockSerial {
public:
    void begin(unsigned long baud) { (void)baud; }
    void println(const char* str) { printf("%s\n", str); }
    void print(const char* str) { printf("%s", str); }
    void printf(const char* format, ...) { 
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
};

extern MockSerial Serial;

#endif // NATIVE_TEST

#endif // __TEST_MOCKS_H__
