**Project: BikeNav — ESP32-S3 round-display motorcycle navigation UI**

Repo: `~/esp32` (firmware, PlatformIO/Arduino/LovyanGFX, uses the
`fbiego/ChronosESP32` library as its BLE server). Sibling repo:
`~/bikenav-android` (custom Kotlin companion app, speaks ChronosESP32's
wire protocol but is otherwise fully custom — see its own `CLAUDE.md`).
Read `~/esp32/CLAUDE.md` first for full architecture/history.

## Goal

When the phone is connected over BLE and no navigation session is active
(i.e. the idle screen is showing), display the **real current time**
instead of a frozen placeholder. This is an implementation task, not
find-only — build it, flash it, and verify on real hardware.

## What already exists (don't rebuild these)

- `src/ui/screens/IdleScreen.cpp` **already draws a clock** from
  `state.clockHour`/`state.clockMinute` (`UIState.h`), with correct
  partial-redraw diffing (`lastHour_`/`lastMinute_`). The drawing code is
  fine as-is.
- The screen-selection logic in `main.cpp` already switches to
  `UIScreen::IDLE` on BLE connect and back to `IDLE` whenever a nav
  session ends (`handleConnectionChanged`, `handleNavigationUpdate`) — so
  "no map shown while connected" already correctly routes to `IdleScreen`.
  Verify this is still true, but don't expect to need to change it.

## The actual gap

`state.clockHour`/`state.clockMinute` are **only ever set in
`DemoController.cpp`** (mock mode). In live mode
(`BIKENAV_MOCK_MODE=0`), nothing updates them — they stay at their
`UIState.h` default of `0`/`0` forever, so the idle screen shows a frozen
"00:00" on real hardware. There is currently no code path at all feeding
real time into the UI.

## What's available to use (verify before relying on it — see caveats below)

- `ChronosESP32` (the library `ChronosAdapter` wraps — see
  `src/bluetooth/ChronosAdapter.h/.cpp`, the **only** file allowed to touch
  `ChronosESP32` types) **publicly inherits `ESP32Time`**
  (`.pio/libdeps/esp32-s3-lcd-1_28/ESP32Time/ESP32Time.h`), which exposes
  `getHour(bool mode)`, `getMinute()`, etc. — a real software RTC.
- When the library receives wire command `0x93` from the phone (dispatched
  in `dataReceived()` in `ChronosESP32.cpp`, `_incomingData.data[4] ==
  0x93`), it calls `configurationReceivedCallback(CF_TIME, ...)` **and**
  internally calls `this->setTime(sec, min, hour, day, month, year)` to set
  its own RTC from the phone's data. `ChronosAdapter::handleConfig()`
  already receives `CF_TIME` today but currently ignores it (see the
  comment listing unhandled `Config` values) — the RTC gets set by the
  library regardless of what our code does with the callback, so once a
  `0x93` frame arrives, `chronos_.getHour(true)`/`getMinute()` should
  reflect real time from that point on.
- **The open question you need to answer first**: does
  `~/bikenav-android`'s BLE client currently send this `0x93` time-sync
  command at all? Search its `ChronosProtocol.kt` / BLE client code. If it
  never does, `chronos_`'s RTC is never set by anything, and you'll need
  to add that send — see "Wire format" below.

## Wire format for command 0x93 (reverse-engineered from library source — UNVERIFIED, confirm against real traffic before trusting it)

Frame prefix matches the already-documented protocol in `CLAUDE.md`:
`0xAB, lenHi, lenLo, marker, command, ...` where `command = data[4] = 0x93`.
Reading the library's own parsing of this case:

```
this->setTime(data[13], data[12], data[11], data[10], data[9], data[7]*256 + data[8]);
// ESP32Time::setTime(sec, min, hour, day, month, year) — confirm this param order
// against ESP32Time.h before trusting the mapping below.
```

So, inferred payload layout (indices into the full frame, 0-based):
- `data[7..8]` = year, big-endian uint16
- `data[9]` = month
- `data[10]` = day
- `data[11]` = hour (confirm 24h vs. phone-local convention)
- `data[12]` = minute
- `data[13]` = second
- `data[5]`/`data[6]` = unclear from this derivation alone (no subtype
  switch on `data[5]` exists for this command, unlike most others) — don't
  assume they're padding without checking; dump the raw bytes of a real
  `0x93` frame if you can find one from another Chronos-protocol client to
  confirm, or just construct a candidate frame and verify by reading back
  `chronos_.getHour()/getMinute()` on the firmware side (temporary Serial
  print) after sending it.

Treat this whole wire-format section as a hypothesis to verify, not a
spec to implement blind.

## Suggested implementation shape (adjust if investigation shows otherwise)

1. **ESP32 side**: Add a method to `ChronosAdapter` (e.g. `bool
   getSyncedTime(uint8_t& hour, uint8_t& minute)`), backed by `chronos_`
   state, that returns false until a `CF_TIME` config has actually been
   received at least once (don't report a confident-looking but meaningless
   "00:00" or boot-time default as if it were real). Expose it through
   `BluetoothManager` (keeping the existing PIMPL boundary — `ChronosAdapter`
   / `ChronosESP32` types must never leak into `BluetoothManager.h` or
   beyond, per the project's established pattern).
2. In `main.cpp`'s `loop()`, poll this every iteration (or on the existing
   heartbeat) and update `uiManager.state().clockHour/clockMinute` whenever
   available — don't rely solely on a push callback, since the RTC should
   keep advancing between syncs and the UI needs to reflect that tick (the
   ESP32Time base class handles the actual time-keeping once set; you just
   need to re-read it periodically).
3. Decide and implement a sane fallback for "BLE connected, but time was
   never synced yet" (e.g. hide the clock / show a placeholder like
   "--:--" instead of a wrong-looking "00:00") rather than silently
   displaying incorrect time as if it were correct.
4. **Android side** (only if step 0's investigation shows the app doesn't
   already send time sync): implement sending the `0x93` frame after BLE
   connects, built from the phone's real system clock. Verify correctness
   by temporarily logging `chronos_.getHour(true)`/`getMinute()` on the
   firmware side right after a sync and comparing to the phone's actual
   clock.

## Constraints

- Keep `BluetoothManager.h`'s PIMPL isolation intact — this already
  collided once before (a `MusicInfo` global-namespace clash), fixed via
  `unique_ptr<ChronosAdapter>` — don't regress it.
- Don't touch navigation/`ManeuverMapper`/BLE nav protocol code — unrelated
  to this task.
- Verify both `BIKENAV_MOCK_MODE=0` (live) and `=1` (demo) still compile.
- Flash to real hardware and confirm: idle screen shows correct real time
  after connecting, time visibly keeps ticking forward while idle (not
  frozen), and nothing misleading is shown before the first real sync.
