#include "ManeuverMapper.h"
#include <cstring>
#include <cstdlib>
#include <cctype>

namespace {

void toLowerCopy(const char* src, char* dst, size_t dstSize) {
    size_t i = 0;
    for (; src[i] != '\0' && i < dstSize - 1; i++) {
        dst[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(src[i])));
    }
    dst[i] = '\0';
}

bool has(const char* haystack, const char* needle) {
    return std::strstr(haystack, needle) != nullptr;
}

} // namespace

namespace ManeuverMapper {

ManeuverType map(const char* instruction) {
    char lower[128];
    toLowerCopy(instruction, lower, sizeof(lower));

    const bool left = has(lower, "left");
    const bool right = has(lower, "right");

    // Order matters: more specific phrases are checked before the generic
    // "turn" + direction fallback, since e.g. "sharp left" and "turn right"
    // both contain "left"/"right".
    //
    // Roundabout is checked first (before the generic u-turn/sharp/slight
    // checks below) so a "roundabout" instruction never falls through to
    // the plain TURN/U_TURN/SHARP/SLIGHT types meant for ordinary turns.
    //
    // Google Maps doesn't always say the literal word "roundabout" - for a
    // named traffic circle it instead says "At <Circle Name>, take the Nth
    // exit" (confirmed via live capture 2026-11-08: "At Teen Murti-Haifa
    // Chowk, take the 4th exit and stay on Teen Murti Marg", which has no
    // "roundabout" keyword at all and was falling through to STRAIGHT). An
    // ordinal exit number ("2nd", "4th", ...) next to the word "exit" is a
    // reliable roundabout-only signal - ordinary highway exits are phrased
    // "take the exit toward X" with no ordinal number.
    const bool namedCircleExit = has(lower, "exit") && extractExitNumber(instruction) > 0;
    if (has(lower, "roundabout") || namedCircleExit) {
        // CW/CCW: which way the roundabout is circulated depends on which
        // side of the road the country drives on (left-hand traffic ->
        // clockwise, right-hand traffic -> counterclockwise) - this is NOT
        // derivable from instruction text alone, and neither this mapper nor
        // NavigationInfo currently carries a locale/region signal, so a
        // proper per-region answer needs a locale source wired in from the
        // phone side. Hardcoded to CW here instead of the previous CCW
        // default: every route this firmware has actually been tested/used
        // on is in India (left-hand traffic, clockwise roundabouts) -
        // confirmed wrong-direction on real hardware 2026-11-08 with the
        // old CCW default. Revisit if this ships outside India.
        if (has(lower, "u-turn") || has(lower, "u turn")) {
            return ManeuverType::ROUNDABOUT_UTURN_CW;
        }
        if (has(lower, "sharp")) {
            return left ? ManeuverType::ROUNDABOUT_SHARP_LEFT_CW : ManeuverType::ROUNDABOUT_SHARP_RIGHT_CW;
        }
        if (has(lower, "slight")) {
            return left ? ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CW : ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CW;
        }
        if (has(lower, "straight") || has(lower, "continue")) {
            return ManeuverType::ROUNDABOUT_STRAIGHT_CW;
        }
        if (left) return ManeuverType::ROUNDABOUT_LEFT_CW;
        if (right) return ManeuverType::ROUNDABOUT_RIGHT_CW;
        if (has(lower, "exit")) return ManeuverType::ROUNDABOUT_EXIT_CW; // "take the exit", no direction/degree word
        // e.g. "take the 2nd exit" with no left/right/sharp/slight word -
        // no bare clockwise asset exists for this one (only
        // ic_roundabout_counterclockwise_png - see NavigationArrow.cpp), so
        // this specific case still renders counterclockwise until a CW
        // asset is added.
        return ManeuverType::ROUNDABOUT;
    }
    if (has(lower, "u-turn") || has(lower, "u turn")) {
        return left ? ManeuverType::U_TURN_LEFT : ManeuverType::U_TURN_RIGHT;
    }
    if (has(lower, "sharp")) {
        return left ? ManeuverType::SHARP_LEFT : ManeuverType::SHARP_RIGHT;
    }
    if (has(lower, "slight")) {
        return left ? ManeuverType::SLIGHT_LEFT : ManeuverType::SLIGHT_RIGHT;
    }
    if (has(lower, "keep")) {
        return left ? ManeuverType::KEEP_LEFT : ManeuverType::KEEP_RIGHT;
    }
    if (has(lower, "fork")) {
        return left ? ManeuverType::FORK_LEFT : ManeuverType::FORK_RIGHT;
    }
    if (has(lower, "merge") || has(lower, "join")) {
        // Google Maps uses "Join with <road>" as a synonym for "Merge onto
        // <road>" - confirmed via live capture 2026-11-08 ("Join with
        // Mahatma Gandhi Rd/Ring Rd").
        if (left) return ManeuverType::MERGE_LEFT;
        if (right) return ManeuverType::MERGE_RIGHT;
        return ManeuverType::MERGE; // e.g. "Merge onto I-95", no direction word
    }
    if (has(lower, "exit") && (left || right)) {
        return left ? ManeuverType::EXIT_LEFT : ManeuverType::EXIT_RIGHT;
    }
    if (has(lower, "turn")) {
        if (left) return ManeuverType::TURN_LEFT;
        if (right) return ManeuverType::TURN_RIGHT;
    }
    if (has(lower, "destination") || has(lower, "arrive")) {
        return ManeuverType::ARRIVED;
    }
    if (left) return ManeuverType::TURN_LEFT;
    if (right) return ManeuverType::TURN_RIGHT;

    return ManeuverType::STRAIGHT; // "Continue", "towards <street>", or anything unrecognized
}

uint8_t extractExitNumber(const char* instruction) {
    char lower[128];
    toLowerCopy(instruction, lower, sizeof(lower));

    for (const char* p = lower; *p != '\0'; p++) {
        if (std::isdigit(static_cast<unsigned char>(*p))) {
            char* end = nullptr;
            long n = std::strtol(p, &end, 10);
            if (n > 0 && n < 20 &&
                (std::strncmp(end, "st", 2) == 0 || std::strncmp(end, "nd", 2) == 0 ||
                 std::strncmp(end, "rd", 2) == 0 || std::strncmp(end, "th", 2) == 0)) {
                return static_cast<uint8_t>(n);
            }
            p = end - 1; // resume scanning right after this number
        }
    }
    return 0;
}

bool isRoundabout(ManeuverType type) {
    switch (type) {
        case ManeuverType::ROUNDABOUT:
        case ManeuverType::ROUNDABOUT_LEFT:
        case ManeuverType::ROUNDABOUT_RIGHT:
        case ManeuverType::ROUNDABOUT_SHARP_LEFT_CW:
        case ManeuverType::ROUNDABOUT_SHARP_LEFT_CCW:
        case ManeuverType::ROUNDABOUT_SHARP_RIGHT_CW:
        case ManeuverType::ROUNDABOUT_SHARP_RIGHT_CCW:
        case ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CW:
        case ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CCW:
        case ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CW:
        case ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CCW:
        case ManeuverType::ROUNDABOUT_EXIT_CW:
        case ManeuverType::ROUNDABOUT_EXIT_CCW:
        case ManeuverType::ROUNDABOUT_UTURN_CW:
        case ManeuverType::ROUNDABOUT_UTURN_CCW:
        case ManeuverType::ROUNDABOUT_LEFT_CW:
        case ManeuverType::ROUNDABOUT_LEFT_CCW:
        case ManeuverType::ROUNDABOUT_RIGHT_CW:
        case ManeuverType::ROUNDABOUT_RIGHT_CCW:
        case ManeuverType::ROUNDABOUT_STRAIGHT_CW:
        case ManeuverType::ROUNDABOUT_STRAIGHT_CCW:
            return true;
        default:
            return false;
    }
}

} // namespace ManeuverMapper
