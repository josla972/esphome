#include "daikin.h"
#include "esphome/components/remote_base/remote_base.h"

namespace esphome {
namespace daikin {

static const char *TAG = "daikin.climate";
// TODO: Make these inputs
static bool ARC432A14 = true;
static bool comfort_mode = false;
static bool powerful_mode = false;
static bool economy_mode = false;
#define FRAME_THREE_OFFSET 16
#define HEADER_ID 3
#define MESSAGE_ID 4
#define COMFORT_ID 6
#define SWING_VERTICAL_ID (FRAME_THREE_OFFSET + 8)
#define SWING_HORIZONTAL_ID (FRAME_THREE_OFFSET + 9)
#define TIMER_A_ID (FRAME_THREE_OFFSET + 10)
#define TIMER_B_ID (FRAME_THREE_OFFSET + 11)
#define TIMER_C_ID (FRAME_THREE_OFFSET + 12)
#define POWERFUL_ID (FRAME_THREE_OFFSET + 13)
#define ECONOMY_ID (FRAME_THREE_OFFSET + 16)

void DaikinClimate::transmit_state() {

  uint8_t remote_state[35] = {0x11, 0xDA, 0x27, 0x00, 0xC5, 0x00, 0x00, 0x00, 0x11, 0xDA, 0x27, 0x00,
                              0x42, 0x49, 0x05, 0xA2, 0x11, 0xDA, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00};
  // Original author's setting. Untested.
  if (!ARC432A14)
  {
	remote_state[27] = 0x06;
	remote_state[28] = 0x60;
  }


  // Frame #1
  if (ARC432A14)
  {
	remote_state[HEADER_ID] = 0xF0;
	remote_state[MESSAGE_ID] = 0x00;
  }
  if (comfort_mode)
  {
	remote_state[COMFORT_ID] = 0x10;
  }
  // Calculate checksum
  for (int i = 0; i < 7; i++) {
    remote_state[7] += remote_state[i];
  }

  // Calculations frame #3
  if (this->swing_mode == climate::CLIMATE_SWING_HORIZONTAL)
  {
	 remote_state[SWING_HORIZONTAL_ID] |= 0xF0;
  }
  // FUTURE:
#if 0
  if (this->swing_mode == climate::CLIMATE_SWING_WIDE)
  {
	 remote_state[SWING_HORIZONTAL_ID] &= 0x0F;
	 remote_state[SWING_HORIZONTAL_ID] |= 0xE0;
  }
#endif

  remote_state[21] = this->operation_mode_();
  // Vertical swing covered in fan speed.
  remote_state[24] = this->fan_speed_();
  remote_state[22] = this->temperature_();
  if (powerful_mode)
  {
	  remote_state[POWERFUL_ID] = 0x01;
  }
  if (economy_mode)
  {
	  remote_state[ECONOMY_ID] |= 0x04;
  }

  // Calculate checksum
  for (int i = 16; i < 34; i++) {
    remote_state[34] += remote_state[i];
  }

  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();
  data->set_carrier_frequency(DAIKIN_IR_FREQUENCY);

  data->mark(DAIKIN_HEADER_MARK);
  data->space(DAIKIN_HEADER_SPACE);
  for (int i = 0; i < 8; i++) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
      data->mark(DAIKIN_BIT_MARK);
      bool bit = remote_state[i] & mask;
      data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
    }
  }
  data->mark(DAIKIN_BIT_MARK);
  data->space(DAIKIN_MESSAGE_SPACE);
  data->mark(DAIKIN_HEADER_MARK);
  data->space(DAIKIN_HEADER_SPACE);

  // Second frame not necessary for ARC432A14
  if (!ARC432A14)
  {
	for (int i = 8; i < 16; i++) {
	  for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
	    data->mark(DAIKIN_BIT_MARK);
	    bool bit = remote_state[i] & mask;
	    data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
	  }
	}
	data->mark(DAIKIN_BIT_MARK);
	data->space(DAIKIN_MESSAGE_SPACE);
	data->mark(DAIKIN_HEADER_MARK);
	data->space(DAIKIN_HEADER_SPACE);
  }

  for (int i = 16; i < 35; i++) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
      data->mark(DAIKIN_BIT_MARK);
      bool bit = remote_state[i] & mask;
      data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
    }
  }
  data->mark(DAIKIN_BIT_MARK);
  data->space(0);

  transmit.perform();
}

uint8_t DaikinClimate::operation_mode_() {
  uint8_t operating_mode = DAIKIN_MODE_ON;
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      operating_mode |= DAIKIN_MODE_COOL;
      break;
    case climate::CLIMATE_MODE_DRY:
      operating_mode |= DAIKIN_MODE_DRY;
      break;
    case climate::CLIMATE_MODE_HEAT:
      operating_mode |= DAIKIN_MODE_HEAT;
      break;
    case climate::CLIMATE_MODE_AUTO:
      operating_mode |= DAIKIN_MODE_AUTO;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      operating_mode |= DAIKIN_MODE_FAN;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      operating_mode = DAIKIN_MODE_OFF;
      break;
  }

  return operating_mode;
}

uint8_t DaikinClimate::fan_speed_() {
  uint8_t fan_speed;
  switch (this->fan_mode) {
    case climate::CLIMATE_FAN_LOW:
      fan_speed = DAIKIN_FAN_1;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      fan_speed = DAIKIN_FAN_3;
      break;
    case climate::CLIMATE_FAN_HIGH:
      fan_speed = DAIKIN_FAN_5;
      break;
    case climate::CLIMATE_FAN_FOCUS:
      fan_speed = DAIKIN_FAN_SILENT;
      break;
    case climate::CLIMATE_FAN_AUTO:
    default:
      fan_speed = DAIKIN_FAN_AUTO;
  }

  // If swing is enabled switch first 4 bits to 1111
  return this->swing_mode == climate::CLIMATE_SWING_VERTICAL ? fan_speed | 0xF : fan_speed;
}

uint8_t DaikinClimate::temperature_() {
  // Force special temperatures depending on the mode
  switch (this->mode) {
    case climate::CLIMATE_MODE_FAN_ONLY:
      return 25;
    case climate::CLIMATE_MODE_DRY:
      return 0xc0;
    default:
      uint8_t temperature = (uint8_t) roundf(clamp(this->target_temperature, DAIKIN_TEMP_MIN, DAIKIN_TEMP_MAX));
      return temperature << 1;
  }
}

}  // namespace daikin
}  // namespace esphome
