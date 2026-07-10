import { useEffect, useRef, useState } from 'react';
import { subscribeMessages } from '../ws';
import { getConfig, sendCanFrame } from '../api/client';
import { buildObd2RequestFrame, decodeSignal, defaultObd2Signals } from '../obd2';
import type { CanFrameMessage, SignalDefinition } from '../types';

const STORAGE_KEY = 'carcandy.signals';
// Standard OBD-II lives on bus 1 per this project's convention (see
// DOC/config.example.json) -- change here if your wiring differs.
const OBD2_BUS = 1;
const POLL_INTERVAL_MS = 400;

function loadSignals(): SignalDefinition[] {
  const raw = localStorage.getItem(STORAGE_KEY);
  if (!raw) {
    return defaultObd2Signals(OBD2_BUS);
  }
  try {
    return JSON.parse(raw) as SignalDefinition[];
  } catch {
    return defaultObd2Signals(OBD2_BUS);
  }
}

interface Reading {
  value: number;
  ts: number;
}

const emptyDraft = (): Omit<SignalDefinition, 'id'> => ({
  name: '',
  unit: '',
  bus: 0,
  canId: 0,
  matches: [],
  byteOffset: 0,
  length: 1,
  scale: 1,
  offset: 0,
});

export function GaugesPage() {
  const [signals, setSignals] = useState<SignalDefinition[]>(loadSignals);
  const [readings, setReadings] = useState<Record<string, Reading>>({});
  const [polling, setPolling] = useState(false);
  const [busWarning, setBusWarning] = useState<string | null>(null);
  const [newSignal, setNewSignal] = useState(emptyDraft());
  const signalsRef = useRef(signals);
  signalsRef.current = signals;

  useEffect(() => {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(signals));
  }, [signals]);

  useEffect(
    () =>
      subscribeMessages((msg) => {
        if (msg.type !== 'can_frame') {
          return;
        }
        const frame = msg as CanFrameMessage;
        const updates: Record<string, Reading> = {};
        for (const def of signalsRef.current) {
          const value = decodeSignal(def, frame);
          if (value !== null) {
            updates[def.id] = { value, ts: frame.ts };
          }
        }
        if (Object.keys(updates).length > 0) {
          setReadings((prev) => ({ ...prev, ...updates }));
        }
      }),
    [],
  );

  // Only relevant to the "Poll OBD-II PIDs" checkbox below -- receiving and
  // decoding frames (for these gauges or any custom signal) never needs
  // transmit mode, since that's purely passive. Polling is the one thing
  // here that writes to the bus (a 0x7DF request frame), and
  // CANManager::sendFrame() on the Teensy will silently reject (NACK,
  // logged only to its debug Serial) requests on a read_only/disabled bus.
  useEffect(() => {
    if (!polling) {
      setBusWarning(null);
      return;
    }
    getConfig()
      .then((config) => {
        const bus = config.canBuses.find((b) => b.id === OBD2_BUS);
        if (!bus || !bus.enabled || bus.mode !== 'transmit') {
          setBusWarning(
            `Bus ${OBD2_BUS} isn't enabled + set to "transmit" in Config -- OBD-II poll requests will be rejected by the Teensy until it is.`,
          );
        } else {
          setBusWarning(null);
        }
      })
      .catch(() => {
        // Config not ready yet -- the Config page surfaces that more prominently, skip the warning here.
      });
  }, [polling]);

  useEffect(() => {
    if (!polling) {
      return;
    }
    const pollable = signals.filter((s) => s.requestPid !== undefined);
    if (pollable.length === 0) {
      return;
    }
    let i = 0;
    const interval = setInterval(() => {
      const def = pollable[i % pollable.length];
      i++;
      sendCanFrame(buildObd2RequestFrame(def.bus, def.requestPid!)).catch(() => {
        // Fire-and-forget; a failure here just means one poll cycle was missed.
      });
    }, POLL_INTERVAL_MS);
    return () => clearInterval(interval);
  }, [polling, signals]);

  const addSignal = () => {
    if (!newSignal.name.trim()) {
      return;
    }
    setSignals([...signals, { ...newSignal, id: `custom-${Date.now()}` }]);
    setNewSignal(emptyDraft());
  };

  const removeSignal = (id: string) => {
    setSignals(signals.filter((s) => s.id !== id));
    setReadings((prev) => {
      const next = { ...prev };
      delete next[id];
      return next;
    });
  };

  return (
    <section>
      <h2>Gauges</h2>

      <p>
        <label>
          <input type="checkbox" checked={polling} onChange={(e) => setPolling(e.target.checked)} /> Poll OBD-II PIDs
          on bus {OBD2_BUS}
        </label>
      </p>
      {busWarning && <p className="error">{busWarning}</p>}

      <div className="gauge-grid">
        {signals.map((def) => {
          const reading = readings[def.id];
          const stale = reading && Date.now() - reading.ts > 5000;
          return (
            <div key={def.id} className="gauge-tile">
              <div className="gauge-name">{def.name}</div>
              <div className={stale ? 'gauge-value gauge-stale' : 'gauge-value'}>
                {reading ? reading.value.toFixed(2) : '--'} <span className="gauge-unit">{def.unit}</span>
              </div>
              <button className="gauge-remove" onClick={() => removeSignal(def.id)} title="Remove signal">
                x
              </button>
            </div>
          );
        })}
        {signals.length === 0 && <p>No signals defined yet -- add one below.</p>}
      </div>

      <h3>Add signal</h3>
      <p className="hint">
        For a raw broadcast signal you found with the Sniffer: bus + CAN ID (hex) + byte offset/length decode it
        directly. Multi-byte match conditions (like OBD-II's mode+PID disambiguation) aren't editable here yet --
        edit localStorage directly for that.
      </p>
      <p>
        <label>
          Name <input type="text" value={newSignal.name} onChange={(e) => setNewSignal({ ...newSignal, name: e.target.value })} />
        </label>{' '}
        <label>
          Unit <input type="text" value={newSignal.unit} onChange={(e) => setNewSignal({ ...newSignal, unit: e.target.value })} />
        </label>{' '}
        <label>
          Bus{' '}
          <input
            type="number"
            value={newSignal.bus}
            onChange={(e) => setNewSignal({ ...newSignal, bus: Number(e.target.value) })}
          />
        </label>
      </p>
      <p>
        <label>
          CAN ID (hex){' '}
          <input
            type="text"
            placeholder="e.g. 201"
            onChange={(e) => setNewSignal({ ...newSignal, canId: parseInt(e.target.value, 16) || 0 })}
          />
        </label>{' '}
        <label>
          Byte offset{' '}
          <input
            type="number"
            value={newSignal.byteOffset}
            onChange={(e) => setNewSignal({ ...newSignal, byteOffset: Number(e.target.value) })}
          />
        </label>{' '}
        <label>
          Length{' '}
          <select
            value={newSignal.length}
            onChange={(e) => setNewSignal({ ...newSignal, length: Number(e.target.value) as 1 | 2 })}
          >
            <option value={1}>1</option>
            <option value={2}>2</option>
          </select>
        </label>
      </p>
      <p>
        <label>
          Scale{' '}
          <input
            type="number"
            step="any"
            value={newSignal.scale}
            onChange={(e) => setNewSignal({ ...newSignal, scale: Number(e.target.value) })}
          />
        </label>{' '}
        <label>
          Offset{' '}
          <input
            type="number"
            step="any"
            value={newSignal.offset}
            onChange={(e) => setNewSignal({ ...newSignal, offset: Number(e.target.value) })}
          />
        </label>{' '}
        <button onClick={addSignal}>Add</button>
      </p>
    </section>
  );
}
