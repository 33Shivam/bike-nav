#pragma once

#include <LovyanGFX.hpp>
#include <cstdint>

namespace StatusIndicator {

// Small filled/outline dot: green+filled when connected, red+outline otherwise.
void drawDot(lgfx::LGFX_Device& canvas, int cx, int cy, int radius, bool connected);

// Simple vector Bluetooth glyph (no bitmap asset) for the waiting screen.
void drawBluetoothIcon(lgfx::LGFX_Device& canvas, int cx, int cy, int size, uint16_t color);

} // namespace StatusIndicator
