# ESPHome MaxxAir Fan

ESPHome external component for IR-controlled Airxcel MaxxAir / MaxxFan roof fans, especially the 7000K and 7500K family that use the 38 kHz infrared remote.

This repository contains two components:

- `maxxfan_protocol`: a Maxxfan IR encoder/decoder for ESPHome `remote_base`, based on the MIT-licensed work from [brown-studios/esphome-maxxfan-protocol](https://github.com/brown-studios/esphome-maxxfan-protocol).
- `maxxair_fan`: a higher-level fan integration that creates Home Assistant entities for the fan, lid cover, ceiling fan mode, auto mode, and optional open/close buttons.

The higher-level behavior follows the practical entity model from [SmartyVan/MaxxAir-Fan-ESPHome](https://github.com/SmartyVan/MaxxAir-Fan-ESPHome): forward is air out/exhaust, reverse is air in/intake, closing the lid while the fan is running enters ceiling fan mode, and the lid can be opened or closed while the fan is off for passive ventilation.

## Features

- 10 fan speeds.
- Forward/reverse fan direction.
- Lid open/close cover entity.
- Ceiling fan mode switch.
- Auto mode switch with configurable thermostat setpoint sent in the IR packet.
- Optional blueprint-style smart auto fan control using a temperature sensor.
- Optional number entities for smart low/high temperature thresholds, smart min/max speed, and the built-in auto setpoint.
- Optional open/close buttons.
- ESPHome sub-device support through standard `device_id` on every entity.
- Low-level `remote_transmitter.transmit_maxxfan`, `dump: maxxfan`, and `on_maxxfan` support from the protocol component.

## Example

```yaml
external_components:
  - source: github://n8detar/esphome-maxxair-fan@main
    components:
      - maxxair_fan
      - maxxfan_protocol

esphome:
  name: maxxair-fan
  friendly_name: MaxxAir Fan
  devices:
    - id: maxxair_fan_device
      name: MaxxAir Fan

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
  id: ir_transmitter
  pin:
    number: GPIO4
    inverted: false
  carrier_duty_percent: 50%
  non_blocking: true

sensor:
  # Replace this with your actual cabin temperature sensor.
  - platform: template
    id: cabin_temperature
    name: Cabin Temperature
    device_class: temperature
    unit_of_measurement: "°F"
    accuracy_decimals: 1
    lambda: return NAN;
    update_interval: 60s

maxxair_fan:
  id: roof_fan
  transmitter_id: ir_transmitter
  temperature_sensor_id: cabin_temperature
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
  auto_mode:
    name: Built-in Auto Mode
    device_id: maxxair_fan_device
  smart_auto:
    name: Smart Auto Fan
    device_id: maxxair_fan_device
  low_temperature:
    name: Smart Low Temperature
    device_id: maxxair_fan_device
  high_temperature:
    name: Smart High Temperature
    device_id: maxxair_fan_device
  min_speed:
    name: Smart Minimum Speed
    device_id: maxxair_fan_device
  max_speed:
    name: Smart Maximum Speed
    device_id: maxxair_fan_device
  auto_setpoint:
    name: Built-in Auto Setpoint
    device_id: maxxair_fan_device
  open_button:
    name: Open Lid
    device_id: maxxair_fan_device
  close_button:
    name: Close Lid
    device_id: maxxair_fan_device
```

See [examples/maxxair-fan.yaml](examples/maxxair-fan.yaml) for a complete upload-ready configuration.

## Smart Auto Fan

The `smart_auto` switch brings the SmartyVan blueprint behavior into ESPHome. When enabled, the component reads `temperature_sensor_id`, maps the current temperature linearly from `low_temperature` to `high_temperature`, and sets a fan speed from `min_speed` to `max_speed`.

Defaults are:

- Low temperature: `72`
- High temperature: `85`
- Minimum speed: `1`
- Maximum speed: `10`

The threshold number entities use the same unit as your ESPHome temperature sensor. If your sensor reports Celsius, set the thresholds in Celsius.

Turning `smart_auto` off turns the fan off. Manual fan, lid, ceiling fan, or built-in auto-mode commands disable `smart_auto` so the fan returns to normal manual control.

## Built-In Auto Mode

The MaxxFan IR protocol includes both an `auto_mode` bit and an `auto_temperature` byte. The `auto_mode` switch enables or disables that built-in thermostat mode. The optional `auto_setpoint` number changes the `auto_temperature` value sent in the IR packet; valid values are `29` to `99`, matching the protocol field used by the remote.

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
          transmitter_id: ir_transmitter
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
