#include "NavigationScreen.h"
#include "../UITheme.h"
#include "../AntiAlias.h"
#include "../components/NavigationArrow.h"
#include "../components/DistanceIndicator.h"
#include "../fonts/VlwFonts.h"
#include <cstring>
#include <cstdio>

namespace {

// Our Android app forwards Google Maps' eta text verbatim, e.g.
// "Arrive 9:04 pm" - the reference design wants "<time> ETA" instead.
// Our Inter fonts only cover standard printable ASCII (0x20-0x7E) - phone-
// originated strings can contain other bytes (we already hit this once: the
// distance string's non-breaking space between number and unit). Replace
// anything outside that range with a plain space so an unexpected byte
// can't render as a garbled/wrong glyph instead of silently degrading.
void sanitizeForFont(const char* src, char* dst, size_t dstSize) {
    size_t i = 0;
    for (; src[i] != '\0' && i < dstSize - 1; i++) {
        const unsigned char c = static_cast<unsigned char>(src[i]);
        dst[i] = (c >= 0x20 && c <= 0x7E) ? static_cast<char>(c) : ' ';
    }
    dst[i] = '\0';
}

bool isSpecialState(ManeuverType m) {
    return m == ManeuverType::DEPART || m == ManeuverType::ARRIVED || m == ManeuverType::UNKNOWN;
}

const char* specialLabel(ManeuverType m) {
    switch (m) {
        case ManeuverType::DEPART:  return "DEPART";
        case ManeuverType::ARRIVED: return "ARRIVED";
        default:                    return "UNKNOWN";
    }
}

// DEPART / ARRIVED / UNKNOWN show just a big icon and a label - no ETA,
// distance, street, matching the reference design's full-screen states.
void drawSpecialState(lgfx::LGFX_Device& canvas, const UIState& state) {
    canvas.fillScreen(UITheme::kBackground);
    NavigationArrow::draw(canvas, state.nav.maneuver, UITheme::kScreenCenterX, 100, 130, UITheme::kPrimaryText);

    // DEPART/ARRIVED/UNKNOWN share this screen instance with the normal
    // layout above, so switching into this branch doesn't go through
    // UIManager's family-transition font reset - reset explicitly here too,
    // otherwise this could inherit an Inter font left set by the normal
    // layout and render wrong at this setTextSize() scale.
    canvas.setFont(&lgfx::fonts::Font0);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UITheme::kPrimaryText, UITheme::kBackground);
    canvas.setTextSize(3);
    canvas.drawString(specialLabel(state.nav.maneuver), UITheme::kScreenCenterX, 190);
}

// Shows duration only (e.g. "6 min") when the transport provides one -
// currently never, since neither Google Maps' notification nor our Android
// app expose a real "total trip remaining" metric (investigated and ruled
// out: Notification.ProgressStyle's progress/progressMax is an abstract
// progress-bar scale, not meters - verified against real captured data
// where progressMax stayed ~16,500 regardless of next-turn distance being
// 0m or 250m). Deliberately does NOT fall back to showing next-turn
// distance here - that's already shown prominently below the arrow, and
// repeating it in this slot would misleadingly look like a second, larger
// trip-distance metric we don't actually have.
void drawSubInfo(lgfx::LGFX_Device& canvas, const UIState& state) {
    canvas.fillRect(UITheme::kScreenCenterX - 90, UITheme::kSubInfoTextY - 11, 180, 22, UITheme::kBackground);

    if (state.nav.duration[0] == '\0') return;

    canvas.loadFont(VlwFonts::InterRegular16_vlw); // VLW instead of the old 1-bit GFXfont - see VlwFonts.h
    canvas.setTextSize(1);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UITheme::kNavDurationColor, UITheme::kBackground);
    canvas.drawString(state.nav.duration, UITheme::kScreenCenterX, UITheme::kSubInfoTextY);
}

