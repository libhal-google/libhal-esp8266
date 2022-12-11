#include <libesp8266/at/socket.hpp>

#include "../helpers.hpp"

#include <boost/ut.hpp>

namespace hal::esp8266::at {
boost::ut::suite socket_test = []() {
  using namespace boost::ut;

  "socket::create() oneshot"_test = []() {
    using namespace std::literals;
    // Setup
    mock_serial mock;
    mock.m_stream_out =
      stream_out("ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n>SEND OK\r\n"sv);
    auto wlan_client =
      wlan_client::create(mock, "ssid", "password", hal::never_timeout())
        .value();

    // Exercise
    // Verify
    auto tcp = socket::create(wlan_client,
                              hal::never_timeout(),
                              { .type = hal::socket::type::tcp,
                                .domain = "example.com",
                                .port = "80" })
                 .value();
  };

  "socket::create() oneshot"_test = []() {
    using namespace std::literals;
    // Setup
    mock_serial mock;
    mock.m_stream_out =
      stream_out("ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n>SEND OK\r\n"sv);
    auto wlan_client =
      wlan_client::create(mock, "ssid", "password", hal::never_timeout())
        .value();
    auto tcp = socket::create(wlan_client,
                              hal::never_timeout(),
                              { .type = hal::socket::type::tcp,
                                .domain = "example.com",
                                .port = "80" })
                 .value();
    tcp.write(hal::as_bytes("Hello, World\r\n\r\n"sv), never_timeout()).value();
    mock.m_stream_out = stream_out("+IPD,15:Goodbye, World!+IPD,8:Packet 2"sv);
    std::array<hal::byte, 1024> buffer;
    buffer.fill('.');

    // Exercise
    auto actual_response = tcp.read(buffer).value().data;

    // Verify
    printf("response1(%d):[%.*s]\n",
           static_cast<int>(actual_response.size()),
           static_cast<int>(actual_response.size()),
           actual_response.data());
  };

  "socket::create() receive chunks"_test = []() {
    using namespace std::literals;
    // Setup
    mock_serial mock;
    mock.m_stream_out =
      stream_out("ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n>SEND OK\r\n"sv);
    auto wlan_client =
      wlan_client::create(mock, "ssid", "password", hal::never_timeout())
        .value();
    auto tcp = socket::create(wlan_client,
                              hal::never_timeout(),
                              { .type = hal::socket::type::tcp,
                                .domain = "example.com",
                                .port = "80" })
                 .value();
    tcp.write(hal::as_bytes("Hello, World\r\n\r\n"sv), never_timeout()).value();
    mock.m_stream_out = stream_out("+IPD,15:Goodbye,"sv);

    // Exercise
    std::array<hal::byte, 1024> buffer;
    buffer.fill('.');
    std::span<hal::byte> span(buffer);
    auto res1 = tcp.read(span).value().data;
    span = span.subspan(res1.size());

    mock.m_stream_out = stream_out(" World!+IPD"sv);
    auto res2 = tcp.read(span).value().data;
    span = span.subspan(res2.size());

    mock.m_stream_out = stream_out(",8:Packet 2"sv);
    auto res3 = tcp.read(span).value().data;
    span = span.subspan(res3.size());

    auto length = buffer.size() - span.size();

    // Verify
    printf("response2(%d):[%.*s]\n",
           static_cast<int>(length),
           static_cast<int>(length),
           buffer.data());
  };

  "socket::create() read chunks"_test = []() {
    using namespace std::literals;
    // Setup
    mock_serial mock;
    mock.m_stream_out =
      stream_out("ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n>SEND OK\r\n"sv);
    auto wlan_client =
      wlan_client::create(mock, "ssid", "password", hal::never_timeout())
        .value();
    auto tcp = socket::create(wlan_client,
                              hal::never_timeout(),
                              { .type = hal::socket::type::tcp,
                                .domain = "example.com",
                                .port = "80" })
                 .value();
    tcp.write(hal::as_bytes("Hello, World\r\n\r\n"sv), never_timeout()).value();
    mock.m_stream_out = stream_out("+IPD,15:Goodbye, World!+IPD,8:Packet 2"sv);

    // Exercise
    std::array<hal::byte, 6> buffer1;
    std::array<hal::byte, 8> buffer2;
    std::array<hal::byte, 6> buffer3;
    std::array<hal::byte, 3> buffer4;
    buffer1.fill('.');
    buffer2.fill('.');
    buffer3.fill('.');
    buffer4.fill('.');

    auto res1 = tcp.read(buffer1).value().data;
    auto res2 = tcp.read(buffer2).value().data;
    auto res3 = tcp.read(buffer3).value().data;
    auto res4 = tcp.read(buffer4).value().data;

    auto size_sum = res1.size() + res2.size() + res3.size() + res4.size();

    // Verify
    printf("response2(%d):[", static_cast<int>(size_sum));
    printf("%.*s", static_cast<int>(buffer1.size()), buffer1.data());
    printf("%.*s", static_cast<int>(buffer2.size()), buffer2.data());
    printf("%.*s", static_cast<int>(buffer3.size()), buffer3.data());
    printf("%.*s]\n", static_cast<int>(buffer4.size()), buffer4.data());
  };
};
}  // namespace hal::esp8266::at