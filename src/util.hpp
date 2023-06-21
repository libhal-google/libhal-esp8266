// Copyright 2023 Google LLC
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

#pragma once

#include <charconv>
#include <concepts>
#include <span>
#include <string_view>

#include <libhal-util/as_bytes.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/serial_coroutines.hpp>
#include <libhal/serial.hpp>
#include <libhal/timeout.hpp>

namespace hal::esp8266 {

/// Default baud rate for the esp8266 AT commands
constexpr std::uint32_t default_baud_rate = 115200;
constexpr auto ok_response = std::string_view("OK\r\n");
constexpr auto got_ip_response = std::string_view("WIFI GOT IP\r\n");
constexpr auto reset_complete = std::string_view("ready\r\n");
constexpr auto start_of_packet = std::string_view("+IPD,");
constexpr auto end_of_line = std::string_view("\r\n");
constexpr auto end_of_header = std::string_view("\r\n\r\n");
constexpr auto send_finished = std::string_view("SEND OK\r\n");
constexpr auto ap_connected = std::string_view("+CWJAP:");
/// The maximum packet size for wlan_client AT commands
constexpr size_t maximum_response_packet_size = 1460UL;
constexpr size_t maximum_transmit_packet_size = 2048UL;
constexpr size_t ssid_max_length = 32;

/**
 * @brief Convert integer to strings and perform error code checking
 *
 * @tparam digits - allocates this many digits to store the integer. Set to 10
 * to be able to hold the maximum value of a uint32_t. Set to 20 to store
 * support.
 *
 */
template<size_t digits = 10>
class integer_string
{
public:
  static result<integer_string> create(std::integral auto p_integer)
  {
    integer_string result;
    auto chars_result = std::to_chars(
      result.m_buffer.begin(), result.m_buffer.end(), p_integer, 10);

    if (chars_result.ec != std::errc()) {
      return hal::new_error(chars_result.ec);
    }

    result.m_length =
      std::span<char>(result.m_buffer.data(), chars_result.ptr).size();

    return result;
  }

  std::string_view str() const
  {
    return std::string_view(m_buffer.data(), m_length);
  }

private:
  integer_string()
  {
  }

  std::array<char, digits> m_buffer{};
  size_t m_length;
};
}  // namespace hal::esp8266
