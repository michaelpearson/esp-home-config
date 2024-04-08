import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID

as5600_cover_ns = cg.esphome_ns.namespace('as5600_cover')
AS5600Cover = as5600_cover_ns.class_('AS5600Cover', cover.Cover, cg.Component)

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(AS5600Cover)
}).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    cg.add_library("Wire", None)
    cg.add_library("robtillaart/AS5600", None)
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield cover.register_cover(var, config)