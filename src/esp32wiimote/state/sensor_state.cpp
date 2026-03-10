// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "sensor_state.h"

SensorStateManager::SensorStateManager(int nunchukStickThreshold)
    : _nunchukStickThreshold(nunchukStickThreshold) {
    // Initialize accelerometer state
    _currentAccel.xAxis = 0;
    _currentAccel.yAxis = 0;
    _currentAccel.zAxis = 0;

    _previousAccel.xAxis = 0;
    _previousAccel.yAxis = 0;
    _previousAccel.zAxis = 0;

    // Initialize nunchuk state
    _currentNunchuk.xStick = 0;
    _currentNunchuk.yStick = 0;
    _currentNunchuk.xAxis = 0;
    _currentNunchuk.yAxis = 0;
    _currentNunchuk.zAxis = 0;

    _previousNunchuk.xStick = 0;
    _previousNunchuk.yStick = 0;
    _previousNunchuk.xAxis = 0;
    _previousNunchuk.yAxis = 0;
    _previousNunchuk.zAxis = 0;
}

void SensorStateManager::updateAccel(AccelState state) {
    _currentAccel = state;
}

void SensorStateManager::updateNunchuk(NunchukState state) {
    _currentNunchuk = state;
}

void SensorStateManager::resetAccel() {
    _currentAccel.xAxis = 0;
    _currentAccel.yAxis = 0;
    _currentAccel.zAxis = 0;
}

void SensorStateManager::resetNunchuk() {
    _currentNunchuk.xStick = 0;
    _currentNunchuk.yStick = 0;
    _currentNunchuk.xAxis = 0;
    _currentNunchuk.yAxis = 0;
    _currentNunchuk.zAxis = 0;
}

AccelState SensorStateManager::getAccel() const {
    return _currentAccel;
}

NunchukState SensorStateManager::getNunchuk() const {
    return _currentNunchuk;
}

AccelState SensorStateManager::getPreviousAccel() const {
    return _previousAccel;
}

NunchukState SensorStateManager::getPreviousNunchuk() const {
    return _previousNunchuk;
}

bool SensorStateManager::accelHasChanged() const {
    return (_currentAccel.xAxis != _previousAccel.xAxis) ||
           (_currentAccel.yAxis != _previousAccel.yAxis) ||
           (_currentAccel.zAxis != _previousAccel.zAxis);
}

bool SensorStateManager::nunchukStickHasChanged() const {
    int xDelta = (int)_currentNunchuk.xStick - _previousNunchuk.xStick;
    int yDelta = (int)_currentNunchuk.yStick - _previousNunchuk.yStick;
    int stickDelta = ((xDelta * xDelta) + (yDelta * yDelta));
    return stickDelta >= _nunchukStickThreshold;
}

void SensorStateManager::resetChangeFlags() {
    _previousAccel = _currentAccel;
    _previousNunchuk = _currentNunchuk;
}
