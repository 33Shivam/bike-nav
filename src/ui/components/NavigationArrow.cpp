#include "NavigationArrow.h"
#include "../assets/ManeuverIcons.h"
#include "../AntiAlias.h"
#include "../UITheme.h"
#include "../fonts/VlwFonts.h"
#include "../../navigation/ManeuverMapper.h"
#include <cmath>
#include <cstdio>

namespace {

constexpr float kDeg2Rad = 3.14159265358979323846f / 180.0f;

struct Vec2 {
    float x;
    float y;
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }

// Direction unit vector for an angle measured clockwise from "up" (screen -Y).
inline Vec2 dirFromAngle(float deg) {
    const float r = deg * kDeg2Rad;
    return {std::sin(r), -std::cos(r)};
}

// Perpendicular (rotated -90deg) used to build arrowhead widths.
inline Vec2 perpOf(Vec2 d) { return {-d.y, d.x}; }

inline int32_t toI(float v) { return static_cast<int32_t>(std::lround(v)); }

int32_t sx(float x, int cx) { return toI(x) + cx; }
int32_t sy(float y, int cy) { return toI(y) + cy; }

// Uses our own AntiAlias::wedgeLine (blends toward a known background color)
// rather than LovyanGFX's drawWedgeLine, whose edge AA blends toward a
// readback of the destination pixel - this panel can't be read back (no
// MISO), so that readback is always black, producing a dark fringe on our
// white background. See AntiAlias.h.
void drawShaft(lgfx::LGFX_Device& c, int cx, int cy, Vec2 a, Vec2 b, float halfWidth, uint16_t color) {
    AntiAlias::wedgeLine(c, sx(a.x, cx), sy(a.y, cy), sx(b.x, cx), sy(b.y, cy), halfWidth, color, UITheme::kBackground);
}

// Solid filled arrowhead - bold, clean silhouette. The base is drawn
// slightly behind baseCenter so it overlaps the end of the shaft, giving a
// seamless join with no notch at the bend. AntiAlias::triangle instead of
// LovyanGFX's fillTriangle for the same read-back-blending reason as
// drawShaft above.
void drawHead(lgfx::LGFX_Device& c, int cx, int cy, Vec2 baseCenter, Vec2 dir, float headLen, float headHalfWidth, uint16_t color) {
    const Vec2 apex = baseCenter + dir * headLen;
    const Vec2 perp = perpOf(dir);
    const Vec2 base = baseCenter - dir * (headLen * 0.22f); // overlap into the shaft
    const Vec2 left = base + perp * headHalfWidth;
    const Vec2 right = base - perp * headHalfWidth;

    AntiAlias::triangle(c, sx(apex.x, cx), sy(apex.y, cy), sx(left.x, cx), sy(left.y, cy), sx(right.x, cx), sy(right.y, cy), color, UITheme::kBackground);
}

// Proportions measured off the Figma dev-mode SVG export (see git history for
// the full derivation) - still used by the two maneuvers that remain
// hand-drawn vectors (EXIT_LEFT/RIGHT, UNKNOWN) now that every other maneuver
// is PNG-backed (see drawPngIcon below).
constexpr float kShaftHalfWRatio = 0.04f;
constexpr float kHeadLenRatio    = 0.21f;
constexpr float kHeadHalfWRatio  = 0.145f;
constexpr float kTailYRatio      = 0.188f;

// EXIT_LEFT/EXIT_RIGHT (highway off-ramp) stay vector: the minimal-nav PNG
// set this project otherwise uses has no highway-ramp icon, only a
// roundabout-exit icon, which isn't visually equivalent.
void drawExit(lgfx::LGFX_Device& c, int cx, int cy, int size, bool mirror, uint16_t color) {
    const float sign = mirror ? -1.0f : 1.0f;
    // The through-lane and exit branch share one trunk x-offset (not x=0) up
    // until the point they diverge.
    const float trunkX = size * 0.13f * sign;
    const Vec2 mainTail{trunkX, size * kTailYRatio};
    const Vec2 mainHead{trunkX, -size * 0.536f};
    drawShaft(c, cx, cy, mainTail, mainHead, size * kShaftHalfWRatio, color);

    const Vec2 branchStart{trunkX, -size * 0.04f};
    const Vec2 dir = dirFromAngle(40.0f * sign);
    const float headLen = size * kHeadLenRatio;
    const Vec2 baseCenter = branchStart + dir * (size * 0.36f - headLen);
    drawShaft(c, cx, cy, branchStart, baseCenter, size * kShaftHalfWRatio, color);
    drawHead(c, cx, cy, baseCenter, dir, headLen, size * kHeadHalfWRatio, color);
}

// Overlays the roundabout exit number (e.g. "2" for "take the 2nd exit") as
// a small filled-circle badge in the icon's bottom-right corner. No PNG
// variant encodes an arbitrary number (see milestone 8 in CLAUDE.md), so
// this composites a badge on top of the base icon at draw time instead,
// using the already-correctly-wired NavigationInfo::roundaboutExitNumber
// that used to be discarded here - added 2026-11-08.
void drawExitNumberBadge(lgfx::LGFX_Device& c, int cx, int cy, int size, uint8_t exitNumber) {
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(exitNumber > 99 ? 99 : exitNumber));

