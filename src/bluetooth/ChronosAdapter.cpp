#include "ChronosAdapter.h"
#include <Arduino.h>

ChronosAdapter* ChronosAdapter::instance_ = nullptr;

ChronosAdapter::ChronosAdapter(const char* deviceName)
    : chronos_(String(deviceName), CS_240x240_128_CTF) { // 240x240, 1.28", round - matches this board exactly
    instance_ = this;
}

void ChronosAdapter::begin() {
    chronos_.setConnectionCallback(&ChronosAdapter::onConnectionTrampoline);
    chronos_.setNotificationCallback(&ChronosAdapter::onNotificationTrampoline);
    chronos_.setConfigurationCallback(&ChronosAdapter::onConfigTrampoline);
    chronos_.setRingerCallback(&ChronosAdapter::onRingerTrampoline);

    chronos_.begin();
    Serial.print("[BLE] MAC address: ");
    Serial.println(chronos_.getAddress());
    // No onboard battery sensor on this board (USB/vehicle powered); 100
    // just tells the Chronos app "not battery constrained" rather than
    // reporting a fabricated reading.
    chronos_.setBattery(100, true);
}

namespace {
constexpr uint32_t kKeepAliveIntervalMs = 20000;
}

void ChronosAdapter::loop() {
    chronos_.loop();

    if (connected_.load(std::memory_order_relaxed)) {
        const uint32_t now = millis();
        if (now - lastKeepAliveMs_ >= kKeepAliveIntervalMs) {
            lastKeepAliveMs_ = now;
            chronos_.syncRequest();
        }
    } else {
        lastKeepAliveMs_ = millis();
    }
}

void ChronosAdapter::onConnectionTrampoline(bool connected) {
    if (instance_) instance_->handleConnection(connected);
}

void ChronosAdapter::onNotificationTrampoline(Notification notification) {
    if (instance_) instance_->handleNotification(notification);
}

void ChronosAdapter::onConfigTrampoline(Config config, uint32_t a, uint32_t b) {
    if (instance_) instance_->handleConfig(config, a, b);
}

void ChronosAdapter::onRingerTrampoline(String caller, bool ringing) {
    if (instance_) instance_->handleRinger(caller, ringing);
}

void ChronosAdapter::handleConnection(bool connected) {
    connected_.store(connected, std::memory_order_seq_cst);
}

void ChronosAdapter::handleNotification(const Notification& notification) {
    RawNotification raw;
    raw.icon = notification.icon;
    copyTruncated(raw.app, sizeof(raw.app), notification.app.c_str());
    copyTruncated(raw.time, sizeof(raw.time), notification.time.c_str());
    copyTruncated(raw.title, sizeof(raw.title), notification.title.c_str());
    copyTruncated(raw.message, sizeof(raw.message), notification.message.c_str());

    portENTER_CRITICAL(&lock_);
    pendingNotif_ = raw;
    notifPending_.store(true, std::memory_order_release);
    portEXIT_CRITICAL(&lock_);
}

