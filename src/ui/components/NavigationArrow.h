#pragma once

#include <LovyanGFX.hpp>
#include "../UIState.h"

// Vector-drawn maneuver glyphs. Each maneuver family has its own distinct
// geometry (bend angle, hook, fork, ring, pin, ...) - left/right variants
// within a family are mirrored, but different maneuvers are never produced
// by simply rotating one shared icon.
namespace NavigationArrow {

// Draws the maneuver glyph centered at (cx, cy) within a size x size
// bounding box, in the given color, onto an already-clipped canvas.
// exitNumber is only used for ROUNDABOUT (0 = no numbered-exit badge).
void draw(lgfx::LGFX_Device& canvas, ManeuverType maneuver, int cx, int cy, int size, uint16_t color, uint8_t exitNumber = 0);

} // namespace NavigationArrow
