#include "wiimote_reports.h"
#include <string.h>

void wiimote_reports_init(WiimoteReports* reports) {
  if (reports == 0) {
    return;
  }
  reports->rb.cnt = 0;
  reports->rb.wp = 0;
  reports->rb.rp = 0;
}

void wiimote_reports_put(WiimoteReports* reports, uint8_t number, uint8_t* data, uint8_t len) {
  if (reports == 0 || data == 0) {
    return;
  }

  if (reports->rb.cnt < RECEIVED_DATA_MAX_NUM) {
    TinyWiimoteData* target = &(reports->data[reports->rb.wp]);
    if (len > RECEIVED_DATA_MAX_LEN) {
      len = RECEIVED_DATA_MAX_LEN;
    }
    memcpy(target->data, data, len);
    target->number = number;
    target->len = len;
    reports->rb.wp = (reports->rb.wp + 1) % RECEIVED_DATA_MAX_NUM;
    reports->rb.cnt++;
  }
}

int wiimote_reports_available(const WiimoteReports* reports) {
  if (reports == 0) {
    return 0;
  }
  return reports->rb.cnt;
}

TinyWiimoteData wiimote_reports_read(WiimoteReports* reports) {
  TinyWiimoteData target;
  target.number = 0;
  target.len = 0;

  if (reports == 0) {
    return target;
  }

  if (reports->rb.cnt > 0) {
    target = reports->data[reports->rb.rp];
    reports->rb.rp = (reports->rb.rp + 1) % RECEIVED_DATA_MAX_NUM;
    reports->rb.cnt--;
  }

  return target;
}
