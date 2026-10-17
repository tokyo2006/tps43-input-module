# TPS43 Input Module for ZMK

ZMK module providing the Azoteq **IQS5xx** trackpad driver (TPS43 / TPS65) with
an **unofficial custom ZMK Studio RPC** for runtime configuration, persistent
custom settings, a split-relay helper, and a web UI — the same shape as
[`tokyo2006/cirque-input-module`](https://github.com/tokyo2006/cirque-input-module).

## Features

- IQS5xx trackpad movement, single-tap (left click), two-finger-tap (right
  click), press-and-hold, and vertical/horizontal scroll.
- Custom Studio RPC subsystem `tokyo2006__tps43` to get/set/reset the runtime
  parameters (tap gestures, scroll, axes, filtering) live.
- Persistent custom settings (subsystem `tokyo2006__tps43`).
- Split-relay of the full runtime state central → peripheral.
- Web UI (React + TypeScript).

## Usage

Add the module to `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: tokyo2006
      url-base: https://github.com/tokyo2006
    - name: cormoran
      url-base: https://github.com/cormoran
  projects:
    - name: tps43-input-module
      remote: tokyo2006
      revision: main
      import: true
    - name: zmk
      remote: cormoran
      revision: main+dya
      import:
        file: app/west.yml
```

Enable in `config/<shield>.conf`:

```conf
CONFIG_ZMK_STUDIO=y
CONFIG_ZMK_TPS43=y
CONFIG_ZMK_TPS43_STUDIO_RPC=y

CONFIG_ZMK_CUSTOM_SETTINGS=y
CONFIG_ZMK_CUSTOM_SETTINGS_STUDIO_RPC=y
```

Devicetree:

```dts
&i2c0 {
    status = "okay";
    compatible = "nordic,nrf-twim";

    tps43: iqs5xx@74 {
        compatible = "azoteq,iqs5xx";
        reg = <0x74>;
        status = "okay";
        reset-gpios = <&gpio0 14 GPIO_ACTIVE_LOW>;
        rdy-gpios = <&gpio0 15 GPIO_ACTIVE_HIGH>;
        one-finger-tap;
        two-finger-tap;
        scroll;
    };
};

/ {
    tps43_input: tps43_input {
        compatible = "zmk,input-listener";
        device = <&tps43>;
    };
};
```

## Studio RPC

- `GetState` / `SetState` / `Reset` over the `tokyo2006__tps43` subsystem.
- `SetState` supports partial updates (proto3 `optional` presence) and optional
  persistence (`persist = true`).
- The web UI connects over WebSerial and edits the state live.

## License

MIT (driver originally by [Mariano Uvalle](https://github.com/AYM1607)).
