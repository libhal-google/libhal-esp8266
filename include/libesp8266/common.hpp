#pragma once

#include <charconv>
#include <span>
#include <string_view>

#include <libhal/as_bytes.hpp>
#include <libhal/serial/coroutines.hpp>
#include <libhal/serial/interface.hpp>
#include <libhal/serial/util.hpp>
#include <libhal/timeout.hpp>

namespace hal::esp8266 {

/// Default baud rate for the wlan_client AT commands
static constexpr std::uint32_t default_baud_rate = 115200;
/// Confirmation response
static constexpr std::string_view ok_response = "OK\r\n";
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

  std::span<const char> str() const
  {
    return std::span<const char>(m_buffer.data(), m_length);
  }

private:
  integer_string()
  {
  }

  std::array<char, 20> m_buffer{};
  size_t m_length;
};

class packet_parser
{
public:
  enum class state
  {
    reset,
    find_packet_header,
    read_packet_length,
    read_packet
  };

  packet_parser(hal::serial& p_serial)
    : m_serial(&p_serial)
    , m_skip_past(p_serial, hal::as_bytes(ok_response))
    , m_uint_reader(p_serial)
  {
  }

  hal::result<std::span<hal::byte>> operator()(std::span<hal::byte> p_data)
  {
    size_t bytes_read = 0;
    switch (m_state) {
      case state::reset:
        m_skip_past = hal::skip_past(*m_serial, hal::as_bytes(start_of_packet));
        m_uint_reader = read_uint32(*m_serial);
        m_packet_length = 0;
        m_state = state::find_packet_header;
        [[fallthrough]];
      case state::find_packet_header:
        if (hal::work_state::finished == HAL_CHECK(m_skip_past())) {
          m_state = state::read_packet_length;
          [[fallthrough]];
        } else {
          break;
        }
      case state::read_packet_length:
        if (hal::work_state::finished == HAL_CHECK(m_uint_reader())) {
          m_packet_length = m_uint_reader.get().value();
          m_state = state::read_packet;
          [[fallthrough]];
        } else {
          break;
        }
      case state::read_packet:
        auto smallest_buffer = std::min(p_data.size(), m_packet_length);
        auto subspan = p_data.subspan(0, smallest_buffer);
        auto read_result = HAL_CHECK(m_serial->read(subspan));

        bytes_read = read_result.received.size();
        m_packet_length -= bytes_read;

        if (m_packet_length == 0) {
          m_state = state::reset;
        }
        break;
    }

    return p_data.subspan(0, bytes_read);
  }

private:
  hal::serial* m_serial;
  hal::skip_past m_skip_past;
  hal::read_uint32 m_uint_reader;
  size_t m_packet_length = 0;
  state m_state = state::reset;
};
}  // namespace hal::esp8266
