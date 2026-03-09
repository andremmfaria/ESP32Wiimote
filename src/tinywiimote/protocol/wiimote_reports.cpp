#include "wiimote_reports.h"
#include <string.h>

#define RECIEVED_DATA_MAX_NUM (5)

struct recv_data_rb {
  uint8_t wp;
  uint8_t rp;
  uint8_t cnt;
};

static recv_data_rb receivedDataRb;
static TinyWiimoteData receivedData[RECIEVED_DATA_MAX_NUM];

void wiimote_reports_init(void) {
  receivedDataRb.cnt = 0;
  receivedDataRb.wp = 0;
  receivedDataRb.rp = 0;
}

void wiimote_reports_put(uint8_t number, uint8_t* data, uint8_t len) {
  if (receivedDataRb.cnt < RECIEVED_DATA_MAX_NUM) {
    TinyWiimoteData* target = &(receivedData[receivedDataRb.wp]);
    if (len > RECIEVED_DATA_MAX_LEN) {
      len = RECIEVED_DATA_MAX_LEN;
    }
    memcpy(target->data, data, len);
    target->number = number;
    target->len = len;
    receivedDataRb.wp = (receivedDataRb.wp + 1) % RECIEVED_DATA_MAX_NUM;
    receivedDataRb.cnt++;
  }
}

int wiimote_reports_available(void) {
  return receivedDataRb.cnt;
}

TinyWiimoteData wiimote_reports_read(void) {
  TinyWiimoteData target;
  target.number = 0;
  target.len = 0;

  if (receivedDataRb.cnt > 0) {
    target = receivedData[receivedDataRb.rp];
    receivedDataRb.rp = (receivedDataRb.rp + 1) % RECIEVED_DATA_MAX_NUM;
    receivedDataRb.cnt--;
  }

  return target;
}
