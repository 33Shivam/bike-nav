#pragma once

#include "Screen.h"

class IdleScreen : public Screen {
public:
    void enter(lgfx::LGFX_Device& canvas, const UIState& state) override;
    void update(lgfx::LGFX_Device& canvas, const UIState& state) override;

private:
    void drawClock(lgfx::LGFX_Device& canvas, const UIState& state);
    void drawStatusLine(lgfx::LGFX_Device& canvas, const UIState& state);

    bool entered_ = false;
    uint8_t lastHour_ = 0xFF;
    uint8_t lastMinute_ = 0xFF;
    bool lastValid_ = false;
    bool lastConnected_ = false;
};
