# CarCANdy — ESP32 firmware

PlatformIO project for the ESP32 half of CarCANdy: bridges to the Teensy,
hosts a WiFi access point, and serves the WebUI.

## Build / flash

```
pio run                       # build firmware
pio run -t upload             # flash firmware over USB
pio run -t uploadfs           # flash the LittleFS image (contents of data/)
pio device monitor            # serial monitor (115200 baud, default USB Serial)
```

## Deploying the WebUI

1. Build the frontend: `cd ../WebUI && npm install && npm run build`
2. Copy the output into this project's filesystem image root:
   `rm -rf data/* && cp -r ../WebUI/dist/* data/`
3. `pio run -t uploadfs`

## API routes

- `GET  /api/status` — AP client count, config-ready state, uptime.
- `GET  /api/config` — cached config JSON last fetched from the Teensy (503 if not yet fetched).
- `POST /api/config` — forwards a new config JSON to the Teensy (fire-and-forget, see [../DOC/protocol.md](../DOC/protocol.md)).
- `POST /api/can/send` — `{"bus":0,"id":291,"dlc":8,"data":[...]}`, forwarded as a CAN transmit request.
- `WS   /ws` — broadcasts `{"type":"can_frame",...}` / `{"type":"log","msg":"..."}` as the Teensy reports them.

## Layout

- `src/main.cpp` — wires up WifiManager -> CommManager -> WebServerManager.
- `src/WifiManager.*` — starts the WiFi access point.
- `src/CommManager.*` + `UARTClient.*` / `SPIClient.*` — link to the Teensy
  (master role for SPI). SPI is paused by choice for now, UART-only — see
  [../DOC/open-risks.md](../DOC/open-risks.md).
- `src/WebServerManager.*` — ESPAsyncWebServer + LittleFS + WebSocket, serves
  the WebUI bundle plus the API routes above.
- `src/Logging.*` — debug/status logging over the default USB Serial
  (Serial2, GPIO16/17, is reserved exclusively for the UART link to the
  Teensy).

See [../DOC/architecture.md](../DOC/architecture.md) for the full picture.
