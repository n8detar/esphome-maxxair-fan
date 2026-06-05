#include "maxxair_fan.h"

#include "esphome/core/log.h"

namespace esphome {
namespace maxxair_fan {

static const char *const TAG = "maxxair_fan";

void MaxxairFanComponent::setup() { this->publish_all_(); }

void MaxxairFanComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MaxxAir Fan:");
  if (this->transmitter_ == nullptr) {
    ESP_LOGCONFIG(TAG, "  Remote Transmitter: missing");
  }
  if (this->fan_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Fan '%s'", this->fan_->get_name().c_str());
  }
  LOG_COVER("  ", "Cover", this->cover_);
  LOG_SWITCH("  ", "Ceiling Fan", this->ceiling_fan_switch_);
  LOG_SWITCH("  ", "Auto Mode", this->auto_mode_switch_);
  LOG_BUTTON("  ", "Open", this->open_button_);
  LOG_BUTTON("  ", "Close", this->close_button_);
  ESP_LOGCONFIG(TAG, "  Auto Temperature: %u F", this->state_.auto_temperature);
}

void MaxxairFanComponent::control_fan(const fan::FanCall &call) {
  bool next_on = this->state_.fan_on;
  if (call.get_state().has_value()) {
    next_on = *call.get_state();
  }

  if (call.get_speed().has_value()) {
    const int requested_speed = clamp(*call.get_speed(), 1, 10);
    this->state_.fan_speed = requested_speed * 10;
    next_on = true;
  }

  if (call.get_direction().has_value()) {
    this->state_.fan_exhaust = *call.get_direction() == fan::FanDirection::FORWARD;
  }

  if (next_on) {
    this->state_.fan_on = true;
    this->state_.auto_mode = false;
    if (!this->state_.special) {
      this->state_.cover_open = true;
    }
  } else {
    this->set_fan_off_();
  }

  this->transmit_();
  this->publish_all_();
}

void MaxxairFanComponent::control_cover(const cover::CoverCall &call) {
  if (call.get_stop()) {
    return;
  }
  if (!call.get_position().has_value()) {
    return;
  }

  if (*call.get_position() > 0.5f) {
    this->set_cover_open_(true);
  } else {
    this->set_cover_open_(false);
  }

  this->transmit_();
  this->publish_all_();
}

void MaxxairFanComponent::control_switch(MaxxairSwitchKind kind, bool state) {
  if (kind == MaxxairSwitchKind::CEILING_FAN) {
    this->set_ceiling_fan_(state);
  } else {
    this->set_auto_mode_(state);
  }

  this->transmit_();
  this->publish_all_();
}

void MaxxairFanComponent::press_button(MaxxairButtonKind kind) {
  this->set_cover_open_(kind == MaxxairButtonKind::OPEN);
  this->transmit_();
  this->publish_all_();
}

void MaxxairFanComponent::set_cover_open_(bool open) {
  this->state_.cover_open = open;

  if (open) {
    this->state_.special = this->state_.auto_mode;
    return;
  }

  if (this->state_.fan_on) {
    this->state_.auto_mode = false;
    this->state_.special = true;
    this->state_.fan_exhaust = false;
  } else {
    this->state_.special = false;
  }
}

void MaxxairFanComponent::set_fan_off_() {
  this->state_.fan_on = false;
  this->state_.auto_mode = false;
  this->state_.special = false;
  this->state_.cover_open = false;
}

void MaxxairFanComponent::set_ceiling_fan_(bool enabled) {
  if (enabled) {
    this->state_.fan_on = true;
    this->state_.fan_exhaust = false;
    this->state_.cover_open = false;
    this->state_.auto_mode = false;
    this->state_.special = true;
  } else if (this->state_.fan_on) {
    this->state_.cover_open = true;
    this->state_.auto_mode = false;
    this->state_.special = false;
  }
}

void MaxxairFanComponent::set_auto_mode_(bool enabled) {
  if (enabled) {
    this->state_.fan_on = true;
    this->state_.cover_open = true;
    this->state_.auto_mode = true;
    this->state_.special = true;
  } else {
    this->set_fan_off_();
  }
}

void MaxxairFanComponent::transmit_() {
  if (this->transmitter_ == nullptr) {
    ESP_LOGW(TAG, "Cannot transmit MaxxAir command without a remote_transmitter");
    return;
  }

  remote_base::MaxxfanData data{};
  data.fan_on = this->state_.fan_on;
  data.fan_speed = this->state_.fan_speed;
  data.fan_exhaust = this->state_.fan_exhaust;
  data.cover_open = this->state_.cover_open;
  data.auto_mode = this->state_.auto_mode;
  data.auto_temperature = this->state_.auto_temperature;
  data.special = this->state_.special;
  data.warn = false;
  this->transmitter_->transmit<remote_base::MaxxfanProtocol>(data);
}

void MaxxairFanComponent::publish_all_() {
  if (this->fan_ != nullptr) {
    this->fan_->update_state(this->state_);
  }
  if (this->cover_ != nullptr) {
    this->cover_->update_state(this->state_);
  }
  if (this->ceiling_fan_switch_ != nullptr) {
    this->ceiling_fan_switch_->update_state(this->state_);
  }
  if (this->auto_mode_switch_ != nullptr) {
    this->auto_mode_switch_->update_state(this->state_);
  }
}

fan::FanTraits MaxxairFanEntity::get_traits() {
  fan::FanTraits traits(false, true, true, 10);
  this->wire_preset_modes_(traits);
  return traits;
}

void MaxxairFanEntity::update_state(const MaxxairFanState &state) {
  this->state = state.fan_on;
  this->speed = clamp(static_cast<int>(state.fan_speed / 10), 1, 10);
  this->direction = state.fan_exhaust ? fan::FanDirection::FORWARD : fan::FanDirection::REVERSE;
  this->publish_state();
}

cover::CoverTraits MaxxairFanCover::get_traits() {
  cover::CoverTraits traits;
  traits.set_is_assumed_state(false);
  traits.set_supports_position(false);
  traits.set_supports_stop(false);
  traits.set_supports_toggle(false);
  return traits;
}

void MaxxairFanCover::update_state(const MaxxairFanState &state) {
  this->position = state.cover_open ? cover::COVER_OPEN : cover::COVER_CLOSED;
  this->current_operation = cover::COVER_OPERATION_IDLE;
  this->publish_state();
}

void MaxxairFanSwitch::update_state(const MaxxairFanState &state) {
  const bool next_state = this->kind_ == MaxxairSwitchKind::CEILING_FAN
                              ? state.fan_on && state.special && !state.auto_mode && !state.cover_open
                              : state.auto_mode;
  this->publish_state(next_state);
}

}  // namespace maxxair_fan
}  // namespace esphome
