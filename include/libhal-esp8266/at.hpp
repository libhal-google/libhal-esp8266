// Copyright 2024 Khalil Estell
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

#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include <libhal/functional.hpp>
#include <libhal/serial.hpp>
#include <libhal/timeout.hpp>

/**
 * @brief libhal compatible libraries for the esp8266 device and microcontroller
 *
 */
namespace hal::esp8266 {

/**
 * @brief AT Command network driver for the esp8266
 *
 * The esp8266::at driver can be used to connect to a WiFi access points (AP)
 * and sending network traffic using TCP and UDP over IP.
 */
class at
{
public:
  using deadline = hal::function_ref<hal::timeout_function>;

  enum class socket_type : std::uint8_t
  {
    tcp,
    udp,
  };

  struct socket_config
  {
    socket_type type = socket_type::tcp;
    std::string_view domain;
    std::uint16_t port = 80;
  };

  /**
   * @param p_serial the serial port connected to the wlan_client
   *
   */
  at(hal::serial& p_serial, deadline p_timeout);

  /**
   * @brief Soft reset the device
   *
   * @param p_timeout - amount of time to wait for the device to boot up before
   * throwing a hal::timed_out exception.
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   */
  void reset(deadline p_timeout);

  /**
   * @brief Connect to an access point (AP)
   *
   * @param p_ssid - target WiFi's SSID
   * @param p_password - target WiFi's password
   * @param p_timeout - amount of time to wait for the device to connect to an
   * access point.
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   */
  void connect_to_ap(std::string_view p_ssid,
                     std::string_view p_password,
                     deadline p_timeout);
  /**
   * @brief Set the ip address
   *
   * Must be called after calling `connect_to_ap`
   *
   * @param p_ip - string representation of an IPv4 address
   * @param p_timeout - amount of time to wait for the device to set the IP
   * address.
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   */
  void set_ip_address(std::string_view p_ip, deadline p_timeout);

  /**
   * @brief Check if the device is connected to a WiFi access point (AP)
   *
   * @param p_timeout - amount of time to determine if the device is connected
   * to an access point.
   * @return true - device is connected to the access point
   * @return false - device is not connected to an access point
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   * @throws hal::io_error - if the device does not respond with a valid
   * response
   */
  [[nodiscard]] bool is_connected_to_ap(deadline p_timeout);

  /**
   * @brief Disconnect from an access point (AP)
   *
   * @param p_timeout - amount of time to disconnect from an access point and
   * get verification that it has occurred.
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   */
  void disconnect_from_ap(deadline p_timeout);

  /**
   * @brief Connect to a server
   *
   * @param p_config - socket configuration and server information
   * @param p_timeout - amount of time to wait for the device to boot up before
   * throwing a hal::timed_out exception.
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   */
  void connect_to_server(socket_config p_config, deadline p_timeout);

  /**
   * @brief Determine if the device is connected to a server
   *
   * @param p_timeout - amount of time to wait for the device to determine if it
   * is connected to a server.
   * @return true - device is connected to the access point
   * @return false - device is not connected to an access point
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   * @throws hal::io_error - if the device does not respond with a valid
   * response
   */
  [[nodiscard]] bool is_connected_to_server(deadline p_timeout);

  /**
   * @brief Write data to the server
   *
   * NOTE: Must have already connected to the server via `connect_to_server`.
   *
   * @param p_data - buffer of data to write the server
   * @param p_timeout - amount of time to wait for the device to boot up before
   * throwing a hal::timed_out exception.
   * @return std::span<const hal::byte> - the amount of bytes transmitted to the
   * server. This may be equal to or less than the data that was passed by
   * p_data.
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   */
  [[nodiscard]] std::span<const hal::byte> server_write(
    std::span<const hal::byte> p_data,
    deadline p_timeout);

  /**
   * @brief Read response data from the server
   *
   * @param p_data - buffer to receive data from the server.
   * @return std::span<hal::byte> - The filled portion of the input buffer. The
   * size of this buffer indicates the number of bytes read. The address points
   * to the start of the buffer passed into the read() function.
   *
   */
  [[nodiscard]] std::span<hal::byte> server_read(std::span<hal::byte> p_data);

  /**
   * @brief Disconnect from the server
   *
   * @param p_timeout - amount of time to wait for the device to boot up before
   * throwing a hal::timed_out exception.
   * @throws hal::timed_out - if the operation did not complete in the time
   * specified.
   */
  void disconnect_from_server(deadline p_timeout);

private:
  class packet_manager
  {
  public:
    packet_manager() noexcept;
    void find(hal::serial& p_serial) noexcept;
    bool is_complete_header() noexcept;
    [[nodiscard]] std::uint16_t packet_length() noexcept;
    [[nodiscard]] std::span<hal::byte> read_packet(
      hal::serial& p_serial,
      std::span<hal::byte> p_buffer) noexcept;
    void reset() noexcept;
    void set_state(std::uint8_t p_state) noexcept;

  private:
    void update_state(hal::byte p_byte) noexcept;
    std::uint8_t m_state;
    std::uint16_t m_length;
  };

  hal::serial* m_serial;
  packet_manager m_packet_manager;
};
}  // namespace hal::esp8266
