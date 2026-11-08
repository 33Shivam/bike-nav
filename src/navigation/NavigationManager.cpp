#include "NavigationManager.h"
#include <Arduino.h>
#include <cstring>

void NavigationManager::onRawNavigationUpdate(const RawNavigationSnapshot& s) {
    data_.active = s.active;
    data_.isTurnByTurn = s.isTurnByTurn;
    data_.hasIcon = s.hasIcon;
    data_.rawIconId = s.iconCRC;
    copyTruncated(data_.instruction, sizeof(data_.instruction), s.directions);
    copyTruncated(data_.distanceText, sizeof(data_.distanceText), s.distance);
    copyTruncated(data_.durationText, sizeof(data_.durationText), s.duration);
    copyTruncated(data_.etaText, sizeof(data_.etaText), s.eta);
    copyTruncated(data_.titleText, sizeof(data_.titleText), s.title);

    if (s.active && !lastActive_) {
        Serial.println("[NAV] Navigation started");
    } else if (!s.active && lastActive_) {
        Serial.println("[NAV] Navigation stopped");
    }

    if (s.active && s.isTurnByTurn) {
        const bool changed = !everLogged_ ||
                              s.iconCRC != lastLoggedIconId_ ||
                              std::strcmp(s.distance, lastLoggedDistance_) != 0 ||
                              std::strcmp(s.directions, lastLoggedInstruction_) != 0;
        if (changed) {
            Serial.println("[NAV] Maneuver updated");
            Serial.printf("  raw icon id (CRC) : 0x%08X\n", s.iconCRC);
            Serial.printf("  instruction       : %s\n", s.directions);
            Serial.printf("  distance          : %s\n", s.distance);
            Serial.printf("  eta               : %s\n", s.eta);

            lastLoggedIconId_ = s.iconCRC;
            copyTruncated(lastLoggedDistance_, sizeof(lastLoggedDistance_), s.distance);
            copyTruncated(lastLoggedInstruction_, sizeof(lastLoggedInstruction_), s.directions);
            everLogged_ = true;
        }
    }

    lastActive_ = s.active;
    lastIsTurnByTurn_ = s.isTurnByTurn;
}

void NavigationManager::onBluetoothDisconnected() {
    data_ = NavigationData{};
    lastActive_ = false;
    lastIsTurnByTurn_ = false;
    everLogged_ = false;
    lastLoggedIconId_ = 0xFFFFFFFF;
    lastLoggedDistance_[0] = '\0';
    lastLoggedInstruction_[0] = '\0';
}
