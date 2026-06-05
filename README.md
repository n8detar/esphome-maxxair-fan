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

maxxair_fan:
  id: roof_fan
  transmitter_id: ir_transmitter
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
    name: Auto Mode
    device_id: maxxair_fan_device
  open_button:
    name: Open Lid
    device_id: maxxair_fan_device
  close_button:
    name: Close Lid
    device_id: maxxair_fan_device
```

See [examples/maxxair-fan.yaml](examples/maxxair-fan.yaml) for a complete upload-ready configuration.

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
