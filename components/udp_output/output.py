import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID
from esphome.components import udp

UdpOutput_ns = cg.esphome_ns.namespace('udp_output')
UdpOutput = UdpOutput_ns.class_('UdpOutput', output.FloatOutput, cg.Component)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(UdpOutput),
    cv.Required('udp'): cv.use_id(udp.Udp),
    cv.Exclusive('channel', 'channels'): cv.int_,
    cv.Exclusive('channels', 'channels'): cv.ensure_list(cv.int_),
    cv.Required('passthrough'): cv.use_id(output.BinaryOutput),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await output.register_output(var, config)
    chans = [config['channel']] if 'channel' in config else config['channels'];
    cg.add(var.setup(
        await cg.get_variable(config['udp']),
        await cg.get_variable(config['passthrough']),
        chans
    ))
