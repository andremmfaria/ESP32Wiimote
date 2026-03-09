// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "data_parser.h"
#include "../utils/serial_logging.h"

WiimoteDataParser::WiimoteDataParser(ButtonStateManager* buttonState, SensorStateManager* sensorState)
    : _buttonState(buttonState), _sensorState(sensorState), _filter(FILTER_NONE)
{
}

int WiimoteDataParser::parseData(void)
{
    int buttonChanged = false;
    int accelChanged = false;
    int nunchukStickChanged = false;

    // Check if TinyWiimote has data
    if (!TinyWiimoteAvailable())
        return 0;

    TinyWiimoteData rd = TinyWiimoteRead();

    if (rd.len < 4) {
        VERBOSE_PRINT("[DataParser] Data too short: len=%d\n", rd.len);
        return 0;
    }
    if (rd.data[0] != 0xA1) { // HID data report type
        VERBOSE_PRINT("[DataParser] Invalid report type: 0x%02x\n", rd.data[0]);
        return 0;
    }

    // Parse all data components
    parseButtonData(rd, buttonChanged);
    parseAccelData(rd, accelChanged);
    parseNunchukData(rd, nunchukStickChanged, accelChanged, buttonChanged);

    // Reset change flags and store current as previous
    _buttonState->resetChangeFlag();
    _sensorState->resetChangeFlags();

    return (buttonChanged | nunchukStickChanged | accelChanged);
}

void WiimoteDataParser::setFilter(int filter)
{
    _filter = filter;
}

int WiimoteDataParser::getFilter(void) const
{
    return _filter;
}

void WiimoteDataParser::parseButtonData(const TinyWiimoteData& data, int& buttonChanged)
{
    int offs = 0;

    // Check for button data in report
    if ((data.data[1] >= 0x30) && (data.data[1] <= 0x37)) {
        offs = 2;
        ButtonState buttonState = (ButtonState)((data.data[offs] << 8) | data.data[offs + 1]);
        _buttonState->update(buttonState);
    }

    // Check for button state change
    if (!(_filter & FILTER_BUTTON)) {
        if (_buttonState->hasChanged()) {
            buttonChanged = true;
        }
    }
}

void WiimoteDataParser::parseAccelData(const TinyWiimoteData& data, int& accelChanged)
{
    int offs = 0;

    // Determine accelerometer data offset based on report type
    switch (data.data[1]) {
        case 0x31: // Core Buttons and Accelerometer
        case 0x35: // Core Buttons and Accelerometer with 16 Extension Bytes
            offs = 4;
            break;
        default:
            offs = 0;
    }

    if (offs) {
        AccelState accelState;
        accelState.xAxis = data.data[offs + 0];
        accelState.yAxis = data.data[offs + 1];
        accelState.zAxis = data.data[offs + 2];
        _sensorState->updateAccel(accelState);

        if (!(_filter & FILTER_ACCEL)) {
            accelChanged = true;
        }
    } else {
        _sensorState->resetAccel();
    }
}

void WiimoteDataParser::parseNunchukData(const TinyWiimoteData& data, int& nunchukStickChanged, int& accelChanged, int& buttonChanged)
{
    int offs = 0;

    // Determine nunchuk/extension data offset based on report type
    switch (data.data[1]) {
        case 0x32: // Core Buttons with 8 Extension bytes
            offs = 4;
            break;
        case 0x35: // Core Buttons and Accelerometer with 16 Extension Bytes
            offs = 7;
            break;
        default:
            offs = 0;
    }

    if (offs) {
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
        if (cBtn)
            buttonState = (ButtonState)((int)buttonState | BUTTON_C);
        if (zBtn)
            buttonState = (ButtonState)((int)buttonState | BUTTON_Z);
        _buttonState->update(buttonState);

        // Check for nunchuk stick change
        if (!(_filter & FILTER_NUNCHUK_STICK)) {
            if (_sensorState->nunchukStickHasChanged()) {
                nunchukStickChanged = true;
            }
        }

        // Check for accel change when nunchuk is active
        if (!(_filter & FILTER_ACCEL)) {
            accelChanged = true;
        }
    } else {
        _sensorState->resetNunchuk();
    }
}
