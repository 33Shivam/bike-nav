#pragma once

#include "BluetoothEvents.h"
#include <memory>

class ChronosAdapter; // defined in ChronosAdapter.h, which (transitively)
                        // pulls in ChronosESP32.h - only BluetoothManager.cpp
                        // includes that; this header stays Chronos-free so
                        // nothing that includes BluetoothManager.h (main.cpp,
                        // eventually the UI) ever sees ChronosESP32 types.

// Application-facing Bluetooth lifecycle + event forwarding. Every public
// member here uses only BluetoothEvents.h plain structs.
class BluetoothManager {
public:
    using ConnectionCallback = void (*)(bool connected);
    using NavigationCallback = void (*)(const RawNavigationSnapshot&);
    using NotificationCallback = void (*)(const RawNotification&);
    using CallCallback = void (*)(const RawCallEvent&);
    using MusicCallback = void (*)(const RawMusicSnapshot&);

    explicit BluetoothManager(const char* deviceName);
    ~BluetoothManager();

    void begin();

    // Call every Arduino loop() iteration. Pumps the adapter and dispatches
    // any new events to registered callbacks - always from loop() context,
    // never from a BLE callback.
    void update();

    bool isConnected() const;

    // See ChronosAdapter::getSyncedTime - false until the phone has completed
    // at least one time sync.
    bool getSyncedTime(uint8_t& hour, uint8_t& minute);

    void onConnectionChanged(ConnectionCallback cb) { connectionCb_ = cb; }
    void onNavigationUpdate(NavigationCallback cb) { navigationCb_ = cb; }
    void onNotification(NotificationCallback cb) { notificationCb_ = cb; }
    void onCallEvent(CallCallback cb) { callCb_ = cb; }
    void onMusicUpdate(MusicCallback cb) { musicCb_ = cb; }

private:
    std::unique_ptr<ChronosAdapter> adapter_;
    bool lastConnected_ = false;

    ConnectionCallback connectionCb_ = nullptr;
    NavigationCallback navigationCb_ = nullptr;
    NotificationCallback notificationCb_ = nullptr;
    CallCallback callCb_ = nullptr;
    MusicCallback musicCb_ = nullptr;
};
