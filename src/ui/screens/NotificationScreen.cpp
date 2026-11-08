#include "NotificationScreen.h"
#include "../UITheme.h"
#include <cstring>

namespace {
constexpr int kIconCx = UITheme::kScreenCenterX;
constexpr int kIconCy = 80;
constexpr int kPrimaryTextY = 140;
constexpr int kSecondaryTextY = 164;
constexpr int kPlaybackY = 190;

void drawPhoneIcon(lgfx::LGFX_Device& canvas, int cx, int cy, int size, uint16_t color) {
    const int half = size / 2;
    canvas.drawWedgeLine(cx - half, cy + half, cx + half, cy - half, size * 0.16f, size * 0.16f, color);
    canvas.fillCircle(cx - half, cy + half, size * 0.22f, color);
    canvas.fillCircle(cx + half, cy - half, size * 0.22f, color);
}

void drawMusicIcon(lgfx::LGFX_Device& canvas, int cx, int cy, int size, uint16_t color) {
    const int noteX = cx - size / 4;
    const int noteY = cy + size / 3;
    const int stemTopY = cy - size / 2;

    canvas.fillCircle(noteX, noteY, size * 0.16f, color);
    canvas.drawLine(noteX + static_cast<int>(size * 0.15f), noteY, noteX + static_cast<int>(size * 0.15f), stemTopY, color);
    canvas.drawLine(noteX + static_cast<int>(size * 0.15f) + 1, noteY, noteX + static_cast<int>(size * 0.15f) + 1, stemTopY, color);
    canvas.fillTriangle(noteX + static_cast<int>(size * 0.15f), stemTopY,
                         noteX + static_cast<int>(size * 0.15f) + static_cast<int>(size * 0.3f), stemTopY + static_cast<int>(size * 0.12f),
                         noteX + static_cast<int>(size * 0.15f), stemTopY + static_cast<int>(size * 0.22f), color);
}

void drawPlaybackIcon(lgfx::LGFX_Device& canvas, int cx, int cy, bool playing, uint16_t color) {
    if (playing) {
        canvas.fillTriangle(cx - 6, cy - 8, cx - 6, cy + 8, cx + 8, cy, color);
    } else {
        canvas.fillRect(cx - 7, cy - 8, 5, 16, color);
        canvas.fillRect(cx + 2, cy - 8, 5, 16, color);
    }
}

} // namespace

void NotificationScreen::drawCall(lgfx::LGFX_Device& canvas, const UIState& state) {
    drawPhoneIcon(canvas, kIconCx, kIconCy, 44, UITheme::kApproachingColor);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UITheme::kApproachingColor, UITheme::kBackground);
    canvas.setTextSize(1);
    canvas.drawString("INCOMING CALL", kIconCx, kIconCy + 48);

    canvas.fillRect(kIconCx - 80, kPrimaryTextY - 12, 160, 24, UITheme::kBackground);
    canvas.setTextColor(UITheme::kPrimaryText, UITheme::kBackground);
    canvas.setTextSize(2);
    canvas.drawString(state.call.callerName, kIconCx, kPrimaryTextY);
}

void NotificationScreen::drawMusic(lgfx::LGFX_Device& canvas, const UIState& state) {
    drawMusicIcon(canvas, kIconCx, kIconCy, 44, UITheme::kNavigationColor);

    canvas.fillRect(kIconCx - 90, kPrimaryTextY - 12, 180, 24, UITheme::kBackground);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UITheme::kPrimaryText, UITheme::kBackground);
    canvas.setTextSize(2);
    canvas.drawString(state.music.title, kIconCx, kPrimaryTextY);

    canvas.fillRect(kIconCx - 80, kSecondaryTextY - 9, 160, 18, UITheme::kBackground);
    canvas.setTextColor(UITheme::kSecondaryText, UITheme::kBackground);
    canvas.setTextSize(1);
    canvas.drawString(state.music.artist, kIconCx, kSecondaryTextY);

    canvas.fillRect(kIconCx - 10, kPlaybackY - 10, 20, 20, UITheme::kBackground);
    drawPlaybackIcon(canvas, kIconCx, kPlaybackY, state.music.isPlaying, UITheme::kNavigationColor);
}

void NotificationScreen::enter(lgfx::LGFX_Device& canvas, const UIState& state) {
    entered_ = true;
    canvas.fillScreen(UITheme::kBackground);

    if (state.screen == UIScreen::INCOMING_CALL) {
        drawCall(canvas, state);
    } else {
        drawMusic(canvas, state);
    }

    lastMode_ = state.screen;
    std::strncpy(lastCaller_, state.call.callerName, sizeof(lastCaller_) - 1);
    lastCaller_[sizeof(lastCaller_) - 1] = '\0';
    std::strncpy(lastTitle_, state.music.title, sizeof(lastTitle_) - 1);
    lastTitle_[sizeof(lastTitle_) - 1] = '\0';
    std::strncpy(lastArtist_, state.music.artist, sizeof(lastArtist_) - 1);
    lastArtist_[sizeof(lastArtist_) - 1] = '\0';
    lastPlaying_ = state.music.isPlaying;
}

void NotificationScreen::update(lgfx::LGFX_Device& canvas, const UIState& state) {
    if (!entered_ || state.screen != lastMode_) {
        enter(canvas, state);
        return;
    }

    if (state.screen == UIScreen::INCOMING_CALL) {
        if (std::strcmp(state.call.callerName, lastCaller_) != 0) {
            canvas.fillRect(kIconCx - 80, kPrimaryTextY - 12, 160, 24, UITheme::kBackground);
            canvas.setTextDatum(middle_center);
            canvas.setTextColor(UITheme::kPrimaryText, UITheme::kBackground);
            canvas.setTextSize(2);
            canvas.drawString(state.call.callerName, kIconCx, kPrimaryTextY);
            std::strncpy(lastCaller_, state.call.callerName, sizeof(lastCaller_) - 1);
            lastCaller_[sizeof(lastCaller_) - 1] = '\0';
        }
        return;
    }

    bool changed = false;
    if (std::strcmp(state.music.title, lastTitle_) != 0 || std::strcmp(state.music.artist, lastArtist_) != 0) {
        changed = true;
    }
    if (changed) {
        drawMusic(canvas, state);
        std::strncpy(lastTitle_, state.music.title, sizeof(lastTitle_) - 1);
        lastTitle_[sizeof(lastTitle_) - 1] = '\0';
        std::strncpy(lastArtist_, state.music.artist, sizeof(lastArtist_) - 1);
        lastArtist_[sizeof(lastArtist_) - 1] = '\0';
        lastPlaying_ = state.music.isPlaying;
    } else if (state.music.isPlaying != lastPlaying_) {
        canvas.fillRect(kIconCx - 10, kPlaybackY - 10, 20, 20, UITheme::kBackground);
        drawPlaybackIcon(canvas, kIconCx, kPlaybackY, state.music.isPlaying, UITheme::kNavigationColor);
        lastPlaying_ = state.music.isPlaying;
    }
}
