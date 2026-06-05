import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components.remote_base import declare_protocol, register_trigger, register_dumper, register_action

CODEOWNERS = ["@j9brown"]

DEPENDENCIES = ["remote_base"]

MULTI_CONF = False

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA


async def to_code(config):
    pass


MaxxfanData, MaxxfanBinarySensor, MaxxfanTrigger, MaxxfanAction, MaxxfanDumper = declare_protocol("Maxxfan")


@register_trigger("maxxfan", MaxxfanTrigger, MaxxfanData)
def maxxfan_trigger(var, config):
    pass


@register_dumper("maxxfan", MaxxfanDumper)
def maxxfan_dumper(var, config):
    pass


CONF_FAN_ON = "fan_on"
CONF_FAN_SPEED = "fan_speed"
CONF_FAN_EXHAUST = "fan_exhaust"
CONF_COVER_OPEN = "cover_open"
CONF_AUTO_MODE = "auto_mode"
CONF_AUTO_TEMPERATURE = "auto_temperature"
CONF_SPECIAL = "special"
CONF_WARN = "warn"

MAXXFAN_ACTION_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_FAN_ON, default=False): cv.boolean,
        cv.Optional(CONF_FAN_SPEED, default=10): cv.All(cv.int_, cv.one_of(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, int=True)),
        cv.Optional(CONF_FAN_EXHAUST, default=False): cv.boolean,
        cv.Optional(CONF_COVER_OPEN, default=False): cv.boolean,
        cv.Optional(CONF_AUTO_MODE, default=False): cv.boolean,
        cv.Optional(CONF_AUTO_TEMPERATURE, default=78): cv.int_range(min=29, max=99),
        cv.Optional(CONF_SPECIAL, default=False): cv.boolean,
        cv.Optional(CONF_WARN, default=False): cv.boolean,
    }
)

@register_action("maxxfan", MaxxfanAction, MAXXFAN_ACTION_SCHEMA)
async def maxxfan_action(var, config, args):
    template_ = await cg.templatable(config[CONF_FAN_ON], args, cg.bool_)
    cg.add(var.set_fan_on(template_))
    template_ = await cg.templatable(config[CONF_FAN_SPEED], args, cg.uint8)
    cg.add(var.set_fan_speed(template_))
    template_ = await cg.templatable(config[CONF_FAN_EXHAUST], args, cg.bool_)
    cg.add(var.set_fan_exhaust(template_))
    template_ = await cg.templatable(config[CONF_COVER_OPEN], args, cg.bool_)
    cg.add(var.set_cover_open(template_))
    template_ = await cg.templatable(config[CONF_AUTO_MODE], args, cg.bool_)
    cg.add(var.set_auto_mode(template_))
    template_ = await cg.templatable(config[CONF_AUTO_TEMPERATURE], args, cg.uint8)
    cg.add(var.set_auto_temperature(template_))
    template_ = await cg.templatable(config[CONF_SPECIAL], args, cg.bool_)
    cg.add(var.set_special(template_))
    template_ = await cg.templatable(config[CONF_WARN], args, cg.bool_)
    cg.add(var.set_warn(template_))
