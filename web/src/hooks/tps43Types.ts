/**
 * Inline TypeScript types mirroring `proto/tokyo2006/tps43/tps43.proto`.
 *
 * These are hand-maintained (not generated) because the `buf generate` /
 * ts-proto codegen path is not wired up in this scaffold (same reason as the
 * cirque-input-module). Field names follow ts-proto's snake_case -> camelCase
 * convention so the hook surface matches what generated code would produce.
 */

export interface Tps43State {
  oneFingerTap: boolean;
  pressAndHold: boolean;
  twoFingerTap: boolean;
  pressAndHoldTime: number;

  scroll: boolean;
  naturalScrollX: boolean;
  naturalScrollY: boolean;

  switchXy: boolean;
  flipX: boolean;
  flipY: boolean;

  bottomBeta: number;
  stationaryThreshold: number;
}

// eslint-disable-next-line @typescript-eslint/no-empty-object-type -- empty proto message
export interface GetStateRequest {}

export interface GetStateResponse {
  state: Tps43State;
}

export interface SetStateRequest {
  state: Tps43State;
  persist: boolean;
}

export interface SetStateResponse {
  state: Tps43State;
  persisted: boolean;
}

export interface ResetRequest {
  factoryDefaults: boolean;
}

export interface ResetResponse {
  state: Tps43State;
}

export interface ErrorResponse {
  message: string;
}

export type Tps43Request =
  | { getState: GetStateRequest }
  | { setState: SetStateRequest }
  | { reset: ResetRequest };

export type Tps43Response =
  | { error: ErrorResponse }
  | { getState: GetStateResponse }
  | { setState: SetStateResponse }
  | { reset: ResetResponse };
