#include <libhal-esp8266/at/socket.hpp>
#include <libhal-esp8266/at/wlan_client.hpp>
#include <libhal-esp8266/http_response.hpp>
#include <libhal-esp8266/util.hpp>
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
  auto& esp = *p_map.esp;
  auto& console = *p_map.debug;

  hal::print(console, "ESP8266 WiFi Client Application Starting...\n");

  // Connect to WiFi AP
  auto wlan_client_result = hal::esp8266::at::wlan_client::create(
    esp, "ssid", "password", HAL_CHECK(hal::create_timeout(counter, 10s)));

  // Check if the wlan_client creation was successful, if not return error and
  // reset device
  if (!wlan_client_result) {
    hal::print(console, "Failed to create wifi client!\n\n");
    return wlan_client_result.error();
  } else {
    hal::print(console, "WiFi Connection made!!\n\n");
  }

  // Move wlan client out of the result object
  auto wlan_client = wlan_client_result.value();

  // Create a TCP socket and connect it to example.com
  auto socket_result = hal::esp8266::at::socket::create(
    wlan_client,
    HAL_CHECK(hal::create_timeout(counter, 5s)),
    {
      .type = hal::socket::type::tcp,
      .domain = "example.com",
      .port = "80",
    });

  // Check if socket creation was successful, if not return error and reset
  // device
  if (!socket_result) {
    hal::print(console, "Socket couldn't be established\n\n");
    return socket_result.error();
  } else {
    hal::print(console, "Socket connection has been established!\n\n");
  }

  // Move socket out of the result object
  auto socket = std::move(socket_result.value());

  // 2kB buffer to read data into
  std::array<hal::byte, 2096> buffer{};

  while (true) {
    buffer.fill('.');

    auto time_limit = HAL_CHECK(hal::create_timeout(counter, 5000ms));

    // Create http get request to example.com/ on port 80
    auto get_request =
      HAL_CHECK(hal::esp8266::http::create(socket,
                                           time_limit,
                                           { .response_buffer = buffer,
                                             .domain = "example.com",
                                             .path = "/",
                                             .port = "80" }));

    hal::print(console, "GET Request Creating... Waiting for results\n");

    // Wait until GET request response is finished
    auto state = HAL_CHECK(hal::try_until(get_request, time_limit));

    // If the state came back as finished, print the response to the user
    if (state == hal::work_state::finished) {
      hal::print(console, "GET Request finished, printing results:");
      HAL_CHECK(print_http_response_info(console, get_request.response()));
    } else {
      hal::print(console, "GET Request failed, attempting again!\n");
    }

    // Wait 500ms before making another GET request
    HAL_CHECK(hal::delay(counter, 100ms));
  }

  return hal::success();
}
