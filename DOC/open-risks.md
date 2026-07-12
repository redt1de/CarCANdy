# Open Technical Risks

Flagged during initial scaffolding and Stage 1 (Teensy) implementation.

## 1. Teensy SPI slave mode (paused by choice)

The plan calls for the ESP32 to act as SPI master and the Teensy as SPI
slave (plus a DRDY GPIO line). Stock Arduino `SPI.h` targets **master** mode.
Teensy 4.x's LPSPI peripheral does support hardware slave mode, but it likely
needs direct register/DMA setup or a community library rather than the stock
`SPI` object.

**Status**: intentionally deferred, not just unresolved -- UART has been
plenty fast on real hardware through Stage 1/2, so this is on hold until/unless
higher throughput is actually needed. `Teensy/src/SPIClient.*` and
`ESP32/src/SPIClient.*` remain compiling stubs (`begin()` returns `false`,
other methods are no-ops) in the meantime.

## 2. Config payload size vs. `PACKET_MAX_PAYLOAD` (partially addressed)

`PACKET_MAX_PAYLOAD` was bumped 200 -> 512 during Stage 1, then 512 -> 1024
once the real on-device config (with the current plugin set's fields) hit
~505 bytes -- right at the old ceiling, not comfortably under it anymore.
This is still just correct sizing, not fragmentation -- if the schema keeps
growing (many more plugins/settings), the same problem recurs, just later.
Options considered, roughly cheapest to most involved: raise this constant
again; encode the wire payload as MessagePack instead of JSON text
(`ArduinoJson` already supports this, no new dependency, saves an
estimated 15-30% but doesn't remove the ceiling); split `CONFIG_GET`/
`CONFIG_SET` into per-section requests (`canBuses`, `plugins`, etc., each
naturally small, no reassembly needed, and can stay transparent to the
WebUI's `/api/config` contract -- only the ESP32<->Teensy transport
underneath would change); or real fragmentation/chunking across multiple
packets (still not implemented, and still needs a bounded reassembly
buffer, so it raises the ceiling rather than removing it too -- its real
advantage is keeping the common-case packet size, e.g. `CAN_FRAME`, small
regardless of config size).

## 3. ESP32 flash partition sizing

The board moved from an ESP32-S3 devkit (8MB flash) to a classic ESP32
(`esp32dev`), which usually means less flash and a smaller default app
partition. As of this pass the firmware binary alone is already ~66% of the
default app partition (868KB / 1.25MB) with no WebUI bundle involved yet
(that lives in a separate LittleFS partition, unaffected by app code size).
Headroom for future firmware growth is real but shrinking -- worth watching,
and may need a custom `partitions.csv` (or a board with more flash) if it
gets tight. The WebUI bundle's size vs. the LittleFS partition itself is
still an open question, unchanged by the board switch.

The project now also supports an ESP32-S3-Mini ("Super Mini ESP32-S3")
board as a second environment (`esp32-s3-mini` in `ESP32/platformio.ini`,
alongside `esp32dev`). It's configured with the same `default.csv`
single-app partition scheme, so it has essentially the same ~64% headroom
situation as `esp32dev`, not meaningfully better -- despite the chip having
4MB of physical flash, the partition table (not the chip) is what's
constraining app size here.

## 4. ESPAsyncWebServer library provenance

The original `me-no-dev/ESPAsyncWebServer` repo is unmaintained. `platformio.ini`
currently points at the `ESP32Async` org's actively maintained fork — verify
the exact registry name/version is still current before relying on it.

## 5. No request/reply correlation over the Teensy<->ESP32 link

`POST /api/config` and `POST /api/can/send` on the ESP32 forward to the
Teensy fire-and-forget (see `DOC/protocol.md`) -- there's no request ID, so
a NACK (e.g. a rejected CAN transmit on a `read_only` bus) is only logged to
the ESP32's debug `Serial` (USB), never surfaced back to the HTTP client
that triggered it. Fine for now since both are hand-tested over serial
monitor, but worth
revisiting once the WebUI (Stage 3) needs to show real success/failure
feedback to the user. This now also applies to `/api/cmd`'s filter-control
commands (`ADD_FILTER`, etc.) -- a rejected/malformed command NACKs
silently on the Teensy's debug `Serial` with no feedback in the WebUI
beyond "sent" in the Sniffer tab's status line.

## 6. FilterManager matching is a linear scan

`FilterManager::matchesAnyEntry()` scans the entry list (capped at
`kMaxEntries = 128`) on every received frame. At realistic vehicle-scale
entry counts and frame rates this is trivially fast on a 600MHz Cortex-M7,
but if the entry count or bus load ever grows enough to matter, this is the
place to optimize (e.g. a sorted array + binary search, or a hash set keyed
on `(bus, id)`) -- not done now since there's no evidence it's needed yet
(same reasoning as the mailbox-vs-software-filtering discussion this
feature came out of).

**Also worth knowing**: `mode: "allow"` with an empty entry list drops
*everything* -- `FilterManager::begin()` logs a warning for this case, but
it's easy to trip over by switching to `allow` mode before populating any
filter entries.

## 7. (Resolved) CANManager never actually received real bus traffic

`CANManager::begin()` originally called only `begin()`/`setBaudRate()`/
`onReceive()` per bus -- this compiles and works for *transmitting*, but
silently never receives anything. Confirmed by reading FlexCAN_T4's actual
ISR (`flexcan_interrupt()` in `FlexCAN_T4.tpp`): its default (non-FIFO)
mailbox setup only calls `enableMBInterrupt()` on the TX half of mailboxes,
so RX mailboxes accept frames at the hardware level but the ISR's
`if (!(imask & (1ULL << mb_num))) continue;` check skips them -- the
software callback never fires. Fixed by adding `setMaxMB(16)` +
`enableFIFO()` + `enableFIFOInterrupt()` before `onReceive()` (FIFO mode is
a separate path with its own interrupt bit that these calls correctly
enable), matching a previously bench-verified standalone sketch for this
hardware. This went unnoticed through Stage 1/2 because nothing had tested
receiving real, unsolicited bus traffic yet -- transmit and the ESP32 link
worked fine without it.
