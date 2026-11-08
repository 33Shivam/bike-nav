#include "DisplayManager.h"
#include "config.h"
#include <Arduino.h>

DisplayManager::LGFX::LGFX() {
    auto busCfg = bus_.config();
    busCfg.spi_host = SPI2_HOST;
    busCfg.spi_mode = 0;
    busCfg.freq_write = HwConfig::kSpiWriteFreqHz;
    busCfg.freq_read = HwConfig::kSpiReadFreqHz;
    busCfg.spi_3wire = true;
    busCfg.use_lock = true;
    busCfg.dma_channel = SPI_DMA_CH_AUTO;
    busCfg.pin_sclk = HwConfig::kPinSclk;
    busCfg.pin_mosi = HwConfig::kPinMosi;
    busCfg.pin_miso = HwConfig::kPinMiso;
    busCfg.pin_dc = HwConfig::kPinDc;
    bus_.config(busCfg);
    panel_.setBus(&bus_);

    auto panelCfg = panel_.config();
    panelCfg.pin_cs = HwConfig::kPinCs;
    panelCfg.pin_rst = HwConfig::kPinRst;
    panelCfg.pin_busy = -1;
    panelCfg.panel_width = HwConfig::kScreenWidth;
    panelCfg.panel_height = HwConfig::kScreenHeight;
    panelCfg.offset_x = 0;
    panelCfg.offset_y = 0;
    panelCfg.offset_rotation = 0;
    panelCfg.readable = false;
    panelCfg.invert = true; // GC9A01 panels require inverted color to render correctly
    panelCfg.rgb_order = false;
    panelCfg.dlen_16bit = false;
    panelCfg.bus_shared = false;
    panel_.config(panelCfg);

    auto lightCfg = backlight_.config();
    lightCfg.pin_bl = HwConfig::kPinBacklight;
    lightCfg.invert = false;
    lightCfg.freq = HwConfig::kBacklightPwmFreqHz;
    lightCfg.pwm_channel = HwConfig::kBacklightPwmChannel;
    backlight_.config(lightCfg);
    panel_.setLight(&backlight_);

    setPanel(&panel_);
}

DisplayManager::DisplayManager() = default;

bool DisplayManager::begin() {
    initialized_ = lcd_.init();
    if (!initialized_) {
        return false;
    }
    lcd_.setRotation(HwConfig::kScreenRotation);
    lcd_.setColorDepth(16);
    setBacklight(HwConfig::kBacklightDefaultBrightness);
    return true;
}

void DisplayManager::clear(uint32_t color) {
    if (!initialized_) return;
    lcd_.fillScreen(color);
}

void DisplayManager::setBacklight(uint8_t brightness) {
    if (!initialized_) return;
    lcd_.setBrightness(brightness);
}

void DisplayManager::drawHardwareTestScreen() {
    if (!initialized_) return;

    const int w = HwConfig::kScreenWidth;
    const int h = HwConfig::kScreenHeight;
    const int cx = w / 2;
    const int cy = h / 2;

    lcd_.startWrite();

    lcd_.fillScreen(TFT_BLACK);

    // Four color-verification quadrants near the top, clear of the center text.
    const int blockSize = 36;
    const int blockY = 24;
    lcd_.fillRect(cx - blockSize - 30, blockY, blockSize, blockSize, TFT_RED);
    lcd_.fillRect(cx - blockSize / 2, blockY, blockSize, blockSize, TFT_GREEN);
    lcd_.fillRect(cx + 30, blockY, blockSize, blockSize, TFT_BLUE);
    lcd_.fillRect(cx - blockSize / 2, blockY + blockSize + 6, blockSize, blockSize, TFT_WHITE);

    // Circular outline following the physical round bezel.
    lcd_.drawCircle(cx, cy, (w / 2) - 4, TFT_WHITE);
    lcd_.drawCircle(cx, cy, (w / 2) - 5, TFT_DARKGREY);

    // Centered title.
    lcd_.setTextDatum(middle_center);
    lcd_.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd_.setTextSize(2);
    lcd_.drawString("BIKENAV", cx, cy - 10);

    // Status line.
    lcd_.setTextSize(1);
    lcd_.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd_.drawString("ESP32-S3 READY", cx, cy + 16);

    // Small hardware status indicator (bottom of the visible circle).
    lcd_.fillCircle(cx, cy + 70, 5, TFT_GREEN);
    lcd_.drawCircle(cx, cy + 70, 5, TFT_WHITE);

    lcd_.endWrite();
}
