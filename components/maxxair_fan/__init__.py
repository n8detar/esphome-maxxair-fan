import esphome.codegen as cg
from esphome.components import (
    button,
    cover,
    fan,
    number,
    remote_transmitter,
    select,
    sensor,
    switch,
)
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_EMPTY,
    DEVICE_CLASS_WINDOW,
)

CODEOWNERS = ["@n8detar"]
DEPENDENCIES = ["remote_transmitter"]
AUTO_LOAD = [
    "button",
    "cover",
    "fan",
    "maxxfan_protocol",
    "number",
    "select",
    "sensor",
    "switch",
]
MULTI_CONF = True

CONF_AUTO_SETPOINT = "auto_setpoint"
CONF_AUTO_TEMPERATURE = "auto_temperature"
CONF_CEILING_FAN = "ceiling_fan"
CONF_CLOSE_BUTTON = "close_button"
CONF_COVER = "cover"
CONF_HIGH_TEMPERATURE = "high_temperature"
CONF_LOW_TEMPERATURE = "low_temperature"
CONF_MAX_SPEED = "max_speed"
CONF_MIN_SPEED = "min_speed"
CONF_MODE = "mode"
CONF_OPEN_BUTTON = "open_button"
CONF_TEMPERATURE_SENSOR_ID = "temperature_sensor_id"
CONF_TRANSMITTER_ID = "transmitter_id"

maxxair_fan_ns = cg.esphome_ns.namespace("maxxair_fan")
MaxxairFanComponent = maxxair_fan_ns.class_("MaxxairFanComponent", cg.Component)
MaxxairFanEntity = maxxair_fan_ns.class_("MaxxairFanEntity", fan.Fan, cg.Component)
MaxxairFanCover = maxxair_fan_ns.class_("MaxxairFanCover", cover.Cover, cg.Component)
MaxxairFanSwitch = maxxair_fan_ns.class_("MaxxairFanSwitch", switch.Switch, cg.Component)
MaxxairFanButton = maxxair_fan_ns.class_("MaxxairFanButton", button.Button, cg.Component)
MaxxairFanNumber = maxxair_fan_ns.class_("MaxxairFanNumber", number.Number, cg.Component)
MaxxairFanModeSelect = maxxair_fan_ns.class_(
    "MaxxairFanModeSelect", select.Select, cg.Component
)


