# Google Maps maneuver vocabulary vs. BikeNav coverage

Compiled 2026-11-08 from a live maneuver-mapping audit: 6 simulated drives
across real Delhi roads (Ashram Chowk, Dhaula Kuan, AIIMS, Sarai Kale Khan,
India Gate), GPS fed via a custom mock-location app using Google's Routes
API for real road-following polylines, Google Maps running live turn-by-turn
navigation, captured via the actual Android→BLE→firmware pipeline (not a
synthetic/API-field source — this is the literal notification text Google
Maps shows a rider). ~95 distinct raw instruction strings observed.

## 1. Confirmed real-world phrasings, by category

Each row is text actually seen live. "Firmware result" is what
`ManeuverMapper` produces as of this audit (after the fixes below).

| Google Maps phrasing (real examples) | ManeuverType |
|---|---|
| "Turn left" / "Turn left onto Mustafa Kemal Attaturk Rd" / "Turn left after Sri Sathya Sai International Centre (on the left)" / "Turn left onto the ramp to Golden Quadrilateral" | TURN_LEFT |
| "Turn right" / "Turn right onto Kautilya Marg" / "Turn right toward Motilal Nehru Marg" | TURN_RIGHT |
| "Slight right" | SLIGHT_RIGHT |
| "Use any lane to turn slightly left to stay on NH 44" / "Use the 2nd from the right lane to turn slightly left to stay on Golden Quadrilateral" / "Use the left 2 lanes to turn slightly left onto Purana Quila Rd" | SLIGHT_LEFT |
| "Sharp right onto Delhi - Ajmer Hwy/Golden Quadrilateral" | SHARP_RIGHT |
| "Make a U-turn" | U_TURN_RIGHT (direction not derivable from text — see Known Gaps) |
| "Keep right to take the flyover" | KEEP_RIGHT |
| "Join with NH 44" / "Join with Mahatma Gandhi Rd/Ring Rd" / "Join with C - Hexagon/Copernicus Marg/India Gate Cir" | MERGE |
| "At the roundabout, turn right" | ROUNDABOUT_RIGHT |
| "At the roundabout, take the 1st/2nd/3rd/4th exit [and stay on / onto <road>]" | ROUNDABOUT_EXIT_CW |
| "Exit the roundabout [onto <road>]" | ROUNDABOUT_EXIT_CW |
| "At `<Named Circle>`, take the Nth exit and stay on `<road>`" (e.g. "At Teen Murti-Haifa Chowk, take the 4th exit...", "At Maharaja Ranjeet Singh Rd, take the 4th exit...") — named traffic circles, no literal word "roundabout" | ROUNDABOUT_EXIT_CW |
| Plain road/street names with no verb ("Delhi Rd", "Mathura Rd", "NH 44", "Golden Quadrilateral", "Vinay Marg", "Teen Murti Marg", ...) — these are genuine "continue on this road" cruise instructions, not turns | STRAIGHT |
| "Continue straight" / "Continue straight onto `<road>`" | STRAIGHT |

## 2. Fixes applied this session

1. **`bikenav-android`'s `NotificationParser.kt`** was reading only
   `secondaryInfo`, which Google's notification silently reduces to just the
   destination road name for any "`<verb>` onto/to stay on/toward
   `<road>`" instruction — losing the verb (turn/keep/slight/sharp/merge)
   entirely. Fixed to prefer the instruction parsed from `android.title`'s
   "`<distance>` · `<instruction>`" form, which always retains the verb.
2. **Firmware instruction buffers were 64 bytes**, silently truncating any
   instruction over 63 characters — confirmed on real data (a 68-char
   instruction got cut to 63, and a Hindi-script instruction got cut
   mid-character, producing invalid bytes). Bumped to 128 bytes throughout
   (`BluetoothEvents.h`, `NavigationData.h`, `NavigationManager.h`,
   `ManeuverMapper.cpp`'s internal buffers) and made `copyTruncated()`
   UTF-8-boundary-aware.
3. **`ManeuverMapper` didn't recognize "join"** as a merge synonym (only
   "merge") — Google Maps uses "Join with `<road>`" interchangeably.
