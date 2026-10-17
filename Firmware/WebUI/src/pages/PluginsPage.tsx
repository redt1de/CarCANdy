import { useEffect, useState } from 'react';
import { getConfig, setConfig } from '../api/client';
import type { CarCandyConfig } from '../types';

type SaveState = 'idle' | 'saving' | 'saved' | 'error';

// Generic over whatever plugins exist in the config -- new plugins
// registered on the Teensy (see Teensy/src/PluginManager.cpp) show up here
// automatically with a checkbox per config field, no frontend changes
// needed, as long as their config values are numbers/strings/booleans.
export function PluginsPage() {
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
        <h2>Plugins</h2>
        <p className="error">{loadError}</p>
        <button onClick={load}>Retry</button>
      </section>
    );
  }

  if (!draft) {
    return (
      <section>
        <h2>Plugins</h2>
        <p>Loading...</p>
      </section>
    );
  }

  const setPluginEnabled = (name: string, enabled: boolean) => {
    setDraft({
      ...draft,
      plugins: { ...draft.plugins, [name]: { ...draft.plugins[name], enabled } },
    });
  };

  const setPluginConfigField = (name: string, field: string, value: unknown) => {
    setDraft({
      ...draft,
      plugins: {
        ...draft.plugins,
        [name]: { ...draft.plugins[name], config: { ...draft.plugins[name].config, [field]: value } },
      },
    });
  };

  const save = () => {
    setSaveState('saving');
    setConfig(draft)
      .then(() => {
        setSaveState('saved');
        setTimeout(load, 800);
      })
      .catch(() => setSaveState('error'));
  };

  const pluginNames = Object.keys(draft.plugins);

  return (
    <section>
      <h2>Plugins</h2>

      {pluginNames.length === 0 && <p>No plugins found in the Teensy's config.</p>}

      {pluginNames.map((name) => {
        const plugin = draft.plugins[name];
        return (
          <fieldset key={name} className="plugin-card">
            <legend>
              <label>
                <input type="checkbox" checked={plugin.enabled} onChange={(e) => setPluginEnabled(name, e.target.checked)} />{' '}
                {name}
              </label>
            </legend>
            {Object.entries(plugin.config).map(([field, value]) => (
              <p key={field}>
                <label>
                  {field}:{' '}
                  {typeof value === 'boolean' ? (
                    <input
                      type="checkbox"
                      checked={value}
                      onChange={(e) => setPluginConfigField(name, field, e.target.checked)}
                    />
                  ) : typeof value === 'number' ? (
                    <input
                      type="number"
                      value={value}
                      onChange={(e) => setPluginConfigField(name, field, Number(e.target.value))}
                    />
                  ) : (
                    <input
                      type="text"
                      value={String(value)}
                      onChange={(e) => setPluginConfigField(name, field, e.target.value)}
                    />
                  )}
                </label>
              </p>
            ))}
          </fieldset>
        );
      })}

      <button onClick={save} disabled={saveState === 'saving'}>
        {saveState === 'saving' ? 'Saving...' : 'Save'}
      </button>
      {saveState === 'saved' && <span className="pill pill-ok">saved, re-syncing...</span>}
      {saveState === 'error' && <span className="pill pill-bad">save failed</span>}
    </section>
  );
}
