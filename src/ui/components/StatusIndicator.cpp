#include "StatusIndicator.h"
#include "../UITheme.h"

namespace StatusIndicator {

void drawDot(lgfx::LGFX_Device& canvas, int cx, int cy, int radius, bool connected) {
    if (connected) {
        canvas.fillCircle(cx, cy, radius, UITheme::kConnectedColor);
    } else {
        canvas.fillCircle(cx, cy, radius, UITheme::kBackground);
        canvas.drawCircle(cx, cy, radius, UITheme::kWarningColor);
    }
}

void drawBluetoothIcon(lgfx::LGFX_Device& canvas, int cx, int cy, int size, uint16_t color) {
    const float h = size * 0.5f;
    const float w = size * 0.28f;

    const int topX = cx, topY = cy - static_cast<int>(h);
    const int botX = cx, botY = cy + static_cast<int>(h);
    const int upperRightX = cx + static_cast<int>(w), upperRightY = cy - static_cast<int>(h * 0.5f);
    const int lowerRightX = cx + static_cast<int>(w), lowerRightY = cy + static_cast<int>(h * 0.5f);

    canvas.drawLine(topX, topY, botX, botY, color);
    canvas.drawLine(topX, topY, lowerRightX, lowerRightY, color);
    canvas.drawLine(botX, botY, upperRightX, upperRightY, color);
}

} // namespace StatusIndicator
