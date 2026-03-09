#include "wiimote_reports.h"
#include <string.h>

WiimoteReports::WiimoteReports() {
  clear();
}

void WiimoteReports::clear() {
  cnt = 0;
  wp = 0;
  rp = 0;
}

void WiimoteReports::put(uint8_t number, const uint8_t* data, uint8_t len) {
  if (data == 0) {
    return;
  }

  if (cnt < RECEIVED_DATA_MAX_NUM) {
    TinyWiimoteData* target = &(reports[wp]);
    if (len > RECEIVED_DATA_MAX_LEN) {
      len = RECEIVED_DATA_MAX_LEN;
    }
    memcpy(target->data, data, len);
    target->number = number;
    target->len = len;
    wp = (wp + 1) % RECEIVED_DATA_MAX_NUM;
    cnt++;
  }
}

int WiimoteReports::available() const {
  return cnt;
}

TinyWiimoteData WiimoteReports::read() {
  TinyWiimoteData target;
  target.number = 0;
  target.len = 0;

  if (cnt > 0) {
    target = reports[rp];
    rp = (rp + 1) % RECEIVED_DATA_MAX_NUM;
    cnt--;
  }

  return target;
}
