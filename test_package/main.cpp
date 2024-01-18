// Copyright 2024 Khalil Estell
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

#include <libhal-esp8266/at.hpp>
#include <libhal-util/steady_clock.hpp>
#include <libhal/timeout.hpp>

#include "util.test.hpp"

#define HAL_IGNORE(expr) static_cast<void>((expr))

int main()
{
  using namespace std::literals;
  hal::esp8266::mock_serial mock;
  mock.m_stream_out = hal::esp8266::stream_out(
    "ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n>SEND OK\r\n"sv);

  auto at = hal::esp8266::at::create(mock, hal::never_timeout()).value();
  HAL_IGNORE(at.connect_to_ap("ssid", "password", hal::never_timeout()));
  HAL_IGNORE(at.connect_to_server(
    {
      .type = hal::esp8266::at::socket_type::tcp,
      .domain = "example.com",
      .port = 80,
    },
    hal::never_timeout()));
  HAL_IGNORE(at.server_write(hal::as_bytes("Hello, World\r\n\r\n"sv),
                             hal::never_timeout()));
  mock.m_stream_out = hal::esp8266::stream_out(
    "+IPD,14:Hello, World!\n+IPD,16:Goodbye, World!\n"sv);

  std::array<hal::byte, 1024> buffer;
  buffer.fill('.');

  auto actual_response = at.server_read(buffer).value().data;

  printf("response(%d):\n%.*s\n",
         static_cast<int>(actual_response.size()),
         static_cast<int>(actual_response.size()),
         actual_response.data());
}

namespace boost {
void throw_exception(std::exception const& e)
{
  hal::halt();
}
}  // namespace boost
