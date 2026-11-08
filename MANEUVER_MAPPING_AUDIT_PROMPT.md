**Project: BikeNav — ESP32-S3 round-display motorcycle navigation UI**

Repo: `~/esp32` (firmware, PlatformIO/Arduino/LovyanGFX). Sibling repo:
`~/bikenav-android` (custom Kotlin companion app that reads Google Maps
notifications and forwards them over BLE — read its own `CLAUDE.md` too).
Read `~/esp32/CLAUDE.md` first for full architecture/history before doing
anything else.

**Your task is find-only. Do not change any code in this session.** The
goal is a list of concrete mismatches between what Google Maps actually
instructed and what the firmware displayed — nothing more. Fixing is a
separate, later phase.

## Background

`src/navigation/ManeuverMapper.h/.cpp` classifies the raw instruction text
Google Maps sends (e.g. "Turn left", "At the roundabout, take the 2nd
exit") into a `ManeuverType` enum via keyword matching — there's no
bitmap/icon ID from Maps, just text, so this is inherently fuzzy. The user
has observed that "it misses some navigation signs sometimes" but hasn't
pinned down which specific phrases map wrong. Known *already-accepted* gap
(don't re-report this one): roundabout CW/CCW direction always defaults to
CCW, since it's not derivable from instruction text alone — only report
*new* mismatches, not this documented one.

## Setup

- The Android phone is already connected via `adb` (wireless). Confirm with
  `adb devices -l` — if more than one transport shows up for the same
  device, use `-s <serial>` consistently for every command afterward
  (handshake/session IDs can rotate between calls otherwise).
- The phone has **MockGps Pro** installed, used to simulate GPS movement
  along a route without actually riding anywhere.
- ESP32 is connected over USB serial (`/dev/ttyACM0`, 115200 baud). The
  user account isn't in the `dialout` group by default — wrap any serial
  access with `sg dialout -c "<command>"`. `pio device monitor` requires an
  interactive TTY and will fail in a non-interactive tool context — use a
  small pyserial script instead (open the port, read for N seconds, print
  lines) to capture boot/runtime serial output to a log file.
- The BikeNav Android app must be running ("Link: running" in its UI) and
  BLE connected to the ESP32 before starting.

## Procedure

1. In MockGps Pro, define a route that deliberately passes through as many
   distinct maneuver types as practical in one simulated drive: left turn,
   right turn, slight left/right, sharp left/right, U-turn, a roundabout
   with at least 2 different exits, a highway merge, a fork/keep-left/
   keep-right, a highway exit ramp, and arrival at a destination. (Doesn't
   need to be a real place — any route geometry that produces these
   maneuvers in Google Maps' turn-by-turn is fine.)
2. Start Google Maps navigation on that mocked route.
3. Simultaneously capture, for the full duration of the simulated drive:
   - **ESP32 serial output** — grep for `[MANEUVER]` lines, which log in
     the exact format `[MANEUVER] "<raw instruction text>" -> <ManeuverType>`
     (see `ManeuverMapper.cpp` for where this is logged).
   - **Android logcat** — filtered for the app's own tags (`BikeNavService`,
     and whatever tag `NotificationParser`/the notification-capture code
     uses — check `~/bikenav-android` for exact tag strings) to see the raw
     `primaryInfo`/instruction text as read directly from Maps' notification,
     in case the firmware's `[MANEUVER]` log and the actual Maps notification
     text ever disagree (e.g. truncation, encoding issues) independently of
     the keyword-matching logic itself.
   - Optionally, your own timestamped notes of what Google Maps' own UI
     visually showed at each maneuver (its icon/instruction), to serve as
     ground truth for judging whether the firmware's chosen `ManeuverType`
     was actually correct.
4. After the drive, go through each distinct `[MANEUVER] "..." -> ...` line
   and judge: does the right-hand `ManeuverType` match what that instruction
   text / Google Maps' own displayed maneuver actually meant? Pay particular
   attention to:
   - Compound/qualified phrasing Maps sometimes uses (e.g. "Turn left to
     stay on X", "Keep right to continue on Y", "Take the ramp to Z").
   - Roundabout phrasing variants ("At the roundabout, take the Nth exit"
     vs "Enter the roundabout" vs exit-counting language).
   - Merge/fork/keep-left vs keep-right phrasing, which is easy to
     confuse via simple keyword matching.
   - Anything that mapped to `STRAIGHT` or `UNKNOWN` that should have been
     something more specific.

## Deliverable

A plain list of findings, each with: the exact raw instruction string
captured, the `ManeuverType` the firmware assigned, and what you believe
the correct `ManeuverType` should have been (with brief reasoning). Group
obvious duplicates. Flag anything ambiguous rather than guessing. Do not
edit `ManeuverMapper.cpp`, `ManeuverMapper.h`, or any other file — this
phase is strictly observation and reporting.
