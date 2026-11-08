#include "DistanceIndicator.h"
#include "../UITheme.h"
#include "../fonts/VlwFonts.h"
#include <cstdio>

namespace DistanceIndicator {

void format(uint32_t meters, char* outBuf, size_t bufSize) {
    if (meters < 1000) {
        std::snprintf(outBuf, bufSize, "%u m", static_cast<unsigned>(meters));
    } else {
        std::snprintf(outBuf, bufSize, "%.1f km", meters / 1000.0f);
    }
}

void draw(lgfx::LGFX_Device& canvas, int cx, int cy, uint32_t meters, uint16_t color) {
    char buf[16];
    format(meters, buf, sizeof(buf));

    // VLW instead of the old 1-bit InterBold36 GFXfont - real anti-aliased
    // glyph edges, blended against the exact background color passed to
    // setTextColor() below (not a destination readback - see VlwFonts.h).
    canvas.loadFont(VlwFonts::InterBold36_vlw);
    canvas.setTextSize(1); // the font is already rendered at its target size - no scaling, or it blurs/pixelates
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(color, UITheme::kBackground);
    canvas.drawString(buf, cx, cy);
}

} // namespace DistanceIndicator
