// Standard OBD-II (SAE J1979, ISO 15765-4) Mode 01 "show current data"
// helpers. Unlike most CAN signals, these ARE publicly standardized --
// unlike the vehicle's proprietary HS-CAN broadcast IDs, which need to be
// discovered via the Sniffer and added as custom SignalDefinitions.
//
// OBD-II is request/response, not a passive broadcast: a scan tool (or, in
// our case, the Gauges page) sends a request frame on ID 0x7DF asking for
// a specific PID, and the ECU replies on 0x7E8 with that PID's value a
// short time later. See buildObd2RequestFrame()/GaugesPage's polling loop.
import type { CanFrameMessage, CanFrameRequest, SignalDefinition } from './types';

export const OBD2_REQUEST_ID = 0x7df;
export const OBD2_RESPONSE_ID = 0x7e8;

interface PidDef {
  pid: number;
  name: string;
  unit: string;
  length: 1 | 2;
  scale: number;
  offset: number;
}

// A single-frame OBD-II response is [PCI, 0x41, PID, A, (B), ...padding],
// so the value bytes always start at index 3.
const RESPONSE_BYTE_OFFSET = 3;

const STANDARD_PIDS: PidDef[] = [
  { pid: 0x0c, name: 'Engine RPM', unit: 'rpm', length: 2, scale: 0.25, offset: 0 },
  { pid: 0x0d, name: 'Vehicle Speed', unit: 'km/h', length: 1, scale: 1, offset: 0 },
  { pid: 0x05, name: 'Coolant Temp', unit: '°C', length: 1, scale: 1, offset: -40 },
  { pid: 0x0f, name: 'Intake Air Temp', unit: '°C', length: 1, scale: 1, offset: -40 },
  { pid: 0x11, name: 'Throttle Position', unit: '%', length: 1, scale: 100 / 255, offset: 0 },
  { pid: 0x04, name: 'Engine Load', unit: '%', length: 1, scale: 100 / 255, offset: 0 },
  { pid: 0x0b, name: 'Intake Manifold Pressure', unit: 'kPa', length: 1, scale: 1, offset: 0 },
  { pid: 0x10, name: 'MAF Air Flow', unit: 'g/s', length: 2, scale: 0.01, offset: 0 },
];

// Default signal definitions for the bus configured for standard OBD-II
// (per the project's config -- bus index passed in, typically 1).
export function defaultObd2Signals(bus: number): SignalDefinition[] {
  return STANDARD_PIDS.map((p) => ({
    id: `obd2-${p.pid.toString(16)}`,
    name: p.name,
    unit: p.unit,
    bus,
    canId: OBD2_RESPONSE_ID,
    matches: [
      { index: 1, value: 0x41 },
      { index: 2, value: p.pid },
    ],
    byteOffset: RESPONSE_BYTE_OFFSET,
    length: p.length,
    scale: p.scale,
    offset: p.offset,
    requestPid: p.pid,
  }));
}

// [0x02, 0x01, pid, 0x00 x5] -- "2 bytes follow: mode 1, this PID".
export function buildObd2RequestFrame(bus: number, pid: number): CanFrameRequest {
  return {
    bus,
    id: OBD2_REQUEST_ID,
    dlc: 8,
    extended: false,
    rtr: false,
    data: [0x02, 0x01, pid, 0x00, 0x00, 0x00, 0x00, 0x00],
  };
}

function hexToBytes(hex: string): number[] {
  const bytes: number[] = [];
  for (let i = 0; i + 1 < hex.length; i += 2) {
    bytes.push(parseInt(hex.substring(i, i + 2), 16));
  }
  return bytes;
}

// Returns the decoded value for this signal from a live frame, or null if
// the frame doesn't match (wrong bus/ID, or a match condition fails).
export function decodeSignal(def: SignalDefinition, frame: CanFrameMessage): number | null {
  if (frame.bus !== def.bus || frame.id !== def.canId) {
    return null;
  }
  const bytes = hexToBytes(frame.data);
  for (const m of def.matches) {
    if (bytes[m.index] !== m.value) {
      return null;
    }
  }
  if (def.byteOffset + def.length > bytes.length) {
    return null;
  }
  let raw = 0;
  for (let i = 0; i < def.length; i++) {
    raw = (raw << 8) | bytes[def.byteOffset + i];
  }
  return raw * def.scale + def.offset;
}
