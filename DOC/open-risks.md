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

`PACKET_MAX_PAYLOAD` was bumped from 200 to 512 bytes during Stage 1, which
comfortably fits the current config schema (~370 bytes minified). This is
just correct sizing, not fragmentation -- if the schema grows a lot further
(many more plugins/settings), `ConfigManager::toJson()` will return 0 and
`CommManager::sendConfigData()` will NACK instead of sending a truncated
config. Real chunking across multiple packets is still not implemented.

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
feedback to the user.