void drawArrow(lgfx::LGFX_Device& canvas, const UIState& state, uint16_t color) {
    // Bent/branched glyphs (turns near a 90 degree bend especially) reach
    // further from center than a tight 1.05x pad covers once the
    // perpendicular arrowhead spread is added on top of the bend reach -
    // a too-small erase rect here was the actual cause of arrows looking
    // "cut off" (stale pixels from the previous glyph's wider silhouette
    // never got cleared). 2x size is a generous, verified-safe margin.
    const int pad = UITheme::kArrowSize * 2;
    canvas.fillRect(UITheme::kArrowCenterX - pad / 2, UITheme::kArrowCenterY - pad / 2, pad, pad, UITheme::kBackground);
    NavigationArrow::draw(canvas, state.nav.maneuver, UITheme::kArrowCenterX, UITheme::kArrowCenterY, UITheme::kArrowSize, color, state.nav.roundaboutExitNumber);
}

void drawDistance(lgfx::LGFX_Device& canvas, const UIState& state, uint16_t color) {
    canvas.fillRect(UITheme::kScreenCenterX - 75, UITheme::kDistanceTextY - 20, 150, 40, UITheme::kBackground);
    DistanceIndicator::draw(canvas, UITheme::kScreenCenterX, UITheme::kDistanceTextY, state.nav.distanceMeters, color);
}

// Word-boundary-aware wrap into two lines. Overflow on line2 breaks at a
// word boundary where possible and ends in "..." - a real street name can
// run well past 2x17 chars (e.g. "Greater Noida West Link Rd / Noida-
// Greater Noida Link Rd", 57 chars), and silently hard-cutting mid-word
// with no indicator reads as broken/truncated-by-accident rather than
// "there's more text than fits".
void wrapStreetName(const char* src, char* line1, size_t line1Size, char* line2, size_t line2Size) {
    line1[0] = '\0';
    line2[0] = '\0';
    const size_t maxLine = static_cast<size_t>(UITheme::kStreetLineMaxChars);
    const size_t srcLen = std::strlen(src);
    if (srcLen == 0) return;

    if (srcLen <= maxLine) {
        std::strncpy(line1, src, line1Size - 1);
        line1[line1Size - 1] = '\0';
        return;
    }

    size_t breakAt = maxLine;
    for (size_t i = maxLine; i > 0; i--) {
        if (src[i] == ' ') { breakAt = i; break; }
    }
    const size_t line1Len = breakAt < line1Size - 1 ? breakAt : line1Size - 1;
    std::strncpy(line1, src, line1Len);
    line1[line1Len] = '\0';

    const char* rest = src + breakAt;
    while (*rest == ' ') rest++;
    const size_t restLen = std::strlen(rest);
    if (restLen <= maxLine) {
        std::strncpy(line2, rest, line2Size - 1);
        line2[line2Size - 1] = '\0';
        return;
    }

    constexpr size_t kEllipsisLen = 3;
    const size_t avail = maxLine > kEllipsisLen ? maxLine - kEllipsisLen : maxLine;
    size_t cutAt = avail; // falls back to a hard cut if no space found below
    for (size_t i = avail; i > 0; i--) {
        if (rest[i] == ' ') { cutAt = i; break; }
    }
    const size_t maxCopy = line2Size - 1 > kEllipsisLen ? line2Size - 1 - kEllipsisLen : 0;
    const size_t copyLen = cutAt < maxCopy ? cutAt : maxCopy;
    std::strncpy(line2, rest, copyLen);
    line2[copyLen] = '\0';
    std::strncat(line2, "...", line2Size - 1 - copyLen);
}

void drawStreet(lgfx::LGFX_Device& canvas, const UIState& state) {
    canvas.fillRect(UITheme::kScreenCenterX - 95, UITheme::kStreetLine1Y - 11, 190, 44, UITheme::kBackground);

    char cleanStreet[48];
    sanitizeForFont(state.nav.streetName, cleanStreet, sizeof(cleanStreet));

    char line1[UITheme::kStreetLineMaxChars + 1];
    char line2[UITheme::kStreetLineMaxChars + 1];
    wrapStreetName(cleanStreet, line1, sizeof(line1), line2, sizeof(line2));

    canvas.loadFont(VlwFonts::InterRegular16_vlw); // VLW instead of the old 1-bit GFXfont - see VlwFonts.h
    canvas.setTextSize(1);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UITheme::kStreetTextColor, UITheme::kBackground);
    canvas.drawString(line1, UITheme::kScreenCenterX, UITheme::kStreetLine1Y);
    if (line2[0] != '\0') {
        canvas.drawString(line2, UITheme::kScreenCenterX, UITheme::kStreetLine2Y);
    }
}

} // namespace

