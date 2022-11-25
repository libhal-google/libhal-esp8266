#include <libesp8266/util.hpp>

#include "helpers.hpp"
#include <boost/ut.hpp>

namespace hal::esp8266::http {
boost::ut::suite util_test = []() {
  using namespace boost::ut;
  using namespace std::literals;

  "http::header() find all header values"_test = []() {
    // Setup
    constexpr std::string_view example_header =
      "HTTP/1.1 200 OK\r\n"
      "Age: 133983\r\n"
      "Cache-Control: max-age=604800\r\n"
      "Content-Type: text/html; charset=UTF-8\r\n"
      "Date: Wed, 23 Nov 2022 04:16:56 GMT\r\n"
      "Etag: \" 3147526947 + ident \"\r\n"
      "Expires: Wed, 30 Nov 2022 04:16:56 GMT\r\n"
      "Last-Modified: Thu, 17 Oct 2019 07:18:26 GMT\r\n"
      "Server: ECS (oxr/8323)\r\n"
      "Vary: Accept-Encoding\r\n"
      "X-Cache: HIT\r\n"
      "Content-Length: 1256\r\n"
      "\r\n";

    std::array headers{
      "Age"sv,  "Cache-Control"sv, "Content-Type"sv,   "Date"sv,
      "Etag"sv, "Expires"sv,       "Last-Modified"sv,  "Server"sv,
      "Vary"sv, "X-Cache"sv,       "Content-Length"sv,
    };

    std::array expected{
      "133983"sv,
      "max-age=604800"sv,
      "text/html; charset=UTF-8"sv,
      "Wed, 23 Nov 2022 04:16:56 GMT"sv,
      "\" 3147526947 + ident \""sv,
      "Wed, 30 Nov 2022 04:16:56 GMT"sv,
      "Thu, 17 Oct 2019 07:18:26 GMT"sv,
      "ECS (oxr/8323)"sv,
      "Accept-Encoding"sv,
      "HIT"sv,
      "1256"sv,
    };

    decltype(headers) values;

    // Exercise
    values[0] = header(headers[0], example_header);
    values[1] = header(headers[1], example_header);
    values[2] = header(headers[2], example_header);
    values[3] = header(headers[3], example_header);
    values[4] = header(headers[4], example_header);
    values[5] = header(headers[5], example_header);
    values[6] = header(headers[6], example_header);
    values[7] = header(headers[7], example_header);
    values[8] = header(headers[8], example_header);
    values[9] = header(headers[9], example_header);
    values[10] = header(headers[10], example_header);

    // Verify
    expect(that % expected[0] == values[0]);
    expect(that % expected[1] == values[1]);
    expect(that % expected[2] == values[2]);
    expect(that % expected[3] == values[3]);
    expect(that % expected[4] == values[4]);
    expect(that % expected[5] == values[5]);
    expect(that % expected[6] == values[6]);
    expect(that % expected[7] == values[7]);
    expect(that % expected[8] == values[8]);
    expect(that % expected[9] == values[9]);
    expect(that % expected[10] == values[10]);
  };

  "http::status() status string"_test = []() {
    // Setup
    std::array protocol_line{
      "HTTP/1.1 200 OK\r\n"
      "Server: ECS (oxr/8323)\r\n"
      "Content-Length: 5\r\n"
      "\r\n"
      "abcde"sv,
      "HTTP/1.1 101 Switching Protocols\r\n"
      "\r\n"sv,
      "HTTP/1.1 301 Moved Permanently\r\n"
      "Server: ECS (oxr/8323)\r\n"
      "\r\n"sv,
      "HTTP/1.1 308 Permanent Redirect\r\n"
      "\r\n"sv,
      "HTTP/1.1 400 Bad Request\r\n"
      "\r\n"sv,
      "HTTP/1.1 404 Not Found\r\n"
      "Server: ECS (oxr/8323)\r\n"
      "\r\n"sv,
      "HTTP/1.1 500 Internal Server Error\r\n"
      "\r\n"sv,
      "HTTP/1.1 507 Insufficient Storage\r\n"
      "Server: ECS (oxr/8323)\r\n"
      "\r\n"sv,
    };

    std::array<size_t, protocol_line.size()> expected{
      200, 101, 301, 308, 400, 404, 500, 507,
    };

    decltype(expected) values;

    // Exercise
    values[0] = status(protocol_line[0]);
    values[1] = status(protocol_line[1]);
    values[2] = status(protocol_line[2]);
    values[3] = status(protocol_line[3]);
    values[4] = status(protocol_line[4]);
    values[5] = status(protocol_line[5]);
    values[6] = status(protocol_line[6]);
    values[7] = status(protocol_line[7]);

    // Verify
    expect(that % expected[0] == values[0]);
    expect(that % expected[1] == values[1]);
    expect(that % expected[2] == values[2]);
    expect(that % expected[3] == values[3]);
    expect(that % expected[4] == values[4]);
    expect(that % expected[5] == values[5]);
    expect(that % expected[6] == values[6]);
    expect(that % expected[7] == values[7]);
  };

  "http::body()"_test = []() {
    // Setup
    std::array protocol_line{
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 5\r\n"
      "\r\n"
      "abcde          "sv,
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 22\r\n"
      "\r\n"
      "galaxy brains thinking    "sv,
    };

    std::array expected{
      "abcde"sv,
      "galaxy brains thinking"sv,
    };

    decltype(expected) values;

    // Exercise
    values[0] = body(protocol_line[0]);
    values[1] = body(protocol_line[1]);

    // Verify
    expect(that % expected[0] == values[0]);
    expect(that % expected[1] == values[1]);
  };

  "http::body() fails due to no 'end_of_header'"_test = []() {
    // Setup
    std::array protocol_line{
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 5\r\n"
      "abcde          "sv,
      "ffsafsssasffassaf\r\n"
      "galfafasssftyjtyjytfg"sv,
    };

    std::array expected{
      "abcde"sv,
      "galaxy brains thinking"sv,
    };

    decltype(expected) values;

    // Exercise
    values[0] = body(protocol_line[0]);
    values[1] = body(protocol_line[1]);

    // Verify
    expect(that % ""sv == values[0]);
    expect(that % ""sv == values[1]);
  };

  "http::body() fails due to no 'content-length' header"_test = []() {
    // Setup
    std::array protocol_line{
      "HTTP/1.1 200 OK\r\n"
      "\r\n"
      "abcde          "sv,
      "HTTP/1.1 200 OK\r\n"
      "\r\n"
      "galaxy brains thinking    "sv,
    };

    std::array expected{
      "abcde"sv,
      "galaxy brains thinking"sv,
    };

    decltype(expected) values;

    // Exercise
    values[0] = body(protocol_line[0]);
    values[1] = body(protocol_line[1]);

    // Verify
    expect(that % ""sv == values[0]);
    expect(that % ""sv == values[1]);
  };

  "http::body() fails due to non-numeric 'content-length' value"_test = []() {
    // Setup
    std::array protocol_line{
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: abcd\r\n"
      "\r\n"
      "abcde          "sv,
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: asf876\r\n"
      "\r\n"
      "galaxy brains thinking    "sv,
    };

    std::array expected{
      "abcde"sv,
      "galaxy brains thinking"sv,
    };

    decltype(expected) values;

    // Exercise
    values[0] = body(protocol_line[0]);
    values[1] = body(protocol_line[1]);

    // Verify
    expect(that % ""sv == values[0]);
    expect(that % ""sv == values[1]);
  };
};
}  // namespace hal::esp8266::http
