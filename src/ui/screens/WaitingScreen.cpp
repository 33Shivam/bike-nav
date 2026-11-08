#include "WaitingScreen.h"
#include "../UITheme.h"
#include "../fonts/VlwFonts.h"

namespace {
// Fixed 3-line greeting - not derived from UIState, so no runtime word-wrap
// needed (compare NavigationScreen's wrapStreetName(), which handles phone-
// provided text that actually varies). Lines are short enough at
// InterRegular16 that even the top/bottom lines stay well inside the
// circle's chord width at their Y (checked against UITheme::kSafeRadius),
// so no per-line width math is needed either.
constexpr int kLine1Y = UITheme::kScreenCenterY - 24;
constexpr int kLine2Y = UITheme::kScreenCenterY;
constexpr int kLine3Y = UITheme::kScreenCenterY + 24;
} // namespace

void WaitingScreen::enter(lgfx::LGFX_Device& canvas, const UIState& state) {
    (void)state;
    canvas.fillScreen(UITheme::kBackground);

    canvas.loadFont(VlwFonts::InterRegular16_vlw);
    canvas.setTextSize(1);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UITheme::kPrimaryText, UITheme::kBackground);
    canvas.drawString("Hey Shivam,", UITheme::kScreenCenterX, kLine1Y);
    canvas.drawString("connect your", UITheme::kScreenCenterX, kLine2Y);
    canvas.drawString("phone", UITheme::kScreenCenterX, kLine3Y);
}

void WaitingScreen::update(lgfx::LGFX_Device& canvas, const UIState& state) {
    // Nothing on this screen depends on UIState - the greeting is a fixed
    // string drawn once in enter(). UIManager guarantees enter() runs before
    // the first update() on a screen transition, so there's no partial
    // redraw work to do here.
    (void)canvas;
    (void)state;
}
