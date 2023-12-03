#include "esphome/core/log.h"
#include "udp.h"

namespace esphome {
namespace udp {

void Udp::setup(const char * host, int port) {
    _host = host;
    _port = port;
}

void Udp::loop() {
    if (_update_required) {
        if (millis() > _resolve_at) {
            WiFi.hostByName(_host, _address);
            // Set the next resolution to happen in 10 seconds.
            _resolve_at = millis() + 10000;
        }
        ESP_LOGI("udp", "writing %lu %lu %lu %lu", _levels[0], _levels[1], _levels[2], _levels[3]);
        _udp.beginPacket(_address, _port);
        _udp.write((uint8_t *)_levels, 8);
        _udp.endPacket();
        _update_required = false;
    }
}

void Udp::set_level(int channel, float level) {
    _levels[channel] = (uint16_t)(level * 1024);
    _update_required = true;
}

} //namespace udp
} //namespace esphome