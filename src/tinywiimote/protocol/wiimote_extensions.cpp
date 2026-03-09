#include "wiimote_extensions.h"
#include "wiimote_protocol.h"
#include "wiimote_state.h"
#include <HardwareSerial.h>
#include <string.h>

#define UNVERBOSE_PRINT(...) Serial.printf(__VA_ARGS__)

enum {
  REPORT_STATE_INIT = 0,
  REPORT_STATE_WAIT_ACK_OUT_REPORT,
  REPORT_STATE_WAIT_READ_COTRLLER_TYPE,
  REPORT_STATE_WAIT_READ_RESPONSE,
};

void wiimote_extensions_init(WiimoteExtensions* extensions, WiimoteState* state,
                             const L2capConnectionTable* connections,
                             L2capPacketSender* sender) {
  if (extensions == 0) {
    return;
  }
  extensions->state = state;
  extensions->connections = connections;
  extensions->sender = sender;
  extensions->controllerReportState = REPORT_STATE_INIT;
}

void wiimote_extensions_handle_report(WiimoteExtensions* extensions, uint16_t ch,
                                      uint8_t* data, uint16_t len) {
  if (extensions == 0 || extensions->state == 0 || extensions->connections == 0 ||
      extensions->sender == 0) {
    return;
  }

  if (len < 8) {
    return;
  }

  switch (extensions->controllerReportState) {
    case REPORT_STATE_INIT:
      if (data[1] == 0x20) {
        wiimote_state_set_battery_level(extensions->state, data[7]);
        if (data[4] & 0x02) {
          UNVERBOSE_PRINT("Extension controller connected\n");
          wiimote_write_memory(extensions->connections, extensions->sender,
                               ch, CONTROL_REGISTER, 0xA400F0,
                               (const uint8_t[]){0x55}, 1);
          extensions->controllerReportState = REPORT_STATE_WAIT_ACK_OUT_REPORT;
        } else {
          UNVERBOSE_PRINT("Extension controller NOT connected\n");
          wiimote_state_set_nunchuk_connected(extensions->state, false);
          if (wiimote_state_get_use_accelerometer(extensions->state)) {
            wiimote_set_reporting_mode(extensions->connections, extensions->sender,
                                       ch, 0x31, false);
          } else {
            wiimote_set_reporting_mode(extensions->connections, extensions->sender,
                                       ch, 0x30, false);
          }
        }
      }
      break;

    case REPORT_STATE_WAIT_ACK_OUT_REPORT:
      if (len < 6) {
        break;
      }
      if ((data[1] == 0x22) && (data[4] == 0x16)) {
        if (data[5] == 0x00) {
          wiimote_write_memory(extensions->connections, extensions->sender,
                               ch, CONTROL_REGISTER, 0xA400FB,
                               (const uint8_t[]){0x00}, 1);
          extensions->controllerReportState = REPORT_STATE_WAIT_READ_COTRLLER_TYPE;
        } else {
          extensions->controllerReportState = REPORT_STATE_INIT;
        }
      }
      break;

    case REPORT_STATE_WAIT_READ_COTRLLER_TYPE:
      if (len < 6) {
        break;
      }
      if ((data[1] == 0x22) && (data[4] == 0x16)) {
        if (data[5] == 0x00) {
          wiimote_read_memory(extensions->connections, extensions->sender,
                              ch, CONTROL_REGISTER, 0xA400FA, 6);
          extensions->controllerReportState = REPORT_STATE_WAIT_READ_RESPONSE;
        } else {
          extensions->controllerReportState = REPORT_STATE_INIT;
        }
      }
      break;

    case REPORT_STATE_WAIT_READ_RESPONSE:
      if (len < 13) {
        break;
      }
      if (data[1] == 0x21) {
        if (memcmp(data + 5, (const uint8_t[]){0x00, 0xFA}, 2) == 0) {
          if (memcmp(data + 7, (const uint8_t[]){0x00, 0x00, 0xA4, 0x20, 0x00, 0x00}, 6) == 0) {
            UNVERBOSE_PRINT("Nunchuk detected\n");
            wiimote_state_set_nunchuk_connected(extensions->state, true);
            if (wiimote_state_get_use_accelerometer(extensions->state)) {
              wiimote_set_reporting_mode(extensions->connections, extensions->sender,
                                         ch, 0x35, false);
            } else {
              wiimote_set_reporting_mode(extensions->connections, extensions->sender,
                                         ch, 0x32, false);
            }
          }
          extensions->controllerReportState = REPORT_STATE_INIT;
        }
      }
      break;

    default:
      extensions->controllerReportState = REPORT_STATE_INIT;
      break;
  }
}
