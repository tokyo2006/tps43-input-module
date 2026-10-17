import "./App.css";
import { connect as gattConnect } from "@zmkfirmware/zmk-studio-ts-client/transport/gatt";
import {
  ZMKConnection,
  isWebSerialSupported,
  isWebBluetoothSupported,
  connectSerial,
} from "@cormoran/zmk-studio-react-hook";
import { useTps43State } from "./hooks/useTps43State";
import { SectionCard } from "./components/SectionCard";
import { Slider } from "./components/Slider";
import { Switch } from "./components/Switch";
import { NumberStepper } from "./components/NumberStepper";

export const GITHUB_REPO = "tokyo2006/tps43-input-module";

export const TEMPLATE_CREDIT_REPO = "cormoran/zmk-module-template"; // zmk-module-template:keep

function App() {
  return (
    <div className="app">
      <header className="app-header">
        <h1>TPS43 Trackpad — DYA Studio</h1>
        <p>Custom Studio RPC Demo</p>
      </header>

      <ZMKConnection
        autoReconnect
        renderDisconnected={({ connect, isLoading, error }) => (
          <section className="card">
            <h2>Device Connection</h2>
            {isLoading && <p>⏳ Connecting...</p>}
            {error && (
              <div className="error-message">
                <p>🚨 {error}</p>
              </div>
            )}
            {!isLoading && (
              <>
                <div className="connect-buttons">
                  {isWebSerialSupported() && (
                    <button
                      className="btn btn-primary"
                      onClick={() => connect(connectSerial)}
                    >
                      🔌 Connect USB
                    </button>
                  )}
                  {isWebBluetoothSupported() && (
                    <button
                      className="btn btn-primary"
                      onClick={() => connect(gattConnect)}
                    >
                      📶 Connect Bluetooth
                    </button>
                  )}
                  {!isWebSerialSupported() && !isWebBluetoothSupported() && (
                    <div className="warning-message">
                      <p>
                        ⚠️ Web Serial and Web Bluetooth are unavailable here.
                        Use a Chromium-based browser (Chrome, Edge, ...) over
                        HTTPS or localhost to connect to your keyboard.
                      </p>
                    </div>
                  )}
                </div>
                {isWebBluetoothSupported() && (
                  <p className="hint-message">
                    📶 Not showing up? Some firmware only advertises the Studio
                    Bluetooth service once unlocked — press the unlock key (
                    <code>&amp;studio_unlock</code> behavior) on your keyboard,
                    then try connecting again.
                  </p>
                )}
              </>
            )}
          </section>
        )}
        renderConnected={({ disconnect, deviceName }) => (
          <>
            <section className="card">
              <h2>Device Connection</h2>
              <div className="device-info">
                <h3>✅ Connected to: {deviceName}</h3>
              </div>
              <button className="btn btn-secondary" onClick={disconnect}>
                Disconnect
              </button>
            </section>

            <StudioSection />
          </>
        )}
      />

      <footer className="app-footer">
        <p>
          <strong>TPS43 Trackpad — DYA Studio</strong> — DYA Studio tab for the
          tps43-input-module firmware
        </p>
        <p>
          <a
            href={`https://github.com/${GITHUB_REPO}`}
            target="_blank"
            rel="noreferrer"
          >
            {GITHUB_REPO}
          </a>
        </p>
        <p className="template-credit">
          Built from{" "}
          <a
            href={`https://github.com/${TEMPLATE_CREDIT_REPO}`}
            target="_blank"
            rel="noreferrer"
          >
            {TEMPLATE_CREDIT_REPO}
          </a>{" "}
          - AI ready ZMK module template by{" "}
          <a
            href="https://github.com/cormoran"
            target="_blank"
            rel="noreferrer"
          >
            @cormoran
          </a>
        </p>
      </footer>
    </div>
  );
}

/**
 * The settings editor. It is the SINGLE owner of `useTps43State()` state: it
 * calls the hook once and prop-drills `value`/`onChange` down to the leaf
 * components, so every control reads/writes the same device state instead of
 * each firing its own `getState`.
 */
export function StudioSection() {
  const { state, isConnected, isLoading, error, setField, reset, refresh } =
    useTps43State();

  if (!isConnected) {
    return (
      <div className="connect-prompt">
        Connect via DYA Studio to edit settings.
      </div>
    );
  }

  if (!state) {
    return (
      <div className="tps43-studio">
        <div className="loading">
          {isLoading ? "Loading…" : "No state received"}
        </div>
        {error && (
          <div className="error" role="alert">
            {error}
          </div>
        )}
      </div>
    );
  }

  const set = setField;

  return (
    <div className="tps43-studio">
      <div className="studio-header">
        <button className="btn btn-secondary" onClick={() => void reset(true)}>
          Reset to defaults
        </button>
        <button className="btn btn-secondary" onClick={() => void refresh()}>
          Refresh
        </button>
      </div>

      {error && (
        <div className="error" role="alert">
          {error}
        </div>
      )}

      <SectionCard title="Gestures">
        <Switch
          label="One-finger tap (left click)"
          value={state.oneFingerTap}
          onChange={(v) => void set("oneFingerTap", v, true)}
        />
        <Switch
          label="Two-finger tap (right click)"
          value={state.twoFingerTap}
          onChange={(v) => void set("twoFingerTap", v, true)}
        />
        <Switch
          label="Press and hold"
          value={state.pressAndHold}
          onChange={(v) => void set("pressAndHold", v, true)}
        />
        <Slider
          label="Press and hold time"
          value={state.pressAndHoldTime}
          min={100}
          max={2000}
          step={50}
          unit=" ms"
          onChange={(v) => void set("pressAndHoldTime", v, true)}
        />
      </SectionCard>

      <SectionCard title="Scrolling">
        <Switch
          label="Enable scroll"
          value={state.scroll}
          onChange={(v) => void set("scroll", v, true)}
        />
        <Switch
          label="Natural scroll X"
          value={state.naturalScrollX}
          onChange={(v) => void set("naturalScrollX", v, true)}
        />
        <Switch
          label="Natural scroll Y"
          value={state.naturalScrollY}
          onChange={(v) => void set("naturalScrollY", v, true)}
        />
      </SectionCard>

      <SectionCard title="Axes">
        <Switch
          label="Switch X/Y"
          value={state.switchXy}
          onChange={(v) => void set("switchXy", v, true)}
        />
        <Switch
          label="Flip X"
          value={state.flipX}
          onChange={(v) => void set("flipX", v, true)}
        />
        <Switch
          label="Flip Y"
          value={state.flipY}
          onChange={(v) => void set("flipY", v, true)}
        />
      </SectionCard>

      <SectionCard title="Sensitivity & Filtering">
        <Slider
          label="Bottom beta (speed filtering)"
          value={state.bottomBeta}
          min={0}
          max={255}
          step={1}
          onChange={(v) => void set("bottomBeta", v, true)}
        />
        <Slider
          label="Stationary threshold"
          value={state.stationaryThreshold}
          min={0}
          max={255}
          step={1}
          onChange={(v) => void set("stationaryThreshold", v, true)}
        />
      </SectionCard>

      <SectionCard title="Stationary threshold (stepper)" defaultOpen={false}>
        <NumberStepper
          label="Stationary threshold"
          value={state.stationaryThreshold}
          values={[0, 2, 3, 4, 5, 6, 8, 10, 15, 20]}
          onChange={(v) => void set("stationaryThreshold", v, true)}
        />
      </SectionCard>
    </div>
  );
}

export default App;
