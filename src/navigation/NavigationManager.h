#pragma once

#include "NavigationData.h"
#include "../bluetooth/BluetoothEvents.h"

// Owns the latest application-level navigation state. Converts transport
// events (RawNavigationSnapshot, transport-agnostic) into NavigationData,
// and logs structured, de-duplicated [NAV] diagnostics. Knows nothing about
// ChronosESP32 or the UI.
class NavigationManager {
public:
    void onRawNavigationUpdate(const RawNavigationSnapshot& snapshot);
    void onBluetoothDisconnected();

    const NavigationData& data() const { return data_; }

private:
    NavigationData data_;

    // De-dup state so repeated identical updates don't flood Serial.
    bool everLogged_ = false;
    bool lastActive_ = false;
    bool lastIsTurnByTurn_ = false;
    uint32_t lastLoggedIconId_ = 0xFFFFFFFF;
    char lastLoggedDistance_[32] = "";
    char lastLoggedInstruction_[128] = "";
};
