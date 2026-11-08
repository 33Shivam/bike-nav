#include "IdleScreen.h"
#include "../UITheme.h"
#include "../fonts/VlwFonts.h"
#include <cstdio>
#include <cstring>

namespace {
constexpr int kClockY = 110;
constexpr int kStatusY = 160;
constexpr int kStatusLetterSpacing = 1; // extra px between characters - LGFX has
                                         // no built-in letter-spacing API, so this
                                         // is drawn char-by-char instead of via a
                                         // single drawString() call
} // namespace

void IdleScreen::drawClock(lgfx::LGFX_Device& canvas, const UIState& state) {
    canvas.fillRect(UITheme::kScreenCenterX - 70, kClockY - 24, 140, 48, UITheme::kBackground);

    char buf[6];
    if (state.clockValid) {
        std::snprintf(buf, sizeof(buf), "%02u:%02u", state.clockHour, state.clockMinute);
    } else {
        std::snprintf(buf, sizeof(buf), "--:--");
    }

    // VLW instead of the old blocky built-in GFX font (setTextSize(5)) - real
    // anti-aliased glyph edges, same InterBold36 font used for distance text
    // elsewhere (see DistanceIndicator.cpp).
    canvas.loadFont(VlwFonts::InterBold36_vlw);
    canvas.setTextSize(1); // the font is already rendered at its target size - no scaling, or it blurs/pixelates
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UITheme::kPrimaryText, UITheme::kBackground);
    canvas.drawString(buf, UITheme::kScreenCenterX, kClockY);
}

void IdleScreen::drawStatusLine(lgfx::LGFX_Device& canvas, const UIState& state) {
    canvas.fillRect(UITheme::kScreenCenterX - 80, kStatusY - 9, 160, 18, UITheme::kBackground);

    // VLW instead of the old blocky built-in GFX font - same InterRegular16
    // font used for secondary text elsewhere (see NavigationArrow.cpp,
    // NavigationScreen.cpp).
    canvas.loadFont(VlwFonts::InterRegular16_vlw);
    canvas.setTextSize(0.75); // smaller than the font's native 16px render size
    canvas.setTextColor(UITheme::kSecondaryText, UITheme::kBackground);

    const char* text = state.connected ? "Connected" : "Disconnected";
    const size_t n = std::strlen(text);
    char glyph[2] = {0, 0};

    int totalWidth = 0;
    for (size_t i = 0; i < n; i++) {
        glyph[0] = text[i];
        totalWidth += canvas.textWidth(glyph);
    }
    totalWidth += kStatusLetterSpacing * static_cast<int>(n > 0 ? n - 1 : 0);

    canvas.setTextDatum(middle_left);
    int x = UITheme::kScreenCenterX - totalWidth / 2;
    for (size_t i = 0; i < n; i++) {
        glyph[0] = text[i];
        canvas.drawString(glyph, x, kStatusY);
        x += canvas.textWidth(glyph) + kStatusLetterSpacing;
    }
}

void IdleScreen::enter(lgfx::LGFX_Device& canvas, const UIState& state) {
    entered_ = true;
    canvas.fillScreen(UITheme::kBackground);
    drawClock(canvas, state);
    drawStatusLine(canvas, state);

    lastHour_ = state.clockHour;
    lastMinute_ = state.clockMinute;
    lastValid_ = state.clockValid;
    lastConnected_ = state.connected;
}

void IdleScreen::update(lgfx::LGFX_Device& canvas, const UIState& state) {
    if (!entered_) {
        enter(canvas, state);
        return;
    }

    if (state.clockHour != lastHour_ || state.clockMinute != lastMinute_ || state.clockValid != lastValid_) {
        drawClock(canvas, state);
        lastHour_ = state.clockHour;
        lastMinute_ = state.clockMinute;
        lastValid_ = state.clockValid;
    }
    if (state.connected != lastConnected_) {
        drawStatusLine(canvas, state);
        lastConnected_ = state.connected;
    }
}
