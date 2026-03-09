#ifndef WIIMOTE_REPORTS_H
#define WIIMOTE_REPORTS_H

#include <stdint.h>

#define RECEIVED_DATA_MAX_LEN (50)

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint8_t number;
        uint8_t data[RECEIVED_DATA_MAX_LEN];
        uint8_t len;
    } TinyWiimoteData;

    typedef struct
    {
        uint8_t wp;
        uint8_t rp;
        uint8_t cnt;
    } WiimoteReportsRb;

#define RECEIVED_DATA_MAX_NUM (5)

    typedef struct
    {
        WiimoteReportsRb rb;
        TinyWiimoteData data[RECEIVED_DATA_MAX_NUM];
    } WiimoteReports;

    void wiimote_reports_init(WiimoteReports *reports);
    void wiimote_reports_put(WiimoteReports *reports, uint8_t number, uint8_t *data, uint8_t len);
    int wiimote_reports_available(const WiimoteReports *reports);
    TinyWiimoteData wiimote_reports_read(WiimoteReports *reports);

#ifdef __cplusplus
}
#endif

#endif
