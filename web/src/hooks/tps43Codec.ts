/**
 * Minimal, dependency-free protobuf encoder/decoder for the TPS43 Studio RPC
 * messages declared in `tps43Types.ts`.
 *
 * Only the constructs actually used by `tps43.proto` are implemented:
 * varint (bool/uint32) and length-delimited (string + nested message) wire
 * types. This keeps the codec small and deterministic, and avoids a runtime
 * dependency on generated code (see the note in `tps43Types.ts`).
 */

import {
  type Tps43Request,
  type Tps43Response,
  type Tps43State,
  type SetStateResponse,
} from "./tps43Types";

const WIRE_VARINT = 0;
const WIRE_LENGTH_DELIMITED = 2;

type StateFieldKind = "bool" | "uint32";

interface StateFieldDescriptor {
  name: keyof Tps43State;
  num: number;
  kind: StateFieldKind;
}

const STATE_FIELDS: StateFieldDescriptor[] = [
  { name: "oneFingerTap", num: 1, kind: "bool" },
  { name: "pressAndHold", num: 2, kind: "bool" },
  { name: "twoFingerTap", num: 3, kind: "bool" },
  { name: "pressAndHoldTime", num: 4, kind: "uint32" },
  { name: "scroll", num: 5, kind: "bool" },
  { name: "naturalScrollX", num: 6, kind: "bool" },
  { name: "naturalScrollY", num: 7, kind: "bool" },
  { name: "switchXy", num: 8, kind: "bool" },
  { name: "flipX", num: 9, kind: "bool" },
  { name: "flipY", num: 10, kind: "bool" },
  { name: "bottomBeta", num: 11, kind: "uint32" },
  { name: "stationaryThreshold", num: 12, kind: "uint32" },
];

const FIELDS_BY_NUM = new Map<number, StateFieldDescriptor>(
  STATE_FIELDS.map((f) => [f.num, f])
);

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------

function encodeVarint(value: number, out: number[]): void {
  let v = value >>> 0;
  while (v >= 0x80) {
    out.push((v & 0x7f) | 0x80);
    v >>>= 7;
  }
  out.push(v);
}

function writeTag(out: number[], fieldNum: number, wireType: number): void {
  encodeVarint((fieldNum << 3) | wireType, out);
}

function writeVarintField(
  out: number[],
  fieldNum: number,
  value: number
): void {
  writeTag(out, fieldNum, WIRE_VARINT);
  encodeVarint(value, out);
}

function writeLengthDelimited(
  out: number[],
  fieldNum: number,
  body: number[]
): void {
  writeTag(out, fieldNum, WIRE_LENGTH_DELIMITED);
  encodeVarint(body.length, out);
  for (const b of body) out.push(b);
}

function encodeTps43State(state: Tps43State): number[] {
  const out: number[] = [];
  for (const field of STATE_FIELDS) {
    const value = state[field.name] as number;
    // Tps43State fields are proto3 `optional`, so presence is meaningful on
    // the device: `handle_set_state` only applies a field when its `has_<f>`
    // bit is set. The web always sends full state, so encode every field
    // explicitly (including zero/false) to preserve presence.
    if (field.kind === "bool") {
      writeVarintField(out, field.num, value ? 1 : 0);
    } else {
      writeVarintField(out, field.num, value);
    }
  }
  return out;
}

function encodeSetState(request: {
  state: Tps43State;
  persist: boolean;
}): number[] {
  const out: number[] = [];
  writeLengthDelimited(out, 1, encodeTps43State(request.state));
  if (request.persist) writeVarintField(out, 2, 1);
  return out;
}

function encodeReset(request: { factoryDefaults: boolean }): number[] {
  const out: number[] = [];
  if (request.factoryDefaults) writeVarintField(out, 1, 1);
  return out;
}

export function encodeRequest(request: Tps43Request): Uint8Array {
  const out: number[] = [];
  if ("getState" in request) {
    writeLengthDelimited(out, 1, []);
  } else if ("setState" in request) {
    writeLengthDelimited(out, 2, encodeSetState(request.setState));
  } else if ("reset" in request) {
    writeLengthDelimited(out, 3, encodeReset(request.reset));
  }
  return new Uint8Array(out);
}

// ---------------------------------------------------------------------------
// Decoding
// ---------------------------------------------------------------------------

interface VarintResult {
  value: number;
  pos: number;
}

