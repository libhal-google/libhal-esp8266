// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define BOOST_LEAF_EMBEDDED
#define BOOST_LEAF_NO_THREADS

#include <array>

#include <libhal-esp8266/at/socket.hpp>
#include <libhal-esp8266/at/wlan_client.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/steady_clock.hpp>
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
