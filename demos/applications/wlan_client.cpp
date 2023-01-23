#include <libhal-esp8266/at/socket.hpp>
#include <libhal-esp8266/at/wlan_client.hpp>
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

  auto& clock = *p_map.counter;
  auto& esp = *p_map.esp;
  auto& console = *p_map.debug;

  hal::print(console, "ESP8266 WiFi Client Application Starting...\n");

  // 8kB buffer to read data into
  std::array<hal::byte, 8192> buffer{};

  // Connect to WiFi AP
  auto wlan_client_result = hal::esp8266::at::wlan_client::create(
    esp, "ssid", "password", HAL_CHECK(hal::create_timeout(clock, 10s)));

  // Return error and restart
  if (!wlan_client_result) {
    hal::print(console, "Failed to create wifi client!\n");
    return wlan_client_result.error();
  }

  // Create a tcp_socket and connect it to example.com port 80
  auto wlan_client = wlan_client_result.value();
  auto tcp_socket_result =
    hal::esp8266::at::socket::create(wlan_client,
                                     HAL_CHECK(hal::create_timeout(clock, 5s)),
                                     {
                                       .type = hal::socket::type::tcp,
                                       .domain = "example.com",
                                       .port = "80",
                                     });

  if (!tcp_socket_result) {
    hal::print(console, "TCP Socket couldn't be established\n\n");
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
    hal::print(console, "Sending:\n\n");
    hal::print(console, get_request);
    hal::print(console, "\n\n");
    HAL_CHECK(tcp_socket.write(hal::as_bytes(get_request),
                               HAL_CHECK(hal::create_timeout(clock, 500ms))));

    // Wait 1 second before reading response back
    HAL_CHECK(hal::delay(clock, 1000ms));

    // Read response back from serial port
    auto received = HAL_CHECK(tcp_socket.read(buffer)).data;

    print_http_response_info(console, to_string_view(received));
  }

  return hal::success();
}