void NavigationScreen::redrawAll(lgfx::LGFX_Device& canvas, const UIState& state) {
    const bool special = isSpecialState(state.nav.maneuver);
    const bool approaching = !special && state.screen == UIScreen::TURN_APPROACHING;

    if (special) {
        drawSpecialState(canvas, state);
    } else {
        const uint16_t accent = approaching ? UITheme::kApproachingColor : UITheme::kNavigationColor;

        canvas.fillScreen(UITheme::kBackground);
        // AntiAlias::ring instead of drawCircle: LovyanGFX's own circle isn't
        // anti-aliased at all, and its smooth-circle variants blend edges
        // toward a destination readback this write-only panel can't provide
        // (always reads back as black) - see AntiAlias.h for why.
        AntiAlias::ring(canvas, UITheme::kScreenCenterX, UITheme::kScreenCenterY, 118.0f, 0.6f, UITheme::kBezelOutline, UITheme::kBackground);
        drawSubInfo(canvas, state);
        drawArrow(canvas, state, accent);
        drawDistance(canvas, state, accent);
        drawStreet(canvas, state);
    }

    lastSpecial_ = special;
    lastManeuver_ = state.nav.maneuver;
    lastExitNumber_ = state.nav.roundaboutExitNumber;
    lastDistance_ = state.nav.distanceMeters;
    std::strncpy(lastStreet_, state.nav.streetName, sizeof(lastStreet_) - 1);
    lastStreet_[sizeof(lastStreet_) - 1] = '\0';
    std::strncpy(lastDuration_, state.nav.duration, sizeof(lastDuration_) - 1);
    lastDuration_[sizeof(lastDuration_) - 1] = '\0';
    lastApproaching_ = approaching;
}

void NavigationScreen::enter(lgfx::LGFX_Device& canvas, const UIState& state) {
    entered_ = true;
    redrawAll(canvas, state);
}

void NavigationScreen::update(lgfx::LGFX_Device& canvas, const UIState& state) {
    if (!entered_) {
        redrawAll(canvas, state);
        return;
    }

    const bool special = isSpecialState(state.nav.maneuver);
    if (special != lastSpecial_) {
        redrawAll(canvas, state);
        return;
    }
    if (special) {
        if (state.nav.maneuver != lastManeuver_) {
            redrawAll(canvas, state);
        }
        return;
    }

    const bool approaching = state.screen == UIScreen::TURN_APPROACHING;
    const uint16_t accent = approaching ? UITheme::kApproachingColor : UITheme::kNavigationColor;

    const bool maneuverChanged = state.nav.maneuver != lastManeuver_ ||
                                  state.nav.roundaboutExitNumber != lastExitNumber_ ||
                                  approaching != lastApproaching_;
    const bool distanceChanged = state.nav.distanceMeters != lastDistance_ || approaching != lastApproaching_;
    const bool durationChanged = std::strcmp(state.nav.duration, lastDuration_) != 0;
    const bool streetChanged = std::strcmp(state.nav.streetName, lastStreet_) != 0;

    if (maneuverChanged) drawArrow(canvas, state, accent);
    if (distanceChanged) drawDistance(canvas, state, accent);
    if (distanceChanged || durationChanged) drawSubInfo(canvas, state);
    if (streetChanged) drawStreet(canvas, state);

    lastSpecial_ = false;
    lastManeuver_ = state.nav.maneuver;
    lastExitNumber_ = state.nav.roundaboutExitNumber;
    lastDistance_ = state.nav.distanceMeters;
    std::strncpy(lastStreet_, state.nav.streetName, sizeof(lastStreet_) - 1);
    lastStreet_[sizeof(lastStreet_) - 1] = '\0';
    std::strncpy(lastDuration_, state.nav.duration, sizeof(lastDuration_) - 1);
    lastDuration_[sizeof(lastDuration_) - 1] = '\0';
    lastApproaching_ = approaching;
}
