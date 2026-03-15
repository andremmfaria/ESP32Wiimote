#include "wiimote_reports.h"

#include <algorithm>
#include <string.h>

WiimoteReports::WiimoteReports() {
    clear();
}

void WiimoteReports::clear() {
    itemCount_ = 0;
    writeIndex_ = 0;
    readIndex_ = 0;
}

void WiimoteReports::put(uint8_t number, const uint8_t *data, uint8_t len) {
    if (data == nullptr || itemCount_ >= RECEIVED_DATA_MAX_NUM) {
        return;
    }

    const uint8_t kBoundedLen = std::min<uint8_t>(len, RECEIVED_DATA_MAX_LEN);
    TinyWiimoteData *target = &reportBuffer_[writeIndex_];
    memcpy(target->data, data, kBoundedLen);
    target->number = number;
    target->len = kBoundedLen;

    writeIndex_ = (writeIndex_ + 1) % RECEIVED_DATA_MAX_NUM;
    itemCount_++;
}

int WiimoteReports::available() const {
    return itemCount_;
}

TinyWiimoteData WiimoteReports::read() {
    TinyWiimoteData target = {};

    if (itemCount_ == 0) {
        return target;
    }

    target = reportBuffer_[readIndex_];
    readIndex_ = (readIndex_ + 1) % RECEIVED_DATA_MAX_NUM;
    itemCount_--;

    return target;
}
