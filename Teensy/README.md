# CarCANdy — Teensy firmware

PlatformIO project for the Teensy 4.1 half of CarCANdy: owns the 3 CAN buses,
SD-card config/logging, and the plugin system.

## Build / flash

```
pio run                  # build
pio run -t upload        # flash over USB
pio device monitor       # serial monitor (115200 baud)
```

## Layout

- `src/main.cpp` — wires up ConfigManager -> Logging -> CANManager ->
  CommManager -> PluginManager.
- `src/ConfigManager.*` — loads `/config.json` from the SD card via ArduinoJson.
- `src/Logging.*` — Serial + SD CSV logging.
- `src/CANManager.*` — owns the 3 `FlexCAN_T4` instances.
- `src/CommManager.*` + `UARTClient.*` / `SPIClient.*` — link to the ESP32.
  SPI is currently a stub — see [../DOC/open-risks.md](../DOC/open-risks.md).
- `src/PluginBase.h` / `PluginManager.*` / `plugins/` — plugin system.

See [../DOC/architecture.md](../DOC/architecture.md) for the full picture.
