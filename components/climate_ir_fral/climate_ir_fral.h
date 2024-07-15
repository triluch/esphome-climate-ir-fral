#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

#include <cinttypes>

namespace esphome {
namespace climate_ir_fral {

// Temperature
const uint8_t FRAL_TEMP_MIN = 15;          // Celsius
const uint8_t FRAL_TEMP_MAX = 30;          // Celsius
const uint8_t FRAL_TEMP_COOLING_MIN = 17;  // Celsius

class FralClimate : public climate_ir::ClimateIR {
 public:
  FralClimate()
      : climate_ir::ClimateIR(FRAL_TEMP_MIN, FRAL_TEMP_MAX, 1.0f, true, false,
                              {climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH,
                               climate::CLIMATE_FAN_QUIET},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_HORIZONTAL}, {}) {}

 protected:
  // Transmit via IR the state of this climate controller.
  void transmit_state() override;

  climate::ClimateTraits traits() override;
};

}  // namespace climate_ir_fral
}  // namespace esphome