    const float badgeRadius = size * 0.22f;
    const int badgeCx = cx + static_cast<int>(size * 0.34f);
    const int badgeCy = cy + static_cast<int>(size * 0.34f);

    // Solid dark badge with a light ring (AntiAlias, not LovyanGFX's built-in
    // smooth primitives - this panel can't be read back, see AntiAlias.h)
    // so the number reads clearly regardless of what's under it.
    AntiAlias::filledCircle(c, badgeCx, badgeCy, badgeRadius, UITheme::kPrimaryText, UITheme::kBackground);
    AntiAlias::ring(c, badgeCx, badgeCy, badgeRadius, 1.0f, UITheme::kBackground, UITheme::kPrimaryText);

    c.loadFont(VlwFonts::InterRegular16_vlw);
    c.setTextSize(1);
    c.setTextDatum(middle_center);
    c.setTextColor(UITheme::kBackground, UITheme::kPrimaryText);
    c.drawString(buf, badgeCx, badgeCy);
}

// UNKNOWN (no equivalent PNG asset either) stays a vector "?" glyph - a
// stroked hook over a separate dot, rather than falling back to a font
// character.
void drawQuestionMark(lgfx::LGFX_Device& c, int cx, int cy, int size, uint16_t color) {
    const float shaftHalfW = size * kShaftHalfWRatio;
    const Vec2 hookCenter{0, -size * 0.27f};
    const float hookR = size * 0.17f;

    // The hook used to be a single fillArc() call; approximated here as a
    // chain of AntiAlias::wedgeLine segments along the same arc path so it
    // gets the same known-background AA treatment as everything else in
    // this file (fillArc's edges, like fillTriangle's, blend toward a
    // destination readback this panel can't actually provide).
    constexpr int kHookSegments = 24;
    constexpr float kHookStartDeg = -150.0f;
    constexpr float kHookSweepDeg = 220.0f; // -150 -> 70
    Vec2 prev = hookCenter + dirFromAngle(kHookStartDeg) * hookR;
    for (int i = 1; i <= kHookSegments; i++) {
        const float angle = kHookStartDeg + kHookSweepDeg * i / kHookSegments;
        const Vec2 cur = hookCenter + dirFromAngle(angle) * hookR;
        AntiAlias::wedgeLine(c, sx(prev.x, cx), sy(prev.y, cy), sx(cur.x, cx), sy(cur.y, cy), shaftHalfW, color, UITheme::kBackground);
        prev = cur;
    }

    const Vec2 stubTop = hookCenter + dirFromAngle(70.0f + 180.0f) * hookR;
    const Vec2 stubBottom{0, size * 0.02f};
    drawShaft(c, cx, cy, stubTop, stubBottom, shaftHalfW, color);

    const Vec2 dotCenter{0, size * 0.135f};
    AntiAlias::filledCircle(c, sx(dotCenter.x, cx), sy(dotCenter.y, cy), shaftHalfW * 1.15f, color, UITheme::kBackground);
}

