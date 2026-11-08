#pragma once

#include <LovyanGFX.hpp>
#include <cstdint>

// Circular progress ring around the edge of the round display. Progress is
// a 0..1 fraction (how much of the "lookahead window" to the next maneuver
// has been covered); the caller picks the color so it can flip to the
// approaching-color without this component knowing about UI state.
namespace NavigationRing {

void draw(lgfx::LGFX_Device& canvas, int cx, int cy, int outerRadius, int thickness,
          float progress, uint16_t color, uint16_t trackColor);

} // namespace NavigationRing
