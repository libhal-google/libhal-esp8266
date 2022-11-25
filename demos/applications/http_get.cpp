#include <libarmcortex/dwt_counter.hpp>
#include <libesp8266/at/socket.hpp>
#include <libesp8266/at/wlan_client.hpp>
#include <libesp8266/http_response.hpp>
#include <libesp8266/util.hpp>
#include <libhal/serial/util.hpp>
#include <libhal/steady_clock/util.hpp>
#include <libhal/timeout.hpp>
#include <liblpc40xx/constants.hpp>
#include <liblpc40xx/system_controller.hpp>
#include <liblpc40xx/uart.hpp>

#include "helper.hpp"

hal::status application()
{
  using namespace std::chrono_literals;
  using namespace hal::literals;

  // Set the MCU to the maximum clock speed
  HAL_CHECK(hal::lpc40xx::clock::maximum(10.0_MHz));

  // Create a hardware counter
  auto& clock = hal::lpc40xx::clock::get();
  auto cpu_frequency = clock.get_frequency(hal::lpc40xx::peripheral::cpu);
  hal::cortex_m::dwt_counter counter(cpu_frequency);

  // Get and initialize UART0 for UART based logging
  auto& uart0 = hal::lpc40xx::uart::get<0>(hal::serial::settings{
    .baud_rate = 921600,
  });

  HAL_CHECK(hal::write(uart0, "ESP8266 WiFi Client Application Starting...\n"));

  // Get and initialize UART3 with a 8kB receive buffer
  auto& uart3 = hal::lpc40xx::uart::get<3, 8192>();

  // Create a uart mirror object to help with debugging uart3 transactions
  serial_mirror mirror(uart3, uart0);

  // 8kB buffer to read data into
  std::array<hal::byte, 2096> buffer{};

  // Connect to WiFi AP
  auto wlan_client_result = hal::esp8266::at::wlan_client::create(
    mirror,
    "KAMMCE-PHONE",
    "roverteam",
    HAL_CHECK(hal::create_timeout(counter, 10s)));

  // Check if the wlan_client creation was successful, if not return error and
  // reset device
  if (!wlan_client_result) {
    HAL_CHECK(hal::write(uart0, "Failed to create wifi client!\n"));
    return wlan_client_result.error();
  } else {
    HAL_CHECK(hal::write(uart0, "WiFi Connection made!!\n"));
  }

  // Move wlan client out of the result object
  auto wlan_client = wlan_client_result.value();

  // Create a TCP socket and connect it to example.com
  auto socket_result = hal::esp8266::at::socket::create(
    wlan_client,
    hal::socket::type::tcp,
    HAL_CHECK(hal::create_timeout(counter, 5s)),
    "example.com");

  // Check if socket creation was successful, if not return error and reset
  // device
  if (!socket_result) {
    HAL_CHECK(hal::write(uart0, "Socket couldn't be established\n"));
    return socket_result.error();
  } else {
    HAL_CHECK(hal::write(uart0, "Socket connection has been established!\n"));
  }

  // Move socket out of the result object
  auto socket = std::move(socket_result.value());

  while (true) {
    buffer.fill('.');

    // Create http get request to example.com/ on port 80
    auto get_request = HAL_CHECK(hal::esp8266::http::get::create(
      socket, buffer, "/", "example.com", "80"));

    // Wait until GET request response is finished
    auto state = HAL_CHECK(hal::try_until(
      get_request, HAL_CHECK(hal::create_timeout(counter, 5000ms))));

    // If the state came back as finished, print the response to the user
    if (state == hal::work_state::finished) {
      HAL_CHECK(print_http_response_info(uart0, get_request.response()));
    } else {
      HAL_CHECK(hal::write(uart0, "GET Request failed, attempting again!\n"));
    }

    // Wait 500ms before making another GET request
    HAL_CHECK(hal::delay(counter, 500ms));
  }

  return hal::success();
}
