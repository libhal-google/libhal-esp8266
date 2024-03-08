// Copyright 2024 Khalil Estell
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <libhal-esp8266/at.hpp>

#include <algorithm>
#include <array>
#include <span>

#include <libhal-util/serial.hpp>
#include <libhal-util/serial_coroutines.hpp>
#include <libhal-util/streams.hpp>
#include <libhal-util/timeout.hpp>
#include <libhal/error.hpp>

#include "util.hpp"

namespace hal::esp8266 {

using namespace std::string_literals;

namespace {
[[nodiscard]] auto skip(hal::serial* p_serial, std::string_view p_string)
{
  return hal::skip_past(*p_serial, as_bytes(p_string));
}

void wait_for_ok(hal::serial* p_serial, timeout auto p_timeout)
{
  // No need to check the worker state because skip never returns `failure`
  try_until(skip(p_serial, ok_response), p_timeout);
}

void wait_for_reset_complete(hal::serial* p_serial, timeout auto p_timeout)
{
  // No need to check the worker state because skip never returns `failure`
  try_until(skip(p_serial, reset_complete), p_timeout);
}
}  // namespace

enum packet_manager_state : std::uint8_t
{
  expect_plus,
  expect_i,
  expect_p,
  expect_d,
  expect_comma,
  expect_digit1,
  expect_digit2,
  expect_digit3,
  expect_digit4,
  expect_colon,
  header_complete
};

at::packet_manager::packet_manager() noexcept
  : m_state(packet_manager_state::expect_plus)
  , m_length(0)
{
}

void at::packet_manager::find(hal::serial& p_serial) noexcept
{
  if (is_complete_header()) {
    return;
  }

  std::array<hal::byte, 1> byte;
  auto result = p_serial.read(byte);
  while (result.data.size() != 0) {
    update_state(byte[0]);
    if (is_complete_header()) {
      return;
    }
    result = p_serial.read(byte);
  }
}

void at::packet_manager::set_state(std::uint8_t p_state) noexcept
{
  m_state = p_state;
}

void at::packet_manager::update_state(hal::byte p_byte) noexcept
{
  char c = static_cast<char>(p_byte);
  switch (m_state) {
    case packet_manager_state::expect_plus:
      if (c == '+') {
        m_state = packet_manager_state::expect_i;
      }
      break;
    case packet_manager_state::expect_i:
      if (c == 'I') {
        m_state = packet_manager_state::expect_p;
      } else {
        m_state = packet_manager_state::expect_plus;
      }
      break;
    case packet_manager_state::expect_p:
      if (c == 'P') {
        m_state = packet_manager_state::expect_d;
      } else {
        m_state = packet_manager_state::expect_plus;
      }
      break;
    case packet_manager_state::expect_d:
      if (c == 'D') {
        m_state = packet_manager_state::expect_comma;
      } else {
        m_state = packet_manager_state::expect_plus;
      }
      break;
    case packet_manager_state::expect_comma:
      if (c == ',') {
        m_state = packet_manager_state::expect_digit1;
        m_length = 0;  // Reset the length because we're about to parse it
      } else {
        m_state = packet_manager_state::expect_plus;
      }
      break;
    case packet_manager_state::expect_digit1:
    case packet_manager_state::expect_digit2:
    case packet_manager_state::expect_digit3:
    case packet_manager_state::expect_digit4:
      if (isdigit(c)) {
        m_length = m_length * 10 + (c - '0');  // Accumulate the length
        m_state += 1;
      } else if (c == ':') {
        m_state = packet_manager_state::header_complete;
      } else {
        // It's not a digit or a ':', so this is an error
        m_state = packet_manager_state::expect_plus;
      }
      break;
    case packet_manager_state::expect_colon:
      if (c == ':') {
        m_state = packet_manager_state::header_complete;
      } else {
        m_state = packet_manager_state::expect_plus;
      }
      break;
    default:
      m_state = packet_manager_state::expect_plus;
  }
}

bool at::packet_manager::is_complete_header() noexcept
{
  return m_state == packet_manager_state::header_complete;
}

std::uint16_t at::packet_manager::packet_length() noexcept
{
  return is_complete_header() ? m_length : 0;
}

std::span<hal::byte> at::packet_manager::read_packet(
  hal::serial& p_serial,
  std::span<hal::byte> p_buffer) noexcept
{
  if (!is_complete_header()) {
    return p_buffer.first(0);
  }

  auto buffer_size = static_cast<std::uint16_t>(p_buffer.size());
  auto bytes_capable_of_reading = std::min(m_length, buffer_size);
  auto subspan = p_buffer.first(bytes_capable_of_reading);
  auto bytes_read_array = p_serial.read(subspan).data;

  m_length = m_length - bytes_read_array.size();

  if (m_length == 0) {
    reset();
  }

  return bytes_read_array;
}

void at::packet_manager::reset() noexcept
{
  m_state = packet_manager_state::expect_plus;
  m_length = 0;
}

// constructor
at::at(hal::serial& p_serial, deadline p_timeout)
  : m_serial(&p_serial)
  , m_packet_manager{}
{
  reset(p_timeout);
}

void at::reset(deadline p_timeout)
{
  // Reset the device
  write(*m_serial, "AT+RST\r\n", p_timeout);
  wait_for_reset_complete(m_serial, p_timeout);

  // Turn off echo
  write(*m_serial, "ATE0\r\n", p_timeout);
  wait_for_ok(m_serial, p_timeout);
}

// NOLINTNEXTLINE
void at::connect_to_ap(std::string_view p_ssid,
                       std::string_view p_password,
                       deadline p_timeout)
{
  // Configure as WiFi Station (client) mode
  write(*m_serial, "AT+CWMODE=1\r\n", p_timeout);
  wait_for_ok(m_serial, p_timeout);

  // Connect to wifi access point
  write(*m_serial, "AT+CWJAP=\"", p_timeout);
  write(*m_serial, p_ssid, p_timeout);
  write(*m_serial, "\",\"", p_timeout);
  write(*m_serial, p_password, p_timeout);
  write(*m_serial, "\"\r\n", p_timeout);
  wait_for_ok(m_serial, p_timeout);
}

void at::set_ip_address(std::string_view p_ip, deadline p_timeout)
{
  write(*m_serial, "AT+CIPSTA=\"", p_timeout);
  write(*m_serial, p_ip, p_timeout);
  write(*m_serial, "\"\r\n", p_timeout);
  wait_for_ok(m_serial, p_timeout);
}

bool at::is_connected_to_ap(deadline p_timeout)
{
  // Query the device to determine if it is still connected
  write(*m_serial, "AT+CWJAP?\r\n", p_timeout);

  auto find_confirm = hal::stream_find(hal::as_bytes(ap_connected));
  auto find_ok = hal::stream_find(hal::as_bytes(ok_response));

  while (hal::in_progress(find_confirm) && hal::in_progress(find_ok)) {
    std::array<hal::byte, 1> buffer;
    auto read_result = m_serial->read(buffer);

    // Pipe data into both streams
    read_result.data | find_confirm;
    read_result.data | find_ok;

    // Check if we've timed out
    p_timeout();
  }

  // We should fine the confirmation before we find the "OK" response
  if (hal::finished(find_confirm) && hal::in_progress(find_ok)) {
    // Read the last of the stream and find the OK to be sure
    wait_for_ok(m_serial, p_timeout);
    return true;
  }

  // We should fine the confirmation before we find the "OK" response
  if (hal::in_progress(find_confirm) && hal::finished(find_ok)) {
    return false;
  }

  hal::safe_throw(hal::io_error(this));
}

void at::disconnect_from_ap(deadline p_timeout)
{
  write(*m_serial, "AT+CWQAP\r\n", p_timeout);
  wait_for_ok(m_serial, p_timeout);
}

void at::connect_to_server(socket_config p_config, deadline p_timeout)
{
  const auto expected_response = hal::as_bytes(ok_response);

  std::string_view socket_type_str;

  switch (p_config.type) {
    case socket_type::tcp:
      socket_type_str = "TCP";
      break;
    case socket_type::udp:
      socket_type_str = "UDP";
      break;
  }

  // Connect to web server
  hal::write(*m_serial, "AT+CIPSTART=\"", p_timeout);
  hal::write(*m_serial, socket_type_str, p_timeout);
  hal::write(*m_serial, "\",\"", p_timeout);
  hal::write(*m_serial, p_config.domain, p_timeout);
  hal::write(*m_serial, "\",", p_timeout);
  hal::write(*m_serial, uint_to_string(p_config.port).str(), p_timeout);
  hal::write(*m_serial, "\r\n", p_timeout);
  hal::try_until(skip_past(*m_serial, expected_response), p_timeout);
}

std::span<const hal::byte> at::server_write(std::span<const hal::byte> p_data,
                                            deadline p_timeout)
{
  using namespace std::literals;

  // Trim data to the maximum packet length.
  p_data = p_data.subspan(0, maximum_transmit_packet_size);

  hal::write(*m_serial, "AT+CIPSEND=", p_timeout);
  hal::write(*m_serial, uint_to_string(p_data.size()).str(), p_timeout);
  hal::write(*m_serial, "\r\n", p_timeout);
  try_until(skip_past(*m_serial, hal::as_bytes(">"sv)), p_timeout);
  hal::write(*m_serial, p_data, p_timeout);

  auto find_packet = hal::stream_find(hal::as_bytes(start_of_packet));
  auto find_send_finish = hal::stream_find(hal::as_bytes(send_finished));

  while (hal::in_progress(find_packet) && hal::in_progress(find_send_finish)) {
    std::array<hal::byte, 1> buffer;
    auto read_result = m_serial->read(buffer);

    // Pipe data into both streams
    read_result.data | find_packet;
    read_result.data | find_send_finish;

    // Check if we've timed out
    p_timeout();
  }

  // If we found the start of a packet, we need to set the state of the packet
  // manager to expect the first digit.
  if (hal::finished(find_packet)) {
    m_packet_manager.set_state(packet_manager_state::expect_digit1);
  }

  return p_data;
}

bool at::is_connected_to_server(deadline p_timeout)
{
  constexpr std::string_view response_status = "STATUS";
  constexpr std::string_view response_start = "+CIPSTATUS:";

  // Query the device to determine if it is still connected
  write(*m_serial, "AT+CIPSTATUS\r\n", p_timeout);

  auto find_status = hal::stream_find(hal::as_bytes(response_status));
  auto find_start = hal::stream_find(hal::as_bytes(response_start));
  auto find_ok = hal::stream_find(hal::as_bytes(ok_response));

  while (hal::in_progress(find_start) && hal::in_progress(find_ok)) {
    std::array<hal::byte, 1> buffer;
    auto read_result = m_serial->read(buffer);

    // Pipe data into both streams
    read_result.data | find_status | find_start;
    read_result.data | find_ok;

    // Check if we've timed out
    p_timeout();
  }

  // We should fine the confirmation before we find the "OK" response
  if (hal::finished(find_start) && hal::in_progress(find_ok)) {
    // Read the last of the stream and find the OK to be sure
    wait_for_ok(m_serial, p_timeout);
    return true;
  }

  // We should find the confirmation before we find the "OK" response
  if (hal::in_progress(find_start) && hal::finished(find_ok)) {
    return false;
  }

  hal::safe_throw(hal::io_error(this));
}

std::span<hal::byte> at::server_read(std::span<hal::byte> p_buffer)
{
  // Format of a TCP packet for the ESP8266 AT commands:
  //
  //  +IDP,[0-9]+:[.*]{1460}
  //
  // Example:
  //
  //  +IDP,1230:
  //
  // Starts with a header, then length, then a ':' character, then 1 to 1460
  // bytes worth of payload data.

  size_t bytes_read = 0;
  auto buffer = p_buffer;
  auto read = std::span<hal::byte>();

  do {
    m_packet_manager.find(*m_serial);
    read = m_packet_manager.read_packet(*m_serial, buffer);
    bytes_read += read.size();
    buffer = buffer.subspan(read.size());
  } while (read.size() != 0 && buffer.size() != 0);

  return p_buffer.first(bytes_read);
}

void at::disconnect_from_server(deadline p_timeout)
{
  write(*m_serial, "AT+CIPCLOSE=0\r\n", p_timeout);
  wait_for_ok(m_serial, p_timeout);
}
}  // namespace hal::esp8266
