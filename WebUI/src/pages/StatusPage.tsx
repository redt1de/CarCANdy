import { useEffect, useState } from 'react';
import { getStatus } from '../api/client';
import { subscribeStatus } from '../ws';
import type { StatusResponse } from '../types';

function formatUptime(ms: number): string {
  const totalSeconds = Math.floor(ms / 1000);
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  return `${hours}h ${minutes}m ${seconds}s`;
}

export function StatusPage() {
  const [status, setStatus] = useState<StatusResponse | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [wsConnected, setWsConnected] = useState(false);

  const refresh = () => {
    getStatus()
      .then((res) => {
        setStatus(res);
        setError(null);
      })
      .catch((err: Error) => setError(err.message));
  };

  useEffect(() => {
    refresh();
    const interval = setInterval(refresh, 3000);
    return () => clearInterval(interval);
  }, []);

  useEffect(() => subscribeStatus(setWsConnected), []);

  return (
    <section>
      <h2>Status</h2>

      <table className="kv-table">
        <tbody>
          <tr>
            <th>Live data stream (/ws)</th>
            <td>
              <span className={wsConnected ? 'pill pill-ok' : 'pill pill-bad'}>
                {wsConnected ? 'connected' : 'disconnected'}
              </span>
            </td>
          </tr>
          <tr>
            <th>Teensy config</th>
            <td>
              {status ? (
                <span className={status.configReady ? 'pill pill-ok' : 'pill pill-bad'}>
                  {status.configReady ? 'ready' : 'not yet fetched'}
                </span>
              ) : (
                '...'
              )}
            </td>
          </tr>
          <tr>
            <th>WiFi AP clients</th>
            <td>{status?.apClients ?? '...'}</td>
          </tr>
          <tr>
            <th>ESP32 uptime</th>
            <td>{status ? formatUptime(status.uptimeMs) : '...'}</td>
          </tr>
        </tbody>
      </table>

      {error && <p className="error">/api/status error: {error}</p>}

      <button onClick={refresh}>Refresh</button>
    </section>
  );
}
