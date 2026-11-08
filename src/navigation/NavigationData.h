#pragma once

#include <cstdint>

// Application-level navigation model. Every field is populated only from
// data the connected transport (ChronosESP32 today) actually provides -
// there is no destination field because the library exposes none, and no
// normalized distanceMeters because the library only gives a pre-formatted,
// locale-dependent distance string that can't be parsed without guessing.
struct NavigationData {
    bool active = false;       // a navigation session exists
    bool isTurnByTurn = false; // false => library placeholder/info text, not a real maneuver

    char instruction[128] = ""; // street name / instruction text ("directions")
    char distanceText[32] = "";
    char durationText[32] = "";
    char etaText[32] = "";
    char titleText[48] = "";

    bool hasIcon = false;
    uint32_t rawIconId = 0; // raw maneuver/icon identifier (bitmap CRC) for
                             // diagnostics and the future mapping milestone -
                             // not yet mapped to any symbolic maneuver type
};
