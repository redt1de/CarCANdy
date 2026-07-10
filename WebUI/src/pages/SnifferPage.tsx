import { useEffect, useRef, useState } from 'react';
import { subscribeMessages } from '../ws';
import type { CanFrameMessage } from '../types';

const MAX_ROWS = 300;

export function SnifferPage() {
  const [frames, setFrames] = useState<CanFrameMessage[]>([]);
  const [paused, setPaused] = useState(false);
  const [busFilter, setBusFilter] = useState<string>('any');
  const [idFilter, setIdFilter] = useState('');
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

  return (
    <section>
      <h2>Sniffer</h2>

      <p>
        <label>
          Bus:{' '}
          <select value={busFilter} onChange={(e) => setBusFilter(e.target.value)}>
            <option value="any">any</option>
            <option value="0">0</option>
            <option value="1">1</option>
            <option value="2">2</option>
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
            </tr>
          ))}
        </tbody>
      </table>
      {filtered.length === 0 && <p>No frames yet -- waiting on /ws.</p>}
    </section>
  );
}
