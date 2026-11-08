#pragma once

#include <LovyanGFX.hpp>
#include "../UIState.h"

// Minimal interface so UIManager can hold screens polymorphically and
// decide when to do a full redraw (enter) vs an incremental one (update).
class Screen {
public:
    virtual ~Screen() = default;

    // Called once when this screen becomes active: clears the screen and
    // draws every component from scratch.
    virtual void enter(lgfx::LGFX_Device& canvas, const UIState& state) = 0;

    // Called every loop tick while this screen stays active: redraws only
    // the components whose backing data changed since the last call.
    virtual void update(lgfx::LGFX_Device& canvas, const UIState& state) = 0;
};
