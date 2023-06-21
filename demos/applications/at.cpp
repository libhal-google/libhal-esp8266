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

#include <libhal-esp8266/at.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/steady_clock.hpp>
#include <libhal/timeout.hpp>

#include "../hardware_map.hpp"
#include "helper.hpp"

hal::status application(hal::esp8266::hardware_map& p_map)
{
  using namespace std::chrono_literals;
  using namespace hal::literals;

  auto& counter = *p_map.counter;
  auto& serial = *p_map.serial;
  auto& console = *p_map.console;

  std::string_view ssid = "KAMMCE-PHONE";
  std::string_view password = "roverteam";

  hal::print(console, "ESP8266 WiFi Client Application Starting...\n");

  // 8kB buffer to read data into
  std::array<hal::byte, 8192> buffer{};

  // Connect to WiFi AP
  hal::print(console, "Create esp8266 object...\n");
  auto timeout = HAL_CHECK(hal::create_timeout(counter, 20s));
  auto esp8266 = HAL_CHECK(hal::esp8266::at::create(serial, timeout));
  hal::print(console, "Esp8266 created! \n");

  hal::print(console, "Connecting to AP...\n");
  HAL_CHECK(esp8266.connect_to_ap(ssid, password, timeout));
  hal::print(console, "AP Connected!\n");

  bool is_connected = HAL_CHECK(esp8266.is_connected_to_ap(timeout));

  if (is_connected) {
    hal::print(console, "AP connection verified!\n");
  }

  hal::print(console, "Connecting to server...\n");
  HAL_CHECK(esp8266.connect_to_server(
    {
      .type = hal::esp8266::at::socket_type::tcp,
      .domain = "example.com",
      .port = 80,
    },
    timeout));
  hal::print(console, "Server connected!\n");

  while (true) {
    // Minimalist GET request to example.com domain
    static constexpr std::string_view get_request = "GET / HTTP/1.1\r\n"
                                                    "Host: example.com:80\r\n"
                                                    "\r\n";
    // Fill buffer with underscores to determine which blocks aren't filled.
    buffer.fill('.');

    // Send out HTTP GET request
    hal::print(console, "\n\n================= SENDING! =================\n\n");
    hal::print(console, get_request);

    timeout = HAL_CHECK(hal::create_timeout(counter, 500ms));
    HAL_CHECK(esp8266.server_write(hal::as_bytes(get_request), timeout));

    // Wait 1 second before reading response back
    HAL_CHECK(hal::delay(counter, 1000ms));

    // Read response back from serial port
    auto received = HAL_CHECK(esp8266.server_read(buffer)).data;

    hal::print(console, "\n>>>>>>>>>>>>>>>>> RESPONSE <<<<<<<<<<<<<<<<<\n\n");
    hal::print(console, received);
  }

  return hal::success();
}
