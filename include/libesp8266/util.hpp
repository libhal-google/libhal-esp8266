#pragma once

#include <charconv>
#include <concepts>
#include <span>
#include <string_view>

#include <libhal/as_bytes.hpp>
#include <libhal/serial/coroutines.hpp>
#include <libhal/serial/interface.hpp>
#include <libhal/serial/util.hpp>
#include <libhal/timeout.hpp>

namespace hal::esp8266 {

/// Default baud rate for the esp8266 AT commands
static constexpr std::uint32_t default_baud_rate = 115200;
/// Confirmation response
static constexpr std::string_view ok_response = "OK\r\n";
/// Confirmation response
static constexpr std::string_view got_ip_response = "WIFI GOT IP\r\n";
/// Confirmation response after a reset completes
static constexpr std::string_view reset_complete = "ready\r\n";
/// Confirmation response after a reset completes
static constexpr std::string_view start_of_packet = "+IPD,";
/// End of header
static constexpr std::string_view end_of_header = "\r\n\r\n";
/// The maximum packet size for wlan_client AT commands
static constexpr size_t maximum_response_packet_size = 1460UL;
static constexpr size_t maximum_transmit_packet_size = 2048UL;

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

  std::array<char, 20> m_buffer{};
  size_t m_length;
};
}  // namespace hal::esp8266
