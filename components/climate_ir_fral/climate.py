import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID

CODEOWNERS = ["@RubyBailey"]
AUTO_LOAD = ["climate_ir"]

climate_ir_fral_ns = cg.esphome_ns.namespace("climate_ir_fral")
FralClimate = climate_ir_fral_ns.class_("FralClimate", climate_ir.ClimateIR)

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {cv.GenerateID(): cv.declare_id(FralClimate)}
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)
