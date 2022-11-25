#include <libarmcortex/dwt_counter.hpp>
#include <libesp8266/at/socket.hpp>
#include <libesp8266/at/wlan_client.hpp>
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
  HAL_CHECK(hal::lpc40xx::clock::maximum(12.0_MHz));

  // Create a hardware counter
  auto& clock = hal::lpc40xx::clock::get();
  auto cpu_frequency = clock.get_frequency(hal::lpc40xx::peripheral::cpu);
  hal::cortex_m::dwt_counter counter(cpu_frequency);

  // Get and initialize UART0 for UART based logging
  auto& uart0 = hal::lpc40xx::uart::get<0>(hal::serial::settings{
    .baud_rate = 38400,
  });

  HAL_CHECK(hal::write(uart0, "ESP8266 WiFi Client Application Starting...\n"));

  // Get and initialize UART3 with a 8kB receive buffer
  auto& uart3 = hal::lpc40xx::uart::get<3, 8192>();

  // Create a uart mirror object to help with debugging uart3 transactions
  serial_mirror mirror(uart3, uart0);

  // 8kB buffer to read data into
  std::array<hal::byte, 8192> buffer{};

  // Connect to WiFi AP
  auto wlan_client_result = hal::esp8266::at::wlan_client::create(
    mirror, "ssid", "password", HAL_CHECK(hal::create_timeout(counter, 10s)));

  // Return error and restart
  if (!wlan_client_result) {
    HAL_CHECK(hal::write(uart0, "Failed to create wifi client!\n"));
    return wlan_client_result.error();
  }

  // Create a tcp_socket and connect it to example.com port 80
  auto wlan_client = wlan_client_result.value();
  auto tcp_socket_result = hal::esp8266::at::socket::create(
    wlan_client,
    hal::socket::type::tcp,
    HAL_CHECK(hal::create_timeout(counter, 1s)),
    "example.com",
    "80");

  if (!tcp_socket_result) {
    HAL_CHECK(hal::write(uart0, "TCP Socket couldn't be established\n"));
    return tcp_socket_result.error();
  }

  // Move tcp_socket out of the result object
  auto tcp_socket = std::move(tcp_socket_result.value());

  while (true) {
    // Minimalist GET request to example.com domain
    static constexpr std::string_view get_request = "GET / HTTP/1.1\r\n"
                                                    "Host: example.com:80\r\n"
                                                    "\r\n";
    // Fill buffer with underscores to determine which blocks aren't filled.
    buffer.fill('.');

    // Send out HTTP GET request
    HAL_CHECK(hal::write(uart0, "Sending:\n\n"));
    HAL_CHECK(hal::write(uart0, get_request));
    HAL_CHECK(hal::write(uart0, "\n\n"));
    HAL_CHECK(tcp_socket.write(hal::as_bytes(get_request)));

    // Wait 1 second before reading response back
    HAL_CHECK(hal::delay(counter, 1000ms));

    // Read response back from serial port
    auto received = HAL_CHECK(tcp_socket.read(buffer)).data;

    HAL_CHECK(print_http_response_info(uart0, to_string_view(received)));
  }

  return hal::success();
}
