#include "AntiAlias.h"
#include <cmath>
#include <algorithm>

namespace {

uint16_t lerp565(uint16_t bg, uint16_t fg, float t) {
    const int bgR = (bg >> 11) & 0x1F, bgG = (bg >> 5) & 0x3F, bgB = bg & 0x1F;
    const int fgR = (fg >> 11) & 0x1F, fgG = (fg >> 5) & 0x3F, fgB = fg & 0x1F;
    auto mix = [t](int from, int to, int maxVal) {
        int v = from + static_cast<int>(std::lround((to - from) * t));
        return std::min(std::max(v, 0), maxVal);
    };
    const int r = mix(bgR, fgR, 0x1F);
    const int g = mix(bgG, fgG, 0x3F);
    const int b = mix(bgB, fgB, 0x1F);
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

float distToSegment(float px, float py, float ax, float ay, float bx, float by) {
    const float abx = bx - ax, aby = by - ay;
    const float apx = px - ax, apy = py - ay;
    const float ab2 = abx * abx + aby * aby;
    float t = ab2 > 0.0f ? (apx * abx + apy * aby) / ab2 : 0.0f;
    t = std::min(std::max(t, 0.0f), 1.0f);
    const float cx = ax + abx * t, cy = ay + aby * t;
    const float dx = px - cx, dy = py - cy;
    return std::sqrt(dx * dx + dy * dy);
}

// Signed perpendicular distance from P to line AB; positive on the left of
// AB when walking from A to B.
float edgeDist(float px, float py, float ax, float ay, float bx, float by) {
    const float abx = bx - ax, aby = by - ay;
    const float len = std::sqrt(abx * abx + aby * aby);
    if (len < 1e-6f) return 0.0f;
    return ((bx - ax) * (py - ay) - (by - ay) * (px - ax)) / len;
}

} // namespace

namespace AntiAlias {

void blendPixel(lgfx::LGFX_Device& c, int x, int y, uint16_t fg, uint16_t bg, float coverage) {
    if (coverage <= 0.0f) return;
    if (coverage >= 1.0f) { c.drawPixel(x, y, fg); return; }
    c.drawPixel(x, y, lerp565(bg, fg, coverage));
}

void filledCircle(lgfx::LGFX_Device& c, int cx, int cy, float radius, uint16_t fg, uint16_t bg) {
    const int r = static_cast<int>(std::ceil(radius)) + 1;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            const float coverage = radius - d + 0.5f;
            blendPixel(c, cx + dx, cy + dy, fg, bg, std::min(coverage, 1.0f));
        }
    }
}

void ring(lgfx::LGFX_Device& c, int cx, int cy, float radius, float halfThickness, uint16_t fg, uint16_t bg) {
    const int r = static_cast<int>(std::ceil(radius + halfThickness)) + 1;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            const float coverage = halfThickness - std::fabs(d - radius) + 0.5f;
            blendPixel(c, cx + dx, cy + dy, fg, bg, std::min(coverage, 1.0f));
        }
    }
}

void wedgeLine(lgfx::LGFX_Device& c, float ax, float ay, float bx, float by, float halfWidth, uint16_t fg, uint16_t bg) {
    const int x0 = static_cast<int>(std::floor(std::min(ax, bx) - halfWidth - 1));
    const int x1 = static_cast<int>(std::ceil(std::max(ax, bx) + halfWidth + 1));
    const int y0 = static_cast<int>(std::floor(std::min(ay, by) - halfWidth - 1));
    const int y1 = static_cast<int>(std::ceil(std::max(ay, by) + halfWidth + 1));
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            const float d = distToSegment(static_cast<float>(x), static_cast<float>(y), ax, ay, bx, by);
            const float coverage = halfWidth - d + 0.5f;
            blendPixel(c, x, y, fg, bg, std::min(coverage, 1.0f));
        }
    }
}

void triangle(lgfx::LGFX_Device& c, float x0, float y0, float x1, float y1, float x2, float y2, uint16_t fg, uint16_t bg) {
    // Normalize to CCW winding so "inside" is a positive signed distance on
    // every edge, regardless of the order mirrored/rotated callers pass in.
    const float area2 = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
    if (area2 < 0.0f) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    const int minX = static_cast<int>(std::floor(std::min({x0, x1, x2}))) - 1;
    const int maxX = static_cast<int>(std::ceil(std::max({x0, x1, x2}))) + 1;
    const int minY = static_cast<int>(std::floor(std::min({y0, y1, y2}))) - 1;
    const int maxY = static_cast<int>(std::ceil(std::max({y0, y1, y2}))) + 1;
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            const float px = static_cast<float>(x), py = static_cast<float>(y);
            const float d0 = edgeDist(px, py, x0, y0, x1, y1);
            const float d1 = edgeDist(px, py, x1, y1, x2, y2);
            const float d2 = edgeDist(px, py, x2, y2, x0, y0);
            const float coverage = std::min({d0, d1, d2}) + 0.5f;
            blendPixel(c, x, y, fg, bg, std::min(coverage, 1.0f));
        }
    }
}

} // namespace AntiAlias
