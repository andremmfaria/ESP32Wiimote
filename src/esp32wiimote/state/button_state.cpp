// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "button_state.h"

ButtonStateManager::ButtonStateManager() : current_(NoButton), previous_(NoButton) {}

void ButtonStateManager::update(ButtonState currentState) {
    current_ = currentState;
}

ButtonState ButtonStateManager::getCurrent() const {
    return current_;
}

ButtonState ButtonStateManager::getPrevious() const {
    return previous_;
}

bool ButtonStateManager::hasChanged() const {
    return current_ != previous_;
}

void ButtonStateManager::resetChangeFlag() {
    previous_ = current_;
}
