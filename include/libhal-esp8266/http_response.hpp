#pragma once

#include <cstdio>
#include <span>
#include <string_view>

#include <etl/string.h>
#include <libhal-util/as_bytes.hpp>
#include <libhal-util/streams.hpp>
#include <libhal/units.hpp>

#include "at/socket.hpp"
#include "util.hpp"

namespace hal::esp8266 {

enum class http_method
{
  get,
  head,
  post,
};

constexpr std::string_view to_string(http_method p_method)
{
  switch (p_method) {
    case http_method::head:
      return "HEAD";
    case http_method::post:
      return "POST";
    case http_method::get:
      [[fallthrough]];
    default:
      return "GET";
  }
}

enum class http_connection
{
  keep_alive,
  close
};

constexpr std::string_view to_string(http_connection p_connection)
{
  switch (p_connection) {
    case http_connection::keep_alive:
      return "Connection: keep-alive\r\n";
    case http_connection::close:
      [[fallthrough]];
    default:
      return "Connection: close\r\n";
  }
}

struct http_request
{
  std::span<hal::byte> response_buffer;
  std::string_view domain;
  http_method method = http_method::get;
  std::string_view path = "/";
  std::string_view port = "80";
  http_connection connection = http_connection::keep_alive;
  std::span<hal::byte> payload{};
};

template<size_t StringSize>
void append(etl::string<StringSize>& p_string, std::string_view p_view)
{
  p_string.append(p_view.data(), p_view.size());
}

class http
{
public:
  static constexpr std::string_view content_length_header = "Content-Length: ";

  /**
   * @brief
   *
   * @tparam BufferSize - Stack size of the request string buffer. Must be big
   * enough to hold the contents of the header, except the payload.
   * @param p_socket - network socket to make http request over
   * @param p_timeout - time limit to send HTTP request
   * @param p_request - specifies the HTTP request details
   * @return result<http> - A http response worker which, when called, will
   * stream packets of data from the socket into
   * @throws std::errc::invalid_argument - if the response buffer has zero
   * length
   * @throws std::errc::not_enough_memory - if the http header could not fit
   * within the BufferSize given.
   * @throws std::errc::destination_address_required - if domain address is
   * empty
   */
  template<size_t BufferSize = 1024U>
  static result<http> create(hal::socket& p_socket,
                             timeout auto p_timeout,
                             http_request p_request)
  {
    using namespace std::literals;

    etl::string<BufferSize> request_str;

    if (p_request.response_buffer.empty()) {
      return hal::new_error(std::errc::invalid_argument);
    }

    if (p_request.domain.empty()) {
      return hal::new_error(std::errc::destination_address_required);
    }

    // Add METHOD & HTTP version header line
    append(request_str, to_string(p_request.method));
    append(request_str, " "sv);
    append(request_str, p_request.path);
    append(request_str, " HTTP/1.1\r\n"sv);

    // Add HOST line
    append(request_str, "Host: "sv);
    append(request_str, p_request.domain);
    if (!p_request.port.empty()) {
      append(request_str, ":"sv);
      append(request_str, p_request.port);
    }
    append(request_str, "\r\n"sv);

    // Add Connection line
    append(request_str, to_string(p_request.connection));

    // Add final newline
    append(request_str, "\r\n"sv);

    if (request_str.is_truncated()) {
      return hal::new_error(std::errc::not_enough_memory);
    }

    // Send header
    HAL_CHECK(p_socket.write(as_bytes(request_str), p_timeout));

    // Send payload
    if (!p_request.payload.empty()) {
      HAL_CHECK(p_socket.write(p_request.payload, p_timeout));
    }

    return http(p_socket, p_request.response_buffer);
  }

  result<work_state> operator()()
  {
    auto buffer_remaining = m_buffer.subspan(m_length);

    if (buffer_remaining.empty()) {
      return work_state::finished;
    }

    if (!terminated(m_find_end_of_header)) {  // Read header
      auto bytes_read = HAL_CHECK(m_socket->read(buffer_remaining)).data;

      // Pass bytes through stream pipeline
      bytes_read | m_find_content_length | m_parse_packet_length |
        m_find_end_of_header;

      m_length += bytes_read.size();
    } else {  // Read buffer
      auto min =
        std::min(buffer_remaining.size(), m_parse_packet_length.value());
      auto body_span = buffer_remaining.first(min);
      auto bytes_read = HAL_CHECK(m_socket->read(body_span)).data;
      m_length += bytes_read.size();
    }

    auto end_of_header_position =
      std::string_view(reinterpret_cast<const char*>(m_buffer.data()),
                       m_buffer.size())
        .find(end_of_header);

    if (end_of_header_position != std::string_view::npos) {
      if (m_length >= end_of_header_position + end_of_header.size() +
                        m_parse_packet_length.value()) {
        return work_state::finished;
      }
    }

    return work_state::in_progress;
  }

  std::string_view response()
  {
    return std::string_view(reinterpret_cast<const char*>(m_buffer.data()),
                            m_length);
  }

private:
  http(hal::socket& p_socket, std::span<hal::byte> p_buffer)
    : m_socket(&p_socket)
    , m_buffer(p_buffer)
    , m_find_content_length(as_bytes(content_length_header))
    , m_parse_packet_length()
    , m_find_end_of_header(as_bytes(end_of_header))
  {
  }

  hal::socket* m_socket;
  std::span<hal::byte> m_buffer;
  hal::stream::find m_find_content_length;
  hal::stream::parse<size_t> m_parse_packet_length;
  hal::stream::find m_find_end_of_header;
  size_t m_length = 0;
};
}  // namespace hal::esp8266
