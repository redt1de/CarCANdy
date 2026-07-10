// Single shared WebSocket connection to the ESP32's /ws endpoint (see
// ESP32/src/WebServerManager.cpp), so the Gauges and Sniffer pages don't
// each open their own socket. Auto-reconnects on close/error.
import type { WsMessage } from './types';

type MessageHandler = (msg: WsMessage) => void;
type StatusHandler = (connected: boolean) => void;

const messageHandlers = new Set<MessageHandler>();
const statusHandlers = new Set<StatusHandler>();

let socket: WebSocket | null = null;
let reconnectTimer: number | undefined;
let connected = false;

function setConnected(value: boolean) {
  connected = value;
  statusHandlers.forEach((h) => h(value));
}

function connect() {
  reconnectTimer = undefined;
  const url = `ws://${window.location.hostname}/ws`;
  const ws = new WebSocket(url);

  ws.onopen = () => setConnected(true);

  ws.onmessage = (event) => {
    try {
      const msg = JSON.parse(event.data) as WsMessage;
      messageHandlers.forEach((h) => h(msg));
    } catch {
      // Ignore malformed frames rather than crashing the page.
    }
  };

  ws.onclose = () => {
    setConnected(false);
    socket = null;
    reconnectTimer = window.setTimeout(connect, 2000);
  };

  ws.onerror = () => {
    ws.close();
  };

  socket = ws;
}

function ensureConnected() {
  if (!socket && reconnectTimer === undefined) {
    connect();
  }
}

export function subscribeMessages(handler: MessageHandler): () => void {
  ensureConnected();
  messageHandlers.add(handler);
  return () => messageHandlers.delete(handler);
}

export function subscribeStatus(handler: StatusHandler): () => void {
  ensureConnected();
  handler(connected);
  statusHandlers.add(handler);
  return () => statusHandlers.delete(handler);
}

export function isConnected(): boolean {
  return connected;
}
