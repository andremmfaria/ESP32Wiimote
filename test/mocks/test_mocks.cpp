// Test mock implementations for native tests
// Provides TinyWiimote boundary stubs and mock state variables

#include "test_mocks.h"

// Mock state variables with external linkage
bool mockHasData = false;
TinyWiimoteData mockData = {0, {0}, 0};

uint8_t mockLastPacket[256] = {0};
int mockLastPacketLen = 0;
int mockSendCallCount = 0;
uint16_t mockLastChannelHandle = 0;
uint16_t mockLastRemoteCID = 0;

// TinyWiimote input boundary mocks
int tinyWiimoteAvailable(void) {
    return mockHasData ? 1 : 0;
}

TinyWiimoteData tinyWiimoteRead(void) {
    mockHasData = false;
    return mockData;
}
