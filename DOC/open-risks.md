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

## 8. (Resolved) WebSocket CAN frame broadcast silently dropped almost everything under real load

`WebServerManager` used to call `ws_.textAll()` once per `CAN_FRAME` packet
received from the Teensy. Confirmed by reading `AsyncWebSocket.cpp`/`.h`:
each client has a fixed `WS_MAX_QUEUED_MESSAGES` send queue (32 on ESP32),
and `_queueMessage()` silently returns `false` (message discarded, not
retried) once it's full. A live vehicle easily produces hundreds of CAN
frames/sec across 3 buses -- far faster than WiFi can drain a 32-slot
queue -- so almost every broadcast was being dropped, which looked like
"the Sniffer page doesn't stream, it only updates in small chunks."

Fixed by batching: `WebServerManager::queueCanFrame()` now buffers frames
(capped at `kMaxBatch = 64`, dropping the excess on overflow -- the
Teensy's SD card CSV remains the complete, lossless record regardless) and
`loop()` flushes them as one `{"type":"can_frames","frames":[...]}` WS
message every `kFlushIntervalMs = 50`, instead of one message per frame.
This keeps WS sends at ~20/sec regardless of CAN bus load, comfortably
under the queue cap. `WebUI/src/ws.ts` unwraps the batch back into
individual `can_frame` message dispatches, so `SnifferPage`/`GaugesPage`
never needed to change at all -- the batching is invisible above `ws.ts`.

If bus load or plugin/consumer count ever grows enough that 64 frames over
50ms isn't enough headroom, raise `kMaxBatch`/shorten `kFlushIntervalMs`
first before reaching for anything more complex -- there's no evidence a
fancier scheme is needed yet.

## 9. (Partially resolved) Teensy SD (SdFat) multi-file-open corruption/reliability issues

Two related findings from real-vehicle testing of debug logging (`Logging`
holding `logFile_`/`debugFile_` open for the entire runtime):

**a) Cross-file corruption (fixed).** `config.json` was found on the SD card
with literal debug-log text (`"[INFO] CarCANdy Teensy firmware
started[CommManager] CONFIG_GET received..."`) overwriting part of its JSON.
Teensy's SD (SdFat) library appears to share a single sector cache across
all open files -- `ConfigManager::save()`/`begin()` and
`FilterManager::loadFromFile()`/`writeToFile()` were each opening
`config.json`/`filters.csv` transiently while `logFile_`/`debugFile_` were
also open, and writes to one file apparently landed on the other's disk
sectors. Fixed with `Logging::suspend()`/`resume()` (close both files with a
flush, do the other work, reopen) and a `LoggingPause` RAII wrapper used at
all four of those call sites, plus exposed to plugins via `PluginBase::init()`'s
`Logging&` parameter for their own SD access. Not independently verified on
hardware that this specific fix holds under all conditions -- see (b).

**b) Reopen-after-close can itself fail, and failed completely silently
(fixed).** On the very next real-vehicle test, `resume()`'s `SD.open()` calls
(reopening `logFile_`/`debugFile_` after (a)'s `suspend()`) were observed to
fail -- both the CSV and debug log contained *only* their first line (the
header / the "debug logging to..." message printed inside `begin()`),
meaning logging died within the first second or two of boot, during
`PluginManager::begin()`'s unconditional `config_.save()` call -- which
happens on literally every boot. `resume()` didn't check whether its reopen
succeeded, so `sdReady()`/the debug equivalent kept reporting healthy while
the underlying `File` handles were silently dead for the rest of that boot;
the status LED (see `main.cpp`) couldn't catch this either, since it only
read a one-time snapshot from `setup()`, not live state.

Fixed three ways: `resume()` now retries the reopen a few times and, on
persistent failure, explicitly marks that subsystem down (`sdReady_`/
`debugEnabled_ = false`) rather than lying about it; `Logging::loop()`
periodically (`kRecoveryIntervalMs`) retries recovering from that state
instead of staying down for the rest of the boot; and the status LED now
polls `logging.sdReady()` live every `loop()` iteration. Still unknown:
*why* the reopen fails right after a clean close() on this hardware/library
-- the fix makes failure recoverable and visible rather than addressing a
root cause we don't have direct evidence for yet. If this keeps recurring
often in the field, worth investigating whether avoiding the close()
entirely (e.g. `flush()`-only during the paused window, if that turns out
to be sufficient to avoid (a)'s corruption) is viable, and/or whether
`PluginManager::begin()`'s unconditional every-boot `config_.save()` can be
skipped when nothing actually changed, to reduce how often the risky
suspend/resume window happens at all.
