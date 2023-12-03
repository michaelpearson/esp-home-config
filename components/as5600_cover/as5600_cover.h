#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"

#include <Wire.h>
#include <AS5600.h>

namespace esphome {
namespace as5600_cover {

class AS5600Cover : public cover::Cover, public Component {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    cover::CoverTraits get_traits() override;
  
  protected:
    void control(const cover::CoverCall &call) override;
  
  private:

    enum State : uint32_t {
      OPENING = 0,
      CLOSING = 1,
      STOPPED_OPENING = 2,
      STOPPED_CLOSING = 3,
    };

    State internal_state { State::STOPPED_CLOSING };
    AS5600 as5600;
    long last_movement_at = 0;
    long previous_raw_position = 0;
    long raw_position = 0;

    long state_change_lockout_until = 0;
    long pin_reset_time = 0;

    float stop_at = 0;
    long stop_at_timout = 0;

    long last_debug = 0;
};

}  // namespace as5600_cover
}  // namespace esphome