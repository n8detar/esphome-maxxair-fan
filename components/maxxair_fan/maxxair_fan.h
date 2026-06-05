#pragma once

#include "esphome/components/button/button.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/maxxfan_protocol/maxxfan_protocol.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace maxxair_fan {

class MaxxairFanEntity;
class MaxxairFanCover;
class MaxxairFanSwitch;
class MaxxairFanButton;

enum class MaxxairSwitchKind : uint8_t {
  CEILING_FAN = 0,
  AUTO_MODE = 1,
};

enum class MaxxairButtonKind : uint8_t {
  OPEN = 0,
  CLOSE = 1,
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
  void set_auto_temperature(uint8_t auto_temperature) { this->state_.auto_temperature = auto_temperature; }
  void set_fan(MaxxairFanEntity *fan) { this->fan_ = fan; }
  void set_cover(MaxxairFanCover *cover) { this->cover_ = cover; }
  void set_ceiling_fan_switch(MaxxairFanSwitch *ceiling_fan_switch) { this->ceiling_fan_switch_ = ceiling_fan_switch; }
  void set_auto_mode_switch(MaxxairFanSwitch *auto_mode_switch) { this->auto_mode_switch_ = auto_mode_switch; }
  void set_open_button(MaxxairFanButton *open_button) { this->open_button_ = open_button; }
  void set_close_button(MaxxairFanButton *close_button) { this->close_button_ = close_button; }

  void control_fan(const fan::FanCall &call);
  void control_cover(const cover::CoverCall &call);
  void control_switch(MaxxairSwitchKind kind, bool state);
  void press_button(MaxxairButtonKind kind);

 protected:
  void transmit_();
  void publish_all_();
  void set_cover_open_(bool open);
  void set_fan_off_();
  void set_ceiling_fan_(bool enabled);
  void set_auto_mode_(bool enabled);

  remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};
  MaxxairFanEntity *fan_{nullptr};
  MaxxairFanCover *cover_{nullptr};
  MaxxairFanSwitch *ceiling_fan_switch_{nullptr};
  MaxxairFanSwitch *auto_mode_switch_{nullptr};
  MaxxairFanButton *open_button_{nullptr};
  MaxxairFanButton *close_button_{nullptr};
  MaxxairFanState state_{};
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

}  // namespace maxxair_fan
}  // namespace esphome
