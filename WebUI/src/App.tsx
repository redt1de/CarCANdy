import { useState } from 'react';
import './App.css';
import { StatusPage } from './pages/StatusPage';
import { ConfigPage } from './pages/ConfigPage';
import { GaugesPage } from './pages/GaugesPage';
import { SnifferPage } from './pages/SnifferPage';
import { PluginsPage } from './pages/PluginsPage';

// No client-side router: ESPAsyncWebServer serving static LittleFS files
// has no SPA-fallback for deep links, so a hard refresh on e.g. /gauges
// would 404. Tabs are plain in-memory state instead.
const TABS = [
  { key: 'status', label: 'Status', component: StatusPage },
  { key: 'config', label: 'Config', component: ConfigPage },
  { key: 'gauges', label: 'Gauges', component: GaugesPage },
  { key: 'sniffer', label: 'Sniffer', component: SnifferPage },
  { key: 'plugins', label: 'Plugins', component: PluginsPage },
] as const;

type TabKey = (typeof TABS)[number]['key'];

function App() {
  const [activeTab, setActiveTab] = useState<TabKey>('status');
  const ActiveComponent = TABS.find((t) => t.key === activeTab)!.component;

  return (
    <div className="app">
      <header className="app-header">
        <h1>CarCANdy</h1>
      </header>
      <nav className="tabs">
        {TABS.map((tab) => (
          <button
            key={tab.key}
            className={tab.key === activeTab ? 'tab active' : 'tab'}
            onClick={() => setActiveTab(tab.key)}
          >
            {tab.label}
          </button>
        ))}
      </nav>
      <main className="tab-content">
        <ActiveComponent />
      </main>
    </div>
  );
}

export default App;
