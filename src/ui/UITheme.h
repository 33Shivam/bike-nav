#pragma once

#include <LovyanGFX.hpp>
#include <cstdint>

// Centralized UI palette and layout constants, mirroring how config.h
// centralizes hardware configuration. No drawing code should hardcode
// a color or layout number outside this file.

namespace UITheme {

// --- Palette (light theme: white background, black foreground) ---
// Inverted from the original Figma-matched near-black theme at the user's
// explicit request, app-wide (every screen, not just NavigationScreen) -
// mainly so the imported PNG maneuver icons (black/gray pixels, can't be
// recolored - see NavigationArrow.cpp) read correctly without needing a
// white backing badge behind them.
constexpr uint16_t kBackground      = TFT_WHITE;
constexpr uint16_t kBezelOutline    = 0xCEBC; // light slate gray (#CBD5E1) - subtle ring on white, same intent as the old dark-on-black ring
constexpr uint16_t kPrimaryText     = TFT_BLACK;
constexpr uint16_t kSecondaryText   = TFT_DARKGREY; // shared by IdleScreen/NotificationScreen - mid-gray reads as "dimmed" on either background
// #475569 (slate-600) - NavigationScreen's duration line only. Darker than
// kSecondaryText's mid-gray so it stays legible as "dimmed" against white
// (the old #94A3B8 slate-400 was tuned for a dark background and would be
// too light/low-contrast here).
constexpr uint16_t kNavDurationColor = 0x42AD;
constexpr uint16_t kStreetTextColor  = TFT_BLACK; // same prominence as ETA/distance, not dimmed
constexpr uint16_t kNavigationColor = TFT_BLACK;   // normal navigation
constexpr uint16_t kApproachingColor = TFT_BLACK; // approaching maneuver - no color distinction, same as kNavigationColor
constexpr uint16_t kWarningColor    = TFT_RED;     // warning / disconnected
constexpr uint16_t kConnectedColor  = TFT_GREEN;   // connected state
constexpr uint16_t kRingTrackColor  = TFT_DARKGREY;

// --- Screen geometry (240x240 round panel) ---
constexpr int kScreenCenterX = 120;
constexpr int kScreenCenterY = 120;
constexpr int kSafeRadius    = 116; // usable radius before the bezel clips content

// --- Navigation screen layout (reference: bold arrow, big distance,
// two-line street name - no ETA, no ring, no connection dot on this
// screen) ---
// Y positions measured directly off the Figma dev-mode SVG export (each
// maneuver frame is exported as an exact 240x240 screen rect, so its
// internal coordinates map 1:1 onto our real panel pixels - no scaling
// guesswork). Previously hand-guessed values ran ~5-7px high across the
// board versus the reference.
// Re-spaced after removing the ETA block: the icon/distance/street-name
// group is shifted up and spread out (bigger gaps between each) so it reads
// as one centered composition on the now-taller empty top area, instead of
// three tightly-packed elements sitting low in the circle.
constexpr int kSubInfoTextY   = 38; // duration summary line (rarely populated - see drawSubInfo)
constexpr int kArrowCenterX   = 120;
constexpr int kArrowCenterY   = 79;
constexpr int kArrowSize      = 82;
constexpr int kRingOuterRadius = 114; // unused by NavigationScreen now, kept for NavigationRing component
constexpr int kRingThickness   = 8;
constexpr int kDistanceTextY  = 144;
// 13px apart (the originally Figma-measured gap) overlapped on real hardware -
// InterRegular16's actual rendered line height needs more room than that.
// Widened based on what's visibly correct on the device, not re-derived from
// the reference. The 19px gap between these two is the inter-line spacing
// within the street-name block - kept as-is while the whole group above
// shifted up for the re-spacing described above.
constexpr int kStreetLine1Y   = 177;
constexpr int kStreetLine2Y   = 196;
constexpr int kStreetLineMaxChars = 17; // Inter is wider per character than the old bitmap font

// Ring "lookahead" window: progress fraction shown = min(1, distance / this)
constexpr float kRingLookaheadMeters = 500.0f;

// Distance formatting / emphasis thresholds (configurable per spec).
constexpr uint32_t kApproachingThresholdMeters = 80;
constexpr uint32_t kLargeTextDistanceMeters    = 150;

// --- Demo mode (active only when built with BIKENAV_MOCK_MODE=1) ---
constexpr uint32_t kDemoStepIntervalMs    = 4000;
constexpr uint32_t kDemoDistanceTickMs    = 500;
constexpr uint32_t kDemoDistanceStepM     = 20;
constexpr uint32_t kDemoMinDistanceM      = 20;

// --- Night mode ---
constexpr uint8_t kNightModeBrightness = 60;
constexpr uint8_t kDayModeBrightness   = 255;

} // namespace UITheme
