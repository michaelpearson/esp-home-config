#include "esphome/core/log.h"
#include "tuya_cover.h"

namespace esphome {
namespace tuya_cover {

static const char *TAG = "tuya_cover.cover";

using namespace esphome::cover;

static const uint8_t STATUS[] = { 0x55, 0xaa, 0x00, 0x08, 0x00, 0x00, 0x07 };
static const uint8_t HB[] = { 0x55, 0xaa, 0x00, 0x00, 0x00, 0x00, 0xff };

static const uint8_t PAIRING_MODE_RESP[] = { 0x55, 0xaa, 0x00, 0x05, 0x00, 0x00, 0x04 };

static const uint8_t QUERY_PRODUCT[] = { 0x55, 0xaa, 0x00, 0x01, 0x00, 0x00, 0x00 };
static const uint8_t QUERY_WORKING_MODE[] = { 0x55, 0xaa, 0x00, 0x02, 0x00, 0x00, 0x01 };
static const uint8_t WIFI_STATUS_1[] = { 0x55, 0xaa, 0x00, 0x03, 0x00, 0x01, 0x00, 0x03 };
static const uint8_t WIFI_STATUS_3[] = { 0x55, 0xaa, 0x00, 0x03, 0x00, 0x01, 0x02, 0x05 };
static const uint8_t WIFI_STATUS_4[] = { 0x55, 0xaa, 0x00, 0x03, 0x00, 0x01, 0x03, 0x06 };
static const uint8_t WIFI_STATUS_5[] = { 0x55, 0xaa, 0x00, 0x03, 0x00, 0x01, 0x04, 0x07 };


void TuyaCover::setup(uart::UARTComponent * uart) {
  _uart = uart;
}

void TuyaCover::loop() {
  handle_uart();

  if (millis() < 15000) {
    return;
  }

  if ((millis() - _loop_time) > 1000) {
    _loop_time = millis();
    _uart->write_array(STATUS, 7);
  }
}

void TuyaCover::handle_uart() {
  // Wait 50ms before parsing a message.
  int len = _uart->available();
  if (len && !_message_pending) {
    _message_pending = true;
    _message_start = millis();
    _message_size = len;
    return;
  }

  if (_message_pending && len > _message_size) {
    _message_size = len;
    return;
  }

  if (_message_pending && millis() - _message_start < 10) {
    return;
  }

  if (!_message_pending) {
    return;
  }

  _message_pending = false;

  uint8_t buff[1024];
  _uart->read_array(buff, len);

  int index = 0;
  while (index < len) {
    index += parse_message(buff + index, len - index);
  }
}

int TuyaCover::parse_message(uint8_t * message, int max_length) {
  if (message[0] != 0x55) {
    return 1;
  }
  if (message[1] != 0xAA) {
    return 1;
  }
  auto version = message[2];
  auto command = message[3];
  auto length = (message[4] << 8) | message[5];
  // Data is 6 until (6 + length)
  uint8_t * data = message + 6;
  auto checksum = message[6 + length];

  int sum = 0;
  for (int a = 0; a < (6 + length); a++) {
    sum += message[a];
  }
  if ((sum % 256) != checksum) {
    ESP_LOGW(TAG, "Checksum invalid %d %d", sum % 256, checksum);
    return 6 + length;
  }

  // Report wifi status
  if (command == 0x03) {
    //ESP_LOGI(TAG, "Wifi status response");
    if (_mcu_ready == 0) {
      _uart->write_array(WIFI_STATUS_3, 8);
      _mcu_ready++;
    }
    if (_mcu_ready == 1) {
      _uart->write_array(WIFI_STATUS_4, 8);
      _mcu_ready++;
    }
    if (_mcu_ready == 2) {
      _uart->write_array(WIFI_STATUS_5, 8);
      _mcu_ready++;
    }
  }

  // Enter pairing mode
  if (command == 0x05) {
    //ESP_LOGI(TAG, "Enter pairing mode");
    _uart->write_array(PAIRING_MODE_RESP, 7);
    _uart->write_array(WIFI_STATUS_1, 8);
    _mcu_ready = 0;
  }

  // Status update
  if (command == 0x07) {
    auto dp_id = data[0];
    auto dp_type = data[1];
    auto dp_len = (data[2] << 8) | data[3];
    auto dp_value = data + 4;

    if (dp_id == 3 && dp_type == 2) {
      position = ((dp_value[0] << 24) | (dp_value[1] << 16) | (dp_value[2] << 8) | dp_value[3]) / 100.0;
      publish_state();
    }

    if (dp_id == 0x01) {
      switch(dp_value[0]) {
        case 0x00:
          current_operation = CoverOperation::COVER_OPERATION_CLOSING;
          break;
        case 0x01:
          // ESP_LOGI(TAG, "Set idle");
          current_operation = CoverOperation::COVER_OPERATION_IDLE;
          break;
        case 0x02:
          current_operation = CoverOperation::COVER_OPERATION_OPENING;
          break;
      }
      publish_state();
      // ESP_LOGI(TAG, "DP 1: %d", dp_value[0]);
    }
  }
  return 6 + length;
}

int TuyaCover::encode_message(uint8_t * buffer, uint8_t command, uint8_t * data, int data_len) {
  buffer[0] = 0x55;
  buffer[1] = 0xAA;
  buffer[2] = 0x00;
  buffer[3] = command;
  buffer[4] = data_len >> 8;
  buffer[5] = 0xFF & data_len;
  for (int a = 0; a < data_len; a++) {
    buffer[6 + a] = data[a];
  }
  int checksum = 0;
  for (int a = 0; a < (5 + data_len + 1); a++) {
    checksum += buffer[a];
  }
  buffer[5 + data_len + 1] = checksum;
  return 5 + data_len + 2;
}

cover::CoverTraits TuyaCover::get_traits() {
    auto traits = cover::CoverTraits();
    traits.set_is_assumed_state(false);
    traits.set_supports_position(true);
    traits.set_supports_tilt(false);
    return traits;
}

void TuyaCover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    uint8_t data[] = { 0x01, 0x04, 0x00, 0x01, 0x01 }; 
    uint8_t buffer[1024];
    uint8_t len = encode_message(buffer, 0x06, data, 5);
    _uart->write_array(buffer, len);
  } else if (call.get_position().has_value()) {
    auto position = *call.get_position();
    uint8_t percentage = (uint8_t)(position * 100);
    uint8_t data[] = { 0x02, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, percentage }; 
    uint8_t buffer[1024];
    uint8_t len = encode_message(buffer, 0x06, data, 8);
    _uart->write_array(buffer, len);
  }
}

}  // namespace tuya_cover
}  // namespace esphome
