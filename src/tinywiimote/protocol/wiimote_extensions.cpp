#include "wiimote_extensions.h"

#include "../../utils/serial_logging.h"
#include "wiimote_protocol.h"
#include "wiimote_state.h"

#include <HardwareSerial.h>

#include <string.h>

WiimoteExtensions::WiimoteExtensions() = default;

static void setDefaultReportingMode(WiimoteProtocol *protocol, WiimoteState *state, uint16_t ch) {
    const uint8_t kMode = state->getUseAccelerometer() ? 0x31 : 0x30;
    WiimoteReportingModeCommand reportingModeCommand = {kMode, false};
    protocol->setReportingMode(ch, reportingModeCommand);
}

static void setNunchukReportingMode(WiimoteProtocol *protocol, WiimoteState *state, uint16_t ch) {
    const uint8_t kMode = state->getUseAccelerometer() ? 0x35 : 0x32;
    WiimoteReportingModeCommand reportingModeCommand = {kMode, false};
    protocol->setReportingMode(ch, reportingModeCommand);
}

void WiimoteExtensions::init(WiimoteState *state,
                             const L2capConnectionTable *connectionTable,
                             L2capPacketSender *sender) {
    state_ = state;
    connections_ = connectionTable;
    sender_ = sender;
    controllerReportState_ = ControllerReportState::Init;
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
        case ControllerReportState::Init:
            if (data[1] == 0x20) {
                uint8_t rawBattery = data[7];
                LOG_DEBUG("Wiimote: Status report 0x20 - Raw battery value: 0x%02x (%d)\n",
                          rawBattery, rawBattery);
                state_->setBatteryLevel(rawBattery);
                if ((data[4] & 0x02) != 0) {
                    LOG_INFO("Extension controller connected\n");
                    protocol.writeMemory(ch, WiimoteAddressSpace::ControlRegister, 0xA400F0,
                                         (const uint8_t[]){0x55}, 1);
                    controllerReportState_ = ControllerReportState::WaitAckOutReport;
                } else {
                    LOG_INFO("Extension controller NOT connected\n");
                    state_->setNunchukConnected(false);
                    setDefaultReportingMode(&protocol, state_, ch);
                }
            }
            break;

        case ControllerReportState::WaitAckOutReport:
            if (len < 6) {
                break;
            }
            if ((data[1] == 0x22) && (data[4] == 0x16)) {
                if (data[5] == 0x00) {
                    protocol.writeMemory(ch, WiimoteAddressSpace::ControlRegister, 0xA400FB,
                                         (const uint8_t[]){0x00}, 1);
                    controllerReportState_ = ControllerReportState::WaitReadControllerType;
                } else {
                    controllerReportState_ = ControllerReportState::Init;
                }
            }
            break;

        case ControllerReportState::WaitReadControllerType:
            if (len < 6) {
                break;
            }
            if ((data[1] == 0x22) && (data[4] == 0x16)) {
                if (data[5] == 0x00) {
                    protocol.readMemory(ch, WiimoteAddressSpace::ControlRegister, 0xA400FA, 6);
                    controllerReportState_ = ControllerReportState::WaitReadResponse;
                } else {
                    controllerReportState_ = ControllerReportState::Init;
                }
            }
            break;

        case ControllerReportState::WaitReadResponse:
            if (len < 13) {
                break;
            }
            if (data[1] == 0x21) {
                if (memcmp(data + 5, (const uint8_t[]){0x00, 0xFA}, 2) == 0) {
                    if (memcmp(data + 7, (const uint8_t[]){0x00, 0x00, 0xA4, 0x20, 0x00, 0x00},
                               6) == 0) {
                        LOG_INFO("Nunchuk detected\n");
                        state_->setNunchukConnected(true);
                        setNunchukReportingMode(&protocol, state_, ch);
                    }
                    controllerReportState_ = ControllerReportState::Init;
                }
            }
            break;

        default:
            controllerReportState_ = ControllerReportState::Init;
            break;
    }
}
