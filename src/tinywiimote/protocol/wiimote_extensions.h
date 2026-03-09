#ifndef WIIMOTE_EXTENSIONS_H
#define WIIMOTE_EXTENSIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void wiimote_extensions_init(void);
void wiimote_extensions_handle_report(uint16_t ch, uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
