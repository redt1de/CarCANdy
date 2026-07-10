# CarCANdy — WebUI

React + Vite frontend for CarCANdy, served as a static bundle by the ESP32
(no Node runtime on-device — Node/npm/Vite are build-time tooling only).

## Develop

```
npm install
npm run dev
```

Runs against a dev server on your machine; `/api/*` calls will fail until
pointed at a real ESP32 (or proxied) -- expected during Stage 1/2.

## Build + deploy to the ESP32

```
npm run build
rm -rf ../ESP32/data/* && cp -r dist/* ../ESP32/data/
cd ../ESP32 && pio run -t uploadfs
```

## Layout

- `src/App.tsx` — in-memory tab switcher (no router; see comment in file
  for why) rendering the 5 pages below.
- `src/pages/` — `StatusPage`, `ConfigPage`, `GaugesPage`, `SnifferPage`,
  `PluginsPage` — currently placeholders, see `DOC/architecture.md`.
- `src/api/client.ts` — fetch wrapper for the ESP32's JSON API.
