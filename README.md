# CarCANdy

A CAN bus sniffing/analysis/control tool for a 2015 Ford Mustang GT, built on
a Teensy 4.1 (3x SN65HVD230 CAN transceivers) bridged to an ESP32 that
hosts a WiFi AP and a web UI.

See [plan.md](plan.md) for the original design goals and
[DOC/architecture.md](DOC/architecture.md) for how the pieces fit together.

## Layout

- [Teensy/](Teensy/) — PlatformIO project for the Teensy 4.1
- [ESP32/](ESP32/) — PlatformIO project for the ESP32
- [WebUI/](WebUI/) — React + Vite frontend
- [Shared/](Shared/) — protocol headers shared by both firmware projects
- [DOC/](DOC/) — notes, pinouts, protocol writeup, open risks

## Status

Initial scaffolding — both firmware projects build clean and the WebUI
serves its 5 tabs, but most logic is still `TODO`. See
[DOC/open-risks.md](DOC/open-risks.md) for known unresolved issues, notably
Teensy SPI slave mode.
