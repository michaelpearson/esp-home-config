#include "esphome/core/log.h"
#include "udp_output.h"

namespace esphome {
namespace udp_output {

void UdpOutput::setup(udp::Udp * udp, output::BinaryOutput * passthrough, const std::set<int> &channels) {
    _udp = udp;
    _passthrough = passthrough;
    _channels = channels;
}

void UdpOutput::write_state(float state) {
    for (auto channel : _channels) {
        _udp->set_level(channel, state);
    }
    _passthrough->set_state(state > 0);
}

} //namespace udp_output
} //namespace esphome