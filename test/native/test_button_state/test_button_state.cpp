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

// Test: Initial state should be NoButton
void testInitialStateIsNoButton() {
    TEST_ASSERT_EQUAL(NoButton, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(NoButton, buttonState->getPrevious());
    TEST_ASSERT_FALSE(buttonState->hasChanged());
}

// Test: Update changes current state
void testUpdateChangesCurrentState() {
    buttonState->update(ButtonA);
    TEST_ASSERT_EQUAL(ButtonA, buttonState->getCurrent());
}

// Test: Update sets changed flag
void testUpdateSetsChangedFlag() {
    buttonState->update(ButtonA);
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Update preserves previous state
void testUpdatePreservesPreviousState() {
    buttonState->update(ButtonA);
    buttonState->resetChangeFlag();
    buttonState->update(ButtonB);

    TEST_ASSERT_EQUAL(ButtonB, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(ButtonA, buttonState->getPrevious());
}

// Test: Update with same state
void testUpdateWithSameState() {
    buttonState->update(ButtonA);
    buttonState->resetChangeFlag();
    buttonState->update(ButtonA);

    // Should not set changed flag if state is identical
    TEST_ASSERT_EQUAL(ButtonA, buttonState->getCurrent());
}

// Test: Reset change flag
void testResetChangeFlag() {
    buttonState->update(ButtonHome);
    TEST_ASSERT_TRUE(buttonState->hasChanged());

    buttonState->resetChangeFlag();
    TEST_ASSERT_FALSE(buttonState->hasChanged());
}

// Test: Multiple button combination
void testMultipleButtonsCombination() {
    ButtonState combo = (ButtonState)(ButtonA | ButtonB | ButtonOne);
    buttonState->update(combo);

    ButtonState current = buttonState->getCurrent();
    TEST_ASSERT_EQUAL(combo, current);
    TEST_ASSERT_TRUE(current & ButtonA);
    TEST_ASSERT_TRUE(current & ButtonB);
    TEST_ASSERT_TRUE(current & ButtonOne);
}

// Test: Nunchuk buttons
void testNunchukButtons() {
    buttonState->update(ButtonZ);
    TEST_ASSERT_EQUAL(ButtonZ, buttonState->getCurrent());

    buttonState->resetChangeFlag();
    buttonState->update(ButtonC);
    TEST_ASSERT_EQUAL(ButtonC, buttonState->getCurrent());
}

// Test: D-pad buttons
void testDpadButtons() {
    buttonState->update(ButtonUp);
    TEST_ASSERT_EQUAL(ButtonUp, buttonState->getCurrent());

    buttonState->resetChangeFlag();
    buttonState->update(ButtonRight);
    TEST_ASSERT_EQUAL(ButtonRight, buttonState->getCurrent());
}

// Test: Transition from no button to button
void testTransitionNoneToButton() {
    TEST_ASSERT_EQUAL(NoButton, buttonState->getCurrent());

    buttonState->update(ButtonPlus);
    TEST_ASSERT_EQUAL(ButtonPlus, buttonState->getCurrent());
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Transition from button to no button
void testTransitionButtonToNone() {
    buttonState->update(ButtonMinus);
    buttonState->resetChangeFlag();

    buttonState->update(NoButton);
    TEST_ASSERT_EQUAL(NoButton, buttonState->getCurrent());
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Rapid state changes
void testRapidStateChanges() {
    buttonState->update(ButtonA);
    buttonState->resetChangeFlag();

    buttonState->update(ButtonB);
    buttonState->resetChangeFlag();

    buttonState->update(ButtonOne);
    buttonState->resetChangeFlag();

    buttonState->update(ButtonTwo);

    TEST_ASSERT_EQUAL(ButtonTwo, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(ButtonOne, buttonState->getPrevious());
}

// Test: All individual buttons
void testAllIndividualButtons() {
    ButtonState buttons[] = {ButtonTwo,  ButtonOne,  ButtonB,     ButtonA,    ButtonMinus,
                             ButtonHome, ButtonLeft, ButtonRight, ButtonDown, ButtonUp,
                             ButtonPlus, ButtonC,    ButtonZ};

    for (ButtonState button : buttons) {
        buttonState->update(button);
        TEST_ASSERT_EQUAL(button, buttonState->getCurrent());
        buttonState->resetChangeFlag();
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(testInitialStateIsNoButton);
    RUN_TEST(testUpdateChangesCurrentState);
    RUN_TEST(testUpdateSetsChangedFlag);
    RUN_TEST(testUpdatePreservesPreviousState);
    RUN_TEST(testUpdateWithSameState);
    RUN_TEST(testResetChangeFlag);
    RUN_TEST(testMultipleButtonsCombination);
    RUN_TEST(testNunchukButtons);
    RUN_TEST(testDpadButtons);
    RUN_TEST(testTransitionNoneToButton);
    RUN_TEST(testTransitionButtonToNone);
    RUN_TEST(testRapidStateChanges);
    RUN_TEST(testAllIndividualButtons);

    return UNITY_END();
}

#ifdef ARDUINO
void setup() {
    delay(2000);  // Wait for serial
    UNITY_BEGIN();

    RUN_TEST(testInitialStateIsNoButton);
    RUN_TEST(testUpdateChangesCurrentState);
    RUN_TEST(testUpdateSetsChangedFlag);
    RUN_TEST(testUpdatePreservesPreviousState);
    RUN_TEST(testUpdateWithSameState);
    RUN_TEST(testResetChangeFlag);
    RUN_TEST(testMultipleButtonsCombination);
    RUN_TEST(testNunchukButtons);
    RUN_TEST(testDpadButtons);
    RUN_TEST(testTransitionNoneToButton);
    RUN_TEST(testTransitionButtonToNone);
    RUN_TEST(testRapidStateChanges);
    RUN_TEST(testAllIndividualButtons);

    UNITY_END();
}

void loop() {
    // Nothing here
}
#endif
