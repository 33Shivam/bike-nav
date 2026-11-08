#pragma once

#include <cstdint>
#include <cstring>

// Pure UI-layer state model. No Chronos / BLE types may appear here -
// a future transport layer populates these plain fields directly.

enum class UIScreen : uint8_t {
    WAITING_FOR_PHONE,
    IDLE,
    NAVIGATION,
    TURN_APPROACHING,
    INCOMING_CALL,
    MUSIC
};

enum class ManeuverType : uint8_t {
    STRAIGHT,
    TURN_LEFT,
    TURN_RIGHT,
    SLIGHT_LEFT,
    SLIGHT_RIGHT,
    SHARP_LEFT,
    SHARP_RIGHT,
    U_TURN_LEFT,
    U_TURN_RIGHT,
    KEEP_LEFT,
    KEEP_RIGHT,
    MERGE,
    MERGE_LEFT,
    MERGE_RIGHT,
    ROUNDABOUT,
    ROUNDABOUT_LEFT,
    ROUNDABOUT_RIGHT,
    FORK_LEFT,
    FORK_RIGHT,
    EXIT_LEFT,
    EXIT_RIGHT,
    DESTINATION,
    DEPART,
    ARRIVED,
    UNKNOWN,

    // --- PNG-backed maneuvers (drawn via NavigationArrow's drawPngIcon path,
    // not hand-drawn vector geometry like everything above) - bitmap source:
    // github.com/pdhruv93/minimal-nav. _CW/_CCW distinguish which direction
    // traffic circulates the roundabout (left-hand-traffic countries go
    // clockwise, right-hand-traffic countries go counterclockwise) - this is
    // NOT derivable from maneuver instruction text alone; see ManeuverMapper.
    ROUNDABOUT_SHARP_LEFT_CW,
    ROUNDABOUT_SHARP_LEFT_CCW,
    ROUNDABOUT_SHARP_RIGHT_CW,
    ROUNDABOUT_SHARP_RIGHT_CCW,
    ROUNDABOUT_SLIGHT_LEFT_CW,
    ROUNDABOUT_SLIGHT_LEFT_CCW,
    ROUNDABOUT_SLIGHT_RIGHT_CW,
    ROUNDABOUT_SLIGHT_RIGHT_CCW,
    ROUNDABOUT_EXIT_CW,
    ROUNDABOUT_EXIT_CCW,
    ROUNDABOUT_UTURN_CW,
    ROUNDABOUT_UTURN_CCW,
    ROUNDABOUT_LEFT_CW,
    ROUNDABOUT_LEFT_CCW,
    ROUNDABOUT_RIGHT_CW,
    ROUNDABOUT_RIGHT_CCW,
    ROUNDABOUT_STRAIGHT_CW,
    ROUNDABOUT_STRAIGHT_CCW
};

struct NavigationInfo {
    ManeuverType maneuver = ManeuverType::STRAIGHT;
    uint32_t distanceMeters = 0;
    char streetName[48] = "";
    char eta[16] = "";
    char duration[16] = ""; // e.g. "6 min" - empty when the transport doesn't provide one
    uint8_t roundaboutExitNumber = 0; // 0 = no numbered-exit badge

    void setStreetName(const char* s) { std::strncpy(streetName, s, sizeof(streetName) - 1); streetName[sizeof(streetName) - 1] = '\0'; }
    void setEta(const char* s) { std::strncpy(eta, s, sizeof(eta) - 1); eta[sizeof(eta) - 1] = '\0'; }
    void setDuration(const char* s) { std::strncpy(duration, s, sizeof(duration) - 1); duration[sizeof(duration) - 1] = '\0'; }
};

struct MusicInfo {
    char title[32] = "";
    char artist[32] = "";
    bool isPlaying = false;

    void setTitle(const char* s) { std::strncpy(title, s, sizeof(title) - 1); title[sizeof(title) - 1] = '\0'; }
    void setArtist(const char* s) { std::strncpy(artist, s, sizeof(artist) - 1); artist[sizeof(artist) - 1] = '\0'; }
};

struct CallInfo {
    char callerName[32] = "";

    void setCallerName(const char* s) { std::strncpy(callerName, s, sizeof(callerName) - 1); callerName[sizeof(callerName) - 1] = '\0'; }
};

struct UIState {
    UIScreen screen = UIScreen::WAITING_FOR_PHONE;
    bool connected = false;
    bool nightMode = false;
    uint8_t brightness = 255;

    uint8_t clockHour = 0;
    uint8_t clockMinute = 0;
    bool clockValid = false; // true once a real time source (demo mode, or a
                              // BLE time sync in live mode) has set the clock -
                              // IdleScreen shows a placeholder instead of a
                              // misleading 00:00 until this is true

    NavigationInfo nav;
    MusicInfo music;
    CallInfo call;
};
