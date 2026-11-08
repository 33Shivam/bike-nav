#include <Arduino.h>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include "display/DisplayManager.h"
#include "diagnostics/HardwareDiagnostics.h"
#include "ui/UIManager.h"
#include "ui/UITheme.h"

#ifndef BIKENAV_MOCK_MODE
#define BIKENAV_MOCK_MODE 0
#endif

#if BIKENAV_MOCK_MODE
#include "ui/DemoController.h"
#else
#include "bluetooth/BluetoothManager.h"
#include "navigation/NavigationManager.h"
#include "navigation/ManeuverMapper.h"
#endif

namespace {

DisplayManager displayManager;
UIManager uiManager(displayManager);

#if BIKENAV_MOCK_MODE
DemoController demoController;
#else
BluetoothManager bluetoothManager("BikeNav");
NavigationManager navigationManager;
#endif

uint32_t lastHeartbeatMs = 0;
constexpr uint32_t kHeartbeatIntervalMs = 10000;

#if !BIKENAV_MOCK_MODE

// Our Android companion app confirmed (via a real captured Google Maps
// session) that distance arrives as plain strings like "260 m" / "0 m" -
// this parses that observed format into meters rather than inventing a
// schema. Returns false (leaving outMeters untouched) for non-distance
// strings like "Rerouting..." or a bare heading ("Head east"), which Maps
// sometimes sends in this same slot when no turn distance is available yet.
bool tryParseDistanceMeters(const char* text, uint32_t& outMeters) {
    char* end = nullptr;
    float value = std::strtof(text, &end);
    if (end == text) return false; // no leading number at all

    // Android's locale-aware distance formatting separates the number and
    // unit with a non-breaking space (U+00A0, UTF-8 bytes 0xC2 0xA0), not a
    // plain ASCII space - it looks identical to a normal space in the Serial
    // Monitor, which is why this silently failed on every single real
    // distance string until caught by an explicit parse-success log. Skip
    // any non-letter byte(s) rather than just ' ' so this (and any other
    // stray whitespace-ish byte) doesn't matter.
    while (*end != '\0' && !std::isalpha(static_cast<unsigned char>(*end))) end++;
    if (*end == '\0') return false; // number with no unit isn't trustworthy

    const char unit0 = static_cast<char>(std::tolower(static_cast<unsigned char>(end[0])));
    const char unit1 = static_cast<char>(std::tolower(static_cast<unsigned char>(end[1])));

    float meters;
    if (unit0 == 'k' && unit1 == 'm') meters = value * 1000.0f;
    else if (unit0 == 'm' && unit1 == 'i') meters = value * 1609.34f;
    else if (unit0 == 'f' && unit1 == 't') meters = value * 0.3048f;
    else if (unit0 == 'm') meters = value;
    else return false;

    if (meters < 0.0f) return false;
    outMeters = static_cast<uint32_t>(meters + 0.5f);
    return true;
}

const char* maneuverTypeName(ManeuverType m) {
    switch (m) {
        case ManeuverType::STRAIGHT: return "STRAIGHT";
        case ManeuverType::TURN_LEFT: return "TURN_LEFT";
        case ManeuverType::TURN_RIGHT: return "TURN_RIGHT";
        case ManeuverType::SLIGHT_LEFT: return "SLIGHT_LEFT";
        case ManeuverType::SLIGHT_RIGHT: return "SLIGHT_RIGHT";
        case ManeuverType::SHARP_LEFT: return "SHARP_LEFT";
        case ManeuverType::SHARP_RIGHT: return "SHARP_RIGHT";
        case ManeuverType::U_TURN_LEFT: return "U_TURN_LEFT";
        case ManeuverType::U_TURN_RIGHT: return "U_TURN_RIGHT";
        case ManeuverType::KEEP_LEFT: return "KEEP_LEFT";
        case ManeuverType::KEEP_RIGHT: return "KEEP_RIGHT";
        case ManeuverType::MERGE: return "MERGE";
        case ManeuverType::MERGE_LEFT: return "MERGE_LEFT";
        case ManeuverType::MERGE_RIGHT: return "MERGE_RIGHT";
        case ManeuverType::ROUNDABOUT: return "ROUNDABOUT";
        case ManeuverType::ROUNDABOUT_LEFT: return "ROUNDABOUT_LEFT";
        case ManeuverType::ROUNDABOUT_RIGHT: return "ROUNDABOUT_RIGHT";
        case ManeuverType::FORK_LEFT: return "FORK_LEFT";
        case ManeuverType::FORK_RIGHT: return "FORK_RIGHT";
        case ManeuverType::EXIT_LEFT: return "EXIT_LEFT";
        case ManeuverType::EXIT_RIGHT: return "EXIT_RIGHT";
        case ManeuverType::DESTINATION: return "DESTINATION";
        case ManeuverType::DEPART: return "DEPART";
        case ManeuverType::ARRIVED: return "ARRIVED";
        case ManeuverType::UNKNOWN: return "UNKNOWN";
        case ManeuverType::ROUNDABOUT_SHARP_LEFT_CW: return "ROUNDABOUT_SHARP_LEFT_CW";
        case ManeuverType::ROUNDABOUT_SHARP_LEFT_CCW: return "ROUNDABOUT_SHARP_LEFT_CCW";
        case ManeuverType::ROUNDABOUT_SHARP_RIGHT_CW: return "ROUNDABOUT_SHARP_RIGHT_CW";
        case ManeuverType::ROUNDABOUT_SHARP_RIGHT_CCW: return "ROUNDABOUT_SHARP_RIGHT_CCW";
        case ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CW: return "ROUNDABOUT_SLIGHT_LEFT_CW";
        case ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CCW: return "ROUNDABOUT_SLIGHT_LEFT_CCW";
        case ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CW: return "ROUNDABOUT_SLIGHT_RIGHT_CW";
        case ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CCW: return "ROUNDABOUT_SLIGHT_RIGHT_CCW";
        case ManeuverType::ROUNDABOUT_EXIT_CW: return "ROUNDABOUT_EXIT_CW";
        case ManeuverType::ROUNDABOUT_EXIT_CCW: return "ROUNDABOUT_EXIT_CCW";
        case ManeuverType::ROUNDABOUT_UTURN_CW: return "ROUNDABOUT_UTURN_CW";
        case ManeuverType::ROUNDABOUT_UTURN_CCW: return "ROUNDABOUT_UTURN_CCW";
        case ManeuverType::ROUNDABOUT_LEFT_CW: return "ROUNDABOUT_LEFT_CW";
        case ManeuverType::ROUNDABOUT_LEFT_CCW: return "ROUNDABOUT_LEFT_CCW";
        case ManeuverType::ROUNDABOUT_RIGHT_CW: return "ROUNDABOUT_RIGHT_CW";
        case ManeuverType::ROUNDABOUT_RIGHT_CCW: return "ROUNDABOUT_RIGHT_CCW";
        case ManeuverType::ROUNDABOUT_STRAIGHT_CW: return "ROUNDABOUT_STRAIGHT_CW";
        case ManeuverType::ROUNDABOUT_STRAIGHT_CCW: return "ROUNDABOUT_STRAIGHT_CCW";
    }
    return "?";
}

void handleConnectionChanged(bool connected) {
    uiManager.state().connected = connected;
    if (!connected) {
        navigationManager.onBluetoothDisconnected();
        uiManager.state().screen = UIScreen::WAITING_FOR_PHONE;
    } else {
        uiManager.state().screen = UIScreen::IDLE;
    }
}

void handleNavigationUpdate(const RawNavigationSnapshot& snapshot) {
    navigationManager.onRawNavigationUpdate(snapshot);
    const NavigationData& nav = navigationManager.data();

    if (nav.active && nav.isTurnByTurn) {
        // Our own Android companion app forwards the real Google Maps
        // instruction text (e.g. "Turn right", "Sharp left", "At the
        // roundabout, turn right"), so unlike the old Chronos pipeline
        // (opaque icon bitmap + CRC, no semantic meaning) we can classify
        // the actual maneuver via keyword matching - see ManeuverMapper.
        // Street name and ETA are safe string pass-throughs. Distance is
        // parsed from the confirmed-observed "260 m" format (see
        // tryParseDistanceMeters); when the slot instead holds something
        // unparseable (a bare heading, "Rerouting..."), we keep the last
        // known numeric distance rather than overwrite it with a wrong
        // value.
        uiManager.state().nav.maneuver = ManeuverMapper::map(nav.instruction);
        Serial.printf("[MANEUVER] \"%s\" -> %s\n", nav.instruction, maneuverTypeName(uiManager.state().nav.maneuver));
        uiManager.state().nav.roundaboutExitNumber =
            ManeuverMapper::isRoundabout(uiManager.state().nav.maneuver)
                ? ManeuverMapper::extractExitNumber(nav.instruction)
                : 0;
        uiManager.state().nav.setStreetName(nav.instruction);
        uiManager.state().nav.setEta(nav.etaText);
        uiManager.state().nav.setDuration(nav.durationText); // currently always empty - our
                                                               // Android app doesn't extract a
                                                               // duration value yet; UI degrades
                                                               // gracefully to distance-only

        uint32_t meters;
        if (tryParseDistanceMeters(nav.distanceText, meters)) {
            uiManager.state().nav.distanceMeters = meters;
            uiManager.state().screen = meters <= UITheme::kApproachingThresholdMeters
                ? UIScreen::TURN_APPROACHING
                : UIScreen::NAVIGATION;
        } else {
            Serial.printf("[UI] distance parse FAILED for: \"%s\" (keeping %u m)\n", nav.distanceText, uiManager.state().nav.distanceMeters);
            if (uiManager.state().screen != UIScreen::TURN_APPROACHING) {
                uiManager.state().screen = UIScreen::NAVIGATION;
            }
        }
    } else if (!nav.active) {
        if (uiManager.state().screen == UIScreen::NAVIGATION || uiManager.state().screen == UIScreen::TURN_APPROACHING) {
            uiManager.state().screen = UIScreen::IDLE;
        }
    }
    // nav.active && !nav.isTurnByTurn: Chronos "general info" placeholder
    // (e.g. "Check Chronos app settings") - not real maneuver data, so the
    // UI is deliberately left as-is rather than showing it as a street name.
}

void handleNotification(const RawNotification& notification) {
    Serial.printf("[NOTIF] %s: %s - %s\n", notification.app, notification.title, notification.message);
}

void handleCallEvent(const RawCallEvent& call) {
    if (call.ringing) {
        Serial.printf("[CALL] Incoming: %s\n", call.caller);
        uiManager.state().screen = UIScreen::INCOMING_CALL;
        uiManager.state().call.setCallerName(call.caller);
    } else {
        Serial.println("[CALL] Dismissed");
        if (uiManager.state().screen == UIScreen::INCOMING_CALL) {
            uiManager.state().screen = uiManager.state().connected ? UIScreen::IDLE : UIScreen::WAITING_FOR_PHONE;
        }
    }
}

void handleMusicUpdate(const RawMusicSnapshot& music) {
    uiManager.state().music.isPlaying = music.isPlaying;
    uiManager.state().music.setTitle(music.title);
    uiManager.state().music.setArtist(music.artist);
    // Chronos has no "music session ended" signal, only play/pause state,
    // so once music appears the screen stays MUSIC until disconnect or a
    // call interrupts it.
    if (uiManager.state().screen != UIScreen::INCOMING_CALL) {
        uiManager.state().screen = UIScreen::MUSIC;
    }
}

#endif // !BIKENAV_MOCK_MODE

} // namespace

