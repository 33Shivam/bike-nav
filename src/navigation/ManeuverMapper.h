#pragma once

#include "../ui/UIState.h"
#include <cstdint>

// Classifies a Google-Maps-style turn instruction string (as forwarded by
// our own Android companion app - see ~/bikenav-android) into a UI maneuver
// icon via keyword matching. This is a fundamentally different, better-
// grounded mapping than the Chronos-era "guess meaning from an opaque icon
// bitmap CRC" problem: we now have real instruction text with real semantic
// content, confirmed against live captures ("Turn right", "Sharp left",
// "At the roundabout, turn right").
namespace ManeuverMapper {

// Falls back to STRAIGHT (not UNKNOWN) for unrecognized text - UNKNOWN is a
// rare full-screen fallback state in NavigationScreen that hides the real
// distance/street data we have, which we don't want for routine phrases
// like "Continue" or "towards <street name>".
ManeuverType map(const char* instruction);

// Extracts a roundabout exit number from phrasing like "take the 2nd exit"
// (0 if none found). Only meaningful when map() returned a ROUNDABOUT*
// type; harmless to call unconditionally otherwise.
uint8_t extractExitNumber(const char* instruction);

// True for any of the ROUNDABOUT* family (bare, CW/CCW-qualified, or the
// legacy non-direction-qualified LEFT/RIGHT values) - used to gate whether
// an exit-number badge is meaningful for a given maneuver.
bool isRoundabout(ManeuverType type);

} // namespace ManeuverMapper
