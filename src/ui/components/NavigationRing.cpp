#include "NavigationRing.h"
#include <algorithm>

namespace NavigationRing {

void draw(lgfx::LGFX_Device& canvas, int cx, int cy, int outerRadius, int thickness,
          float progress, uint16_t color, uint16_t trackColor) {
    progress = std::min(1.0f, std::max(0.0f, progress));
    const int innerRadius = outerRadius - thickness;

    canvas.fillArc(cx, cy, innerRadius, outerRadius, 0, 360, trackColor);
    if (progress > 0.0f) {
        canvas.fillArc(cx, cy, innerRadius, outerRadius, 0, progress * 360.0f, color);
    }
}

} // namespace NavigationRing