void setup() {
    Serial.begin(115200);
    uint32_t serialWaitStart = millis();
    while (!Serial && millis() - serialWaitStart < 2000) {
        // Give USB-CDC serial a moment to enumerate; not a long-term block.
    }

    HardwareDiagnostics::printBootBanner();
    HardwareDiagnostics::printChipInfo();
    HardwareDiagnostics::printMemoryInfo();

    bool displayOk = displayManager.begin();
    HardwareDiagnostics::printDisplayStatus(displayOk);

    if (displayOk) {
        uiManager.begin();

#if BIKENAV_MOCK_MODE
        Serial.println("[APP] Mock demo mode enabled (BIKENAV_MOCK_MODE=1)");
#else
        bluetoothManager.onConnectionChanged(handleConnectionChanged);
        bluetoothManager.onNavigationUpdate(handleNavigationUpdate);
        bluetoothManager.onNotification(handleNotification);
        bluetoothManager.onCallEvent(handleCallEvent);
        bluetoothManager.onMusicUpdate(handleMusicUpdate);
        bluetoothManager.begin();
#endif
    }
}

void loop() {
    uint32_t now = millis();
    if (now - lastHeartbeatMs >= kHeartbeatIntervalMs) {
        lastHeartbeatMs = now;
        HardwareDiagnostics::printMemoryInfo();
    }

#if BIKENAV_MOCK_MODE
    demoController.update(uiManager.state());
#else
    bluetoothManager.update();

    uint8_t clockHour, clockMinute;
    if (bluetoothManager.getSyncedTime(clockHour, clockMinute)) {
        if (!uiManager.state().clockValid) {
            Serial.printf("[TIME] synced from phone: %02u:%02u\n", clockHour, clockMinute);
        }
        uiManager.state().clockHour = clockHour;
        uiManager.state().clockMinute = clockMinute;
        uiManager.state().clockValid = true;
    }
#endif
    uiManager.update();
}