4. **Named traffic circles weren't detected as roundabouts** — Google Maps
   sometimes names the circle instead of saying "roundabout" (e.g. "At Teen
   Murti-Haifa Chowk, take the 4th exit..."). Now detected via an ordinal
   number ("2nd", "4th", ...) next to "exit", which is a roundabout-only
   signal (ordinary highway exits say "take the exit toward X", no ordinal).
5. **Roundabout rotation hardcoded to CCW** — wrong for India (left-hand
   traffic → clockwise roundabouts), confirmed visually wrong on real
   hardware. Flipped the default to CW. This is still a hardcoded
   assumption, not a proper per-region solution (see Known Gaps).
6. **Roundabout exit-number badge wasn't rendered** — `NavigationInfo`
   already carried `roundaboutExitNumber` correctly, but
   `NavigationArrow::draw()` explicitly discarded it (no PNG asset encodes
   an arbitrary number). Added a composited number badge (filled circle +
   VLW text, drawn on top of the base icon) instead of requiring a new PNG
   per number. Also fixed two related bugs this exposed: `main.cpp`'s gating
   for *when* to compute the exit number was checking stale pre-fix enum
   values (would never match post-fix), and `NavigationScreen`'s
   partial-redraw diff only checked maneuver type, not exit number, so a
   badge could go stale between two same-type roundabouts with different
   exit counts.

## 3. BikeNav `ManeuverType` enum — asset coverage

| ManeuverType | Asset | Notes |
|---|---|---|
| STRAIGHT | dedicated PNG | |
| TURN_LEFT / TURN_RIGHT | dedicated PNG | |
| SLIGHT_LEFT / SLIGHT_RIGHT | dedicated PNG | |
| SHARP_LEFT / SHARP_RIGHT | dedicated PNG | |
| U_TURN_LEFT / U_TURN_RIGHT | dedicated PNG | |
| KEEP_LEFT / KEEP_RIGHT | **shared** — reuses FORK_LEFT/RIGHT art | intentional, visually similar |
| MERGE / MERGE_LEFT / MERGE_RIGHT | dedicated PNG | |
| FORK_LEFT / FORK_RIGHT | dedicated PNG | |
| EXIT_LEFT / EXIT_RIGHT | **vector**, not PNG | no highway-ramp asset in the icon set used |
| DESTINATION | dedicated PNG | |
| DEPART | dedicated PNG | |
| ARRIVED | **shared** — reuses DESTINATION art | intentional |
| UNKNOWN | **vector** "?" glyph | intentional full-screen fallback state, not for routine use |
| ROUNDABOUT (bare) | dedicated PNG, **CCW only** | **missing asset**: no clockwise variant exists; this specific bare case (roundabout mentioned, no left/right/exit/sharp/slight/straight word) still renders CCW even after the CW fix |
| ROUNDABOUT_LEFT / ROUNDABOUT_RIGHT (bare, non-CW/CCW-qualified) | dedicated PNG, CCW only | no longer emitted by `ManeuverMapper` post-fix (routes through the `_CW`-qualified family below instead); kept for any other caller (e.g. demo mode) |
| ROUNDABOUT_SHARP_LEFT/RIGHT_CW/CCW | dedicated PNG, both directions | |
| ROUNDABOUT_SLIGHT_LEFT/RIGHT_CW/CCW | dedicated PNG, both directions | |
| ROUNDABOUT_EXIT_CW/CCW | dedicated PNG, both directions | now gets the number badge overlay |
| ROUNDABOUT_UTURN_CW/CCW | dedicated PNG, both directions | |
| ROUNDABOUT_LEFT_CW/CCW / ROUNDABOUT_RIGHT_CW/CCW | dedicated PNG, both directions | |
| ROUNDABOUT_STRAIGHT_CW/CCW | dedicated PNG, both directions | |

**The one genuinely missing PNG asset found**: a clockwise variant of the
bare `ic_roundabout_counterclockwise_png` (no left/right/exit/sharp/slight
qualifier). Low real-world impact — never observed in 95 captured real
instructions, since Maps virtually always qualifies a roundabout instruction
with a direction or exit count. Not fixed (would require generating a new
PNG asset, out of scope for a text/logic session) — flagged for awareness.

## 4. Google's broader maneuver vocabulary — anything BikeNav has zero
   concept of?

Cross-referencing BikeNav's `ManeuverType` enum against Google's documented
Directions API `maneuver` field vocabulary (turn-slight-left/right,
turn-sharp-left/right, uturn-left/right, turn-left/right, straight,
ramp-left/right, merge, fork-left/right, ferry, ferry-train,
roundabout-left/right, depart, keep-left/right): every category has a
BikeNav equivalent **except**:

- **FERRY / FERRY-TRAIN** — no `ManeuverType` value, no icon, no keyword
  handling in `ManeuverMapper` at all. **Unconfirmed / untested** — no Delhi
  route can exercise this (no ferries), so this is a theoretical gap based
  on Google's documented vocabulary, not something observed live. Marking
  as unknown rather than guessing at real-world phrasing or adding
  speculative handling.

## 5. Known, already-accepted gaps (not fixed this session, not actionable
   from instruction text alone)

- Bare "Make a U-turn" (no left/right word in the text) always resolves to
  `U_TURN_RIGHT` — Google's own text doesn't encode direction here, same
  root cause as the roundabout CW/CCW problem.
- The CW/CCW roundabout default is now hardcoded to CW for India rather
  than solved generally — correct for every route this firmware is actually
  used on, but wrong if ever deployed in a right-hand-traffic country. A
  real fix needs a locale/region signal threaded in from the phone side,
  which doesn't exist in the current BLE protocol.
