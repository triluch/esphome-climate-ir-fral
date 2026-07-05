import esphome.codegen as cg
from esphome.components import climate_ir

CODEOWNERS = ["@RubyBailey"]
AUTO_LOAD = ["climate_ir"]

climate_ir_fral_ns = cg.esphome_ns.namespace("climate_ir_fral")
FralClimate = climate_ir_fral_ns.class_("FralClimate", climate_ir.ClimateIR)

CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(FralClimate)


async def to_code(config):
    await climate_ir.new_climate_ir(config)
