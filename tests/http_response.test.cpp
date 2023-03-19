#include <libhal-esp8266/http_response.hpp>

#include "helpers.hpp"

#include <string>

#include <libhal/functional.hpp>

#include <boost/ut.hpp>

namespace hal::esp8266 {
void http_response_test()
{
  using namespace boost::ut;
  using namespace std::literals;

  class fake_socket : public hal::socket
  {
  public:
    stream_out m_stream_out;
    std::string m_request;

  private:
    hal::result<write_t> driver_write(
      std::span<const hal::byte> p_data,
      [[maybe_unused]] hal::function_ref<hal::timeout_function> p_timeout)
    {
      m_request.append(reinterpret_cast<const char*>(p_data.data()),
                       p_data.size());
      printf("%.*s", static_cast<int>(p_data.size()), p_data.data());
      return write_t{ p_data };
    }

    hal::result<read_t> driver_read(std::span<hal::byte> p_data)
    {
      auto min = std::min(p_data.size(), size_t(5));
      return read_t{ m_stream_out(p_data.first(min)).data };
    }
  };

  "http::header() find all header values"_test = []() {
    // Setup
    constexpr std::string_view example_response =
      "HTTP/1.1 200 OK\r\n"
      "Content-Encoding: gzip \r\n"
      "Accept-Ranges: bytes\r\n"
      "Age: 578213\r\n"
      "Cache-Control: max-age=604800\r\n"
      "Content-Type: text/html; charset=UTF-8\r\n"
      "Date: Fri, 25 Nov 2022 01:10:25 GMT\r\n"
      "Etag: \" 3147526947 \"\r\n"
      "Expires: Fri, 02 Dec 2022 01:10:25 GMT\r\n"
      "Last-Modified: Thu, 17 Oct 2019 07:18:26 GMT\r\n"
      "Server: ECS (oxr/830D)\r\n"
      "Vary: Accept-Encoding\r\n"
      "X-Cache: HIT\r\n"
      "Content-Length: 10\r\n"
      "\r\n"
      "0123456789"sv;
    // 386 UL

    constexpr std::string_view expected_response =
      "GET / HTTP/1.1\r\n"
      "Host: example.com:80\r\n"
      "Connection: keep-alive\r\n\r\n";
    std::array<hal::byte, 1024> test_buffer;
    fake_socket test_socket;
    test_socket.m_stream_out = stream_out(example_response);
    auto get_request = http::create(test_socket,
                                    never_timeout(),
                                    {
                                      .response_buffer = test_buffer,
                                      .domain = "example.com",
                                      .path = "/",
                                      .port = "80",
                                    })
                         .value();

    std::array<work_state, 80> work_states{};

    // Exercise
    for (auto& state : work_states) {
      state = get_request().value();
    }

    // Verify
    expect(that % expected_response == test_socket.m_request);

    for (size_t i = 0; i < 77; i++) {
      expect(that % work_states[i] == work_state::in_progress);
    }

    for (size_t i = 77; i < work_states.size(); i++) {
      expect(that % work_states[i] == work_state::finished);
    }

    printf("\n%.*s\n\n",
           static_cast<int>(get_request.response().size()),
           get_request.response().data());
  };
};
}  // namespace hal::esp8266
