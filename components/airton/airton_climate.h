#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

namespace esphome::airton {

// Bits of the settings byte, one per switch.
static constexpr uint8_t FLAG_DISPLAY = 0x80;
static constexpr uint8_t FLAG_IONIZER = 0x40;
static constexpr uint8_t FLAG_HEALTH = 0x10;
static constexpr uint8_t FLAG_SLEEP = 0x02;
static constexpr uint8_t FLAG_ECO = 0x01;

static constexpr uint8_t MAX_FRAME_LEN = 64;

enum class AirtonSetting : uint8_t {
  AIRTON_SETTING_DISPLAY,
  AIRTON_SETTING_IONIZER,
  AIRTON_SETTING_HEALTH,
  AIRTON_SETTING_SLEEP,
  AIRTON_SETTING_ECO,
};

/// Climate on the Airton serial bus. The ESP takes the place of the original Wi-Fi module (address 0x21):
/// it announces itself to the display (A5), polls the climate board (AB), parses the A3 state and AC poll
/// frames and sends A1 commands.
class AirtonClimate final : public uart::UARTDevice, public climate::Climate, public Component {
 public:
  AirtonClimate();

  void setup() override;
  void loop() override;
  void dump_config() override;

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  /// Set or clear one FLAG_* bit and send it to the unit; false until the unit has reported its state.
  bool set_flag(uint8_t flag, bool state);

  // Settings exposed as switches. The board shares one bit between anti-mould and auxiliary heat.
  bool set_display_state(bool state);
  bool set_ionizer_state(bool state);
  bool set_sleep_state(bool state);
  bool set_eco_state(bool state);
  bool set_health_state(bool state);
  bool set_setting(AirtonSetting setting, bool state);

#ifdef USE_SWITCH
  SUB_SWITCH(display)
  SUB_SWITCH(ionizer)
  SUB_SWITCH(sleep)
  SUB_SWITCH(eco)
  SUB_SWITCH(health)
#endif
#ifdef USE_SENSOR
  SUB_SENSOR(total_usage_time)
  SUB_SENSOR(revision)
#endif

 protected:
  void send_a5_();
  void send_ab_();
  bool can_send_command_();
  void send_command_();
  void write_frame_(uint8_t *frame, size_t len);
  void handle_byte_(uint8_t byte);
  void drop_first_byte_();
  void scan_buffer_();
  void handle_frame_(const uint8_t *data, size_t len);
  void handle_a3_(const uint8_t *data);
  void handle_ac_(const uint8_t *data);
  void publish_settings_();

  static uint16_t crc16(const uint8_t *data, size_t len);

  std::array<uint8_t, MAX_FRAME_LEN> rx_buffer_;
  uint8_t rx_pos_{0};
  uint8_t flags_{FLAG_DISPLAY};
  bool state_received_{false};
  uint8_t swing_{0};      // Raw swing byte
  uint8_t mode_bits_{0};  // Mode bits, kept while the unit is off
  uint8_t fan_raw_{0};    // Fan nibble last reported or sent
  bool fahrenheit_{false};
  uint32_t last_rx_ms_{0};
};

#ifdef USE_SWITCH
class AirtonSwitch final : public switch_::Switch, public Parented<AirtonClimate> {
 public:
  void set_setting(AirtonSetting setting) { this->setting_ = setting; }

 protected:
  void write_state(bool state) override {
    if (this->parent_->set_setting(this->setting_, state))
      this->publish_state(state);
  }

  AirtonSetting setting_{AirtonSetting::AIRTON_SETTING_DISPLAY};
};
#endif

}  // namespace esphome::airton
