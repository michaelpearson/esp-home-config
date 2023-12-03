import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

udp_ns = cg.esphome_ns.namespace("udp")
Udp = udp_ns.class_("Udp", cg.Component)

MULTI_CONF = True

CONFIG_SCHEMA = cv.All(
  cv.Schema({
    cv.GenerateID(): cv.declare_id(Udp),
    cv.Required('host'): cv.string,
    cv.Required('port'): cv.int_
  }).extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  cg.add(var.setup(config['host'], config['port']))

