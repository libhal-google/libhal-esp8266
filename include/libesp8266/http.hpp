#pragma once

#include <charconv>
#include <span>
#include <string_view>

#include <libhal/units.hpp>

namespace hal::esp8266 {

constexpr std::size_t digit_count(std::size_t p_value)
{
  size_t digits = 0;

  while (p_value) {
    p_value /= 10;
    digits++;
  }

  return digits;
}

enum class http_method
{
  /// The GET method requests a representation of the specified resource.
  /// Requests using GET should only retrieve data.
  GET,
  /// The HEAD method asks for a response identical to a GET request, but
  /// without the response body.
  HEAD,
  /// The POST method submits an entity to the specified resource, often
  /// causing a change in state or side effects on the server.
  POST,
  /// The PUT method replaces all current representations of the target
  /// resource with the request payload.
  PUT,
  /// The DELETE method deletes the specified resource.
  DELETE,
  /// The CONNECT method establishes a tunnel to the server identified by the
  /// target resource.
  CONNECT,
  /// The OPTIONS method describes the communication options for the target
  /// resource.
  OPTIONS,
  /// The TRACE method performs a message loop-back test along the path to the
  /// target resource.
  TRACE,
  /// The PATCH method applies partial modifications to a resource.
  PATCH,
};

constexpr std::string_view to_string(http_method p_method)
{
  switch (p_method) {
    case http_method::GET:
      return "GET";
    case http_method::HEAD:
      return "HEAD";
    case http_method::POST:
      return "POST";
    case http_method::PUT:
      return "PUT";
    case http_method::DELETE:
      return "DELETE";
    case http_method::CONNECT:
      return "CONNECT";
    case http_method::OPTIONS:
      return "OPTIONS";
    case http_method::TRACE:
      return "TRACE";
    case http_method::PATCH:
      return "PATCH";
  }
}

struct request_t
{
  /**
   * @brief Domain name of the server to connect to. This should not include
   * stuff like "http://" or "www". An example would be `google.com`,
   * `example.com`, or `developer.mozilla.org`.
   *
   */
  std::string_view domain;
  /**
   * @brief path to the resource within the domain url. To get the root page,
   * use "/" (or "/index.html"). URL parameters can also be placed in the path
   * as well such as "/search?query=wifi_client&price=lowest"
   *
   */
  std::string_view path = "/";
  /**
   * @brief which http method to use for this request. Most web servers use
   * GET and POST and tend to ignore the others.
   *
   */
  http_method method = http_method::GET;
  /**
   * @brief Body of the request
   *
   * Typically used for POST requests. This field is ignored if the method is
   * set to HEAD or GET.
   *
   */
  std::string_view body = {};
  /**
   * @brief which server port number to connect to.
   *
   */
  std::string_view port = "80";

  constexpr auto method_line()
  {
    return std::array<std::string_view, 4>{
      to_string(method),
      " ",
      path,
      " HTTP/1.1\r\n",
    };
  }

  constexpr auto host_line()
  {
    return std::array<std::string_view, 5>{
      "Host: ", domain, ":", port, "\r\n",
    };
  }

  constexpr size_t content_length()
  {
    constexpr std::string_view content_length = "Content-Length: \r\n";
    return content_length.size() + digit_count(body.size());
  }

  constexpr auto end_of_header_string()
  {
    return std::string_view("\r\n");
  }

  constexpr size_t total_length()
  {
    size_t size = 0;

    for (const auto& str : method_line()) {
      size += str.size();
    }

    for (const auto& str : host_line()) {
      size += str.size();
    }

    size += content_length();
    size += end_of_header_string().size();
    size += body.size();

    return size;
  }
};

struct header_t
{
  uint32_t status_code = 0;
  size_t content_length = 0;

  bool is_valid()
  {
    return status_code != 0 && content_length != 0;
  }
};

template<size_t BufferSize>
struct response_t
{
  header_t header{};
  std::array<hal::byte, BufferSize> raw{};
  size_t length;

  std::span<const hal::byte> data()
  {
    return std::span<const hal::byte>(raw.data(), length);
  }
};
}  // namespace hal::esp8266
