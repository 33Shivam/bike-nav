# BikeNav — ESP32-S3 Firmware

Motorcycle navigation display. ESP32-S3 + round GC9A01 LCD receives live
Google Maps navigation data over BLE and renders it on a 240×240 round
screen. PlatformIO + Arduino framework + LovyanGFX.

**Sibling project:** `~/bikenav-android` — a custom Android app that feeds
this firmware real navigation data. See its own `CLAUDE.md` for details.
The two projects share a BLE wire protocol (documented below) but have no
code dependency on each other.

## Hardware

- Board: Waveshare ESP32-S3-LCD-1.28 (Non-Touch)
- MCU: **ESP32-S3R2** — 16MB external **QIO** flash, 2MB **QSPI** PSRAM (not
  Octal — a generic ESP32-S3 config would get this wrong). Verified against
  Waveshare's official wiki.
- Display: GC9A01, 240×240, round, SPI. Pins: SCLK=10, MOSI=11, DC=8, CS=9,
  RST=12, Backlight=40 (write-only 4-wire SPI, no MISO).
- `platformio.ini` board_build overrides (`qio_qspi` memory type,
  `default_16MB.csv` partitions) are required and verified — see comments
  in that file for why.

## Architecture

```
src/
├── main.cpp                    — wiring only; #if BIKENAV_MOCK_MODE switches
│                                  between demo mode and live BLE mode
├── display/DisplayManager      — LovyanGFX + GC9A01 init, backlight, drawing primitives
├── diagnostics/                — boot/chip/heap Serial diagnostics
├── ui/
│   ├── UIState.h                — UIScreen enum, ManeuverType enum, app-level state model
│   ├── UITheme.h                 — colors, layout constants, demo timing (centralized, like config.h).
│   │                                Light theme (white bg/black fg) as of the icon-consistency pass below —
│   │                                inverted from the original Figma-matched dark theme at user request.
│   ├── UIManager                 — owns UIState + one instance per screen, dispatches enter()/update()
│   ├── DemoController             — mock-mode-only: cycles through all screens/maneuvers
│   ├── AntiAlias.h/.cpp           — custom shape anti-aliasing (blends toward an explicit bg color,
│   │                                 NOT a destination readback — see Gotchas, this panel can't read back)
│   ├── assets/ManeuverIcons.h     — 35 maneuver-icon PNGs embedded as byte arrays, pre-resized to 82x82
│   │                                 offline (see Gotchas — don't scale PNGs at draw time on this panel)
│   ├── fonts/VlwFonts.h           — anti-aliased runtime fonts (VLW format), replacing the old 1-bit
│   │                                 GFXfont bitmaps; generated via tools/fontgen/ttf_to_vlw.py
│   ├── components/                — NavigationArrow (maneuver glyphs — mostly PNG-backed now, see
│   │                                 milestone below; EXIT_LEFT/RIGHT and UNKNOWN are still hand-drawn
│   │                                 vectors), DistanceIndicator, NavigationRing, StatusIndicator
│   └── screens/                   — WaitingScreen, IdleScreen, NavigationScreen, NotificationScreen
│                                     each does partial redraws (diff against cached last-rendered state)
├── bluetooth/
│   ├── ChronosAdapter             — the ONLY file touching ChronosESP32 library types.
│   │                                 Thread-safety: its callbacks fire on NimBLE's BLE host
│   │                                 task, not Arduino's loop() task (verified by reading
│   │                                 ChronosESP32.cpp). Hands data to loop() via a
│   │                                 portMUX_TYPE-protected pending slot per event category.
│   ├── BluetoothManager            — app-facing facade; PIMPL (unique_ptr<ChronosAdapter>)
│   │                                 so nothing that includes this header ever sees
│   │                                 ChronosESP32 types — this isolation is load-bearing,
│   │                                 see "Gotchas" below.
│   └── BluetoothEvents.h           — plain structs (RawNavigationSnapshot, RawNotification,
│                                      RawCallEvent, RawMusicSnapshot) crossing the adapter boundary
└── navigation/
    ├── NavigationData.h            — app-level nav model (no Chronos types)
    ├── NavigationManager            — owns latest NavigationData, de-duplicated [NAV] logging
    └── ManeuverMapper                — classifies real instruction text ("Turn left", "Sharp
                                         right", "At the roundabout, turn right") into
                                         ManeuverType via keyword matching. See "Open work" below.
```

