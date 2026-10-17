import { useCallback, useContext, useEffect, useRef, useState } from "react";
import {
  ZMKAppContext,
  useCustomSubsystem,
} from "@cormoran/zmk-studio-react-hook";
import { decodeResponse, encodeRequest } from "./tps43Codec";
import type { Tps43Request, Tps43Response, Tps43State } from "./tps43Types";

export type { Tps43State };

/**
 * Custom Studio RPC subsystem identifier registered by the tps43 firmware
 * module. Must match the identifier declared on the device (see the module's
 * `src/studio/tps43_handler.c`).
 */
export const SUBSYSTEM_IDENTIFIER = "tokyo2006__tps43";

export interface UseTps43StateReturn {
  /** Current device state, or `null` before the first successful fetch. */
  state: Tps43State | null;
  /**
   * Optimistically update a single field and push a SetStateRequest to the
   * device. On failure the previous state is restored.
   */
  setField: <K extends keyof Tps43State>(
    field: K,
    value: Tps43State[K],
    persist?: boolean
  ) => Promise<void>;
  /** Reset the device state (optionally to factory defaults). */
  reset: (factoryDefaults?: boolean) => Promise<void>;
  /** Re-fetch the current state from the device. */
  refresh: () => Promise<void>;
  isLoading: boolean;
  error: string | null;
  isConnected: boolean;
}

export function useTps43State(): UseTps43StateReturn {
  const zmkApp = useContext(ZMKAppContext);
  const isConnected = zmkApp?.isConnected ?? false;

  const { ready, call } = useCustomSubsystem<Tps43Request, Tps43Response>(
    SUBSYSTEM_IDENTIFIER,
    { encode: encodeRequest, decode: decodeResponse }
  );

  const [state, setState] = useState<Tps43State | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  const handleResponse = useCallback((resp: Tps43Response | null) => {
    if (!resp) return;
    if ("error" in resp) {
      setError(resp.error.message);
      return;
    }
    setError(null);
    if ("getState" in resp) {
      setState(resp.getState.state);
    } else if ("setState" in resp) {
      setState(resp.setState.state);
    } else if ("reset" in resp) {
      setState(resp.reset.state);
    }
  }, []);

  const refresh = useCallback(async () => {
    if (!ready) return;
    setIsLoading(true);
    try {
      handleResponse(await call({ getState: {} }));
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    } finally {
      setIsLoading(false);
    }
  }, [call, ready, handleResponse]);

  const setField = useCallback(
    async <K extends keyof Tps43State>(
      field: K,
      value: Tps43State[K],
      persist = true
    ) => {
      if (!state) return;
      const next: Tps43State = { ...state, [field]: value };
      setState(next);
      try {
        handleResponse(await call({ setState: { state: next, persist } }));
      } catch (e) {
        setState(state);
        setError(e instanceof Error ? e.message : String(e));
      }
    },
    [call, state, handleResponse]
  );

  const reset = useCallback(
    async (factoryDefaults = false) => {
      try {
        handleResponse(await call({ reset: { factoryDefaults } }));
      } catch (e) {
        setError(e instanceof Error ? e.message : String(e));
      }
    },
    [call, handleResponse]
  );

  // Auto-load the current state as soon as the subsystem becomes ready.
  const didInitialLoad = useRef(false);
  useEffect(() => {
    if (!ready || didInitialLoad.current) return;
    didInitialLoad.current = true;
    void refresh();
  }, [ready, refresh]);

  return { state, setField, reset, refresh, isLoading, error, isConnected };
}
