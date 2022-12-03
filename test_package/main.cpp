#define BOOST_LEAF_EMBEDDED
#define BOOST_LEAF_NO_THREADS

#include <array>

#include <libesp8266/at/socket.hpp>
#include <libesp8266/at/wlan_client.hpp>
#include <libhal/serial/util.hpp>
#include <libhal/steady_clock/util.hpp>
#include <libhal/timeout.hpp>

#include "util.test.hpp"

int main()
{
  using namespace std::literals;
  hal::esp8266::mock_serial mock;
  mock.m_stream_out = hal::esp8266::stream_out(
    "ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n>SEND OK\r\n"sv);
  auto wlan_client = hal::esp8266::at::wlan_client::create(
                       mock, "ssid", "password", hal::never_timeout())
                       .value();
  auto tcp =
    hal::esp8266::at::socket::create(
      wlan_client,
      hal::never_timeout(),
      { .type = hal::socket::type::tcp, .domain = "example.com", .port = "80" })
      .value();
  tcp.write(hal::as_bytes("Hello, World\r\n\r\n"sv), hal::never_timeout())
    .value();
  mock.m_stream_out =
    hal::esp8266::stream_out("+IPD,15:Goodbye, World!+IPD,8:Packet 2"sv);
  std::array<hal::byte, 1024> buffer;
  buffer.fill('.');

  auto actual_response = tcp.read(buffer).value().data;

  printf("response(%d):[%.*s]\n",
         static_cast<int>(actual_response.size()),
         static_cast<int>(actual_response.size()),
         actual_response.data());
}

namespace boost {
void throw_exception(std::exception const& e)
{
  std::abort();
}
}  // namespace boost
