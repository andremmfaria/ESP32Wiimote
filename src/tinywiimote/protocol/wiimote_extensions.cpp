#include "wiimote_extensions.h"

#include "../../utils/serial_logging.h"
#include "wiimote_protocol.h"
#include "wiimote_state.h"

#include <HardwareSerial.h>

#include <string.h>

enum {
    REPORT_STATE_INIT = 0,
    REPORT_STATE_WAIT_ACK_OUT_REPORT,
    REPORT_STATE_WAIT_READ_COTRLLER_TYPE,
    REPORT_STATE_WAIT_READ_RESPONSE,
};

WiimoteExtensions::WiimoteExtensions()
    : controllerReportState(REPORT_STATE_INIT)
    , state(nullptr)
    , connections(nullptr)
    , sender(nullptr) {}

void WiimoteExtensions::init(WiimoteState *wiimoteState,
                             const L2capConnectionTable *connectionTable,
                             L2capPacketSender *packetSender) {
    state = wiimoteState;
    connections = connectionTable;
    sender = packetSender;
    controllerReportState = REPORT_STATE_INIT;
}

void WiimoteExtensions::handleReport(uint16_t ch, uint8_t *data, uint16_t len) {
    if (state == nullptr || connections == nullptr || sender == nullptr) {
        return;
    }

    WiimoteProtocol protocol;
    protocol.init(connections, sender);

    if (len < 8) {
        return;
    }

    switch (controllerReportState) {
        case REPORT_STATE_INIT:
            if (data[1] == 0x20) {
                uint8_t rawBattery = data[7];
                LOG_DEBUG("Wiimote: Status report 0x20 - Raw battery value: 0x%02x (%d)\n",
                          rawBattery, rawBattery);
                state->setBatteryLevel(rawBattery);
                if ((data[4] & 0x02) != 0) {
                    LOG_INFO("Extension controller connected\n");
                    protocol.writeMemory(ch, CONTROL_REGISTER, 0xA400F0, (const uint8_t[]){0x55},
                                         1);
                    controllerReportState = REPORT_STATE_WAIT_ACK_OUT_REPORT;
                } else {
                    LOG_INFO("Extension controller NOT connected\n");
                    state->setNunchukConnected(false);
                    if (state->getUseAccelerometer()) {
                        protocol.setReportingMode(ch, 0x31, false);
                    } else {
                        protocol.setReportingMode(ch, 0x30, false);
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
                    protocol.writeMemory(ch, CONTROL_REGISTER, 0xA400FB, (const uint8_t[]){0x00},
                                         1);
                    controllerReportState = REPORT_STATE_WAIT_READ_COTRLLER_TYPE;
                } else {
                    controllerReportState = REPORT_STATE_INIT;
                }
            }
            break;

        case REPORT_STATE_WAIT_READ_COTRLLER_TYPE:
            if (len < 6) {
                break;
            }
            if ((data[1] == 0x22) && (data[4] == 0x16)) {
                if (data[5] == 0x00) {
                    protocol.readMemory(ch, CONTROL_REGISTER, 0xA400FA, 6);
                    controllerReportState = REPORT_STATE_WAIT_READ_RESPONSE;
                } else {
                    controllerReportState = REPORT_STATE_INIT;
                }
            }
            break;

        case REPORT_STATE_WAIT_READ_RESPONSE:
            if (len < 13) {
                break;
            }
            if (data[1] == 0x21) {
                if (memcmp(data + 5, (const uint8_t[]){0x00, 0xFA}, 2) == 0) {
                    if (memcmp(data + 7, (const uint8_t[]){0x00, 0x00, 0xA4, 0x20, 0x00, 0x00},
                               6) == 0) {
                        LOG_INFO("Nunchuk detected\n");
                        state->setNunchukConnected(true);
                        if (state->getUseAccelerometer()) {
                            protocol.setReportingMode(ch, 0x35, false);
                        } else {
                            protocol.setReportingMode(ch, 0x32, false);
                        }
                    }
                    controllerReportState = REPORT_STATE_INIT;
                }
            }
            break;

        default:
            controllerReportState = REPORT_STATE_INIT;
            break;
    }
}
