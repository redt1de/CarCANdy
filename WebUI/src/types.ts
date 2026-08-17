// Shapes mirrored from the ESP32/Teensy side. Keep in sync with
// DOC/config.example.json (config) and DOC/protocol.md (WS messages).

export interface CanBusConfig {
  id: number;
  enabled: boolean;
  baud: number;
  mode: 'read_only' | 'transmit';
}

export interface PluginConfig {
  enabled: boolean;
  config: Record<string, unknown>;
}

export interface CarCandyConfig {
  commMode: 'UART' | 'SPI';
  canBuses: CanBusConfig[];
  logging: {
    toSerial: boolean;
    toSD: boolean;
    toComm: boolean;
    prefix: string;
  };
  filters: {
    mode: 'allow' | 'deny' | 'none';
    file: string;
  };
  // When true, the Teensy also mirrors all non-CAN-frame Serial output to
  // /debug_NN.log on SD, using the same NN as that boot's CAN CSV log so
  // the two can be compared directly. Off by default -- extra SD writes.
  debug: boolean;
  plugins: Record<string, PluginConfig>;
}

export interface StatusResponse {
  apClients: number;
  configReady: boolean;
  uptimeMs: number;
}

// Messages broadcast on the /ws WebSocket (see ESP32/src/WebServerManager.cpp).
export interface CanFrameMessage {
  type: 'can_frame';
  bus: number;
  id: number;
  extended: boolean;
  rtr: boolean;
  dlc: number;
  data: string; // hex, e.g. "0102AABB"
  ts: number;
}

export interface LogMessage {
  type: 'log';
  msg: string;
}

export type WsMessage = CanFrameMessage | LogMessage;

// A frame to request the Teensy transmit, POSTed to /api/can/send.
export interface CanFrameRequest {
  bus: number;
  id: number;
  dlc: number;
  data: number[];
  extended?: boolean;
  rtr?: boolean;
}

// A user (or default OBD-II)-defined mapping from raw CAN_FRAME bytes to a
// decoded gauge value. See src/obd2.ts for the standard-PID defaults and
// src/pages/GaugesPage.tsx for how these get edited/persisted/decoded.
export interface SignalDefinition {
  id: string;
  name: string;
  unit: string;
  bus: number;
  canId: number;
  // Extra byte-value conditions besides canId, e.g. OBD-II's
  // [{index:1,value:0x41},{index:2,value:PID}] to disambiguate which PID a
  // 0x7E8 response carries.
  matches: { index: number; value: number }[];
  byteOffset: number;
  length: 1 | 2;
  scale: number;
  offset: number;
  // If set, this signal is an OBD-II PID the Gauges page can actively poll
  // for by sending a 0x7DF request frame with this PID.
  requestPid?: number;
}