void ChronosAdapter::handleConfig(Config config, uint32_t a, uint32_t b) {
    switch (config) {
        case CF_NAV_DATA: {
            Navigation& nav = chronos_.getNavigation();
            portENTER_CRITICAL(&lock_);
            pendingNav_.active = nav.active;
            pendingNav_.isTurnByTurn = nav.isNavigation;
            pendingNav_.hasIcon = nav.hasIcon;
            copyTruncated(pendingNav_.distance, sizeof(pendingNav_.distance), nav.distance.c_str());
            copyTruncated(pendingNav_.duration, sizeof(pendingNav_.duration), nav.duration.c_str());
            copyTruncated(pendingNav_.eta, sizeof(pendingNav_.eta), nav.eta.c_str());
            copyTruncated(pendingNav_.title, sizeof(pendingNav_.title), nav.title.c_str());
            copyTruncated(pendingNav_.directions, sizeof(pendingNav_.directions), nav.directions.c_str());
            copyTruncated(pendingNav_.speed, sizeof(pendingNav_.speed), nav.speed.c_str());
            pendingNav_.iconCRC = nav.iconCRC;
            navPending_.store(true, std::memory_order_release);
            portEXIT_CRITICAL(&lock_);
            break;
        }
        case CF_NAV_ICON: {
            // a = chunk index (0..2), b = CRC. Full 48x48 bitmap reassembly
            // is deferred to the maneuver-mapping milestone; for now we just
            // preserve the CRC as the raw maneuver/icon identifier once the
            // final chunk has arrived.
            if (a == 2) {
                portENTER_CRITICAL(&lock_);
                pendingNav_.iconCRC = b;
                pendingNav_.hasIcon = true;
                navPending_.store(true, std::memory_order_release);
                portEXIT_CRITICAL(&lock_);
            }
            break;
        }
        case CF_MUSIC: {
            MusicInfo& music = chronos_.getMusicInfo();
            RawMusicSnapshot raw;
            raw.isPlaying = music.state != 0;
            copyTruncated(raw.title, sizeof(raw.title), music.title.c_str());
            copyTruncated(raw.artist, sizeof(raw.artist), music.artist.c_str());
            copyTruncated(raw.appName, sizeof(raw.appName), music.appName.c_str());

            portENTER_CRITICAL(&lock_);
            pendingMusic_ = raw;
            musicPending_.store(true, std::memory_order_release);
            portEXIT_CRITICAL(&lock_);
            break;
        }
        case CF_TIME:
            // a=0 fires just before the library sets its internal ESP32Time
            // clock from the phone's data, a=1 just after - only the latter
            // means the RTC now holds a real value worth reading.
            if (a == 1) {
                timeSynced_.store(true, std::memory_order_relaxed);
            }
            break;
        default:
            // Other Config values (CF_PBAT, CF_ALARM, CF_WEATHER, ...) are
            // outside this milestone's scope (navigation/notifications/
            // calls/music/time only) and are intentionally ignored.
            break;
    }
}

void ChronosAdapter::handleRinger(const String& caller, bool ringing) {
    RawCallEvent raw;
    copyTruncated(raw.caller, sizeof(raw.caller), caller.c_str());
    raw.ringing = ringing;

    portENTER_CRITICAL(&lock_);
    pendingCall_ = raw;
    callPending_.store(true, std::memory_order_release);
    portEXIT_CRITICAL(&lock_);
}

bool ChronosAdapter::takeNavigationUpdate(RawNavigationSnapshot& out) {
    if (!navPending_.load(std::memory_order_acquire)) return false;
    portENTER_CRITICAL(&lock_);
    out = pendingNav_;
    navPending_.store(false, std::memory_order_release);
    portEXIT_CRITICAL(&lock_);
    return true;
}

bool ChronosAdapter::takeNotification(RawNotification& out) {
    if (!notifPending_.load(std::memory_order_acquire)) return false;
    portENTER_CRITICAL(&lock_);
    out = pendingNotif_;
    notifPending_.store(false, std::memory_order_release);
    portEXIT_CRITICAL(&lock_);
    return true;
}

bool ChronosAdapter::takeMusicUpdate(RawMusicSnapshot& out) {
    if (!musicPending_.load(std::memory_order_acquire)) return false;
    portENTER_CRITICAL(&lock_);
    out = pendingMusic_;
    musicPending_.store(false, std::memory_order_release);
    portEXIT_CRITICAL(&lock_);
    return true;
}

bool ChronosAdapter::takeCallEvent(RawCallEvent& out) {
    if (!callPending_.load(std::memory_order_acquire)) return false;
    portENTER_CRITICAL(&lock_);
    out = pendingCall_;
    callPending_.store(false, std::memory_order_release);
    portEXIT_CRITICAL(&lock_);
    return true;
}

bool ChronosAdapter::getSyncedTime(uint8_t& hour, uint8_t& minute) {
    if (!timeSynced_.load(std::memory_order_relaxed)) return false;
    hour = static_cast<uint8_t>(chronos_.getHour(true)); // true = 24h mode
    minute = static_cast<uint8_t>(chronos_.getMinute());
    return true;
}