ENTITY_NAME_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_NAME): cv.string,
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MaxxairFanComponent),
        cv.Required(CONF_TRANSMITTER_ID): cv.use_id(
            remote_transmitter.RemoteTransmitterComponent
        ),
        cv.Optional(CONF_TEMPERATURE_SENSOR_ID): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_AUTO_TEMPERATURE, default=78): cv.int_range(min=29, max=99),
        cv.Optional("fan", default={}): fan.fan_schema(
            MaxxairFanEntity, icon="mdi:fan", default_restore_mode="ALWAYS_OFF"
        ),
        cv.Optional(CONF_COVER, default={}): cover.cover_schema(
            MaxxairFanCover,
            device_class=DEVICE_CLASS_WINDOW,
            icon="mdi:window-shutter",
        ),
        cv.Optional(CONF_CEILING_FAN): switch.switch_schema(
            MaxxairFanSwitch,
            device_class=DEVICE_CLASS_EMPTY,
            icon="mdi:ceiling-fan",
            default_restore_mode="ALWAYS_OFF",
        ),
        cv.Optional(CONF_LOW_TEMPERATURE): number.number_schema(
            MaxxairFanNumber,
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement="°",
            icon="mdi:thermometer-low",
        ),
        cv.Optional(CONF_HIGH_TEMPERATURE): number.number_schema(
            MaxxairFanNumber,
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement="°",
            icon="mdi:thermometer-high",
        ),
        cv.Optional(CONF_MIN_SPEED): number.number_schema(
            MaxxairFanNumber,
            icon="mdi:fan-speed-1",
        ),
        cv.Optional(CONF_MAX_SPEED): number.number_schema(
            MaxxairFanNumber,
            icon="mdi:fan-speed-3",
        ),
        cv.Optional(CONF_AUTO_SETPOINT): number.number_schema(
            MaxxairFanNumber,
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement="°F",
            icon="mdi:thermostat",
        ),
        cv.Optional(CONF_MODE): select.select_schema(
            MaxxairFanModeSelect,
            icon="mdi:tune-variant",
        ),
        cv.Optional(CONF_OPEN_BUTTON): button.button_schema(
            MaxxairFanButton, icon="mdi:arrow-up-box"
        ),
        cv.Optional(CONF_CLOSE_BUTTON): button.button_schema(
            MaxxairFanButton, icon="mdi:arrow-down-box"
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))
    cg.add(var.set_auto_temperature(config[CONF_AUTO_TEMPERATURE]))
    if temperature_sensor_id := config.get(CONF_TEMPERATURE_SENSOR_ID):
        temperature_sensor = await cg.get_variable(temperature_sensor_id)
        cg.add(var.set_temperature_sensor(temperature_sensor))

    fan_var = cg.new_Pvariable(config["fan"][CONF_ID], var)
    await cg.register_component(fan_var, config["fan"])
    await fan.register_fan(fan_var, config["fan"])
    cg.add(var.set_fan(fan_var))

    cover_var = cg.new_Pvariable(config[CONF_COVER][CONF_ID], var)
    await cg.register_component(cover_var, config[CONF_COVER])
    await cover.register_cover(cover_var, config[CONF_COVER])
    cg.add(var.set_cover(cover_var))

    if ceiling_conf := config.get(CONF_CEILING_FAN):
        ceiling_var = cg.new_Pvariable(ceiling_conf[CONF_ID], var, 0)
        await cg.register_component(ceiling_var, ceiling_conf)
        await switch.register_switch(ceiling_var, ceiling_conf)
        cg.add(var.set_ceiling_fan_switch(ceiling_var))

    if low_conf := config.get(CONF_LOW_TEMPERATURE):
        low_var = cg.new_Pvariable(low_conf[CONF_ID], var, 0)
        await cg.register_component(low_var, low_conf)
        await number.register_number(low_var, low_conf, min_value=-40, max_value=150, step=0.5)
        cg.add(var.set_low_temperature_number(low_var))

    if high_conf := config.get(CONF_HIGH_TEMPERATURE):
        high_var = cg.new_Pvariable(high_conf[CONF_ID], var, 1)
        await cg.register_component(high_var, high_conf)
        await number.register_number(high_var, high_conf, min_value=-40, max_value=150, step=0.5)
        cg.add(var.set_high_temperature_number(high_var))

    if min_speed_conf := config.get(CONF_MIN_SPEED):
        min_speed_var = cg.new_Pvariable(min_speed_conf[CONF_ID], var, 2)
        await cg.register_component(min_speed_var, min_speed_conf)
        await number.register_number(min_speed_var, min_speed_conf, min_value=1, max_value=10, step=1)
        cg.add(var.set_min_speed_number(min_speed_var))

    if max_speed_conf := config.get(CONF_MAX_SPEED):
        max_speed_var = cg.new_Pvariable(max_speed_conf[CONF_ID], var, 3)
        await cg.register_component(max_speed_var, max_speed_conf)
        await number.register_number(max_speed_var, max_speed_conf, min_value=1, max_value=10, step=1)
        cg.add(var.set_max_speed_number(max_speed_var))

    if auto_setpoint_conf := config.get(CONF_AUTO_SETPOINT):
        auto_setpoint_var = cg.new_Pvariable(auto_setpoint_conf[CONF_ID], var, 4)
        await cg.register_component(auto_setpoint_var, auto_setpoint_conf)
        await number.register_number(auto_setpoint_var, auto_setpoint_conf, min_value=29, max_value=99, step=1)
        cg.add(var.set_auto_setpoint_number(auto_setpoint_var))

    if mode_conf := config.get(CONF_MODE):
        mode_var = cg.new_Pvariable(mode_conf[CONF_ID], var)
        await cg.register_component(mode_var, mode_conf)
        options = ["Manual", "Fan Thermostat"]
        if CONF_TEMPERATURE_SENSOR_ID in config:
            options.append("Smart Thermostat")
        await select.register_select(mode_var, mode_conf, options=options)
        cg.add(var.set_mode_select(mode_var))

    if open_conf := config.get(CONF_OPEN_BUTTON):
        open_var = cg.new_Pvariable(open_conf[CONF_ID], var, 0)
        await cg.register_component(open_var, open_conf)
        await button.register_button(open_var, open_conf)
        cg.add(var.set_open_button(open_var))

    if close_conf := config.get(CONF_CLOSE_BUTTON):
        close_var = cg.new_Pvariable(close_conf[CONF_ID], var, 1)
        await cg.register_component(close_var, close_conf)
        await button.register_button(close_var, close_conf)
        cg.add(var.set_close_button(close_var))
