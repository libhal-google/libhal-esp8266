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
  auto& debug = *p_map.debug;

  HAL_CHECK(hal::write(debug, "ESP8266 WiFi Client Application Starting...\n"));

  // 2kB buffer to read data into
  std::array<hal::byte, 2096> buffer{};

  // Connect to WiFi AP
  auto wlan_client_result = hal::esp8266::at::wlan_client::create(
    esp, "ssid", "password", HAL_CHECK(hal::create_timeout(counter, 10s)));

  // Check if the wlan_client creation was successful, if not return error and
  // reset device
  if (!wlan_client_result) {
    HAL_CHECK(hal::write(debug, "Failed to create wifi client!\n"));
    return wlan_client_result.error();
  } else {
    HAL_CHECK(hal::write(debug, "WiFi Connection made!!\n"));
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
    HAL_CHECK(hal::write(debug, "Socket couldn't be established\n"));
    return socket_result.error();
  } else {
    HAL_CHECK(hal::write(debug, "Socket connection has been established!\n"));
  }

  // Move socket out of the result object
  auto socket = std::move(socket_result.value());

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

    // Wait until GET request response is finished
    auto state = HAL_CHECK(hal::try_until(get_request, time_limit));

    // If the state came back as finished, print the response to the user
    if (state == hal::work_state::finished) {
      HAL_CHECK(print_http_response_info(debug, get_request.response()));
    } else {
      HAL_CHECK(hal::write(debug, "GET Request failed, attempting again!\n"));
    }

    // Wait 500ms before making another GET request
    HAL_CHECK(hal::delay(counter, 100ms));
  }

  return hal::success();
}
