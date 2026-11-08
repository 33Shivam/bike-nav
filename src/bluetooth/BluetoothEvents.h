#pragma once

#include <cstdint>
#include <cstring>

// Application-level event payloads produced by ChronosAdapter. These are
// plain structs with fixed buffers (no Chronos/NimBLE types, no String) so
// the rest of the app - including the UI - never depends on ChronosESP32.
//
// Every field here is a direct pass-through of something the ChronosESP32
// library actually exposes (verified by reading its source). Nothing is
// invented: there is no destination field because the library doesn't have
// one, and there is no numeric distance because the library only provides
// a pre-formatted, locale-dependent string.

struct RawNavigationSnapshot {
    bool active = false;       // Chronos 'active' - a navigation session exists
    bool isTurnByTurn = false; // Chronos 'isNavigation' - true only for real turn data,
                                // false for the library's own placeholder/info messages
    bool hasIcon = false;
    char distance[32] = "";
    char duration[32] = "";
    char eta[32] = "";
    char title[48] = "";
    char directions[128] = ""; // street name / instruction text
    char speed[16] = "";      // only populated by some source apps (e.g. OsmAnd)
    uint32_t iconCRC = 0;     // raw identifier for the 48x48 maneuver bitmap - no
                              // symbolic meaning is known yet; preserved for the
                              // maneuver-mapping milestone
};

struct RawNotification {
    int icon = 0;
    char app[32] = "";
    char time[16] = "";
    char title[48] = "";
    char message[96] = "";
};

struct RawCallEvent {
    char caller[48] = "";
    bool ringing = false; // true = incoming call active, false = dismissed/ended
};

struct RawMusicSnapshot {
    bool isPlaying = false;
    char title[48] = "";
    char artist[48] = "";
    char appName[32] = "";
};

inline void copyTruncated(char* dst, size_t dstSize, const char* src) {
    size_t srcLen = std::strlen(src);
    if (srcLen < dstSize) {
        std::strcpy(dst, src);
        return;
    }
    std::strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
    // Truncation happened - if the cut landed inside a multi-byte UTF-8
    // sequence (continuation bytes have the top two bits '10'), back off to
    // before that sequence instead of leaving a corrupted trailing byte.
    size_t len = dstSize - 1;
    int backoff = 0;
    while (len > 0 && backoff < 3 && (static_cast<unsigned char>(dst[len - 1]) & 0xC0) == 0x80) {
        len--;
        backoff++;
    }
    if (len > 0 && backoff > 0 && (static_cast<unsigned char>(dst[len - 1]) & 0x80) == 0x80) {
        len--; // also drop the now-incomplete lead byte
    }
    dst[len] = '\0';
}
