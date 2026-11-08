#pragma once

#include "UIState.h"
#include <cstdint>

// Cycles the UI through every screen with mock data so the display can be
// inspected end-to-end without any Bluetooth/Chronos connectivity. Purely
// additive: it only ever writes into a UIState it's given.
class DemoController {
public:
    // Call every loop iteration; non-blocking (millis()-paced).
    void update(UIState& state);

private:
    enum class Step : uint8_t {
        WAITING, IDLE, DEPART, STRAIGHT, LEFT, RIGHT, MERGE_LEFT, MERGE_RIGHT, ROUNDABOUT,
        // PNG-backed roundabout family (see ManeuverIcons.h) - cycled here so each one
        // gets visually verified on real hardware, not just compiled.
        ROUNDABOUT_SHARP_LEFT_CW, ROUNDABOUT_SHARP_LEFT_CCW,
        ROUNDABOUT_SHARP_RIGHT_CW, ROUNDABOUT_SHARP_RIGHT_CCW,
        ROUNDABOUT_SLIGHT_LEFT_CW, ROUNDABOUT_SLIGHT_LEFT_CCW,
        ROUNDABOUT_SLIGHT_RIGHT_CW, ROUNDABOUT_SLIGHT_RIGHT_CCW,
        ROUNDABOUT_EXIT_CW, ROUNDABOUT_EXIT_CCW,
        ROUNDABOUT_UTURN_CW, ROUNDABOUT_UTURN_CCW,
        ROUNDABOUT_LEFT_CW, ROUNDABOUT_LEFT_CCW,
        ROUNDABOUT_RIGHT_CW, ROUNDABOUT_RIGHT_CCW,
        ROUNDABOUT_STRAIGHT_CW, ROUNDABOUT_STRAIGHT_CCW,
        UTURN, APPROACHING, ARRIVED, UNKNOWN_STATE, CALL, MUSIC, COUNT
    };

    void applyStep(UIState& state, Step step);
    static bool isNavStep(Step step);

    bool started_ = false;
    Step currentStep_ = Step::WAITING;
    uint32_t stepStartMs_ = 0;
    uint32_t lastDistanceTickMs_ = 0;
};
