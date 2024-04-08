#include "esphome/core/log.h"
#include "as5600_cover.h"

namespace esphome {
namespace as5600_cover {

static const char *TAG = "as5600_cover.cover";

using namespace esphome::cover;

void AS5600Cover::setup() {
  as5600.begin(18, 19);
  if (!as5600.isConnected()) {
    ESP_LOGE(TAG, "Not connected");
  }
  position = 0;
  current_operation = COVER_OPERATION_IDLE;
  as5600.resetCumulativePosition(0);
}

void AS5600Cover::loop() {
  long now = millis();
  raw_position = as5600.getCumulativePosition();
  position = (int)(raw_position / 261.000) / 100.0;
  if (position < 0.05) {
    position = 0;
  }

  bool publish = false;
  auto delta = raw_position - previous_raw_position;
  if (delta > 100 || delta < -100) {
    previous_raw_position = raw_position;
    publish = true;
    internal_state = delta > 0 ? OPENING : CLOSING;
    last_movement_at = now;
  } else {
    if ((now - last_movement_at) > 500) {
      if (internal_state == OPENING) {
        internal_state = STOPPED_OPENING;
      }
      if (internal_state == CLOSING) {
        internal_state = STOPPED_CLOSING;
      } 
    }
  }

  if (now - last_debug > 1000) {
    last_debug = now;
    ESP_LOGI(TAG, "published state %s, position %f, raw position %d", cover_operation_to_str(current_operation), position, raw_position);
  }

  if (now < stop_at_timout) {
    if (now > state_change_lockout_until) {
      if (internal_state == OPENING && current_operation == COVER_OPERATION_OPENING
          || internal_state == CLOSING && current_operation == COVER_OPERATION_CLOSING
          || internal_state >= 2 && current_operation == COVER_OPERATION_IDLE
      ) {
        // Correct state.
        auto error = stop_at - position;
        if (stop_at == 1 || stop_at == 0) {
          stop_at_timout = 0;
        } else if (error < 0.05 && error > -0.05) {
          // Reached the endpoint.
          current_operation = COVER_OPERATION_IDLE;
          state_change_lockout_until = now + 2000;
          stop_at_timout = 0;
          pin_reset_time = now + 200;
          digitalWrite(33, HIGH);
          ESP_LOGI(TAG, "Reached the target with error %f", error);
        }
      } else {
        // Advance state.
        state_change_lockout_until = now + 2000;
        pin_reset_time = now + 200;
        digitalWrite(33, HIGH);
      }
    }
  } else {
    auto previous_operation = current_operation;
    if (internal_state == OPENING) {
      current_operation = COVER_OPERATION_OPENING;
    } else if (internal_state == CLOSING) {
      current_operation = COVER_OPERATION_CLOSING;
    } else {
      current_operation = COVER_OPERATION_IDLE;
    }

    if (current_operation != previous_operation) {
      publish = true;
    }
  }

  if (publish) {
    publish_state();
  }

  if (pin_reset_time > 0 && pin_reset_time < now) {
    digitalWrite(33, LOW);
    pin_reset_time = 0;
  }
}

void AS5600Cover::dump_config() {}

cover::CoverTraits AS5600Cover::get_traits() {
    auto traits = cover::CoverTraits();
    traits.set_is_assumed_state(false);
    traits.set_supports_position(true);
    traits.set_supports_tilt(false);
    return traits;
}

void AS5600Cover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    current_operation = COVER_OPERATION_IDLE;
    stop_at_timout = millis() + 6000; // 6 seconds to stop.
  } else if (call.get_position().has_value()) {
    stop_at = *call.get_position();
    if (stop_at > 0.9) {
      stop_at = 1;
    }
    if (stop_at < 0.1) {
      stop_at = 0;
    }
    if (stop_at == 0 || stop_at == 1) {
      stop_at_timout = millis() + 6000; // 6 seconds to get to the correct state.
    } else {
      stop_at_timout = millis() + 30000; // 30 seconds to get to a position.
    }
    current_operation = stop_at > position ? COVER_OPERATION_OPENING : COVER_OPERATION_CLOSING;
    ESP_LOGI(TAG, "Targeting %f", stop_at);
    publish_state();
  }
}

}  // namespace as5600_cover
}  // namespace esphome
