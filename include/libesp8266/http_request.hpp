#pragma once

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <libhal/as_bytes.hpp>
#include <libhal/serial/coroutines.hpp>
#include <libhal/serial/interface.hpp>
#include <libhal/serial/util.hpp>
#include <string_view>

#include "common.hpp"
#include "http.hpp"

// Following:
//
//   https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers
//   https://developer.mozilla.org/en-US/docs/Glossary/Request_header
//   https://developer.mozilla.org/en-US/docs/Glossary/Response_header
//
namespace hal::esp8266::at {
template<size_t ResponseSize>
result<response_t<ResponseSize>> http_request(tcp_socket_client& p_socket,
                                              request_t p_request,
                                              timeout auto p_timeout)
{
  response_t<ResponseSize> response{};

  auto length = p_request.total_length();

  if (length > maximum_transmit_packet_size) {
    return hal::new_error(std::errc::value_too_large);
  }

  auto request_length = HAL_CHECK(integer_string::create(length));
  auto body_length = HAL_CHECK(integer_string::create(p_request.body.size()));

  // Sending Request Header to server
  for (const auto& block : p_request.method_line()) {
    p_socket.write(block);
  }

  for (const auto& block : p_request.host_line()) {
    p_socket.write(block);
  }

  if (p_request.body.size() != 0) {
    p_socket.write("Content-Length: ");
    p_socket.write(hal::as_bytes(body_length.str()));
    p_socket.write("\r\n");
  }

  p_socket.write("\r\n");

  if (p_request.body.size() != 0) {
    HAL_CHECK(p_socket.write(p_request.body));
  }

  std::span<hal::byte> buffer = response.raw;
  while (!buffer.empty()) {
    auto received = HAL_CHECK(p_request.receive(buffer)).received;
    buffer = buffer.subspan(received.size());
    HAL_CHECK(p_timeout);
  }

  response.length = response.raw.size() - buffer.size();

  return response;
}
}  // namespace hal::esp8266::at
