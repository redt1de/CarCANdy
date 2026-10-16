import { useEffect, useRef, useState } from 'react';
import { subscribeMessages, subscribeStatus } from '../ws';
import type { LogMessage } from '../types';

const MAX_LINES = 500;

interface LogEntry {
  msg: string;
  key: number;
}

// Mirrors whatever the Teensy's Logging class is printing (see
// DOC/protocol.md's LOG_LINE section) -- the same lines that go to Serial
// and, if enabled, /debug_NN.log on SD, streamed live over WiFi instead.
// Works even with SD debug logging turned off (Config tab), since this
// stream doesn't depend on that setting.
export function LoggingPage() {
  const [lines, setLines] = useState<LogEntry[]>([]);
  const [paused, setPaused] = useState(false);
  const [wsConnected, setWsConnected] = useState(false);
  const pausedRef = useRef(paused);
  pausedRef.current = paused;
  const nextKeyRef = useRef(0);

  useEffect(() => subscribeStatus(setWsConnected), []);

  useEffect(
    () =>
      subscribeMessages((msg) => {
        if (msg.type !== 'log' || pausedRef.current) {
          return;
        }
        const entry: LogEntry = { msg: (msg as LogMessage).msg, key: nextKeyRef.current++ };
        setLines((prev) => {
          const next = [entry, ...prev];
          return next.length > MAX_LINES ? next.slice(0, MAX_LINES) : next;
        });
      }),
    [],
  );

  return (
    <section>
      <h2>Logging</h2>

      <p>
        <span className={wsConnected ? 'pill pill-ok' : 'pill pill-bad'}>
          {wsConnected ? 'connected' : 'disconnected'}
        </span>{' '}
        <button onClick={() => setPaused((p) => !p)}>{paused ? 'Resume' : 'Pause'}</button>{' '}
        <button onClick={() => setLines([])}>Clear</button>
      </p>

      <p className="hint">
        Live mirror of the Teensy's non-CAN-frame diagnostic output (e.g. <code>[CommManager]</code>,{' '}
        <code>[FilterManager]</code> lines) -- newest first, capped at the last {MAX_LINES} lines in this browser tab.
        This is independent of the Config tab's "Debug logging" SD option -- that controls whether the same output
        also gets written to <code>/debug_NN.log</code> on the card; this view works either way.
      </p>

      <div className="log-view">
        {lines.length === 0 ? (
          <p>No log lines yet -- waiting on /ws.</p>
        ) : (
          lines.map((l) => <div key={l.key}>{l.msg}</div>)
        )}
      </div>
    </section>
  );
}
