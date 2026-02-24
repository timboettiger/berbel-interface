import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import web_server_base

CODEOWNERS = ["@timboettiger"]
DEPENDENCIES = ["wifi", "web_server_base"]

berbel_ns = cg.esphome_ns.namespace("berbel")
BerbelComponent = berbel_ns.class_("BerbelComponent", cg.Component)

CONF_UPDATE_INTERVAL = "update_interval"
CONF_DEVICE_MAC = "device_mac"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BerbelComponent),
        cv.Optional(CONF_UPDATE_INTERVAL, default=18000): cv.positive_int,
        cv.Optional(CONF_DEVICE_MAC, default=""): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    if config[CONF_DEVICE_MAC]:
        cg.add(var.set_device_mac(config[CONF_DEVICE_MAC]))
