#pragma once

#include "Screen.h"

// The primary screen: large bold maneuver arrow, big distance, two-line
// street name - no ETA (removed). Also handles TURN_APPROACHING - same
// layout, just recolors the arrow/distance to yellow instead of redesigning
// the screen.
class NavigationScreen : public Screen {
public:
    void enter(lgfx::LGFX_Device& canvas, const UIState& state) override;
    void update(lgfx::LGFX_Device& canvas, const UIState& state) override;

private:
    void redrawAll(lgfx::LGFX_Device& canvas, const UIState& state);

    bool entered_ = false;
    ManeuverType lastManeuver_ = ManeuverType::STRAIGHT;
    uint8_t lastExitNumber_ = 0xFF; // forces a redraw on the very first frame
    uint32_t lastDistance_ = 0xFFFFFFFF;
    char lastStreet_[48] = "";
    char lastDuration_[16] = "";
    bool lastApproaching_ = false;
    bool lastSpecial_ = false;
};
