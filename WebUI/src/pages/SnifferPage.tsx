import { useEffect, useRef, useState } from 'react';
import { subscribeMessages } from '../ws';
import { sendCmd } from '../api/client';
import type { CanFrameMessage } from '../types';

const MAX_ROWS = 300;

export function SnifferPage() {
  const [frames, setFrames] = useState<CanFrameMessage[]>([]);
  const [paused, setPaused] = useState(false);
  const [busFilter, setBusFilter] = useState<string>('any');
  const [idFilter, setIdFilter] = useState('');
  const [showAll, setShowAll] = useState(false);
  const [newFilterBus, setNewFilterBus] = useState('1');
  const [newFilterId, setNewFilterId] = useState('');
  const [cmdStatus, setCmdStatus] = useState<string | null>(null);
  const pausedRef = useRef(paused);
  pausedRef.current = paused;

  useEffect(
    () =>
      subscribeMessages((msg) => {
        if (msg.type !== 'can_frame' || pausedRef.current) {
          return;
        }
        setFrames((prev) => {
          const next = [msg, ...prev];
          return next.length > MAX_ROWS ? next.slice(0, MAX_ROWS) : next;
        });
      }),
    [],
  );

  const idFilterNum = idFilter.trim() === '' ? null : parseInt(idFilter, 16);
  const filtered = frames.filter((f) => {
    if (busFilter !== 'any' && f.bus !== Number(busFilter)) {
      return false;
    }
    if (idFilterNum !== null && !Number.isNaN(idFilterNum) && f.id !== idFilterNum) {
      return false;
    }
    return true;
  });

  const runCmd = (cmd: string, label: string) => {
    setCmdStatus(`${label}...`);
    sendCmd(cmd)
      .then(() => setCmdStatus(`${label}: sent`))
      .catch((err: Error) => setCmdStatus(`${label} failed: ${err.message}`));
  };

  const toggleShowAll = (checked: boolean) => {
    setShowAll(checked);
    runCmd(checked ? 'SET_FILTER_OVERRIDE:none' : 'SET_FILTER_OVERRIDE:clear', checked ? 'Override on' : 'Override off');
  };

  const addFilter = (bus: number, idHex: string) => {
    runCmd(`ADD_FILTER:${bus},0x${idHex}`, `Add filter ${bus},0x${idHex}`);
  };

  return (
    <section>
      <h2>Sniffer</h2>

      <p>
        <label>
          Bus:{' '}
          <select value={busFilter} onChange={(e) => setBusFilter(e.target.value)}>
            <option value="any">any</option>
            <option value="1">1</option>
            <option value="2">2</option>
            <option value="3">3</option>
          </select>
        </label>{' '}
        <label>
          ID (hex):{' '}
          <input
            type="text"
            placeholder="e.g. 7E8"
            value={idFilter}
            onChange={(e) => setIdFilter(e.target.value)}
          />
        </label>{' '}
        <button onClick={() => setPaused((p) => !p)}>{paused ? 'Resume' : 'Pause'}</button>{' '}
        <button onClick={() => setFrames([])}>Clear</button>
      </p>

      <p className="hint">
        These are display-only filters (this browser tab, above) -- they don't touch the Teensy's saved filter list.
        The controls below talk to the Teensy's actual BUS,ID filter (see the Config tab's Filters section).
      </p>
      <p>
        <label>
          <input type="checkbox" checked={showAll} onChange={(e) => toggleShowAll(e.target.checked)} /> Show
          everything (temporary override, doesn't change saved config)
        </label>{' '}
        <button onClick={() => runCmd('RELOAD_FILTERS', 'Reload filters')}>Reload filters from SD</button>
        {cmdStatus && <span className="hint"> {cmdStatus}</span>}
      </p>
      <p>
        <label>
          Add filter -- Bus{' '}
          <input
            type="number"
            value={newFilterBus}
            onChange={(e) => setNewFilterBus(e.target.value)}
            style={{ width: 50 }}
          />
        </label>{' '}
        <label>
          ID (hex, or start-end range){' '}
          <input
            type="text"
            placeholder="e.g. 201 or 7E8-7EF"
            value={newFilterId}
            onChange={(e) => setNewFilterId(e.target.value)}
          />
        </label>{' '}
        <button
          onClick={() => {
            if (!newFilterId.trim()) return;
            runCmd(`ADD_FILTER:${newFilterBus},0x${newFilterId.trim()}`, `Add filter ${newFilterBus},0x${newFilterId.trim()}`);
            setNewFilterId('');
          }}
        >
          Add
        </button>
      </p>

      <table className="sniffer-table">
        <thead>
          <tr>
            <th>Time (ms)</th>
            <th>Bus</th>
            <th>ID</th>
            <th>Ext</th>
            <th>RTR</th>
            <th>DLC</th>
            <th>Data</th>
            <th></th>
          </tr>
        </thead>
        <tbody>
          {filtered.map((f, i) => (
            <tr key={i}>
              <td>{f.ts}</td>
              <td>{f.bus}</td>
              <td>0x{f.id.toString(16).toUpperCase()}</td>
              <td>{f.extended ? 'Y' : ''}</td>
              <td>{f.rtr ? 'Y' : ''}</td>
              <td>{f.dlc}</td>
              <td>{f.data}</td>
              <td>
                <button onClick={() => addFilter(f.bus, f.id.toString(16).toUpperCase())} title="Add this bus/ID to the filter list">
                  +filter
                </button>
              </td>
            </tr>
          ))}
        </tbody>
      </table>
      {filtered.length === 0 && <p>No frames yet -- waiting on /ws.</p>}
    </section>
  );
}
