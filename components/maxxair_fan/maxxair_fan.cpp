#include "maxxair_fan.h"

#include "esphome/core/log.h"

namespace esphome {
namespace maxxair_fan {

static const char *const TAG = "maxxair_fan";

static float fahrenheit_to_celsius(uint8_t fahrenheit) { return (static_cast<float>(fahrenheit) - 32.0f) * 5.0f / 9.0f; }

static uint8_t celsius_to_protocol_fahrenheit(float celsius) {
  return static_cast<uint8_t>(clamp(static_cast<int>(lroundf(celsius * 9.0f / 5.0f + 32.0f)), 29, 99));
}

void MaxxairFanComponent::setup() {
  if (this->temperature_sensor_ != nullptr) {
    this->temperature_sensor_->add_on_state_callback([this](float) { this->apply_smart_auto_(true); });
  }
  this->publish_all_();
}

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
  LOG_SELECT("  ", "Mode", this->mode_select_);
  LOG_NUMBER("  ", "Low Temperature", this->low_temperature_number_);
  LOG_NUMBER("  ", "High Temperature", this->high_temperature_number_);
  LOG_NUMBER("  ", "Minimum Speed", this->min_speed_number_);
  LOG_NUMBER("  ", "Maximum Speed", this->max_speed_number_);
  LOG_NUMBER("  ", "Built-in Auto Setpoint", this->auto_setpoint_number_);
  LOG_BUTTON("  ", "Open", this->open_button_);
  LOG_BUTTON("  ", "Close", this->close_button_);
  ESP_LOGCONFIG(TAG, "  Auto Temperature: %u F", this->state_.auto_temperature);
  ESP_LOGCONFIG(TAG, "  Smart Low Temperature: %.1f", this->smart_low_temperature_);
  ESP_LOGCONFIG(TAG, "  Smart High Temperature: %.1f", this->smart_high_temperature_);
  ESP_LOGCONFIG(TAG, "  Smart Speed Range: %u-%u", this->smart_min_speed_, this->smart_max_speed_);
}

void MaxxairFanComponent::control_fan(const fan::FanCall &call) {
  this->disable_smart_auto_();

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
  this->disable_smart_auto_();

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
  (void) kind;
  this->disable_smart_auto_();
  this->set_ceiling_fan_(state);

  this->transmit_();
  this->publish_all_();
}

void MaxxairFanComponent::control_number(MaxxairNumberKind kind, float value) {
  switch (kind) {
    case MaxxairNumberKind::LOW_TEMPERATURE:
      this->smart_low_temperature_ = value;
      break;
    case MaxxairNumberKind::HIGH_TEMPERATURE:
      this->smart_high_temperature_ = value;
      break;
    case MaxxairNumberKind::MIN_SPEED:
      this->smart_min_speed_ = clamp(static_cast<uint8_t>(lroundf(value)), uint8_t(1), uint8_t(10));
      break;
    case MaxxairNumberKind::MAX_SPEED:
      this->smart_max_speed_ = clamp(static_cast<uint8_t>(lroundf(value)), uint8_t(1), uint8_t(10));
      break;
    case MaxxairNumberKind::AUTO_SETPOINT:
      this->state_.auto_temperature = celsius_to_protocol_fahrenheit(value);
      if (this->state_.auto_mode) {
        this->transmit_();
      }
      break;
  }

  this->publish_numbers_();
  if (kind != MaxxairNumberKind::AUTO_SETPOINT) {
    this->apply_smart_auto_(true);
  }
  this->publish_all_();
}

void MaxxairFanComponent::control_mode(size_t index) {
  if (index == 1) {
    this->set_smart_auto_(false, false);
    this->set_auto_mode_(true);
    this->transmit_();
  } else if (index == 2) {
    this->set_smart_auto_(true, false);
  } else {
    const bool was_auto = this->state_.auto_mode;
    this->set_smart_auto_(false, false);
    this->state_.auto_mode = false;
    this->state_.special = false;
    if (was_auto) {
      this->transmit_();
    }
  }
  this->publish_all_();
}

void MaxxairFanComponent::press_button(MaxxairButtonKind kind) {
  this->disable_smart_auto_();
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

void MaxxairFanComponent::set_smart_auto_(bool enabled, bool turn_fan_off) {
  this->smart_auto_enabled_ = enabled;
  if (enabled) {
    this->state_.auto_mode = false;
    this->state_.special = false;
    this->apply_smart_auto_(true);
  } else if (turn_fan_off) {
    this->set_fan_off_();
  }
}

void MaxxairFanComponent::disable_smart_auto_() {
  if (this->smart_auto_enabled_) {
    this->set_smart_auto_(false, false);
  }
}

void MaxxairFanComponent::apply_smart_auto_(bool transmit) {
  if (!this->smart_auto_enabled_) {
    return;
  }
  if (this->temperature_sensor_ == nullptr || !this->temperature_sensor_->has_state()) {
    ESP_LOGD(TAG, "Smart auto is enabled but no temperature reading is available yet");
    this->publish_all_();
    return;
  }

  if (this->temperature_sensor_->state <= this->smart_low_temperature_) {
    const bool changed = this->state_.fan_on || this->state_.auto_mode || this->state_.cover_open || this->state_.special;
    this->set_fan_off_();
    if (transmit && changed) {
      this->transmit_();
    }
    this->publish_all_();
    return;
  }

  const uint8_t speed = this->calculate_smart_speed_(this->temperature_sensor_->state);
  const bool changed = !this->state_.fan_on || this->state_.fan_speed != speed * 10 || this->state_.auto_mode ||
                       !this->state_.cover_open || this->state_.special;
  this->state_.fan_on = true;
  this->state_.fan_speed = speed * 10;
  this->state_.fan_exhaust = true;
  this->state_.cover_open = true;
  this->state_.auto_mode = false;
  this->state_.special = false;

  if (transmit && changed) {
    this->transmit_();
  }
  this->publish_all_();
}

uint8_t MaxxairFanComponent::calculate_smart_speed_(float temperature) const {
  const float low = this->smart_low_temperature_;
  const float high = this->smart_high_temperature_;
  const uint8_t min_speed = std::min(this->smart_min_speed_, this->smart_max_speed_);
  const uint8_t max_speed = std::max(this->smart_min_speed_, this->smart_max_speed_);

  if (high <= low) {
    return max_speed;
  }
  if (temperature <= low) {
    return min_speed;
  }
  if (temperature >= high) {
    return max_speed;
  }

  const float ratio = (temperature - low) / (high - low);
  const auto speed = static_cast<uint8_t>(lroundf(ratio * (max_speed - min_speed) + min_speed));
  return clamp(speed, min_speed, max_speed);
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
  this->publish_numbers_();
  this->publish_mode_();
}

void MaxxairFanComponent::publish_numbers_() {
  if (this->low_temperature_number_ != nullptr) {
    this->low_temperature_number_->publish_from_parent(this->smart_low_temperature_);
  }
  if (this->high_temperature_number_ != nullptr) {
    this->high_temperature_number_->publish_from_parent(this->smart_high_temperature_);
  }
  if (this->min_speed_number_ != nullptr) {
    this->min_speed_number_->publish_from_parent(this->smart_min_speed_);
  }
  if (this->max_speed_number_ != nullptr) {
    this->max_speed_number_->publish_from_parent(this->smart_max_speed_);
  }
  if (this->auto_setpoint_number_ != nullptr) {
    this->auto_setpoint_number_->publish_from_parent(fahrenheit_to_celsius(this->state_.auto_temperature));
  }
}

void MaxxairFanComponent::publish_mode_() {
  if (this->mode_select_ == nullptr) {
    return;
  }
  if (this->smart_auto_enabled_) {
    this->mode_select_->publish_from_parent(2);
  } else if (this->state_.auto_mode) {
    this->mode_select_->publish_from_parent(1);
  } else {
    this->mode_select_->publish_from_parent(0);
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
  this->publish_state(state.fan_on && state.special && !state.auto_mode && !state.cover_open);
}

}  // namespace maxxair_fan
}  // namespace esphome
