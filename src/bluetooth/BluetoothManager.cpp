#include "BluetoothManager.h"
#include "ChronosAdapter.h"
#include <Arduino.h>

BluetoothManager::BluetoothManager(const char* deviceName)
    : adapter_(new ChronosAdapter(deviceName)) {}

BluetoothManager::~BluetoothManager() = default;

bool BluetoothManager::isConnected() const {
    return adapter_->isConnected();
}

bool BluetoothManager::getSyncedTime(uint8_t& hour, uint8_t& minute) {
    return adapter_->getSyncedTime(hour, minute);
}

void BluetoothManager::begin() {
    Serial.println("[BLE] Initializing Chronos");
    adapter_->begin();
    Serial.println("[BLE] Advertising started");
}

void BluetoothManager::update() {
    adapter_->loop();

    const bool connected = adapter_->isConnected();
    if (connected != lastConnected_) {
        lastConnected_ = connected;
        Serial.println(connected ? "[BLE] Device connected" : "[BLE] Device disconnected");
        if (connectionCb_) connectionCb_(connected);
    }

    RawNavigationSnapshot nav;
    if (adapter_->takeNavigationUpdate(nav) && navigationCb_) {
        navigationCb_(nav);
    }

    RawNotification notification;
    if (adapter_->takeNotification(notification) && notificationCb_) {
        notificationCb_(notification);
    }

    RawCallEvent call;
    if (adapter_->takeCallEvent(call) && callCb_) {
        callCb_(call);
    }

    RawMusicSnapshot music;
    if (adapter_->takeMusicUpdate(music) && musicCb_) {
        musicCb_(music);
    }
}
