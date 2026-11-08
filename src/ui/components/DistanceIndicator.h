#pragma once

#include <LovyanGFX.hpp>
#include <cstdint>

// Distance formatting is kept separate from drawing so the UI state layer
// (or future tests) can format a value without touching the display.
namespace DistanceIndicator {

// Formats meters as "50 m" / "500 m" / "1.2 km" into outBuf (snprintf, no heap).
void format(uint32_t meters, char* outBuf, size_t bufSize);

// Draws the formatted distance centered at (cx, cy) using the Inter Bold
// VLW distance font (src/ui/fonts/VlwFonts.h).
void draw(lgfx::LGFX_Device& canvas, int cx, int cy, uint32_t meters, uint16_t color);

} // namespace DistanceIndicator