function readVarint(bytes: Uint8Array, pos: number): VarintResult {
  let value = 0;
  let shift = 0;
  for (;;) {
    if (pos >= bytes.length) throw new Error("malformed varint");
    const b = bytes[pos++];
    value |= (b & 0x7f) << shift;
    if ((b & 0x80) === 0) break;
    shift += 7;
    if (shift > 35) throw new Error("varint too long");
  }
  return { value: value >>> 0, pos };
}

interface TagResult {
  fieldNum: number;
  wireType: number;
  pos: number;
}

function readTag(bytes: Uint8Array, pos: number): TagResult {
  const { value, pos: next } = readVarint(bytes, pos);
  return { fieldNum: value >>> 3, wireType: value & 0x7, pos: next };
}

function skipField(bytes: Uint8Array, pos: number, wireType: number): number {
  switch (wireType) {
    case WIRE_VARINT:
      return readVarint(bytes, pos).pos;
    case WIRE_LENGTH_DELIMITED: {
      const { value, pos: next } = readVarint(bytes, pos);
      return next + value;
    }
    case 1: // 64-bit
      return pos + 8;
    case 5: // 32-bit
      return pos + 4;
    default:
      throw new Error(`unsupported wire type: ${wireType}`);
  }
}

export function defaultTps43State(): Tps43State {
  // Mirrors the firmware compile-time defaults in include/zmk/tps43_state.h
  // (shown before the first GetState response arrives).
  return {
    oneFingerTap: false,
    pressAndHold: false,
    twoFingerTap: false,
    pressAndHoldTime: 250,
    scroll: false,
    naturalScrollX: false,
    naturalScrollY: false,
    switchXy: false,
    flipX: false,
    flipY: false,
    bottomBeta: 5,
    stationaryThreshold: 5,
  };
}

function decodeString(bytes: Uint8Array): string {
  return new TextDecoder().decode(bytes);
}

function decodeTps43State(bytes: Uint8Array): Tps43State {
  const state = defaultTps43State();
  let pos = 0;
  while (pos < bytes.length) {
    const tag = readTag(bytes, pos);
    pos = tag.pos;
    const field = FIELDS_BY_NUM.get(tag.fieldNum);
    if (!field || tag.wireType !== WIRE_VARINT) {
      pos = skipField(bytes, pos, tag.wireType);
      continue;
    }
    const { value, pos: next } = readVarint(bytes, pos);
    pos = next;
    const record = state as unknown as Record<string, unknown>;
    if (field.kind === "bool") {
      record[field.name] = value !== 0;
    } else {
      record[field.name] = value;
    }
  }
  return state;
}

function decodeSetStateResponse(bytes: Uint8Array): SetStateResponse {
  let state = defaultTps43State();
  let persisted = false;
  let pos = 0;
  while (pos < bytes.length) {
    const tag = readTag(bytes, pos);
    pos = tag.pos;
    if (tag.fieldNum === 1 && tag.wireType === WIRE_LENGTH_DELIMITED) {
      const { value, pos: next } = readVarint(bytes, pos);
      state = decodeTps43State(bytes.subarray(next, next + value));
      pos = next + value;
    } else if (tag.fieldNum === 2 && tag.wireType === WIRE_VARINT) {
      const { value, pos: next } = readVarint(bytes, pos);
      persisted = value !== 0;
      pos = next;
    } else {
      pos = skipField(bytes, pos, tag.wireType);
    }
  }
  return { state, persisted };
}

export function decodeResponse(bytes: Uint8Array): Tps43Response {
  let pos = 0;
  let response: Tps43Response = { error: { message: "empty response" } };
  while (pos < bytes.length) {
    const tag = readTag(bytes, pos);
    pos = tag.pos;
    if (tag.wireType !== WIRE_LENGTH_DELIMITED) {
      pos = skipField(bytes, pos, tag.wireType);
      continue;
    }
    const { value, pos: next } = readVarint(bytes, pos);
    const body = bytes.subarray(next, next + value);
    pos = next + value;
    switch (tag.fieldNum) {
      case 1:
        response = { error: { message: decodeString(body) } };
        break;
      case 2:
        response = { getState: { state: decodeTps43State(body) } };
        break;
      case 3:
        response = { setState: decodeSetStateResponse(body) };
        break;
      case 4:
        response = { reset: { state: decodeTps43State(body) } };
        break;
      default:
        break;
    }
  }
  return response;
}
