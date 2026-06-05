#pragma once

#include "esphome/components/button/button.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/maxxfan_protocol/maxxfan_protocol.h"
#include "esphome/components/number/number.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

#include <algorithm>
#include <cmath>

namespace esphome {
namespace maxxair_fan {

class MaxxairFanEntity;
class MaxxairFanCover;
class MaxxairFanSwitch;
class MaxxairFanButton;
class MaxxairFanNumber;
class MaxxairFanModeSelect;

enum class MaxxairSwitchKind : uint8_t {
  CEILING_FAN = 0,
};

enum class MaxxairButtonKind : uint8_t {
  OPEN = 0,
  CLOSE = 1,
};

enum class MaxxairNumberKind : uint8_t {
  LOW_TEMPERATURE = 0,
  HIGH_TEMPERATURE = 1,
  MIN_SPEED = 2,
  MAX_SPEED = 3,
  AUTO_SETPOINT = 4,
};

struct MaxxairFanState {
  bool fan_on{false};
  uint8_t fan_speed{10};
  bool fan_exhaust{true};
  bool cover_open{false};
  bool auto_mode{false};
  bool special{false};
  uint8_t auto_temperature{78};
};

class MaxxairFanComponent : public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) { this->transmitter_ = transmitter; }
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { this->temperature_sensor_ = temperature_sensor; }
  void set_auto_temperature(uint8_t auto_temperature) { this->state_.auto_temperature = auto_temperature; }
  void set_fan_off_below_low_temperature(bool fan_off_below_low_temperature) {
    this->fan_off_below_low_temperature_ = fan_off_below_low_temperature;
  }
  void set_fan(MaxxairFanEntity *fan) { this->fan_ = fan; }
  void set_cover(MaxxairFanCover *cover) { this->cover_ = cover; }
  void set_ceiling_fan_switch(MaxxairFanSwitch *ceiling_fan_switch) { this->ceiling_fan_switch_ = ceiling_fan_switch; }
  void set_low_temperature_number(MaxxairFanNumber *low_temperature_number) {
    this->low_temperature_number_ = low_temperature_number;
  }
  void set_high_temperature_number(MaxxairFanNumber *high_temperature_number) {
    this->high_temperature_number_ = high_temperature_number;
  }
  void set_min_speed_number(MaxxairFanNumber *min_speed_number) { this->min_speed_number_ = min_speed_number; }
  void set_max_speed_number(MaxxairFanNumber *max_speed_number) { this->max_speed_number_ = max_speed_number; }
  void set_auto_setpoint_number(MaxxairFanNumber *auto_setpoint_number) {
    this->auto_setpoint_number_ = auto_setpoint_number;
  }
  void set_mode_select(MaxxairFanModeSelect *mode_select) { this->mode_select_ = mode_select; }
  void set_open_button(MaxxairFanButton *open_button) { this->open_button_ = open_button; }
  void set_close_button(MaxxairFanButton *close_button) { this->close_button_ = close_button; }

  void control_fan(const fan::FanCall &call);
  void control_cover(const cover::CoverCall &call);
  void control_switch(MaxxairSwitchKind kind, bool state);
  void control_number(MaxxairNumberKind kind, float value);
  void control_mode(size_t index);
  void press_button(MaxxairButtonKind kind);

 protected:
  void transmit_();
  void publish_all_();
  void publish_numbers_();
  void publish_mode_();
  void set_cover_open_(bool open);
  void set_fan_off_();
  void set_ceiling_fan_(bool enabled);
  void set_auto_mode_(bool enabled);
  void set_smart_auto_(bool enabled, bool turn_fan_off);
  void disable_smart_auto_();
  void apply_smart_auto_(bool transmit);
  uint8_t calculate_smart_speed_(float temperature) const;

  remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  MaxxairFanEntity *fan_{nullptr};
  MaxxairFanCover *cover_{nullptr};
  MaxxairFanSwitch *ceiling_fan_switch_{nullptr};
  MaxxairFanNumber *low_temperature_number_{nullptr};
  MaxxairFanNumber *high_temperature_number_{nullptr};
  MaxxairFanNumber *min_speed_number_{nullptr};
  MaxxairFanNumber *max_speed_number_{nullptr};
  MaxxairFanNumber *auto_setpoint_number_{nullptr};
  MaxxairFanModeSelect *mode_select_{nullptr};
  MaxxairFanButton *open_button_{nullptr};
  MaxxairFanButton *close_button_{nullptr};
  MaxxairFanState state_{};
  bool smart_auto_enabled_{false};
  bool fan_off_below_low_temperature_{true};
  float smart_low_temperature_{23.0f};
  float smart_high_temperature_{27.0f};
  uint8_t smart_min_speed_{1};
  uint8_t smart_max_speed_{10};
};

class MaxxairFanEntity : public fan::Fan, public Component {
 public:
  explicit MaxxairFanEntity(MaxxairFanComponent *parent) : parent_(parent) {}
  fan::FanTraits get_traits() override;
  void update_state(const MaxxairFanState &state);

 protected:
  void control(const fan::FanCall &call) override { this->parent_->control_fan(call); }
  MaxxairFanComponent *parent_;
};

class MaxxairFanCover : public cover::Cover, public Component {
 public:
  explicit MaxxairFanCover(MaxxairFanComponent *parent) : parent_(parent) {}
  cover::CoverTraits get_traits() override;
  void update_state(const MaxxairFanState &state);

 protected:
  void control(const cover::CoverCall &call) override { this->parent_->control_cover(call); }
  MaxxairFanComponent *parent_;
};

class MaxxairFanSwitch : public switch_::Switch, public Component {
 public:
  MaxxairFanSwitch(MaxxairFanComponent *parent, uint8_t kind)
      : parent_(parent), kind_(static_cast<MaxxairSwitchKind>(kind)) {}
  bool assumed_state() override { return false; }
  void update_state(const MaxxairFanState &state);

 protected:
  void write_state(bool state) override { this->parent_->control_switch(this->kind_, state); }
  MaxxairFanComponent *parent_;
  MaxxairSwitchKind kind_;
};

class MaxxairFanButton : public button::Button, public Component {
 public:
  MaxxairFanButton(MaxxairFanComponent *parent, uint8_t kind)
      : parent_(parent), kind_(static_cast<MaxxairButtonKind>(kind)) {}

 protected:
  void press_action() override { this->parent_->press_button(this->kind_); }
  MaxxairFanComponent *parent_;
  MaxxairButtonKind kind_;
};

class MaxxairFanNumber : public number::Number, public Component {
 public:
  MaxxairFanNumber(MaxxairFanComponent *parent, uint8_t kind)
      : parent_(parent), kind_(static_cast<MaxxairNumberKind>(kind)) {}
  void publish_from_parent(float value) { this->publish_state(value); }

 protected:
  void control(float value) override { this->parent_->control_number(this->kind_, value); }
  MaxxairFanComponent *parent_;
  MaxxairNumberKind kind_;
};

class MaxxairFanModeSelect : public select::Select, public Component {
 public:
  explicit MaxxairFanModeSelect(MaxxairFanComponent *parent) : parent_(parent) {}
  void publish_from_parent(size_t index) { this->publish_state(index); }

 protected:
  void control(size_t index) override { this->parent_->control_mode(index); }
  MaxxairFanComponent *parent_;
};

}  // namespace maxxair_fan
}  // namespace esphome
