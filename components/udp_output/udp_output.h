#pragma once

#include <set>

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/udp/udp.h"

namespace esphome {
namespace udp_output {

class UdpOutput : public output::FloatOutput, public Component {
  public:
    void setup(udp::Udp *, output::BinaryOutput *, const std::set<int> &channels);
    virtual void write_state(float state) override;
  private:
    udp::Udp * _udp;
    std::set<int> _channels;
    output::BinaryOutput * _passthrough;
};


} //namespace udp_output
} //namespace esphome