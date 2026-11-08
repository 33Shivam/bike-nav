#pragma once

#include "Screen.h"

// Handles both INCOMING_CALL and MUSIC: small, distinct "notification style"
// overlays rather than a full navigation layout.
class NotificationScreen : public Screen {
public:
    void enter(lgfx::LGFX_Device& canvas, const UIState& state) override;
    void update(lgfx::LGFX_Device& canvas, const UIState& state) override;

private:
    void drawCall(lgfx::LGFX_Device& canvas, const UIState& state);
    void drawMusic(lgfx::LGFX_Device& canvas, const UIState& state);

    bool entered_ = false;
    UIScreen lastMode_ = UIScreen::INCOMING_CALL;
    char lastCaller_[32] = "";
    char lastTitle_[32] = "";
    char lastArtist_[32] = "";
    bool lastPlaying_ = false;
};
