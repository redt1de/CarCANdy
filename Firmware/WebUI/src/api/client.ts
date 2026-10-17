// Thin fetch wrapper for talking to the ESP32's JSON API (see
// ESP32/src/WebServerManager.cpp / DOC/protocol.md for the routes).
import type { CanFrameRequest, CarCandyConfig, StatusResponse } from '../types';

export async function getStatus(): Promise<StatusResponse> {
  const res = await fetch('/api/status');
  if (!res.ok) {
    throw new Error(`GET /api/status failed: ${res.status}`);
  }
  return res.json();
}

// Throws if the Teensy's config hasn't been fetched by the ESP32 yet
// (503 from the backend) -- callers should show a "waiting for Teensy"
// state rather than treating this as a hard error.
export async function getConfig(): Promise<CarCandyConfig> {
  const res = await fetch('/api/config');
  if (!res.ok) {
    throw new Error(`GET /api/config failed: ${res.status}`);
  }
  return res.json();
}

// Fire-and-forget from the ESP32's perspective (see DOC/protocol.md) --
// a 2xx here means the packet was queued for the Teensy, not that the
// Teensy accepted it. Callers should re-fetch getConfig() shortly after
// to confirm.
export async function setConfig(config: CarCandyConfig): Promise<void> {
  const res = await fetch('/api/config', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(config),
  });
  if (!res.ok) {
    throw new Error(`POST /api/config failed: ${res.status}`);
  }
}

// Also fire-and-forget -- see setConfig() above.
export async function sendCanFrame(frame: CanFrameRequest): Promise<void> {
  const res = await fetch('/api/can/send', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(frame),
  });
  if (!res.ok) {
    throw new Error(`POST /api/can/send failed: ${res.status}`);
  }
}

// Sends a CMD packet to the Teensy (e.g. "RELOAD_FILTERS",
// "SET_FILTER_OVERRIDE:none", "ADD_FILTER:1,0x201") -- see DOC/protocol.md
// for the grammar. Fire-and-forget, same as setConfig()/sendCanFrame().
export async function sendCmd(cmd: string): Promise<void> {
  const res = await fetch('/api/cmd', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ cmd }),
  });
  if (!res.ok) {
    throw new Error(`POST /api/cmd failed: ${res.status}`);
  }
}
