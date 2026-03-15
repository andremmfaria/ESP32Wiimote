#ifndef WIIMOTE_REPORTS_H
#define WIIMOTE_REPORTS_H

#include <stdint.h>

#define RECEIVED_DATA_MAX_LEN (50)

struct TinyWiimoteData {
    uint8_t number;
    uint8_t data[RECEIVED_DATA_MAX_LEN];
    uint8_t len;
};

#define RECEIVED_DATA_MAX_NUM (5)

class WiimoteReports {
   public:
    WiimoteReports();

    void clear();
    void put(uint8_t number, const uint8_t *data, uint8_t len);
    int available() const;
    TinyWiimoteData read();

   private:
    uint8_t writeIndex_{0};
    uint8_t readIndex_{0};
    uint8_t itemCount_{0};
    TinyWiimoteData reportBuffer_[RECEIVED_DATA_MAX_NUM];
};

#endif
