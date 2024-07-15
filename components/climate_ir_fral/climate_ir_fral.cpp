#include "climate_ir_fral.h"
#include "esphome/core/log.h"

namespace esphome {
namespace climate_ir_fral {
static const char *const TAG = "climate_ir_fral.climate";

// Fral uses climabutler transmission parameters, although seems to have its own codes.
// Ref: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/ir_ClimaButler.cpp
// Thanks to benjy3gg & David Conran (crankyoldgit) for work on decoding protocol.

const uint16_t CLIMABUTLER_BIT_MARK = 511;
const uint16_t CLIMABUTLER_HDR_MARK = CLIMABUTLER_BIT_MARK;
const uint16_t CLIMABUTLER_HDR_SPACE = 3492;
const uint16_t CLIMABUTLER_ONE_SPACE = 1540;
const uint16_t CLIMABUTLER_ZERO_SPACE = 548;
const uint32_t CLIMABUTLER_REPEAT_GAP = 100000;
const uint16_t CLIMABUTLER_FREQ = 38000;

const uint8_t FRAL_STATE_POWER_OFF = 0x0;
const uint8_t FRAL_STATE_POWER_ON = 0x3;

const uint8_t FRAL_MODE_FAN_COOL_LOW = 0x22;
const uint8_t FRAL_MODE_FAN_COOL_MED = 0x11;
const uint8_t FRAL_MODE_FAN_COOL_HIGH = 0x08;

const uint8_t FRAL_MODE_FAN_HEAT_LOW = 0x01;
const uint8_t FRAL_MODE_FAN_HEAT_HIGH = 0x03;

const uint8_t FRAL_MODE_FAN_DRY_LOW = 0x24;

const uint8_t FRAL_SWING_OFF = 0x2;
const uint8_t FRAL_SWING_SLOW = 0x4;
const uint8_t FRAL_SWING_FAST = 0x0;

struct FralRemoteState {
  uint8_t power_on{FRAL_STATE_POWER_OFF};
  uint8_t timer_enabled{0};
  uint8_t timer_hours{0};
  uint8_t temp1{0};  // temperature is set in two places in the IR code
  uint8_t quiet_mode{0};
  uint8_t swing{FRAL_SWING_OFF};
  uint8_t mode_fan{FRAL_MODE_FAN_COOL_LOW};  // fan & mode are in one byte that does not seem to have 2 parts.
  uint8_t temp2{0};
  uint8_t checksum{0};
};

climate::ClimateTraits climate_ir_fral::FralClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_action(false);
  traits.set_visual_min_temperature(FRAL_TEMP_MIN);
  traits.set_visual_max_temperature(FRAL_TEMP_MAX);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF});

  if (this->supports_cool_)
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  if (this->supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  if (this->supports_dry_)
    traits.add_supported_mode(climate::CLIMATE_MODE_DRY);

  traits.set_supported_fan_modes(
      {climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH, climate::CLIMATE_FAN_QUIET});

  traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_HORIZONTAL});

  return traits;
}

