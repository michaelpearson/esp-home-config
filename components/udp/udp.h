#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/output/binary_output.h"
#include <WiFi.h>
#include <WiFiUdp.h>

namespace esphome {
namespace udp {
class Udp : public Component {
  public:
    void setup(const char *, int);
    void loop() override;
    void set_level(int, float);
  private:
    LwIPUDP _udp;
    const char * _host;
    int _port;

    uint16_t _levels[4] = { false, false, false, false };
    bool _update_required = false;

    IPAddress _address;
    long _resolve_at = 0;
};


} //namespace udp
} //namespace esphome