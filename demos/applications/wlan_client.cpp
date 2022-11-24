#include <libarmcortex/dwt_counter.hpp>
#include <libesp8266/at/tcp_socket.hpp>
#include <libesp8266/at/wlan_client.hpp>
#include <libesp8266/util.hpp>
#include <libhal/serial/util.hpp>
#include <libhal/steady_clock/util.hpp>
#include <libhal/timeout.hpp>
#include <liblpc40xx/constants.hpp>
#include <liblpc40xx/system_controller.hpp>
#include <liblpc40xx/uart.hpp>

/**
 * @brief Debug serial data on one serial port via another
 *
 * This class takes two serial ports, a "primary" and a "mirror". When the write
 * and read APIs are used, data is passed directly through to the primary. On
 * write operations, mirror will also write the same data. On read, the mirror
 * will report the read data by also writing it to its port. This way, a user
 * using a serial to USB debugger or some other debugging device can inspect
 * what is being written to and read from the primary serial port.
 *
 */
class serial_mirror : public hal::serial
{
public:
  serial_mirror(hal::serial& p_primary, hal::serial& p_mirror)
    : m_primary(&p_primary)
    , m_mirror(&p_mirror)
  {
  }

  hal::status driver_configure(const settings& p_settings) noexcept override
  {
    return m_primary->configure(p_settings);
  }

  hal::result<write_t> driver_write(
    std::span<const hal::byte> p_data) noexcept override
  {
    HAL_CHECK(hal::write(*m_mirror, "WRITE:["));
    HAL_CHECK(m_mirror->write(p_data));
    HAL_CHECK(hal::write(*m_mirror, "]\n"));
    return m_primary->write(p_data);
  }

  hal::result<read_t> driver_read(std::span<hal::byte> p_data) noexcept override
  {
    auto result = m_primary->read(p_data);

    if (result.has_value() && result.value().data.size() != 0) {
      HAL_CHECK(m_mirror->write(p_data));
    }

    return result;
  }

  hal::status driver_flush() noexcept override
  {
    return m_primary->flush();
  }

private:
  hal::serial* m_primary;
  hal::serial* m_mirror;
};

std::string_view to_string_view(std::span<const hal::byte> p_span)
{
  return std::string_view(reinterpret_cast<const char*>(p_span.data()),
                          p_span.size());
}

namespace hal {
status write(serial& p_serial, std::integral auto p_integer)
{
  auto integer_string = HAL_CHECK(esp8266::integer_string::create(p_integer));
  return write(p_serial, integer_string.str());
}
}  // namespace hal

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
    mirror, "ssid", "password", HAL_CHECK(hal::create_timeout(counter, 30s)));

  // Return error and restart
  if (!wlan_client_result) {
    HAL_CHECK(hal::write(uart0, "Failed to create wifi client!\n"));
    return wlan_client_result.error();
  }

  // Create a tcp_socket and connect it to example.com port 80
  auto wlan_client = wlan_client_result.value();
  auto tcp_socket_result = hal::esp8266::at::tcp_socket::create(
    wlan_client,
    "example.com",
    "80",
    HAL_CHECK(hal::create_timeout(counter, 1s)));

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
    buffer.fill('_');

    // Send out HTTP GET request
    HAL_CHECK(hal::write(uart0, "Sending:\n\n"));
    HAL_CHECK(hal::write(uart0, get_request));
    HAL_CHECK(hal::write(uart0, "\n\n"));
    HAL_CHECK(tcp_socket.write(hal::as_bytes(get_request)));

    // Wait 1 second before reading response back
    HAL_CHECK(hal::delay(counter, 1000ms));

    // Read response back from serial port
    auto received = HAL_CHECK(tcp_socket.read(buffer)).data;
    auto received_string = to_string_view(received);

    // Print out the results from the TCP socket
    HAL_CHECK(hal::write(uart0, "=============== Full Response! ==========\n"));
    HAL_CHECK(hal::write(uart0, received_string));
    HAL_CHECK(hal::write(uart0, "\n\n"));

    // Print out the parsed HTTP meta data from TCP socket string
    HAL_CHECK(hal::write(uart0, "================ Meta Data! ===========\n"));
    // Print HTTP Status Code
    HAL_CHECK(hal::write(uart0, "HTTP Status: "));
    HAL_CHECK(hal::write(uart0, hal::esp8266::http::status(received_string)));
    HAL_CHECK(hal::write(uart0, "\n"));
    // Print out the content type
    HAL_CHECK(hal::write(uart0, "Content-Type: "));
    HAL_CHECK(hal::write(
      uart0, hal::esp8266::http::header("Content-Type", received_string)));
    HAL_CHECK(hal::write(uart0, "\n"));
    // Print out the Date header flag
    HAL_CHECK(hal::write(uart0, "Date: "));
    HAL_CHECK(
      hal::write(uart0, hal::esp8266::http::header("Date", received_string)));
    HAL_CHECK(hal::write(uart0, "\n"));
    // Print out the length of the body of the response
    HAL_CHECK(hal::write(uart0, "Content-Length: "));
    HAL_CHECK(hal::write(
      uart0, hal::esp8266::http::header("Content-Length", received_string)));
    HAL_CHECK(hal::write(uart0, "\n"));

    // Print out the body of the response
    HAL_CHECK(hal::write(uart0, "================ Body! ===========\n"));
    HAL_CHECK(hal::write(uart0, hal::esp8266::http::body(received_string)));
    HAL_CHECK(hal::write(uart0, "\n"));
    HAL_CHECK(hal::write(uart0, "=================== /end ================\n"));
  }

  return hal::success();
}
