#pragma once

#include <algorithm>
#include <cstdint>
#include <libhal/as_bytes.hpp>
#include <libhal/serial/coroutines.hpp>
#include <libhal/serial/interface.hpp>
#include <libhal/serial/util.hpp>
#include <libhal/timeout.hpp>
#include <string_view>

#include "../network.hpp"
#include "../util.hpp"

namespace hal::esp8266::at {
/**
 * @brief wlan_client AT command driver for connecting to WiFi Access points
 *
 */
class wlan_client
{
public:
  static result<wlan_client> create(hal::serial& p_serial,
                                    std::string_view p_ssid,
                                    std::string_view p_password,
                                    timeout auto p_timeout)
  {
    static constexpr auto serial_settings = hal::serial::settings{
      .baud_rate = 115200,
      .stop = hal::serial::settings::stop_bits::one,
      .parity = hal::serial::settings::parity::none,
    };

    HAL_CHECK(p_serial.configure(serial_settings));
    HAL_CHECK(p_serial.flush());

    wlan_client client(p_serial);

    HAL_CHECK(client.reset(p_timeout));
    HAL_CHECK(client.connect(p_ssid, p_password, p_timeout));

    return client;
  }

  class tcp_socket : public hal::esp8266::tcp_socket_client
  {
  public:
    tcp_socket(wlan_client& p_wlan)
      : m_wlan(&p_wlan)
      , m_parser(*m_wlan->m_serial)
    {
    }

    hal::result<send_t> driver_send(std::span<const hal::byte> p_data)
    {
      if (p_data.size() > maximum_transmit_packet_size) {
        return new_error(std::errc::file_too_large);
      }

      auto send_length = HAL_CHECK(integer_string::create(p_data.size()));
      HAL_CHECK(m_wlan->write("AT+CIPSENDBUF="));
      HAL_CHECK(m_wlan->write(send_length.str()));
      HAL_CHECK(m_wlan->write("\r\n"));
      HAL_CHECK(hal::write(*m_wlan->m_serial, p_data));

      return send_t{ .sent = p_data };
    }

    hal::result<receive_t> driver_receive(std::span<hal::byte> p_data)
    {
      auto data = HAL_CHECK(m_parser(p_data));
      return receive_t{ .received = data };
    }

  private:
    wlan_client* m_wlan;
    packet_parser m_parser;
  };

  result<tcp_socket> socket_connect(std::string_view p_domain,
                                    std::string_view p_port,
                                    timeout auto p_timeout)
  {
    // Connect to web server
    HAL_CHECK(write("AT+CIPSTART=\"TCP\",\""));
    HAL_CHECK(write(p_domain));
    HAL_CHECK(write("\","));
    HAL_CHECK(write(p_port));
    HAL_CHECK(write("\r\n"));
    HAL_CHECK(try_until(skip(ok_response), p_timeout));

    return tcp_socket(*this);
  }

private:
  /**
   * @param p_serial the serial port connected to the wlan_client
   * @param p_baud_rate the operating baud rate for the wlan_client
   *
   */
  wlan_client(hal::serial& p_serial)
    : m_serial{ &p_serial }
  {
  }

  [[nodiscard]] status reset(timeout auto p_timeout)
  {
    // Reset the device
    HAL_CHECK(write("AT+RST\r\n"));
    HAL_CHECK(wait_for_reset_complete(p_timeout));

    // Turn off echo
    HAL_CHECK(write("ATE0\r\n"));
    HAL_CHECK(wait_for_ok(p_timeout));

    return hal::success();
  }

  [[nodiscard]] status connect(std::string_view p_ssid,
                               std::string_view p_password,
                               timeout auto p_timeout)
  {
    // Configure as WiFi Station (client) mode
    HAL_CHECK(write("AT+CWMODE=1\r\n"));
    HAL_CHECK(wait_for_ok(p_timeout));

    // Connect to wifi access point
    HAL_CHECK(write("AT+CWJAP_CUR=\""));
    HAL_CHECK(write(p_ssid));
    HAL_CHECK(write("\",\""));
    HAL_CHECK(write(p_password));
    HAL_CHECK(write("\"\r\n"));
    HAL_CHECK(wait_for_ok(p_timeout));

    m_connected = true;
    return hal::success();
  }

  // [[nodiscard]] hal::status write(std::string_view p_string)
  // {
  //   return hal::write(*m_serial, p_string);
  // }

  [[nodiscard]] hal::status write(std::span<const char> p_string)
  {
    return hal::write(*m_serial, hal::as_bytes(p_string));
  }

  [[nodiscard]] hal::skip_past skip(std::string_view p_string)
  {
    return hal::skip_past(*m_serial, as_bytes(p_string));
  }

  [[nodiscard]] hal::status wait_for_ok(timeout auto p_timeout)
  {
    HAL_CHECK(try_until(skip(ok_response), p_timeout));

    return hal::success();
  }

  [[nodiscard]] hal::status wait_for_reset_complete(timeout auto p_timeout)
  {
    HAL_CHECK(try_until(skip(reset_complete), p_timeout));

    return hal::success();
  }

  serial* m_serial;
  bool m_connected = false;
};
}  // namespace hal::esp8266::at