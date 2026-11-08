#pragma once

#include <LovyanGFX.hpp>
#include <lgfx/v1/panel/Panel_GC9A01.hpp>
#include <cstdint>

// Owns the GC9A01 LovyanGFX device and exposes a small, stable drawing
// interface for the rest of the application. All pin/bus configuration
// lives in config.h; this class only wires those values into LovyanGFX.
class DisplayManager {
public:
    DisplayManager();

    // Initializes SPI bus, panel and backlight. Returns false on failure.
    bool begin();

    bool isInitialized() const { return initialized_; }

    void clear(uint32_t color = 0x000000);

    // brightness: 0 (off) - 255 (full)
    void setBacklight(uint8_t brightness);

    // Renders the hardware bring-up test screen (color blocks, ring,
    // centered title/status text, status dot).
    void drawHardwareTestScreen();

    lgfx::LGFX_Device& canvas() { return lcd_; }

private:
    // Concrete LovyanGFX device bound to this board's GC9A01 panel.
    class LGFX : public lgfx::LGFX_Device {
    public:
        LGFX();

    private:
        lgfx::Panel_GC9A01 panel_;
        lgfx::Bus_SPI bus_;
        lgfx::Light_PWM backlight_;
    };

    LGFX lcd_;
    bool initialized_ = false;
};
