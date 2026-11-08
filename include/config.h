#pragma once

// Centralized hardware configuration for BikeNav.
// Board: Waveshare ESP32-S3-LCD-1.28 (Non-Touch), GC9A01 round IPS, 240x240.
// This is the ONLY place pin numbers and bus parameters should be defined.

namespace HwConfig {

// --- Display SPI pins (verified against Waveshare official wiki) ---
constexpr int kPinSclk = 10;
constexpr int kPinMosi = 11;
constexpr int kPinDc   = 8;
constexpr int kPinCs   = 9;
constexpr int kPinRst  = 12;
constexpr int kPinBacklight = 40;

// This board has no MISO wired to the display (write-only 4-wire SPI).
constexpr int kPinMiso = -1;

// --- Display geometry ---
constexpr int kScreenWidth  = 240;
constexpr int kScreenHeight = 240;
constexpr int kScreenRotation = 0;

// --- SPI bus speeds ---
constexpr int kSpiWriteFreqHz = 40 * 1000 * 1000; // GC9A01 datasheet supports up to 80MHz; 40MHz is a safe bring-up default
constexpr int kSpiReadFreqHz  = 16 * 1000 * 1000;

// --- Backlight PWM ---
constexpr int kBacklightPwmChannel = 7;
constexpr int kBacklightPwmFreqHz  = 12000;
constexpr int kBacklightPwmBits    = 8;
constexpr uint8_t kBacklightDefaultBrightness = 255;

} // namespace HwConfig
