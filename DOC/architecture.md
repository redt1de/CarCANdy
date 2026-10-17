# CarCANdy Architecture

## Overview

```
 Ford Mustang GT CAN buses (x3)
        |
        v
 [SN65HVD230] x3  --  Teensy 4.1
        |                 |
        |          CANManager -> FilterManager (BUS,ID allow/deny/none)
        |                 |         |
        |          Logging (SD CSV + Serial), CommManager stream, PluginManager
        |          ConfigManager (JSON on SD)
        |                 |
        |            CommManager
        |          (UARTClient | SPIClient)
        |                 |
        |         UART or SPI + DRDY line
        |                 |
        v                 v
                     ESP32
                CommManager (mirror, master role for SPI)
                WifiManager (AP) + WebServerManager
                     |
                     v
              WebUI (React, static bundle
              served from LittleFS)
              Status | Config | Gauges | Sniffer | Plugins
```

## Modules

**Teensy** — owns the CAN buses, the source of truth for logging and config
(SD card), and future GPIO-driven plugins (shift light, line lock, etc).
Streams CAN frames and log lines to the ESP32 over the shared protocol so the
web UI can show live data.

**FilterManager** — software-side BUS,ID filtering (`allow`/`deny`/`none`,
see `DOC/config.example.json`'s `filters` section and `DOC/filters.example.csv`),
checked once in `CANManager` before a frame reaches logging, the ESP32
stream, or any plugin -- there is exactly one filter point, not a
per-consumer filter. The WebUI's Sniffer tab can temporarily override the
mode (without touching saved config) or add/remove/reload entries live, all
via `CMD` packets -- see `DOC/protocol.md`'s CMD grammar table.

**ESP32** — bridges to the Teensy, hosts a WiFi access point, and serves the
built WebUI bundle plus a small JSON API. Does not implement plugins or CAN
decoding itself — it is a bridge + presentation layer. On boot it repeatedly
sends `CONFIG_GET` until the Teensy replies, caching the JSON for `GET
/api/config`; `POST /api/config` and `POST /api/can/send` forward requests to
the Teensy (fire-and-forget -- see `CommManager::sendConfigSet()`/`sendCanFrame()`).
Inbound `CAN_FRAME`/`LOG_LINE` packets from the Teensy are broadcast to any
connected `/ws` WebSocket clients for the future Gauges/Sniffer pages.

**Firmware/Shared/** — protocol structs (`Packet.h`, `CanTypes.h`) and the abstract
`CommClient` interface, so both firmware projects agree on the wire format
without duplicating struct definitions.

**WebUI** — React + Vite frontend, built to a static bundle and copied into
`Firmware/ESP32/data/` for LittleFS upload. No Node runtime exists on the ESP32 itself;
Node/npm/Vite are purely build-time tooling.

## Development sequence

1. **Stage 1 — Teensy** (done, hardware-verified): CAN sniffing/logging/config
   solid on its own.
2. **Stage 2 — ESP32** (comms + API done): UART bridge with config caching,
   REST (`/api/status`, `/api/config`, `/api/can/send`) and a `/ws` WebSocket
   for live frame/log broadcast. SPI slave mode is paused by choice for now
   (UART is fast enough so far) -- see [open-risks.md](open-risks.md).
3. **Stage 3 — WebUI**: real data wired into the 5 tabs, consuming the API
   above.

See [../plan.md](../plan.md) for the original design goals this restates.
