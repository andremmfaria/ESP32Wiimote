#ifndef WIIMOTE_REPORTS_H
#define WIIMOTE_REPORTS_H

#include <stdint.h>

#define RECIEVED_DATA_MAX_LEN (50)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t number;
  uint8_t data[RECIEVED_DATA_MAX_LEN];
  uint8_t len;
} TinyWiimoteData;

void wiimote_reports_init(void);
void wiimote_reports_put(uint8_t number, uint8_t* data, uint8_t len);
int wiimote_reports_available(void);
TinyWiimoteData wiimote_reports_read(void);

#ifdef __cplusplus
}
#endif

#endif
