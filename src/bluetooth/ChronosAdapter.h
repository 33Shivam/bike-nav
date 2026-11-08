#pragma once

#include <ChronosESP32.h>
#include <atomic>
#include "BluetoothEvents.h"

// The ONLY file in this project allowed to touch ChronosESP32 types
// directly. Everything it hands out is a plain BluetoothEvents.h struct.
//
// ChronosESP32's callbacks (connection/notification/configuration/ringer)
// all fire synchronously from NimBLE's BLE host task, never from Arduino's
// loop() task (verified by reading onWrite/onConnect/onDisconnect in the
// library source). So callbacks here only ever copy data into a small
// spinlock-protected pending slot per event category; nothing is drawn,
// logged, or dispatched to the rest of the app from within them. The
// Arduino loop task drains those slots via the take*() methods below.
class ChronosAdapter {
public:
    explicit ChronosAdapter(const char* deviceName);

    void begin();
    void loop(); // call every Arduino loop(): pumps ChronosESP32's own housekeeping

    bool isConnected() const { return connected_.load(std::memory_order_relaxed); }

    // Each returns true and fills `out` if a new event of that category
    // arrived since the last call; false (out untouched) otherwise. Safe to
    // call only from the Arduino loop task.
    bool takeNavigationUpdate(RawNavigationSnapshot& out);
    bool takeNotification(RawNotification& out);
    bool takeMusicUpdate(RawMusicSnapshot& out);
    bool takeCallEvent(RawCallEvent& out);

    // Returns true and fills hour (24h)/minute from the library's ESP32Time
    // base class if a phone has ever completed a time sync (wire command
    // 0x93, CF_TIME config callback) since boot; false (out params untouched)
    // otherwise - callers must not treat a false return as "00:00", since the
    // RTC defaults to its library-internal epoch until a real sync happens.
    bool getSyncedTime(uint8_t& hour, uint8_t& minute);

private:
    ChronosESP32 chronos_;

    std::atomic<bool> connected_{false};
    std::atomic<bool> timeSynced_{false};

    // Without periodic BLE traffic from the ESP32, Android can background-
    // throttle the Chronos app's notification listener after a while,
    // silently cutting off navigation data even though the BLE link stays
    // connected. syncRequest() is a real, public, intentional round-trip
    // (not a packet we're inventing) that keeps that listener alive.
    uint32_t lastKeepAliveMs_ = 0;

    portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;

    std::atomic<bool> navPending_{false};
    std::atomic<bool> notifPending_{false};
    std::atomic<bool> musicPending_{false};
    std::atomic<bool> callPending_{false};

    RawNavigationSnapshot pendingNav_;
    RawNotification pendingNotif_;
    RawMusicSnapshot pendingMusic_;
    RawCallEvent pendingCall_;

    // Bridges for ChronosESP32's C-style callback pointers, which cannot
    // bind to non-static member functions. Only one ChronosAdapter is ever
    // created in this project.
    static ChronosAdapter* instance_;
    static void onConnectionTrampoline(bool connected);
    static void onNotificationTrampoline(Notification notification);
    static void onConfigTrampoline(Config config, uint32_t a, uint32_t b);
    static void onRingerTrampoline(String caller, bool ringing);

    void handleConnection(bool connected);
    void handleNotification(const Notification& notification);
    void handleConfig(Config config, uint32_t a, uint32_t b);
    void handleRinger(const String& caller, bool ringing);
};