void FralClimate::transmit_state() {
  FralRemoteState remote_state;

  // Heat, cool, dry and fan are one setting, so needs multi-switch statement.
  switch (this->mode) {
    case climate::CLIMATE_MODE_HEAT:
      remote_state.power_on = FRAL_STATE_POWER_ON;
      remote_state.quiet_mode = 0;
      switch (this->fan_mode.value()) {
        case climate::CLIMATE_FAN_HIGH:
          remote_state.mode_fan = FRAL_MODE_FAN_HEAT_HIGH;
          break;
        case climate::CLIMATE_FAN_MEDIUM:
          this->fan_mode = climate::CLIMATE_FAN_LOW;
        case climate::CLIMATE_FAN_LOW:
          remote_state.mode_fan = FRAL_MODE_FAN_COOL_LOW;
          break;
        case climate::CLIMATE_FAN_QUIET:
        default:
          remote_state.mode_fan = FRAL_MODE_FAN_COOL_LOW;
          remote_state.quiet_mode = 1;
          break;
      }
      break;
    case climate::CLIMATE_MODE_DRY:  // Drying has only low fan setting and no quiet mode.
      remote_state.power_on = FRAL_STATE_POWER_ON;
      remote_state.mode_fan = FRAL_MODE_FAN_DRY_LOW;
      remote_state.quiet_mode = 0;
      break;
    case climate::CLIMATE_MODE_COOL:
      remote_state.power_on = FRAL_STATE_POWER_ON;
      remote_state.quiet_mode = 0;
      switch (this->fan_mode.value()) {
        case climate::CLIMATE_FAN_HIGH:
          remote_state.mode_fan = FRAL_MODE_FAN_COOL_HIGH;
          break;
        case climate::CLIMATE_FAN_MEDIUM:
          remote_state.mode_fan = FRAL_MODE_FAN_COOL_MED;
          break;
        case climate::CLIMATE_FAN_LOW:
          remote_state.mode_fan = FRAL_MODE_FAN_COOL_LOW;
          break;
        case climate::CLIMATE_FAN_QUIET:
        default:
          remote_state.mode_fan = FRAL_MODE_FAN_COOL_LOW;
          remote_state.quiet_mode = 1;
          break;
      }
      break;
    case climate::CLIMATE_MODE_OFF:  // OFF sets timer to 0
    default:
      remote_state.power_on = FRAL_STATE_POWER_OFF;
      remote_state.timer_enabled = 0;
      remote_state.timer_hours = 0;
      remote_state.mode_fan = FRAL_MODE_FAN_COOL_LOW;  // We have to set something in mode+fan field.
      break;
  }

  // Temperature
  uint8_t clamped_temp = FRAL_TEMP_COOLING_MIN;
  // Coooling minimal temperature is different than heating.
  if (this->mode == climate::CLIMATE_MODE_COOL) {
    clamped_temp = (uint8_t) roundf(clamp<float>(this->target_temperature, FRAL_TEMP_COOLING_MIN, FRAL_TEMP_MAX));
  } else {
    clamped_temp = (uint8_t) roundf(clamp<float>(this->target_temperature, FRAL_TEMP_MIN, FRAL_TEMP_MAX));
  }
  this->target_temperature = (float) clamped_temp;
  remote_state.temp1 = clamped_temp / 10;
  remote_state.temp2 = clamped_temp - FRAL_TEMP_MIN;

  switch (this->swing_mode) {
    case climate::CLIMATE_SWING_HORIZONTAL:
      remote_state.swing = FRAL_SWING_SLOW;
      break;
    case climate::CLIMATE_SWING_OFF:
    default:
      remote_state.swing = FRAL_SWING_OFF;
      break;
  }

  // Kinda weird way for consturcting state, but hey, it works.
  uint64_t state = 0ULL;
  state |= (uint64_t) (remote_state.temp2 & 0b00001111) << 4;
  state |= (uint64_t) (remote_state.mode_fan << 8);
  state |= (uint64_t) (remote_state.swing & 0b00000111) << 16;
  state |= (uint64_t) (remote_state.quiet_mode & 0b00000001) << 19;
  state |= (uint64_t) (remote_state.temp1 & 0b00001111) << 20;
  state |= (uint64_t) (remote_state.timer_hours & 0b00011111) << 38;
  state |= (uint64_t) (remote_state.timer_enabled & 0b00000001) << 43;
  state |= (uint64_t) (remote_state.power_on & 0b00001111) << 44;

  uint8_t checksum = 0;
  for (int i = 1; i < sizeof(state) * 2; i++) {
    checksum += (uint8_t) (state >> i * 4 & 0b00001111ULL);
  }
  checksum = 0xF - (checksum & 0b00001111);
  remote_state.checksum = checksum;
  state |= (uint64_t) checksum;

  ESP_LOGD(TAG, "Sending Fral code: 0x%llx", state);

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(CLIMABUTLER_FREQ);
  // Header
  data->mark(CLIMABUTLER_HDR_MARK);
  data->space(CLIMABUTLER_HDR_SPACE);
  // Data
  const uint8_t length = 52;

  // Send the supplied data, most significant bits first.
  for (uint64_t mask = 1ULL << (length - 1); mask; mask >>= 1)
    if (state & mask) {
      data->mark(CLIMABUTLER_BIT_MARK);
      data->space(CLIMABUTLER_ONE_SPACE);
    } else {
      data->mark(CLIMABUTLER_BIT_MARK);
      data->space(CLIMABUTLER_ZERO_SPACE);
    }

  // footer
  data->mark(CLIMABUTLER_BIT_MARK);
  data->space(CLIMABUTLER_HDR_SPACE);
  data->mark(CLIMABUTLER_BIT_MARK);

  // We could've changed temperature when clamping it, so also publishing state here.
  this->publish_state();
  transmit.perform();
}

}  // namespace climate_ir_fral
}  // namespace esphome
