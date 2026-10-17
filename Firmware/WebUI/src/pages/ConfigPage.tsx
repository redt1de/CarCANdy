import { useEffect, useState } from 'react';
import { getConfig, setConfig } from '../api/client';
import type { CarCandyConfig } from '../types';

type SaveState = 'idle' | 'saving' | 'saved' | 'error';

export function ConfigPage() {
  const [draft, setDraft] = useState<CarCandyConfig | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [saveState, setSaveState] = useState<SaveState>('idle');

  const load = () => {
    setLoadError(null);
    getConfig()
      .then(setDraft)
      .catch((err: Error) => setLoadError(err.message));
  };

  useEffect(load, []);

  if (loadError) {
    return (
      <section>
        <h2>Config</h2>
        <p className="error">{loadError}</p>
        <p>The ESP32 hasn't received a config from the Teensy yet -- check the serial link and retry.</p>
        <button onClick={load}>Retry</button>
      </section>
    );
  }

  if (!draft) {
    return (
      <section>
        <h2>Config</h2>
        <p>Loading...</p>
      </section>
    );
  }

  const updateBus = (index: number, patch: Partial<CarCandyConfig['canBuses'][number]>) => {
    setDraft({
      ...draft,
      canBuses: draft.canBuses.map((b, i) => (i === index ? { ...b, ...patch } : b)),
    });
  };

  const save = () => {
    setSaveState('saving');
    setConfig(draft)
      .then(() => {
        setSaveState('saved');
        // The ESP32 re-syncs with the Teensy shortly after a CONFIG_SET
        // (see DOC/protocol.md) -- reload to reflect what actually stuck.
        setTimeout(load, 800);
      })
      .catch(() => setSaveState('error'));
  };

  return (
    <section>
      <h2>Config</h2>

      <h3>Comm mode</h3>
      <p>
        <select
          value={draft.commMode}
          onChange={(e) => setDraft({ ...draft, commMode: e.target.value as CarCandyConfig['commMode'] })}
        >
          <option value="UART">UART</option>
          <option value="SPI" disabled>
            SPI (paused -- see DOC/open-risks.md)
          </option>
        </select>
      </p>

      <h3>CAN buses</h3>
      <table className="config-table">
        <thead>
          <tr>
            <th>Bus</th>
            <th>Enabled</th>
            <th>Baud</th>
            <th>Mode</th>
          </tr>
        </thead>
        <tbody>
          {draft.canBuses.map((bus, i) => (
            <tr key={bus.id}>
              <td>{bus.id}</td>
              <td>
                <input
                  type="checkbox"
                  checked={bus.enabled}
                  onChange={(e) => updateBus(i, { enabled: e.target.checked })}
                />
              </td>
              <td>
                <input
                  type="number"
                  value={bus.baud}
                  onChange={(e) => updateBus(i, { baud: Number(e.target.value) })}
                />
              </td>
              <td>
                <select
                  value={bus.mode}
                  onChange={(e) => updateBus(i, { mode: e.target.value as 'read_only' | 'transmit' })}
                >
                  <option value="read_only">read_only</option>
                  <option value="transmit">transmit</option>
                </select>
              </td>
            </tr>
          ))}
        </tbody>
      </table>
      <p className="hint">
        Receiving/sniffing/decoding frames works on any enabled bus regardless of mode -- <code>transmit</code> only
        matters if something needs to write to the bus, like the Gauges page's "Poll OBD-II PIDs" feature.
      </p>

      <h3>Logging</h3>
      <p>
        <label>
          <input
            type="checkbox"
            checked={draft.logging.toSerial}
            onChange={(e) => setDraft({ ...draft, logging: { ...draft.logging, toSerial: e.target.checked } })}
          />{' '}
          Serial
        </label>{' '}
        <label>
          <input
            type="checkbox"
            checked={draft.logging.toSD}
            onChange={(e) => setDraft({ ...draft, logging: { ...draft.logging, toSD: e.target.checked } })}
          />{' '}
          SD card
        </label>{' '}
        <label>
          <input
            type="checkbox"
            checked={draft.logging.toComm}
            onChange={(e) => setDraft({ ...draft, logging: { ...draft.logging, toComm: e.target.checked } })}
          />{' '}
          Stream to ESP32
        </label>
      </p>
      <p>
        <label>
          CSV filename prefix:{' '}
          <input
            type="text"
            value={draft.logging.prefix}
            onChange={(e) => setDraft({ ...draft, logging: { ...draft.logging, prefix: e.target.value } })}
          />
        </label>
      </p>
      <p>
        <label>
          <input
            type="checkbox"
            checked={draft.debug}
            onChange={(e) => setDraft({ ...draft, debug: e.target.checked })}
          />{' '}
          Debug logging
        </label>
      </p>
      <p className="hint">
        When enabled, also writes every non-CAN-frame Serial message (e.g. <code>[CommManager]</code>,{' '}
        <code>[FilterManager]</code> diagnostics) to <code>/log/debug_NN.log</code> on SD, sharing the same{' '}
        <code>NN</code> index as that boot's CAN CSV log so the two can be compared directly. Same 10-file rotation
        limit as the CAN logs. Takes effect on the next boot.
      </p>

      <h3>Filters</h3>
      <p>
        <label>
          Mode{' '}
          <select
            value={draft.filters.mode}
            onChange={(e) =>
              setDraft({ ...draft, filters: { ...draft.filters, mode: e.target.value as 'allow' | 'deny' | 'none' } })
            }
          >
            <option value="none">none (no filtering)</option>
            <option value="deny">deny (drop listed IDs, pass everything else)</option>
            <option value="allow">allow (only pass listed IDs)</option>
          </select>
        </label>{' '}
        <label>
          File{' '}
          <input
            type="text"
            value={draft.filters.file}
            onChange={(e) => setDraft({ ...draft, filters: { ...draft.filters, file: e.target.value } })}
          />
        </label>
      </p>
      <p className="hint">
        Entries live in the CSV file on the Teensy's SD card, not here -- add/remove them from the Sniffer tab, or
        edit the file directly and use Sniffer's "Reload filters" button to pick up the change without reflashing.
        Applies before logging, the ESP32 stream, and plugins alike; the Sniffer tab can temporarily override this
        mode for testing without changing what's saved here.
      </p>

      <button onClick={save} disabled={saveState === 'saving'}>
        {saveState === 'saving' ? 'Saving...' : 'Save'}
      </button>
      {saveState === 'saved' && <span className="pill pill-ok">saved, re-syncing...</span>}
      {saveState === 'error' && <span className="pill pill-bad">save failed</span>}
    </section>
  );
}
