import esphome.codegen as cg
from esphome.components import button, cover, fan, remote_transmitter, switch
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_EMPTY,
    DEVICE_CLASS_WINDOW,
)

CODEOWNERS = ["@n8detar"]
DEPENDENCIES = ["remote_transmitter"]
AUTO_LOAD = ["button", "cover", "fan", "maxxfan_protocol", "switch"]
MULTI_CONF = True

CONF_AUTO_MODE = "auto_mode"
CONF_AUTO_TEMPERATURE = "auto_temperature"
CONF_CEILING_FAN = "ceiling_fan"
CONF_CLOSE_BUTTON = "close_button"
CONF_COVER = "cover"
CONF_OPEN_BUTTON = "open_button"
CONF_TRANSMITTER_ID = "transmitter_id"

maxxair_fan_ns = cg.esphome_ns.namespace("maxxair_fan")
MaxxairFanComponent = maxxair_fan_ns.class_("MaxxairFanComponent", cg.Component)
MaxxairFanEntity = maxxair_fan_ns.class_("MaxxairFanEntity", fan.Fan, cg.Component)
MaxxairFanCover = maxxair_fan_ns.class_("MaxxairFanCover", cover.Cover, cg.Component)
MaxxairFanSwitch = maxxair_fan_ns.class_("MaxxairFanSwitch", switch.Switch, cg.Component)
MaxxairFanButton = maxxair_fan_ns.class_("MaxxairFanButton", button.Button, cg.Component)


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
        cv.Optional(CONF_AUTO_MODE): switch.switch_schema(
            MaxxairFanSwitch,
            device_class=DEVICE_CLASS_EMPTY,
            icon="mdi:thermostat-auto",
            default_restore_mode="ALWAYS_OFF",
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

    if auto_conf := config.get(CONF_AUTO_MODE):
        auto_var = cg.new_Pvariable(auto_conf[CONF_ID], var, 1)
        await cg.register_component(auto_var, auto_conf)
        await switch.register_switch(auto_var, auto_conf)
        cg.add(var.set_auto_mode_switch(auto_var))

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
