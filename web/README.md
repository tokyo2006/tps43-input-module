# TPS43 Web UI

DYA Studio tab for the [tps43-input-module](https://github.com/tokyo2006/tps43-input-module)
firmware. A React + TypeScript single-page app that connects to the keyboard via
WebSerial or Web Bluetooth and edits the TPS43 (Azoteq IQS5xx) trackpad state
live through the `tokyo2006__tps43` custom Studio RPC subsystem.

## Development

```sh
npm install
npm run dev
```

## Build

```sh
npm run build
```

## Tests

```sh
npm test          # jest unit tests
npm run e2e       # playwright (requires `west zmk-web-e2e` to boot a DUT)
```
