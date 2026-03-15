#include "wiimote_extensions.h"

#include "../../utils/serial_logging.h"
#include "wiimote_protocol.h"
#include "wiimote_state.h"

#include <HardwareSerial.h>

#include <string.h>

namespace {
enum ControllerReportState {
    KReportStateInit = 0,
    KReportStateWaitAckOutReport,
    KReportStateWaitReadControllerType,
    KReportStateWaitReadResponse,
};
}  // namespace

WiimoteExtensions::WiimoteExtensions() : controllerReportState_(KReportStateInit) {}

void WiimoteExtensions::init(WiimoteState *state,
                             const L2capConnectionTable *connectionTable,
                             L2capPacketSender *sender) {
    state_ = state;
    connections_ = connectionTable;
    sender_ = sender;
    controllerReportState_ = KReportStateInit;
}

void WiimoteExtensions::handleReport(uint16_t ch, uint8_t *data, uint16_t len) {
    if (state_ == nullptr || connections_ == nullptr || sender_ == nullptr) {
        return;
    }

    WiimoteProtocol protocol;
    protocol.init(connections_, sender_);

    if (len < 8) {
        return;
    }

    switch (controllerReportState_) {
        case KReportStateInit:
            if (data[1] == 0x20) {
                uint8_t rawBattery = data[7];
                LOG_DEBUG("Wiimote: Status report 0x20 - Raw battery value: 0x%02x (%d)\n",
                          rawBattery, rawBattery);
                state_->setBatteryLevel(rawBattery);
                if ((data[4] & 0x02) != 0) {
                    LOG_INFO("Extension controller connected\n");
                    protocol.writeMemory(ch, WiimoteAddressSpace::ControlRegister, 0xA400F0,
                                         (const uint8_t[]){0x55}, 1);
                    controllerReportState_ = KReportStateWaitAckOutReport;
                } else {
                    LOG_INFO("Extension controller NOT connected\n");
                    state_->setNunchukConnected(false);
                    if (state_->getUseAccelerometer()) {
                        WiimoteReportingModeCommand reportingModeCommand = {0x31, false};
                        protocol.setReportingMode(ch, reportingModeCommand);
                    } else {
                        WiimoteReportingModeCommand reportingModeCommand = {0x30, false};
                        protocol.setReportingMode(ch, reportingModeCommand);
                    }
                }
            }
            break;

        case KReportStateWaitAckOutReport:
            if (len < 6) {
                break;
            }
            if ((data[1] == 0x22) && (data[4] == 0x16)) {
                if (data[5] == 0x00) {
                    protocol.writeMemory(ch, WiimoteAddressSpace::ControlRegister, 0xA400FB,
                                         (const uint8_t[]){0x00}, 1);
                    controllerReportState_ = KReportStateWaitReadControllerType;
                } else {
                    controllerReportState_ = KReportStateInit;
                }
            }
            break;

        case KReportStateWaitReadControllerType:
            if (len < 6) {
                break;
            }
            if ((data[1] == 0x22) && (data[4] == 0x16)) {
                if (data[5] == 0x00) {
                    protocol.readMemory(ch, WiimoteAddressSpace::ControlRegister, 0xA400FA, 6);
                    controllerReportState_ = KReportStateWaitReadResponse;
                } else {
                    controllerReportState_ = KReportStateInit;
                }
            }
            break;

        case KReportStateWaitReadResponse:
            if (len < 13) {
                break;
            }
            if (data[1] == 0x21) {
                if (memcmp(data + 5, (const uint8_t[]){0x00, 0xFA}, 2) == 0) {
                    if (memcmp(data + 7, (const uint8_t[]){0x00, 0x00, 0xA4, 0x20, 0x00, 0x00},
                               6) == 0) {
                        LOG_INFO("Nunchuk detected\n");
                        state_->setNunchukConnected(true);
                        if (state_->getUseAccelerometer()) {
                            WiimoteReportingModeCommand reportingModeCommand = {0x35, false};
                            protocol.setReportingMode(ch, reportingModeCommand);
                        } else {
                            WiimoteReportingModeCommand reportingModeCommand = {0x32, false};
                            protocol.setReportingMode(ch, reportingModeCommand);
                        }
                    }
                    controllerReportState_ = KReportStateInit;
                }
            }
            break;

        default:
            controllerReportState_ = KReportStateInit;
            break;
    }
}
