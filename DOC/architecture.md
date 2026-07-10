# CarCANdy Architecture

## Overview

```
 Ford Mustang GT CAN buses (x3)
        |
        v
 [SN65HVD230] x3  --  Teensy 4.1
        |                 |
        |          CANManager, Logging (SD CSV + Serial),
        |          ConfigManager (JSON on SD), PluginManager
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

**ESP32** — bridges to the Teensy, hosts a WiFi access point, and serves the
built WebUI bundle plus a small JSON API. Does not implement plugins or CAN
decoding itself — it is a bridge + presentation layer. On boot it repeatedly
sends `CONFIG_GET` until the Teensy replies, caching the JSON for `GET
/api/config`; `POST /api/config` and `POST /api/can/send` forward requests to
the Teensy (fire-and-forget -- see `CommManager::sendConfigSet()`/`sendCanFrame()`).
Inbound `CAN_FRAME`/`LOG_LINE` packets from the Teensy are broadcast to any
connected `/ws` WebSocket clients for the future Gauges/Sniffer pages.

**Shared/** — protocol structs (`Packet.h`, `CanTypes.h`) and the abstract
`CommClient` interface, so both firmware projects agree on the wire format
without duplicating struct definitions.

**WebUI** — React + Vite frontend, built to a static bundle and copied into
`ESP32/data/` for LittleFS upload. No Node runtime exists on the ESP32 itself;
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
