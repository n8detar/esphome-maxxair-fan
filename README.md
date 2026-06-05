# ESPHome MaxxAir Fan

ESPHome external component for IR-controlled Airxcel MaxxAir / MaxxFan roof fans, especially the 7000K and 7500K family that use the 38 kHz infrared remote.

> Disclaimer: This project is vibe-coded and provided as-is. It controls real hardware over IR, so review the code and configuration, test carefully, and use at your own risk.

This repository contains two components:

- `maxxfan_protocol`: a Maxxfan IR encoder/decoder for ESPHome `remote_base`, based on the MIT-licensed work from [brown-studios/esphome-maxxfan-protocol](https://github.com/brown-studios/esphome-maxxfan-protocol).
- `maxxair_fan`: a higher-level fan integration that creates Home Assistant entities for the fan, lid cover, ceiling fan mode, control mode, smart thermostat settings, and optional open/close buttons.

The higher-level behavior follows the practical entity model from [SmartyVan/MaxxAir-Fan-ESPHome](https://github.com/SmartyVan/MaxxAir-Fan-ESPHome): forward is air out/exhaust, reverse is air in/intake, closing the lid while the fan is running enters ceiling fan mode, and the lid can be opened or closed while the fan is off for passive ventilation.

## Features

- 10 fan speeds.
- Forward/reverse fan direction.
- Lid open/close cover entity.
- Ceiling fan mode switch.
- Control mode selector with `Manual`, `Fan Thermostat`, and optional `Smart Thermostat` modes.
- Fan thermostat mode with configurable built-in setpoint sent in the IR packet.
- Optional blueprint-style smart auto fan control using a temperature sensor.
- Optional number entities for smart low/high temperature thresholds, smart min/max speed, and the built-in auto setpoint.
- Optional open/close buttons.
- ESPHome sub-device support through standard `device_id` on every entity.
- Low-level `remote_transmitter.transmit_maxxfan`, `dump: maxxfan`, and `on_maxxfan` support from the protocol component.

## Examples

### Minimal Manual Control

```yaml
external_components:
  - source: github://n8detar/esphome-maxxair-fan@main
    components:
      - maxxair_fan
      - maxxfan_protocol

esphome:
  name: maxxair-fan
  friendly_name: MaxxAir Fan

esp32:
  board: esp32dev

logger:
api:
ota:
  platform: esphome

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

remote_transmitter:
  id: ir_tx
  pin:
    number: GPIO4
    inverted: false
  carrier_duty_percent: 50%
  non_blocking: true

maxxair_fan:
  id: roof_fan
  transmitter_id: ir_tx
  fan:
    name: Fan
  cover:
    name: Lid
  ceiling_fan:
    name: Ceiling Fan Mode
```

See [examples/minimal.yaml](examples/minimal.yaml).

### Fan Thermostat Mode

This exposes the fan's built-in thermostat mode. ESPHome sends the fan settings and the MaxxFan handles its own temperature control after that.

```yaml
maxxair_fan:
  id: roof_fan
  transmitter_id: ir_tx
  auto_temperature: 78
  fan:
    name: Fan
  cover:
    name: Lid
  mode:
    name: Control Mode
    restore_value: true
  auto_setpoint:
    name: Fan Thermostat Setpoint
    initial_value: 78
    restore_value: true
```

See [examples/fan-thermostat.yaml](examples/fan-thermostat.yaml).

### Full Smart Thermostat

This adds a temperature sensor, sub-device grouping, the mode selector, and the blueprint-style smart thermostat settings.

```yaml
maxxair_fan:
  id: roof_fan
  transmitter_id: ir_tx
  temperature_sensor_id: cabin_temperature
  fan_off_below_low_temperature: true
  auto_temperature: 78
  fan:
    name: Fan
    device_id: maxxair_fan_device
  cover:
    name: Lid
    device_id: maxxair_fan_device
  ceiling_fan:
    name: Ceiling Fan Mode
    device_id: maxxair_fan_device
  mode:
    name: Control Mode
    device_id: maxxair_fan_device
    restore_value: true
  low_temperature:
    name: Smart Low Temperature
    device_id: maxxair_fan_device
    initial_value: 74
    restore_value: true
  high_temperature:
    name: Smart High Temperature
    device_id: maxxair_fan_device
    initial_value: 80
    restore_value: true
  min_speed:
    name: Smart Minimum Speed
    device_id: maxxair_fan_device
    initial_value: 1
    restore_value: true
  max_speed:
    name: Smart Maximum Speed
    device_id: maxxair_fan_device
    initial_value: 10
    restore_value: true
  auto_setpoint:
    name: Fan Thermostat Setpoint
    device_id: maxxair_fan_device
    initial_value: 78
    restore_value: true
```

See [examples/smart-thermostat.yaml](examples/smart-thermostat.yaml). [examples/maxxair-fan.yaml](examples/maxxair-fan.yaml) is kept as the full default example.

## Control Mode

The optional `mode` select is the recommended way to choose between mutually exclusive control strategies:

- `Manual`: no automatic control. Use the fan, lid, and ceiling fan entities directly.
- `Fan Thermostat`: uses the MaxxFan's built-in IR thermostat mode and the `auto_setpoint` / `auto_temperature` value.
- `Smart Thermostat`: uses the ESPHome temperature sensor and linearly maps temperature to fan speed.

When `temperature_sensor_id` is not configured, the mode select exposes only `Manual` and `Fan Thermostat`.

The `mode` select supports `restore_value` and `initial_option`. The fan entity and ceiling fan switch use ESPHome's normal `restore_mode`; the number inputs support `restore_value` and `initial_value`.

## Smart Thermostat

`Smart Thermostat` mode brings the SmartyVan blueprint idea into ESPHome. When enabled, the component reads `temperature_sensor_id`, turns the fan off below `low_temperature` when `fan_off_below_low_temperature` is `true`, and linearly maps temperatures from `low_temperature` through `high_temperature` to fan speeds from `min_speed` to `max_speed`.

The original SmartyVan Home Assistant blueprint runs the fan at the configured minimum speed at or below the low threshold. This component defaults to keeping Smart Thermostat mode selected but turning the fan off below the low threshold, which behaves more like a thermostat. Set `fan_off_below_low_temperature: false` to use the original minimum-speed behavior.

With the default setting, the fan is off below the low threshold and turns on at the minimum speed when the temperature reaches the low threshold.

Defaults are:

- Low temperature: `74 °F` / about `23.3 °C`
- High temperature: `80 °F` / about `26.7 °C`
- Minimum speed: `1`
- Maximum speed: `10`
- Fan off below low temperature: `true`

ESPHome stores temperature sensor state in Celsius internally, but the MaxxFan remote/protocol is Fahrenheit-oriented. To avoid Home Assistant rounding oddities like `80 °F` snapping to `80.6 °F`, the smart threshold and fan setpoint number entities report `°F` with whole-degree steps. The component converts the ESPHome temperature sensor reading from Celsius to Fahrenheit internally before comparing thresholds.

Selecting `Manual` disables smart thermostat control. Manual fan, lid, ceiling fan, or fan thermostat commands also return the mode to manual control.

## Built-In Auto Mode

The MaxxFan IR protocol includes both an `auto_mode` bit and an `auto_temperature` byte. `Fan Thermostat` mode enables that built-in thermostat mode. The optional `auto_setpoint` number is exposed as `°F` and maps directly to the protocol's Fahrenheit byte; valid values are `29 °F` to `99 °F`.

`auto_temperature` is still supported, but you can think of it as the boot/default seed for the built-in thermostat setpoint. It is not required if you are happy with the default `78`, or if you set `auto_setpoint.initial_value`.

## Wiring

Connect an IR LED through a suitable current-limiting resistor to a GPIO that supports output. The example uses `GPIO4`.

Many simple IR LED circuits use the ESP GPIO to sink current with the LED anode connected through a resistor to 3.3 V and the cathode connected to the GPIO. In that case set the transmitter pin `inverted: true`. If you drive the LED or transistor high from the GPIO, use `inverted: false`.

## Low-Level Protocol

You can also use the raw protocol action directly:

```yaml
script:
  - id: fan_high_exhaust
    then:
      - remote_transmitter.transmit_maxxfan:
          transmitter_id: ir_tx
          fan_on: true
          fan_speed: 100
          fan_exhaust: true
          cover_open: true
          auto_mode: false
          auto_temperature: 78
          special: false
          warn: false
```

The IR packet format and timing are documented in the protocol source and in the brown-studios project.

## Credits

- Maxxfan IR protocol implementation from [brown-studios/esphome-maxxfan-protocol](https://github.com/brown-studios/esphome-maxxfan-protocol), MIT License.
- Entity behavior inspired by [SmartyVan/MaxxAir-Fan-ESPHome](https://github.com/SmartyVan/MaxxAir-Fan-ESPHome).

## License

MIT. See [LICENSE](LICENSE).