## Build / flash

```bash
export PATH="$HOME/.local/bin:$PATH"   # pio installed via: pip install --user --break-system-packages platformio
pio run                                # compile
sg dialout -c "pio run -t upload --upload-port /dev/ttyACM0"   # flash
```

**Why `sg dialout -c`:** the user account isn't in the `dialout` group by
default in this environment; `sg dialout -c "<cmd>"` runs a command with
that group applied immediately without needing to log out/in after
`usermod -aG dialout`.

**Serial monitor:** `pio device monitor` requires an interactive TTY and
fails in non-interactive tool contexts. Workaround: a small pyserial script
(open at 115200, optionally toggle `ser.dtr`/`ser.rts` to force a clean
reset so the boot banner is captured, read for N seconds). See any of the
`serial_watch_*.py` patterns used throughout this project's history.

## Font/icon asset pipeline

- `tools/fontgen/ttf_to_vlw.py <ttf> <px> <out.vlw>` — TTF → anti-aliased VLW runtime font
  (8-bit coverage mask per glyph, via Pillow's `getmask(mode="L")`). Replaces the older
  `ttf_to_gfxfont.py` (1-bit, no AA) for any font actually drawn on screen.
- Maneuver icon PNGs (`ui/assets/ManeuverIcons.h`) are pre-resized to `UITheme::kArrowSize`
  (82x82) offline before embedding — see Gotchas for why they must NOT be scaled at draw time.

## BLE wire protocol (shared with ~/bikenav-android)

The firmware uses the `fbiego/ChronosESP32` Arduino library as its BLE
server — it was originally meant to talk to the third-party "Chronos"
Android app, but that app can't read Google Maps navigation on Android 16+
(see "Why the custom Android app exists" below). We kept the ChronosESP32
library on the firmware side unchanged and instead built our own Android
app that speaks its exact wire protocol, reverse-engineered from
`ChronosESP32.cpp`'s `dataReceived()`:

- Service UUID `6e400001-b5a3-f393-e0a9-e50e24dcca9e`, RX characteristic
  `6e400002-...` (phone writes here).
- Frame: `0xAB, lenHi, lenLo, marker, command, subtype, payload...` where
  `len = total frame length - 3`.
- Navigation data: `marker=0xFE, command=0xEF, subtype=0x80`, payload =
  `hasIcon(1B) isNavigation(1B) iconCRC(4B BE) title\0 duration\0 distance\0 eta\0 directions\0 speed\0`.
  `subtype=0x00` (no further payload) signals "navigation stopped."
- Ringer (incoming call): `marker=0xFF, command=0x72`, payload =
  `icon(1B: 0x01=incoming,0x02=cancel) state(1B, unused) callerBytes` (NOT
  null-terminated — reads to end of frame). Less rigorously verified than
  navigation (no real-world capture of this one yet).
- Time sync: `marker=0xFF, command=0x93`, payload = `reserved(2B, unread by
  firmware) year(2B BE) month(1B) day(1B) hour(1B, 24h) minute(1B) second(1B)`.
  `ChronosESP32` (inherits `ESP32Time`) calls its own `setTime(...)` internally
  on receipt — no firmware code needed for this to work, it already did
  nothing extra. The Android app sends this once, immediately after BLE
  connects, from the phone's real wall clock (`ChronosProtocol.buildTimeSyncNow()`).
  Powers the idle-screen clock (see Milestone 14).
- Full details, including music/notification sub-protocols: see
  `~/bikenav-android/src/.../ble/ChronosProtocol.kt`.

**No maneuver icon bitmap is sent** by our Android app (the old Chronos
protocol's 48×48 bitmap+CRC icon system is unused) — maneuver type is
instead classified firmware-side from the real instruction text via
`ManeuverMapper`.

## Milestone history

1. **Hardware bring-up** — DisplayManager (LovyanGFX/GC9A01), HardwareDiagnostics, verified
   board config. Working, stable.
2. **UI engine** — UIManager/UIState/screens/components, 20 maneuver icon types (vector-drawn,
   parametric families not 20 bespoke glyphs), demo mode. Icon visual style was later redone
   to match a Figma reference (bold chevron arrowheads instead of solid triangles; added
   DEPART/ARRIVED/UNKNOWN states for full-screen fallback treatment) — still not a pixel-perfect
   match, flagged as a possible future iteration.
3. **ChronosESP32 BLE integration** — built BluetoothManager/ChronosAdapter/NavigationManager,
   verified BLE pairing/advertising/connection on real hardware. **Discovered the third-party
   Chronos Android app cannot read Google Maps navigation on Android 16+** (confirmed via
   [fbiego/chronos-esp32#29](https://github.com/fbiego/chronos-esp32/issues/29) — Android
   changed Maps' notification template to `Notification$ProgressStyle`, breaking Chronos's
   parser). HERE WeGo also failed. Root-caused via a raw-BLE-byte diagnostic capture before
   deciding this was unfixable from our side.
4. **Pivot: custom Android companion app** (`~/bikenav-android`) that reads notifications
   itself and speaks the same BLE protocol — zero firmware changes needed. See that repo's
   `CLAUDE.md`.
5. **Wired real distance into the UI** — `tryParseDistanceMeters()` in `main.cpp`. Found and
   fixed a nasty bug: Android formats distance with a **non-breaking space** (U+00A0) between
   number and unit, not ASCII space — looks identical in a terminal, silently failed every
   parse until an explicit success/failure diagnostic log (not just re-printing the raw
   string) exposed it. Fix: skip any non-letter byte, not just `' '`.
6. **Maneuver mapping** (`ManeuverMapper`) — classifies real instruction text into
   `ManeuverType` via keyword matching, since our own app provides real text (unlike Chronos's
   opaque icon bitmap). **In progress** — confirmed working for several real phrases ("Turn
   left"→TURN_LEFT, "At the roundabout, turn right"→ROUNDABOUT_RIGHT, plain street names→
   STRAIGHT), but the user has reported some mappings are wrong and asked to revisit later.
   No specifics on which ones yet — next session should ask for a fresh capture of the
   wrong case(s) before changing anything.
7. **Figma dev-mode icon fidelity pass** — Figma MCP connector was unusable (needs Editor-tier
   file access; this account's View-tier seat fails even read-only calls regardless of how the
   file is shared) and Playwright browser automation crashed on launch in this sandbox (Chrome
   SIGTRAPs; `--no-sandbox` fixes it but isn't exposed through the tool). Worked around by having
   the user export per-maneuver frames as SVG directly from Figma and hand them over. Measured
   exact shaft widths, head proportions, bend angles, and layout Y-positions from the SVG
   coordinates (1:1 with real panel pixels) and recalibrated `NavigationArrow.cpp`'s shared
   proportions accordingly — previous values were hand-guessed and measurably off (e.g. SLIGHT
   turn bend angle was 32°, measured 45°).
8. **Full icon-set PNG conversion** — replaced nearly every vector maneuver icon with
   `github.com/pdhruv93/minimal-nav`'s PNG set (35 icons) for one consistent style. Only
   `EXIT_LEFT`/`EXIT_RIGHT` (no highway-ramp asset in that set) and `UNKNOWN` (no asset) are
   still hand-drawn vectors. `ManeuverType` grew `MERGE_LEFT`/`MERGE_RIGHT` and the full
   `ROUNDABOUT_*_CW`/`_CCW` family (18 new values) the vector set never covered. Known
   regressions accepted along the way: `TURN_APPROACHING`'s yellow accent is gone (PNGs can't
   recolor, and the fallback badge-color idea was declined), and the numbered roundabout-exit
   badge ("3rd exit") is gone (no PNG variant encodes an arbitrary number).
9. **Light theme flip** — background/text inverted to white-on-black, app-wide, at the user's
   explicit request (shared `UITheme.h` constants, so it propagated to every screen) — mainly so
   the PNG icons' fixed black/gray pixels read correctly without a backing badge.
10. **Custom anti-aliasing + VLW fonts** — see Gotchas for why LovyanGFX's built-in AA doesn't
    work right on this panel. `AntiAlias.h/.cpp` covers the remaining vector shapes (bezel ring,
    `EXIT_LEFT/RIGHT`, `UNKNOWN`); VLW fonts (`tools/fontgen/ttf_to_vlw.py`) cover distance and
    street-name text.
11. **Scaled-PNG bugs found and fixed** — icons were scaled 192px→82px live at draw time. Traced
    a stray colored-pixel artifact (confirmed on real hardware, not a photo compression artifact)
    and intermittent roundabout-icon clipping to the same cause: LovyanGFX's scaled+alpha+non-
    readable-panel PNG blend path is a narrow, lightly-exercised combination. Fixed by
    pre-resizing all 35 icons to 82x82 *offline* so the common draw path hits `scale=1.0`
    (LovyanGFX's simpler, well-tested unscaled blit) instead.
12. **ETA removed** from `NavigationScreen` per user request; remaining elements (arrow/distance/
    street) re-spaced and recentered to fill the freed vertical space.
13. **Maneuver-mapping audit against real captures (2026-11-08)** — drove a MockGps Pro-simulated
    route covering many maneuver types, correlating ESP32 `[MANEUVER] "..." -> ...` serial lines
    against `adb logcat` on the phone. Found and fixed real mismatches: (1) named traffic circles
    ("At Teen Murti-Haifa Chowk, take the 4th exit...") have no literal word "roundabout" and were
    falling through to `STRAIGHT` — fixed by detecting an ordinal exit number next to "exit" as a
    reliable roundabout-only signal; (2) roundabout CW/CCW default flipped from CCW to CW — every
    real test route is in India (left-hand traffic → clockwise roundabouts), confirmed
    wrong-direction on real hardware with the old default (still not a real locale signal, just a
    better-informed hardcoded default — see Open work); (3) "Join with X" recognized as a merge
    synonym for "Merge onto X"; (4) **the actual biggest source of misses was upstream of
    `ManeuverMapper` entirely** — the Android app's `NotificationParser.kt` was extracting the
    instruction from `secondaryInfo` alone, which drops the maneuver verb for phrasing like "Turn
    left onto X" (leaving just "X"), silently downgrading real turns/merges/exits/keeps to
    `STRAIGHT` downstream. Fixed by preferring the instruction parsed from `android.title`'s
    combined `"<distance> · <instruction>"` string instead, falling back to `secondaryInfo` only
    when that's unavailable.
14. **Idle-screen time sync** — `IdleScreen` already had clock-drawing code, but nothing in live
    BLE mode ever fed it real data (only `DemoController`'s mock mode did), so it was frozen at
    "00:00" on real hardware. Fixed two-sided: `ChronosESP32` (which the firmware already uses)
    publicly inherits `ESP32Time` and already sets its internal RTC whenever it receives wire
    command `0x93` (`CF_TIME`) — previously received but ignored by `ChronosAdapter::handleConfig`.
    Added `BluetoothManager::getSyncedTime(hour, minute)` (returns false until a real sync has
    happened, so the UI never shows a confident-looking-but-wrong "00:00"), polled every
    `main.cpp` `loop()` iteration so the clock keeps ticking between syncs. `UIState` gained a
    `clockValid` flag; `IdleScreen` shows `"--:--"` until the first real sync lands. On the Android
    side, `ChronosProtocol.buildTimeSyncNow()` builds the `0x93` frame from the phone's real wall
    clock and `BikeNavBleClient` sends it immediately after BLE connects
    (`onServicesDiscovered` → `send(buildTimeSyncNow())`).

## Open work / known gaps

- Roundabout CW/CCW (`ManeuverMapper`) now defaults to CW (correct for India, where every real
  test route has been driven) instead of the old CCW default — still not a real locale/region
  signal, just a better-informed hardcoded default. Needs real locale data from the phone side to
  be correct outside left-hand-traffic countries.
- Maneuver mapping was substantially improved via the 2026-11-08 real-capture audit (see milestone
  13), but a keyword-matching approach over free-form instruction text is inherently open-ended —
  treat any newly-reported wrong mapping as plausible and get a fresh `[MANEUVER]` serial capture
  before assuming it's already covered.
- `TURN_APPROACHING` has no visual distinction anymore (yellow accent removed per user request);
  the numbered roundabout-exit badge is also gone (PNG icons can't encode an arbitrary number).
- Music (`handleMusicUpdate`) and incoming-call (`handleCallEvent`) BLE paths exist in
  firmware but are far less tested than navigation — our Android app's `buildIncomingCall()`
  packet format in particular has never been verified against real hardware.
- `BIKENAV_MOCK_MODE` build flag in `platformio.ini` toggles demo mode (no BLE, cycles
  through all screens) vs. live mode (default, `=0`). Both compile and were verified working
  independently.
- `nav.distanceText`/`nav.etaText` are raw strings from whatever the phone sends — distance
  parsing assumes metric units with a recognized suffix (m/km/mi/ft); untested against a
  phone set to a different locale/unit system.

## Gotchas worth knowing before touching this code

- **Don't use `ManeuverType::UNKNOWN` as a generic "don't know the icon" placeholder.**
  `NavigationScreen` treats it as a rare full-screen fallback state (big icon + text label,
  no distance/street/ring) — using it for routine "no mapping yet" cases hides the real data.
  Use `STRAIGHT` as the neutral placeholder instead (this bit us once already).
- **`BluetoothManager.h` must never leak ChronosESP32 types.** It already collided once:
  `ChronosESP32.h`'s `struct MusicInfo` has the exact same name as `ui/UIState.h`'s own
  `MusicInfo`, both in the global namespace. Fixed via PIMPL (`unique_ptr<ChronosAdapter>`,
  forward-declared in the header) — keep it that way.
- Text crossing the Android→BLE→firmware boundary may contain **non-ASCII whitespace** (see
  milestone 5 above) or other locale-specific formatting. Don't assume plain ASCII when
  parsing phone-originated strings.
- This toolchain (Gradle 8.7, Android SDK cmdline-tools) is manually installed at
  `~/android-tools` for the sibling Android project — reuse it rather than reinstalling.
- **This panel cannot be read back (no MISO; `panelCfg.readable = false` in `DisplayManager.cpp`).**
  LovyanGFX's built-in anti-aliasing (`drawWedgeLine`, `fillSmoothCircle`/`fillSmoothRoundRect`,
  `fillArc` edges) all blend by reading back the destination pixel — on this panel that silently
  returns black (`Panel_LCD::readRect` memsets to 0 when unreadable), so built-in AA always
  blends toward assumed-black. Invisible on a dark theme, a visible dark fringe on the current
  white one. Use `AntiAlias.h` (blends toward an explicitly-known bg color) for new shape AA.
  VLW fonts are NOT affected — `VLWfont::drawChar` blends toward the actual color passed to
  `setTextColor()`'s background argument, not a readback.
- **`drawPng()`'s `(x, y)` is the top-left of its `maxWidth`×`maxHeight` box, not a center point**
  — even with `datum=middle_center`. Never pass `maxWidth=0, maxHeight=0` ("auto") with a non-
  `top_left` datum either: it falls back to the whole screen's dimensions for centering math,
  which can collapse the clip rect to nothing and silently draw zero pixels while still
  returning `true` (no crash, no error — just nothing drawn).
- **Don't scale PNG icons at draw time on this panel if avoidable** — pre-resize source assets
  to the actual target size offline instead. The scaled+alpha+non-readable-panel `drawPng()`
  path caused both a stray-pixel artifact and roundabout-icon clipping (see milestone 11).
- **zsh doesn't word-split unquoted variables like bash does** (`for f in $FILES` treats the
  whole string as one item). Use a real array: `FILES=(a b c)` + `for f in "${FILES[@]}"`.

#TODO
MAKE AN OPTION TO RESET THE WATCH WHEN CANNOT RENDER THE NAVIGATION OPTIONS THROUGH APP. It seems force stopping the app and restraing it solves the issue.