// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "data_parser.h"

#include "../utils/protocol_codes.h"
#include "../utils/serial_logging.h"

WiimoteDataParser::WiimoteDataParser(ButtonStateManager *buttonState,
                                     SensorStateManager *sensorState)
    : _buttonState(buttonState), _sensorState(sensorState), _filter(FILTER_NONE) {}

int WiimoteDataParser::parseData() {
    ChangeFlags flags = {0, 0, 0};

    // Check if TinyWiimote has data
    if (TinyWiimoteAvailable() == 0) {
        return 0;
    }

    TinyWiimoteData rd = TinyWiimoteRead();

    if (rd.len < 4) {
        LOG_DEBUG("DataParser: Data too short: len=%d\n", rd.len);
        return 0;
    }
    if (rd.data[0] != (uint8_t)WiimoteHidPrefix::INPUT_REPORT) {
        LOG_DEBUG("DataParser: Invalid HID prefix: 0x%02x (%s)\n", rd.data[0],
                  wiimoteHidPrefixToString(rd.data[0]));
        return 0;
    }

    // Parse all data components
    parseButtonData(rd, flags.buttonChanged);
    parseAccelData(rd, flags.accelChanged);
    parseNunchukData(rd, flags);

    // Reset change flags and store current as previous
    _buttonState->resetChangeFlag();
    _sensorState->resetChangeFlags();

    return (flags.buttonChanged | flags.nunchukStickChanged | flags.accelChanged);
}

void WiimoteDataParser::setFilter(int filter) {
    _filter = filter;
}

int WiimoteDataParser::getFilter() const {
    return _filter;
}

void WiimoteDataParser::parseButtonData(const TinyWiimoteData &data, int &buttonChanged) {
    int offs = 0;

    // Check for button data in report
    if ((data.data[1] >= 0x30) && (data.data[1] <= 0x37)) {
        offs = 2;
        ButtonState buttonState = (ButtonState)((data.data[offs] << 8) | data.data[offs + 1]);
        _buttonState->update(buttonState);
    }

    // Check for button state change
    if ((_filter & FILTER_BUTTON) == 0) {
        if (_buttonState->hasChanged()) {
            buttonChanged = 1;
        }
    }
}

void WiimoteDataParser::parseAccelData(const TinyWiimoteData &data, int &accelChanged) {
    int offs = 0;

    // Determine accelerometer data offset based on report type
    switch (data.data[1]) {
        case 0x31:  // Core Buttons and Accelerometer
        case 0x35:  // Core Buttons and Accelerometer with 16 Extension Bytes
            offs = 4;
            break;
        default:
            offs = 0;
    }

    if (offs != 0) {
        AccelState accelState;
        accelState.xAxis = data.data[offs + 0];
        accelState.yAxis = data.data[offs + 1];
        accelState.zAxis = data.data[offs + 2];
        _sensorState->updateAccel(accelState);

        if ((_filter & FILTER_ACCEL) == 0) {
            accelChanged = 1;
        }
    } else {
        _sensorState->resetAccel();
    }
}

void WiimoteDataParser::parseNunchukData(const TinyWiimoteData &data, ChangeFlags &flags) {
    int offs = 0;

    // Determine nunchuk/extension data offset based on report type
    switch (data.data[1]) {
        case 0x32:  // Core Buttons with 8 Extension bytes
            offs = 4;
            break;
        case 0x35:  // Core Buttons and Accelerometer with 16 Extension Bytes
            offs = 7;
            break;
        default:
            offs = 0;
    }

    if (offs != 0) {
        NunchukState nunchukState;
        nunchukState.xStick = data.data[offs + 0];
        nunchukState.yStick = data.data[offs + 1];
        nunchukState.xAxis = data.data[offs + 2];
        nunchukState.yAxis = data.data[offs + 3];
        nunchukState.zAxis = data.data[offs + 4];

        _sensorState->updateNunchuk(nunchukState);

        // Parse nunchuk buttons
        uint8_t cBtn = ((data.data[offs + 5] & 0x02) >> 1) ^ 0x01;
        uint8_t zBtn = (data.data[offs + 5] & 0x01) ^ 0x01;

        // Add nunchuk buttons to button state
        ButtonState buttonState = _buttonState->getCurrent();
        if (cBtn != 0U) {
            buttonState = (ButtonState)((int)buttonState | BUTTON_C);
        }
        if (zBtn != 0U) {
            buttonState = (ButtonState)((int)buttonState | BUTTON_Z);
        }
        _buttonState->update(buttonState);

        // Check for nunchuk stick change
        if ((_filter & FILTER_NUNCHUK_STICK) == 0) {
            if (_sensorState->nunchukStickHasChanged()) {
                flags.nunchukStickChanged = 1;
            }
        }

        // Check for accel change when nunchuk is active
        if ((_filter & FILTER_ACCEL) == 0) {
            flags.accelChanged = 1;
        }
    } else {
        _sensorState->resetNunchuk();
    }
}
