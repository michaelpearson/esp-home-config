import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover, uart
from esphome.const import CONF_ID

tuya_cover_ns = cg.esphome_ns.namespace('tuya_cover')
TuyaCover = tuya_cover_ns.class_('TuyaCover', cover.Cover, cg.Component)

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(TuyaCover),
    cv.Required('uart'): cv.use_id(uart.UARTComponent)
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.setup(await cg.get_variable(config['uart'])))
    await cover.register_cover(var, config)
