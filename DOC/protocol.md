# Teensy <-> ESP32 Protocol

Defined in [`Shared/Packet.h`](../Shared/Packet.h) and
[`Shared/CanTypes.h`](../Shared/CanTypes.h).

## Frame format

```
[sync: uint8 = 0xAA][type: uint8][length: uint16][payload: uint8[length]][crc8: uint8]
```

`crc8` is computed over `type + length + payload[0..length)`.

## Message types (`MsgType`)

| Type        | Direction        | Purpose                                   |
|-------------|------------------|--------------------------------------------|
| PING        | either           | liveness check                             |
| ACK / NACK  | either           | acknowledge / reject a prior message       |
| CMD         | ESP32 -> Teensy  | generic command (restart, request status) -- not yet implemented, Teensy replies NACK |
| CONFIG_GET  | ESP32 -> Teensy  | request current config                     |
| CONFIG_SET  | ESP32 -> Teensy  | apply new config (JSON bytes as payload)   |
| CONFIG_DATA | Teensy -> ESP32  | config JSON bytes, reply to CONFIG_GET     |
| CAN_FRAME   | either           | Teensy -> ESP32: a frame received off the bus. ESP32 -> Teensy: a frame to transmit, subject to `CANManager::sendFrame()`'s per-bus enabled/mode check (ACK/NACK reply) |
| LOG_LINE    | Teensy -> ESP32  | text log line for the status page          |
| STATUS      | Teensy -> ESP32  | periodic health/status blob                |

The Teensy-side dispatch lives in `Teensy/src/CommManager.cpp`. The ESP32
side (`ESP32/src/CommManager.cpp`) sends `CONFIG_GET` on a retry loop until
the Teensy answers (caching the JSON for `GET /api/config`), forwards
`POST /api/config` as `CONFIG_SET` and `POST /api/can/send` as a `CAN_FRAME`
transmit request (both fire-and-forget -- see below), and forwards inbound
`CAN_FRAME`/`LOG_LINE` out to WebSocket clients via callbacks. `STATUS` isn't
produced by the Teensy yet, so the ESP32 side just has a `TODO` stub for it.

### Fire-and-forget requests (no reply correlation yet)

Neither side's protocol includes a request ID, so `sendConfigSet()` and
`sendCanFrame()` on the ESP32 don't wait for or correlate the Teensy's
ACK/NACK back to the HTTP request that triggered them -- the HTTP response
just confirms the packet was queued for send, not that the Teensy accepted
it. For `CONFIG_SET`, the ESP32 re-requests `CONFIG_GET` ~500ms later so
`GET /api/config` catches up either way. A NACK (e.g. a rejected transmit
because the target bus is `read_only`) is logged on the ESP32's debug
`Serial` (USB)
but not (yet) surfaced back to the web client -- worth adding request IDs
and a real response if that silence turns out to be confusing in practice.

## Transport-specific framing notes

- **UART** is a continuous byte stream. The receiver must scan for `sync`,
  tentatively read `type` + `length`, then validate `crc8` before trusting the
  frame — `0xAA` can legitimately appear inside real CAN payload bytes, so a
  single matching sync byte is not proof of a real frame boundary (false-sync
  risk). `CarCANdy::PacketStreamParser` (in `Shared/Packet.h`) implements this
  as a byte-at-a-time state machine: on a CRC mismatch it resets to scanning
  for the next sync byte rather than assuming the whole link desynced. Both
  the Teensy's and the ESP32's `UARTClient` feed it and queue up to 8
  completed packets so a burst arriving between `poll()` calls isn't dropped
  (a 9th packet before `CommManager` drains the queue is dropped and logged;
  neither side retries on a dropped/missing reply yet, since there's no
  request ID to retry against -- see the fire-and-forget note below).
- **SPI** is paused by choice for now (UART is fast enough so far -- see
  [open-risks.md](open-risks.md) #1); when it resumes: transaction-based, the
  ESP32 (master) initiates fixed-size transfers, so there's no
  continuous-stream resync problem — `length` just says how much of the
  transfer is valid payload. The Teensy (slave) uses a dedicated DRDY GPIO
  line to tell the ESP32 when a frame is queued, since SPI slaves cannot push
  data unsolicited.

## Known limitation

`PACKET_MAX_PAYLOAD` is 512 bytes, sized to comfortably fit the current
config schema (~370 bytes, see `DOC/config.example.json`) with headroom for
a few more plugins. If the config schema grows a lot further (many more
plugins/settings), it could still exceed this — see
[open-risks.md](open-risks.md). `ConfigManager::toJson()`/`CommManager` treat
"doesn't fit" as an explicit failure (NACK) rather than corrupting data, but
real fragmentation/chunking across multiple packets is still not
implemented.

## Endianness / struct layout

Both Teensy 4.1 (Cortex-M7) and ESP32 (Xtensa LX6) are little-endian
GCC-based toolchains, and all wire structs use `#pragma pack(push, 1)`, so raw
struct-to-struct memcpy across the link is safe. This assumption would break
if a big-endian target were ever added to the project.
