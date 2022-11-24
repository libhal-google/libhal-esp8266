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
/// End of line
static constexpr std::string_view end_of_line = "\r\n";
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

namespace http {

constexpr size_t status(std::string_view p_response)
{
  using namespace std::literals;
  // Example header:
  //
  //    HTTP/1.1 200 OK\r\n
  //
  constexpr std::string_view protocol_version_1p1 = "HTTP/1.1 ";
  const auto position_of_status = p_response.find(protocol_version_1p1);
  if (std::string_view::npos == position_of_status) {
    return std::string_view::npos;
  }

  const auto status_and_beyond =
    p_response.substr(position_of_status + protocol_version_1p1.size());
  const auto position_of_first_newline = status_and_beyond.find(end_of_line);
  if (std::string_view::npos == position_of_first_newline) {
    return std::string_view::npos;
  }

  const auto final_status_line =
    status_and_beyond.substr(0, position_of_first_newline);

  size_t status_value = 0;

  auto from_chars_result = std::from_chars(
    final_status_line.begin(), final_status_line.end(), status_value);

  if (from_chars_result.ec != std::errc()) {
    return std::string_view::npos;
  }

  return status_value;
}

constexpr std::string_view header(std::string_view p_header,
                                  std::string_view p_response)
{
  using namespace std::literals;
  // Example header:
  //
  //    Content-Length: 1438\r\n
  //
  const auto position_of_header = p_response.find(p_header);
  if (std::string_view::npos == position_of_header) {
    return "";
  }

  const auto start_of_string = p_response.substr(position_of_header);
  const auto position_of_first_newline = start_of_string.find(end_of_line);

  if (std::string_view::npos == position_of_first_newline) {
    return "";
  }

  const auto size_of_header = p_header.size();
  // Start position is moved 2 values forward to skip over the colon and space
  // characters.
  const auto start_position = size_of_header + ": "sv.size();
  const auto length = position_of_first_newline - start_position;
  const auto full_line = start_of_string.substr(start_position, length);

  return full_line;
}

constexpr std::string_view body(std::string_view p_response)
{
  using namespace std::literals;
  // Example header:
  //
  //    Content-Length: 16\r\n
  //    \r\n
  //    this is the data
  //

  const auto position_of_body = p_response.find(end_of_header);
  if (std::string_view::npos == position_of_body) {
    return "";
  }

  const auto start_of_body = p_response.substr(position_of_body);
  const auto length_string = header("Content-Length", p_response);

  if (length_string.empty()) {
    return "";
  }
  size_t content_length = 0;

  auto from_chars_result =
    std::from_chars(length_string.begin(), length_string.end(), content_length);

  if (from_chars_result.ec != std::errc()) {
    return "";
  }

  const auto full_line =
    start_of_body.substr(end_of_header.size(), content_length);

  return full_line;
}
}  // namespace http
}  // namespace hal::esp8266
