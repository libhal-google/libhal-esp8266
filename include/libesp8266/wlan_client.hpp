#pragma once

#include <cstdint>
#include <cstdio>
#include <libhal/serial/coroutines.hpp>
#include <libhal/serial/interface.hpp>
#include <libhal/serial/util.hpp>
#include <string_view>

#include "http.hpp"
#include "wifi.hpp"

namespace hal::esp8266::at {
/**
 * @brief wlan_client AT command driver for connecting to WiFi Access points
 *
 */
class wlan_client
{
public:
  /// Default baud rate for the wlan_client AT commands
  static constexpr uint32_t default_baud_rate = 115200;
  /// Confirmation response
  static constexpr std::string_view ok_response = "OK\r\n";
  /// Confirmation response after a wifi successfully connected
  static constexpr std::string_view wifi_connected =
    "WIFI GOT IP\r\n\r\nOK\r\n";
  /// Confirmation response after a reset completes
  static constexpr std::string_view reset_complete = "ready\r\n";
  /// End of header
  static constexpr std::string_view end_of_header = "\r\n\r\n";
  /// The maximum packet size for wlan_client AT commands
  static constexpr size_t maximum_response_packet_size = 1460;
  static constexpr size_t maximum_transmit_packet_size = 2048;

  static result<wlan_client> create(hal::serial& p_serial,
                                    std::string_view p_ssid,
                                    std::string_view p_password,
                                    timeout auto p_timeout)
  {
    HAL_CHECK(p_serial.configure({
      .baud_rate = 115200,
      .stop = hal::serial::settings::stop_bits::one,
      .parity = hal::serial::settings::parity::none,
    }));

    HAL_CHECK(p_serial.flush());

    wlan_client client(p_serial);
    HAL_CHECK(client.connect(p_ssid, p_password, p_timeout));

    return client;
  }

  /**
   * @brief checks the connection state to an access point.
   *
   * @return true is connected to access point
   * @return false is NOT connected access point
   */
  bool connected() { return m_connected; }

  status connect(std::string_view p_ssid,
                 std::string_view p_password,
                 timeout auto p_timeout)
  {
    // Reset the device
    HAL_CHECK(write("AT+RST\r\n"));
    auto skipper = hal::skip_past(*m_serial, as_bytes(reset_complete));
    /* HAL_CHECK(try_until(skipper, p_timeout)); */

    // Turn off echo
    HAL_CHECK(write("ATE0\r\n"));
    skipper = hal::skip_past(*m_serial, as_bytes(ok_response));
    /* HAL_CHECK(try_until(skipper, p_timeout)); */

    // Configure as WiFi Station (client) mode
    HAL_CHECK(write("AT+CWMODE=1\r\n"));
    skipper = hal::skip_past(*m_serial, as_bytes(ok_response));
    /* HAL_CHECK(try_until(skipper, p_timeout)); */

    // Connect to wifi access point
    HAL_CHECK(write("AT+CWJAP_CUR=\""));
    HAL_CHECK(write(m_ssid));
    HAL_CHECK(write("\",\""));
    HAL_CHECK(write(m_password));
    HAL_CHECK(write("\"\r\n"));
    skipper = hal::skip_past(*m_serial, as_bytes(ok_response));
    /* HAL_CHECK(try_until(skipper, p_timeout)); */

    return hal::success();
  }

  /**
   * @brief After issuing a request, this function must be called in order to
   * progress the http request. This function manages, connecting to the server,
   * sending the request to server and receiving data from the server.
   *
   * @return state is the state of the current transaction. This value
   * can be checked to determine if a certain stage is taking too long.
   */
  result<work_state> operator()();

private:
  /**
   * @param p_serial the serial port connected to the wlan_client
   * @param p_baud_rate the operating baud rate for the wlan_client
   *
   */
  wlan_client(hal::serial& p_serial)
    : m_serial{ &p_serial }
  {}

  result<void> write(std::string_view p_string)
  {
    return hal::write(*m_serial, p_string);
  }

  serial* m_serial;
  bool m_connected = false;
};
} // namespace hal::esp8266::at