# CANDB — vehicle CAN ID databases

This folder holds one JSON file per vehicle, giving CarCANdy (eventually) a way to decode
raw CAN frames into named signals. Nothing in here is wired into the firmware/WebUI yet —
these are reference databases only.

## Files

- `2015_ford_mustang_gt.json` — 2015 Ford Mustang GT (S550), built from three community
  sources plus one signal verified on the project owner's own car.

## Schema

```
{
  "vehicle": { make, model, years, notes },
  "importantCaveats": [ ... ],
  "sources": [ { id, title, url, byteIndexConvention, note } ],
  "messages": {
    "0xNNN": {
      "idDecimal": ...,
      "vehicleBus": ["HS1"|"HS3"|"MS"|...],
      "periodMs": ... | null,
      "sources": ["sourceId", ...],
      "signals": [
        {
          "name": "...",
          "byteIndex": [0-based indices into CanFrame.data[8]],
          "type": "linear" | "enum" | "bitfield" | "raw" | "unconfirmed" | "conflict",
          "formula": "...",           // for type=linear, in terms of b0..b7
          "unit": "...",
          "enum": { "rawValue": "meaning" },
          "bits": { "bitIndex": "meaning" },
          "confidence": "verified-on-vehicle" | "community-cross-confirmed"
                       | "community-single-source" | "unconfirmed" | "conflict",
          "source": "sourceId" | ["sourceId", ...],
          "sourceRaw": { ... },       // original as-published values, for signals whose
                                      // byte index was converted (see below)
          "notes": "..."
        }
      ]
    }
  }
}
```

## Confidence levels

- `verified-on-vehicle` — confirmed against real captured data from this project's own
  2015 Mustang GT (currently only `0x204` RPM, via `Teensy/src/plugins/ShiftLightPlugin.h`).
- `community-cross-confirmed` — two or more independent community sources agree.
- `community-single-source` — reported by exactly one community source, not yet checked.
- `unconfirmed` — the source itself flagged the signal as a guess (often written in
  *italics* or with a `?` in the original material).
- `conflict` — sources disagree on the same byte/ID; both interpretations are preserved
  side by side rather than one being silently picked. Look at each signal's `notes` for
  the specifics, and verify against your own sniffer capture before trusting either.

## Sources

1. **ivanyshyn** — [`parse_can_logs`](https://github.com/v-ivanyshyn/parse_can_logs/blob/master/Ford%20CAN%20IDs%20Summary.md),
   0-based byte indexing (author's own `b0, b1, ...` convention).
2. **eturner3** — [`s550-canbus`](https://github.com/EricTurner3/s550-canbus/blob/master/database.md),
   explicitly 0-based ("All byte references start at 0, so the range is 0-7"). Tested on a
   2015 Mustang GT Premium and a 2016 Mustang GT350.
3. **mustang6g_forum_csv** — the mustang6g.com thread ["Some results of CAN
   research"](https://www.mustang6g.com/forums/threads/some-results-of-can-research.176383/).
   This thread could not be fetched directly (blocked by anti-bot on a direct fetch and on
   WebFetch, no Wayback Machine snapshot exists, and a reader-proxy fallback was also
   blocked), so its data was supplied by the user as a CSV paste instead of scraped. Its
   original `Byte_position` column reads as **1-based** — this was inferred (not assumed)
   by cross-checking overlapping signals against the two 0-based GitHub sources (e.g. the
   `0x230` gearbox-mode byte and the `0x2A1` media-button byte line up exactly once you
   subtract 1). Every signal sourced from this CSV keeps its original `Byte_position` in a
   `sourceRaw` field alongside the converted 0-based `byteIndex`, so you can double check
   the conversion.
4. **carcandy_verified** — signals actually confirmed against this project's own vehicle
   in firmware code, not just reported by a third party.

## Critical gotcha: vehicle bus names vs. CarCANdy bus numbers

Every source above labels buses by their **vehicle-side** name: `HS1`, `HS3` (both
high-speed CAN, 500kbps-ish) and occasionally `MS` (medium-speed CAN). This has **nothing
to do** with CarCANdy's own `bus` numbering (1/2/3), which just reflects which of the
Teensy's three physical `FlexCAN_T4` controllers (CAN1/CAN2/CAN3) a given transceiver
happens to be wired to (see `Teensy/include/CANManager.h` and `config.json`'s
`canBuses[].id`).

Before using any ID/byte offset from this database in a CarCANdy filter, plugin, or
decoder, you need to know which vehicle bus (HS1/HS3/MS) is physically connected to which
CarCANdy bus number on your car — that mapping lives (or should live, once filled in) in
`DOC/pinout.md`. Don't assume "bus 1" here means "HS1".

## Known unresolved conflicts between sources

- **`0x081`** (steering wheel arrow buttons) — ivanyshyn places the up/down/left/right/ok
  bitmask at byte 0; eturner3 reports the identical bit values at byte 1. Both sources are
  explicitly 0-indexed, so this isn't an indexing artifact.
- **`0x202`** (speed gauge) — ivanyshyn: `raw/100`; eturner3: `mph * 175`. Different scale
  entirely for the same two bytes.
- **`0x3B3` byte 1** (ambient light / hazard) — ivanyshyn/eturner3 read `0x88` as
  "twilight/night"; the forum CSV reads the same value as "hazard warning off" (with
  `0x89`/`0x8A` as hazard-on). Directly conflicting interpretations of the same byte value.

All three are preserved as separate `signal` entries with `confidence: "conflict"` and a
`notes` field explaining the disagreement — verify against a live capture on your car
before relying on any of them.

## Anomaly worth flagging

`0x4C` (eturner3, airbag/seatbelt indicators) is a 2-hex-digit ID, far shorter than every
other ID in this database (all others fall in `0x076`–`0x4B0`). This might be a
transcription shorthand in the original source rather than the literal arbitration ID —
noted in the JSON entry itself, treat with extra suspicion until confirmed against a
capture.
