#pragma once

#include "UIState.h"
#include "../display/DisplayManager.h"
#include "screens/WaitingScreen.h"
#include "screens/IdleScreen.h"
#include "screens/NavigationScreen.h"
#include "screens/NotificationScreen.h"

// Owns the UI state and the one instance of each screen, and dispatches
// enter()/update() based on which screen is active. Any future data source
// (demo mode today, Chronos BLE later) only ever writes into state().
class UIManager {
public:
    explicit UIManager(DisplayManager& display);

    void begin();

    UIState& state() { return state_; }
    const UIState& state() const { return state_; }

    // Call every loop iteration. Detects screen transitions, applies
    // brightness/night-mode changes, and renders.
    void update();

private:
    Screen& screenFor(UIScreen screen);

    DisplayManager& display_;
    UIState state_;

    WaitingScreen waitingScreen_;
    IdleScreen idleScreen_;
    NavigationScreen navigationScreen_;
    NotificationScreen notificationScreen_;

    UIScreen lastRenderedScreen_ = UIScreen::WAITING_FOR_PHONE;
    bool firstRender_ = true;
    uint8_t lastAppliedBrightness_ = 0xFF;
};
