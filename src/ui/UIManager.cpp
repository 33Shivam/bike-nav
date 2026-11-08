#include "UIManager.h"
#include "UITheme.h"

UIManager::UIManager(DisplayManager& display) : display_(display) {}

void UIManager::begin() {
    firstRender_ = true;
    lastAppliedBrightness_ = 0xFF;
}

Screen& UIManager::screenFor(UIScreen screen) {
    switch (screen) {
        case UIScreen::WAITING_FOR_PHONE: return waitingScreen_;
        case UIScreen::IDLE: return idleScreen_;
        case UIScreen::NAVIGATION:
        case UIScreen::TURN_APPROACHING: return navigationScreen_;
        case UIScreen::INCOMING_CALL:
        case UIScreen::MUSIC: return notificationScreen_;
    }
    return idleScreen_;
}

void UIManager::update() {
    if (!display_.isInitialized()) return;

    const uint8_t targetBrightness = state_.nightMode ? UITheme::kNightModeBrightness : state_.brightness;
    if (targetBrightness != lastAppliedBrightness_) {
        display_.setBacklight(targetBrightness);
        lastAppliedBrightness_ = targetBrightness;
    }

    Screen& target = screenFor(state_.screen);
    const bool isTransition = firstRender_ || &target != &screenFor(lastRenderedScreen_);

    display_.canvas().startWrite();
    if (isTransition) {
        // Font selection is persistent canvas state - reset to the default
        // before handing off to a new screen family so a custom font one
        // screen sets (e.g. NavigationScreen's Inter fonts) never leaks into
        // another screen that never calls setFont() itself.
        display_.canvas().setFont(&lgfx::fonts::Font0);
        target.enter(display_.canvas(), state_);
        firstRender_ = false;
    } else {
        target.update(display_.canvas(), state_);
    }
    display_.canvas().endWrite();

    lastRenderedScreen_ = state_.screen;
}
