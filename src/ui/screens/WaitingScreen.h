#pragma once

#include "Screen.h"

class WaitingScreen : public Screen {
public:
    void enter(lgfx::LGFX_Device& canvas, const UIState& state) override;
    void update(lgfx::LGFX_Device& canvas, const UIState& state) override;
};
