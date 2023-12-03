#include "esphome/core/log.h"
#include "tuya_cover.h"

namespace esphome {
namespace tuya_cover {

static const char *TAG = "tuya_cover.cover";

using namespace esphome::cover;

static const uint8_t STATUS[] = { 0x55, 0xaa, 0x00, 0x08, 0x00, 0x00, 0x07 };
static const uint8_t PAIRING_MODE_RESP[] = { 0x55, 0xaa, 0x00, 0x05, 0x00, 0x00, 0x04 };
static const uint8_t WIFI_STATUS_1[] = { 0x55, 0xaa, 0x00, 0x03, 0x00, 0x01, 0x00, 0x03 };
static const uint8_t WIFI_STATUS_3[] = { 0x55, 0xaa, 0x00, 0x03, 0x00, 0x01, 0x02, 0x05 };
static const uint8_t WIFI_STATUS_4[] = { 0x55, 0xaa, 0x00, 0x03, 0x00, 0x01, 0x03, 0x06 };
static const uint8_t WIFI_STATUS_5[] = { 0x55, 0xaa, 0x00, 0x03, 0x00, 0x01, 0x04, 0x07 };


void TuyaCover::setup(uart::UARTComponent * uart) {
  _uart = uart;
}

void TuyaCover::loop() {
  auto old_position = position;
  auto old_direction = _direction;
  auto old_operation = current_operation;

  handle_uart();

  if (old_position != position) {
    _moving_at = millis();
  }

  if (millis() - _moving_at > 1500) {
    current_operation = COVER_OPERATION_IDLE;
  } else {
    current_operation = _direction ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
  }
  if (old_position != position || old_direction != _direction || old_operation != current_operation) {
    publish_state();
  }

  if (millis() < 10000) {
    return;
  }

  if (_last_status_received == 0) {
    return;
  }

  long period = millis() - _last_status_received;
  if ((current_operation != COVER_OPERATION_IDLE && period > 200) || period > 1000) {
    ESP_LOGI(TAG, "Requesting status");
    _uart->write_array(STATUS, 7);
    _last_status_received = 0;
  }
}

void TuyaCover::handle_uart() {
  // Wait 10ms before parsing a message.
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

  // Handle WiFi connection process. The MCU expect each status response in sequence.
  // MCU -> Module. Reset the WiFi connection.
  if (command == 0x05) {
    // Respond to MCU asking for WiFi reset.
    _uart->write_array(PAIRING_MODE_RESP, 7);

    // Let the MCU know that we're in pairing mode.
    _uart->write_array(WIFI_STATUS_1, 8);
    _dummy_connection_state = 0;
  }

  // Perform a sequence of pairing messages.
  if (command == 0x03) {
    if (_dummy_connection_state == 0) {
      _uart->write_array(WIFI_STATUS_3, 8);
      _dummy_connection_state++;
    }
    if (_dummy_connection_state == 1) {
      _uart->write_array(WIFI_STATUS_4, 8);
      _dummy_connection_state++;
    }
    if (_dummy_connection_state == 2) {
      _uart->write_array(WIFI_STATUS_5, 8);
      _dummy_connection_state = 0;
    }
  }

  // Status update response.
  if (command == 0x07) {
    uint8_t dp_id = data[0];
    uint8_t * dp_value = data + 4;
    if (dp_id == 3) {
      // Position is an integer between 0 - 100. The value is reported as an i32.
      position = dp_value[3] / 100.0;
    }
    if (dp_id == 0x07) {
      _direction = dp_value[0];
    }
    _last_status_received = millis();
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
  static uint8_t buffer[1024];
  if (call.get_stop()) {
    uint8_t data[] = { 0x01, 0x04, 0x00, 0x01, 0x01 }; 
    uint8_t len = encode_message(buffer, 0x06, data, 5);
    _uart->write_array(buffer, len);
  } else if (call.get_position().has_value()) {
    uint8_t percentage = (uint8_t)(*call.get_position() * 100);
    uint8_t data[] = { 0x02, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, percentage }; 
    uint8_t len = encode_message(buffer, 0x06, data, 8);
    _uart->write_array(buffer, len);
  }
}

}  // namespace tuya_cover
}  // namespace esphome
