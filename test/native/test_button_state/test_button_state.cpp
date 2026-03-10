#include <unity.h>
#ifdef NATIVE_TEST
// For native tests, delay/millis are not needed
#define delay(x) ((void)0)
#define millis() (0UL)
#else
#include <Arduino.h>
#endif
#include "esp32wiimote/state/button_state.h"

// Test fixtures
ButtonStateManager *buttonState;

void setUp(void) {
    buttonState = new ButtonStateManager();
}

void tearDown(void) {
    delete buttonState;
}

// Test: Initial state should be NO_BUTTON
void test_initial_state_is_no_button() {
    TEST_ASSERT_EQUAL(NO_BUTTON, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(NO_BUTTON, buttonState->getPrevious());
    TEST_ASSERT_FALSE(buttonState->hasChanged());
}

// Test: Update changes current state
void test_update_changes_current_state() {
    buttonState->update(BUTTON_A);
    TEST_ASSERT_EQUAL(BUTTON_A, buttonState->getCurrent());
}

// Test: Update sets changed flag
void test_update_sets_changed_flag() {
    buttonState->update(BUTTON_A);
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Update preserves previous state
void test_update_preserves_previous_state() {
    buttonState->update(BUTTON_A);
    buttonState->resetChangeFlag();
    buttonState->update(BUTTON_B);

    TEST_ASSERT_EQUAL(BUTTON_B, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(BUTTON_A, buttonState->getPrevious());
}

// Test: Update with same state
void test_update_with_same_state() {
    buttonState->update(BUTTON_A);
    buttonState->resetChangeFlag();
    buttonState->update(BUTTON_A);

    // Should not set changed flag if state is identical
    TEST_ASSERT_EQUAL(BUTTON_A, buttonState->getCurrent());
}

// Test: Reset change flag
void test_reset_change_flag() {
    buttonState->update(BUTTON_HOME);
    TEST_ASSERT_TRUE(buttonState->hasChanged());

    buttonState->resetChangeFlag();
    TEST_ASSERT_FALSE(buttonState->hasChanged());
}

// Test: Multiple button combination
void test_multiple_buttons_combination() {
    ButtonState combo = (ButtonState)(BUTTON_A | BUTTON_B | BUTTON_ONE);
    buttonState->update(combo);

    ButtonState current = buttonState->getCurrent();
    TEST_ASSERT_EQUAL(combo, current);
    TEST_ASSERT_TRUE(current & BUTTON_A);
    TEST_ASSERT_TRUE(current & BUTTON_B);
    TEST_ASSERT_TRUE(current & BUTTON_ONE);
}

// Test: Nunchuk buttons
void test_nunchuk_buttons() {
    buttonState->update(BUTTON_Z);
    TEST_ASSERT_EQUAL(BUTTON_Z, buttonState->getCurrent());

    buttonState->resetChangeFlag();
    buttonState->update(BUTTON_C);
    TEST_ASSERT_EQUAL(BUTTON_C, buttonState->getCurrent());
}

// Test: D-pad buttons
void test_dpad_buttons() {
    buttonState->update(BUTTON_UP);
    TEST_ASSERT_EQUAL(BUTTON_UP, buttonState->getCurrent());

    buttonState->resetChangeFlag();
    buttonState->update(BUTTON_RIGHT);
    TEST_ASSERT_EQUAL(BUTTON_RIGHT, buttonState->getCurrent());
}

// Test: Transition from no button to button
void test_transition_none_to_button() {
    TEST_ASSERT_EQUAL(NO_BUTTON, buttonState->getCurrent());

    buttonState->update(BUTTON_PLUS);
    TEST_ASSERT_EQUAL(BUTTON_PLUS, buttonState->getCurrent());
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Transition from button to no button
void test_transition_button_to_none() {
    buttonState->update(BUTTON_MINUS);
    buttonState->resetChangeFlag();

    buttonState->update(NO_BUTTON);
    TEST_ASSERT_EQUAL(NO_BUTTON, buttonState->getCurrent());
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Rapid state changes
void test_rapid_state_changes() {
    buttonState->update(BUTTON_A);
    buttonState->resetChangeFlag();

    buttonState->update(BUTTON_B);
    buttonState->resetChangeFlag();

    buttonState->update(BUTTON_ONE);
    buttonState->resetChangeFlag();

    buttonState->update(BUTTON_TWO);

    TEST_ASSERT_EQUAL(BUTTON_TWO, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(BUTTON_ONE, buttonState->getPrevious());
}

// Test: All individual buttons
void test_all_individual_buttons() {
    ButtonState buttons[] = {BUTTON_TWO,  BUTTON_ONE,  BUTTON_B,     BUTTON_A,    BUTTON_MINUS,
                             BUTTON_HOME, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_DOWN, BUTTON_UP,
                             BUTTON_PLUS, BUTTON_C,    BUTTON_Z};

    for (int i = 0; i < 13; i++) {
        buttonState->update(buttons[i]);
        TEST_ASSERT_EQUAL(buttons[i], buttonState->getCurrent());
        buttonState->resetChangeFlag();
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_initial_state_is_no_button);
    RUN_TEST(test_update_changes_current_state);
    RUN_TEST(test_update_sets_changed_flag);
    RUN_TEST(test_update_preserves_previous_state);
    RUN_TEST(test_update_with_same_state);
    RUN_TEST(test_reset_change_flag);
    RUN_TEST(test_multiple_buttons_combination);
    RUN_TEST(test_nunchuk_buttons);
    RUN_TEST(test_dpad_buttons);
    RUN_TEST(test_transition_none_to_button);
    RUN_TEST(test_transition_button_to_none);
    RUN_TEST(test_rapid_state_changes);
    RUN_TEST(test_all_individual_buttons);

    return UNITY_END();
}

#ifdef ARDUINO
void setup() {
    delay(2000);  // Wait for serial
    UNITY_BEGIN();

    RUN_TEST(test_initial_state_is_no_button);
    RUN_TEST(test_update_changes_current_state);
    RUN_TEST(test_update_sets_changed_flag);
    RUN_TEST(test_update_preserves_previous_state);
    RUN_TEST(test_update_with_same_state);
    RUN_TEST(test_reset_change_flag);
    RUN_TEST(test_multiple_buttons_combination);
    RUN_TEST(test_nunchuk_buttons);
    RUN_TEST(test_dpad_buttons);
    RUN_TEST(test_transition_none_to_button);
    RUN_TEST(test_transition_button_to_none);
    RUN_TEST(test_rapid_state_changes);
    RUN_TEST(test_all_individual_buttons);

    UNITY_END();
}

void loop() {
    // Nothing here
}
#endif
