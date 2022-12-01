#pragma once

#include <cstdio>
#include <span>
#include <string_view>

#include <libhal/as_bytes.hpp>
#include <libhal/streams.hpp>
#include <libhal/units.hpp>

#include "at/socket.hpp"
#include "util.hpp"

namespace hal::esp8266::http {

enum class connection
{
  keep_alive,
  close
};

class get
{
public:
  static constexpr std::string_view content_length_header = "Content-Length: ";

  static result<get> create(hal::socket& p_socket,
                            std::span<hal::byte> p_buffer,
                            std::string_view p_path,
                            std::string_view p_domain,
                            std::string_view p_port = "",
                            connection p_keep_alive = connection::keep_alive)
  {
    using namespace std::literals;

    // Send GET request header line
    HAL_CHECK(p_socket.write(as_bytes("GET "sv)));
    HAL_CHECK(p_socket.write(as_bytes(p_path)));
    HAL_CHECK(p_socket.write(as_bytes(" HTTP/1.1\r\n"sv)));
    // Send HOST line
    HAL_CHECK(p_socket.write(as_bytes("Host: "sv)));
    HAL_CHECK(p_socket.write(as_bytes(p_domain)));
    if (!p_port.empty()) {
      HAL_CHECK(p_socket.write(as_bytes(":"sv)));
      HAL_CHECK(p_socket.write(as_bytes(p_port)));
    }
    HAL_CHECK(p_socket.write(as_bytes("\r\n"sv)));
    // Send keep alive line
    switch (p_keep_alive) {
      case connection::keep_alive:
        HAL_CHECK(p_socket.write(as_bytes("Connection: keep-alive\r\n"sv)));
        break;
      case connection::close:
        HAL_CHECK(p_socket.write(as_bytes("Connection: close\r\n"sv)));
        break;
    }
    // Send final newline
    HAL_CHECK(p_socket.write(as_bytes("\r\n"sv)));

    return get(p_socket, p_buffer);
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
  get(hal::socket& p_socket, std::span<hal::byte> p_buffer)
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
}  // namespace hal::esp8266::http
