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
    TEST_ASSERT_EQUAL(kNoButton, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(kNoButton, buttonState->getPrevious());
    TEST_ASSERT_FALSE(buttonState->hasChanged());
}

// Test: Update changes current state
void testUpdateChangesCurrentState() {
    buttonState->update(kButtonA);
    TEST_ASSERT_EQUAL(kButtonA, buttonState->getCurrent());
}

// Test: Update sets changed flag
void testUpdateSetsChangedFlag() {
    buttonState->update(kButtonA);
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Update preserves previous state
void testUpdatePreservesPreviousState() {
    buttonState->update(kButtonA);
    buttonState->resetChangeFlag();
    buttonState->update(kButtonB);

    TEST_ASSERT_EQUAL(kButtonB, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(kButtonA, buttonState->getPrevious());
}

// Test: Update with same state
void testUpdateWithSameState() {
    buttonState->update(kButtonA);
    buttonState->resetChangeFlag();
    buttonState->update(kButtonA);

    // Should not set changed flag if state is identical
    TEST_ASSERT_EQUAL(kButtonA, buttonState->getCurrent());
}

// Test: Reset change flag
void testResetChangeFlag() {
    buttonState->update(kButtonHome);
    TEST_ASSERT_TRUE(buttonState->hasChanged());

    buttonState->resetChangeFlag();
    TEST_ASSERT_FALSE(buttonState->hasChanged());
}

// Test: Multiple button combination
void testMultipleButtonsCombination() {
    ButtonState combo = buttonStateOr(buttonStateOr(kButtonA, kButtonB), kButtonOne);
    buttonState->update(combo);

    ButtonState current = buttonState->getCurrent();
    TEST_ASSERT_EQUAL(combo, current);
    TEST_ASSERT_TRUE(buttonStateHas(current, kButtonA));
    TEST_ASSERT_TRUE(buttonStateHas(current, kButtonB));
    TEST_ASSERT_TRUE(buttonStateHas(current, kButtonOne));
}

// Test: Nunchuk buttons
void testNunchukButtons() {
    buttonState->update(kButtonZ);
    TEST_ASSERT_EQUAL(kButtonZ, buttonState->getCurrent());

    buttonState->resetChangeFlag();
    buttonState->update(kButtonC);
    TEST_ASSERT_EQUAL(kButtonC, buttonState->getCurrent());
}

// Test: D-pad buttons
void testDpadButtons() {
    buttonState->update(kButtonUp);
    TEST_ASSERT_EQUAL(kButtonUp, buttonState->getCurrent());

    buttonState->resetChangeFlag();
    buttonState->update(kButtonRight);
    TEST_ASSERT_EQUAL(kButtonRight, buttonState->getCurrent());
}

// Test: Transition from no button to button
void testTransitionNoneToButton() {
    TEST_ASSERT_EQUAL(kNoButton, buttonState->getCurrent());

    buttonState->update(kButtonPlus);
    TEST_ASSERT_EQUAL(kButtonPlus, buttonState->getCurrent());
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Transition from button to no button
void testTransitionButtonToNone() {
    buttonState->update(kButtonMinus);
    buttonState->resetChangeFlag();

    buttonState->update(kNoButton);
    TEST_ASSERT_EQUAL(kNoButton, buttonState->getCurrent());
    TEST_ASSERT_TRUE(buttonState->hasChanged());
}

// Test: Rapid state changes
void testRapidStateChanges() {
    buttonState->update(kButtonA);
    buttonState->resetChangeFlag();

    buttonState->update(kButtonB);
    buttonState->resetChangeFlag();

    buttonState->update(kButtonOne);
    buttonState->resetChangeFlag();

    buttonState->update(kButtonTwo);

    TEST_ASSERT_EQUAL(kButtonTwo, buttonState->getCurrent());
    TEST_ASSERT_EQUAL(kButtonOne, buttonState->getPrevious());
}

// Test: All individual buttons
void testAllIndividualButtons() {
    ButtonState buttons[] = {kButtonTwo,  kButtonOne,  kButtonB,     kButtonA,    kButtonMinus,
                             kButtonHome, kButtonLeft, kButtonRight, kButtonDown, kButtonUp,
                             kButtonPlus, kButtonC,    kButtonZ};

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