// Source PNGs are pre-resized to exactly this at build time (not scaled at
// draw time) - see ManeuverIcons.h. Normal-size icon draws (size ==
// kArrowSize, the overwhelming majority) therefore hit drawPng() at
// scale=1.0, LovyanGFX's simpler unscaled alpha-blit path. Only the
// full-screen special states (DEPART/ARRIVED, drawn at 130px) still scale
// these up at draw time - a deliberately accepted smaller residual exposure
// to the issue below, since those are brief, infrequent states.
//
// This used to be 192 (the asset's original resolution) with every icon
// scaled down live via drawPng()'s scale_x/scale_y. Two real bugs traced to
// that: (1) a stray colored-pixel artifact confirmed consistent on real
// hardware - the only code path able to produce a non-gray RGB value from
// our strictly-grayscale source PNGs is drawPng()'s non-readable-panel
// alpha-blend branch (LGFXBase.cpp's png_draw_alpha_scale_callback, used
// whenever scale != 1.0), which sets color directly from decoded pixel
// bytes - a scaled+alpha+non-readable-panel combination that's far less
// exercised upstream than plain 1:1 blits. (2) roundabout icons clipping:
// measured their actual ink margin against the 192x192 canvas at only
// 8-17px, versus 15-62px for every other icon - any few-pixel rounding
// slip in the scale/clip math (ceil(width*zoom) etc.) is invisible on
// icons with generous margin but visibly clips ones with almost none.
// Pre-sizing offline removes the runtime scale math (and therefore both
// failure modes) for the common case entirely, rather than chasing the
// exact rounding/decode bug inside LovyanGFX itself.
constexpr float kPngNativeSize = 82.0f;

// Draws a maneuver PNG (ManeuverIcons::*_png / *_png_len) centered at
// (cx, cy). `color` is accepted for signature symmetry with the vector draw
// functions but is NOT applied to the icon itself - these assets are
// grayscale+alpha (opaque pixels are either pure black or mid-gray, no
// anti-aliasing), not a recolorable flat-color glyph. That's fine now that
// UITheme::kBackground is white: the icons' own black/gray pixels already
// read correctly directly against it, no backing badge needed (there used
// to be one, for the previous near-black theme - removed along with that
// theme). Known tradeoff accepted when every other maneuver moved to this
// PNG set for visual consistency: these icons don't recolor for
// TURN_APPROACHING's yellow accent (only the two remaining vector icons
// still do), and the numbered roundabout-exit badge (e.g. "take the 3rd
// exit") that the old vector roundabout drew is gone - no PNG variant
// encodes an arbitrary number.
void drawPngIcon(lgfx::LGFX_Device& c, const uint8_t* data, uint32_t len, int cx, int cy, int size, uint16_t color) {
    (void)color;
    const float scale = size / kPngNativeSize;
    // drawPng's (x, y) is the TOP-LEFT corner of the maxWidth x maxHeight
    // box it clips/positions into - confirmed by tracing straight into its
    // setClipRect(x, y, maxWidth, maxHeight) call - NOT a datum-relative
    // center point the way text/shape datums work.
    const int32_t left = cx - size / 2;
    const int32_t top = cy - size / 2;
    c.drawPng(data, len, left, top, size, size, 0, 0, scale, scale, top_left);
}

} // namespace

