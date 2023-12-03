#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace tuya_cover {

class TuyaCover : public cover::Cover, public Component {
  public:
    void setup(uart::UARTComponent *);
    void loop() override;
    cover::CoverTraits get_traits() override;
  
  protected:
    void control(const cover::CoverCall &call) override;
  
  private:
    void handle_uart();
    int parse_message(uint8_t *, int);
    int encode_message(uint8_t *, uint8_t, uint8_t *, int);

    uart::UARTComponent * _uart;
    long _message_start;
    bool _message_pending;
    int _message_size;

    long _loop_time;

    int _mcu_ready;
};

}  // namespace tuya_cover
}  // namespace esphome