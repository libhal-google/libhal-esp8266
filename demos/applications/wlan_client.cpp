#include <libesp8266/at/socket.hpp>
#include <libesp8266/at/wlan_client.hpp>
#include <libesp8266/util.hpp>
#include <libhal/serial/util.hpp>
#include <libhal/steady_clock/util.hpp>
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

  // 8kB buffer to read data into
  std::array<hal::byte, 8192> buffer{};

  // Connect to WiFi AP
  auto wlan_client_result = hal::esp8266::at::wlan_client::create(
    esp, "ssid", "password", HAL_CHECK(hal::create_timeout(counter, 10s)));

  // Return error and restart
  if (!wlan_client_result) {
    HAL_CHECK(hal::write(debug, "Failed to create wifi client!\n"));
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
    HAL_CHECK(hal::write(debug, "TCP Socket couldn't be established\n"));
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
    HAL_CHECK(hal::write(debug, "Sending:\n\n"));
    HAL_CHECK(hal::write(debug, get_request));
    HAL_CHECK(hal::write(debug, "\n\n"));
    HAL_CHECK(tcp_socket.write(hal::as_bytes(get_request)));

    // Wait 1 second before reading response back
    HAL_CHECK(hal::delay(counter, 1000ms));

    // Read response back from serial port
    auto received = HAL_CHECK(tcp_socket.read(buffer)).data;

    HAL_CHECK(print_http_response_info(debug, to_string_view(received)));
  }

  return hal::success();
}
