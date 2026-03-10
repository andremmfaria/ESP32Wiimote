// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "button_state.h"

ButtonStateManager::ButtonStateManager() : _current(NO_BUTTON), _previous(NO_BUTTON) {}

void ButtonStateManager::update(ButtonState currentState) {
    _current = currentState;
}

ButtonState ButtonStateManager::getCurrent(void) const {
    return _current;
}

ButtonState ButtonStateManager::getPrevious(void) const {
    return _previous;
}

bool ButtonStateManager::hasChanged(void) const {
    return _current != _previous;
}

void ButtonStateManager::resetChangeFlag(void) {
    _previous = _current;
}