namespace NavigationArrow {

void draw(lgfx::LGFX_Device& canvas, ManeuverType maneuver, int cx, int cy, int size, uint16_t color, uint8_t exitNumber) {
    switch (maneuver) {
        case ManeuverType::STRAIGHT:
            drawPngIcon(canvas, ManeuverIcons::ic_straight_png, ManeuverIcons::ic_straight_png_len, cx, cy, size, color);
            break;
        case ManeuverType::TURN_LEFT:
            drawPngIcon(canvas, ManeuverIcons::ic_turn_left_png, ManeuverIcons::ic_turn_left_png_len, cx, cy, size, color);
            break;
        case ManeuverType::TURN_RIGHT:
            drawPngIcon(canvas, ManeuverIcons::ic_turn_right_png, ManeuverIcons::ic_turn_right_png_len, cx, cy, size, color);
            break;
        case ManeuverType::SLIGHT_LEFT:
            drawPngIcon(canvas, ManeuverIcons::ic_turn_slight_left_png, ManeuverIcons::ic_turn_slight_left_png_len, cx, cy, size, color);
            break;
        case ManeuverType::SLIGHT_RIGHT:
            drawPngIcon(canvas, ManeuverIcons::ic_turn_slight_right_png, ManeuverIcons::ic_turn_slight_right_png_len, cx, cy, size, color);
            break;
        case ManeuverType::SHARP_LEFT:
            drawPngIcon(canvas, ManeuverIcons::ic_turn_sharp_left_png, ManeuverIcons::ic_turn_sharp_left_png_len, cx, cy, size, color);
            break;
        case ManeuverType::SHARP_RIGHT:
            drawPngIcon(canvas, ManeuverIcons::ic_turn_sharp_right_png, ManeuverIcons::ic_turn_sharp_right_png_len, cx, cy, size, color);
            break;

        // The repo labels u-turns by circulation direction (clockwise/
        // counterclockwise), not left/right. Turning left sweeps your
        // heading counterclockwise, so CCW -> U_TURN_LEFT, CW -> U_TURN_RIGHT.
        case ManeuverType::U_TURN_LEFT:
            drawPngIcon(canvas, ManeuverIcons::ic_turn_u_turn_counterclockwise_png, ManeuverIcons::ic_turn_u_turn_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::U_TURN_RIGHT:
            drawPngIcon(canvas, ManeuverIcons::ic_turn_u_turn_clockwise_png, ManeuverIcons::ic_turn_u_turn_clockwise_png_len, cx, cy, size, color);
            break;

        // No dedicated KEEP_LEFT/RIGHT asset exists in this set - reuses the
        // FORK art (keep/fork are visually similar maneuvers).
        case ManeuverType::KEEP_LEFT:
        case ManeuverType::FORK_LEFT:
            drawPngIcon(canvas, ManeuverIcons::ic_fork_left_png, ManeuverIcons::ic_fork_left_png_len, cx, cy, size, color);
            break;
        case ManeuverType::KEEP_RIGHT:
        case ManeuverType::FORK_RIGHT:
            drawPngIcon(canvas, ManeuverIcons::ic_fork_right_png, ManeuverIcons::ic_fork_right_png_len, cx, cy, size, color);
            break;

        case ManeuverType::MERGE:
            drawPngIcon(canvas, ManeuverIcons::ic_merge_png, ManeuverIcons::ic_merge_png_len, cx, cy, size, color);
            break;
        case ManeuverType::MERGE_LEFT:
            drawPngIcon(canvas, ManeuverIcons::ic_merge_left_png, ManeuverIcons::ic_merge_left_png_len, cx, cy, size, color);
            break;
        case ManeuverType::MERGE_RIGHT:
            drawPngIcon(canvas, ManeuverIcons::ic_merge_right_png, ManeuverIcons::ic_merge_right_png_len, cx, cy, size, color);
            break;

        // Plain ROUNDABOUT/_LEFT/_RIGHT (no sharp/slight/exit/u-turn/straight
        // qualifier) default to counterclockwise, same as the CW/CCW family
        // below - see ManeuverMapper for why CW/CCW can't be derived from
        // instruction text.
        case ManeuverType::ROUNDABOUT:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_counterclockwise_png, ManeuverIcons::ic_roundabout_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_LEFT:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_left_counterclockwise_png, ManeuverIcons::ic_roundabout_left_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_RIGHT:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_right_counterclockwise_png, ManeuverIcons::ic_roundabout_right_counterclockwise_png_len, cx, cy, size, color);
            break;

        // PNG-backed roundabout CW/CCW family (sharp/slight/exit/u-turn/
        // left/right/straight) - see ManeuverIcons.h.
        case ManeuverType::ROUNDABOUT_SHARP_LEFT_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_sharp_left_clockwise_png, ManeuverIcons::ic_roundabout_sharp_left_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_SHARP_LEFT_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_sharp_left_counterclockwise_png, ManeuverIcons::ic_roundabout_sharp_left_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_SHARP_RIGHT_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_sharp_right_clockwise_png, ManeuverIcons::ic_roundabout_sharp_right_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_SHARP_RIGHT_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_sharp_right_counterclockwise_png, ManeuverIcons::ic_roundabout_sharp_right_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_slight_left_clockwise_png, ManeuverIcons::ic_roundabout_slight_left_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_slight_left_counterclockwise_png, ManeuverIcons::ic_roundabout_slight_left_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_slight_right_clockwise_png, ManeuverIcons::ic_roundabout_slight_right_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_slight_right_counterclockwise_png, ManeuverIcons::ic_roundabout_slight_right_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_EXIT_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_exit_clockwise_png, ManeuverIcons::ic_roundabout_exit_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_EXIT_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_exit_counterclockwise_png, ManeuverIcons::ic_roundabout_exit_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_UTURN_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_u_turn_clockwise_png, ManeuverIcons::ic_roundabout_u_turn_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_UTURN_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_u_turn_counterclockwise_png, ManeuverIcons::ic_roundabout_u_turn_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_LEFT_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_left_clockwise_png, ManeuverIcons::ic_roundabout_left_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_LEFT_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_left_counterclockwise_png, ManeuverIcons::ic_roundabout_left_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_RIGHT_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_right_clockwise_png, ManeuverIcons::ic_roundabout_right_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_RIGHT_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_right_counterclockwise_png, ManeuverIcons::ic_roundabout_right_counterclockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_STRAIGHT_CW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_straight_clockwise_png, ManeuverIcons::ic_roundabout_straight_clockwise_png_len, cx, cy, size, color);
            break;
        case ManeuverType::ROUNDABOUT_STRAIGHT_CCW:
            drawPngIcon(canvas, ManeuverIcons::ic_roundabout_straight_counterclockwise_png, ManeuverIcons::ic_roundabout_straight_counterclockwise_png_len, cx, cy, size, color);
            break;

        case ManeuverType::EXIT_LEFT:  drawExit(canvas, cx, cy, size, false, color); break;
        case ManeuverType::EXIT_RIGHT: drawExit(canvas, cx, cy, size, true, color); break;

        case ManeuverType::DESTINATION:
        case ManeuverType::ARRIVED: // no dedicated "arrived" asset - reuses the destination pin
            drawPngIcon(canvas, ManeuverIcons::ic_destination_png, ManeuverIcons::ic_destination_png_len, cx, cy, size, color);
            break;
        case ManeuverType::DEPART:
            drawPngIcon(canvas, ManeuverIcons::ic_depart_png, ManeuverIcons::ic_depart_png_len, cx, cy, size, color);
            break;
        case ManeuverType::UNKNOWN:
            drawQuestionMark(canvas, cx, cy, size, color);
            break;
    }

    if (exitNumber > 0 && ManeuverMapper::isRoundabout(maneuver)) {
        drawExitNumberBadge(canvas, cx, cy, size, exitNumber);
    }
}

} // namespace NavigationArrow
