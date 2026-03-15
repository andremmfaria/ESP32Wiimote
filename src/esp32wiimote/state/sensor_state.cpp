// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "sensor_state.h"

SensorStateManager::SensorStateManager(int nunchukStickThreshold)
    : nunchukStickThreshold_(nunchukStickThreshold) {
    // Initialize accelerometer state
    currentAccel_.xAxis = 0;
    currentAccel_.yAxis = 0;
    currentAccel_.zAxis = 0;

    previousAccel_.xAxis = 0;
    previousAccel_.yAxis = 0;
    previousAccel_.zAxis = 0;

    // Initialize nunchuk state
    currentNunchuk_.xStick = 0;
    currentNunchuk_.yStick = 0;
    currentNunchuk_.xAxis = 0;
    currentNunchuk_.yAxis = 0;
    currentNunchuk_.zAxis = 0;

    previousNunchuk_.xStick = 0;
    previousNunchuk_.yStick = 0;
    previousNunchuk_.xAxis = 0;
    previousNunchuk_.yAxis = 0;
    previousNunchuk_.zAxis = 0;
}

void SensorStateManager::updateAccel(AccelState state) {
    currentAccel_ = state;
}

void SensorStateManager::updateNunchuk(NunchukState state) {
    currentNunchuk_ = state;
}

void SensorStateManager::resetAccel() {
    currentAccel_.xAxis = 0;
    currentAccel_.yAxis = 0;
    currentAccel_.zAxis = 0;
}

void SensorStateManager::resetNunchuk() {
    currentNunchuk_.xStick = 0;
    currentNunchuk_.yStick = 0;
    currentNunchuk_.xAxis = 0;
    currentNunchuk_.yAxis = 0;
    currentNunchuk_.zAxis = 0;
}

AccelState SensorStateManager::getAccel() const {
    return currentAccel_;
}

NunchukState SensorStateManager::getNunchuk() const {
    return currentNunchuk_;
}

AccelState SensorStateManager::getPreviousAccel() const {
    return previousAccel_;
}

NunchukState SensorStateManager::getPreviousNunchuk() const {
    return previousNunchuk_;
}

bool SensorStateManager::accelHasChanged() const {
    return (currentAccel_.xAxis != previousAccel_.xAxis) ||
           (currentAccel_.yAxis != previousAccel_.yAxis) ||
           (currentAccel_.zAxis != previousAccel_.zAxis);
}

bool SensorStateManager::nunchukStickHasChanged() const {
    int xDelta = (int)currentNunchuk_.xStick - previousNunchuk_.xStick;
    int yDelta = (int)currentNunchuk_.yStick - previousNunchuk_.yStick;
    int stickDelta = ((xDelta * xDelta) + (yDelta * yDelta));
    return stickDelta >= nunchukStickThreshold_;
}

void SensorStateManager::resetChangeFlags() {
    previousAccel_ = currentAccel_;
    previousNunchuk_ = currentNunchuk_;
}
