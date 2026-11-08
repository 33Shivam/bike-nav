#pragma once

#include <LovyanGFX.hpp>

// Hand-rolled anti-aliasing that blends toward an explicitly-known background
// color, instead of LovyanGFX's built-in smooth/AA primitives (drawWedgeLine,
// fillSmoothCircle, fillArc's edges, ...) which blend by reading back the
// current destination pixel. This panel is write-only SPI (no MISO -
// DisplayManager sets panelCfg.readable = false), so that readback silently
// returns black (Panel_LCD::readRect memsets to 0 when !_cfg.readable)
// instead of the real pixel underneath - meaning LovyanGFX's own AA always
// blends edges toward assumed-black. That happened to look fine under the
// old near-black theme, but produces a visible dark fringe now that the
// background is white. Every caller here always knows its exact backdrop
// color up front (NavigationScreen erases to a flat UITheme color before any
// redraw), so we can blend correctly without needing readback at all.
namespace AntiAlias {

// Blends an arc-length's worth of coverage at a single pixel. coverage<=0
// draws nothing (preserves whatever's already there); coverage>=1 draws pure
// fg; in between, linearly blends bg->fg per RGB565 channel.
void blendPixel(lgfx::LGFX_Device& c, int x, int y, uint16_t fg, uint16_t bg, float coverage);

// Filled disc, radius in pixels (may be fractional).
void filledCircle(lgfx::LGFX_Device& c, int cx, int cy, float radius, uint16_t fg, uint16_t bg);

// Thin circular ring: centerline at `radius`, extends `halfThickness` to
// either side - e.g. radius=118, halfThickness=0.6 approximates the old
// 1px-wide drawCircle outline, just smoothed.
void ring(lgfx::LGFX_Device& c, int cx, int cy, float radius, float halfThickness, uint16_t fg, uint16_t bg);

// Thick line segment with rounded caps (same shape drawWedgeLine draws, just
// blended against a known bg instead of a readback).
void wedgeLine(lgfx::LGFX_Device& c, float ax, float ay, float bx, float by, float halfWidth, uint16_t fg, uint16_t bg);

// Filled convex triangle (winding-order agnostic).
void triangle(lgfx::LGFX_Device& c, float x0, float y0, float x1, float y1, float x2, float y2, uint16_t fg, uint16_t bg);

} // namespace AntiAlias
