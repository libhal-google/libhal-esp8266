// Copyright 2023 Google LLC
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

#include <array>
#include <cstdint>
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

  struct read_t
  {
    // The buffer containing the bytes read from the server
    std::span<hal::byte> data;
  };

  struct write_t
  {
    // The buffer that was written to the server
    std::span<const hal::byte> data;
  };

  [[nodiscard]] static result<at> create(hal::serial& p_serial,
                                         deadline p_timeout);
  template<unsigned id>
  [[nodiscard]] static result<at&> initialize(hal::serial& p_serial,
                                              deadline p_timeout);

  // System Control Commands
  [[nodiscard]] hal::status reset(deadline p_timeout);

  // WiFi access point commands
  [[nodiscard]] hal::status connect_to_ap(std::string_view p_ssid,
                                          std::string_view p_password,
                                          deadline p_timeout);
  [[nodiscard]] hal::status set_ip_address(std::string_view p_ip,
                                           deadline p_timeout);
  [[nodiscard]] hal::result<bool> is_connected_to_ap(deadline p_timeout);
  [[nodiscard]] hal::status disconnect_from_ap(deadline p_timeout);

  // TCP/UDP AT commands
  [[nodiscard]] hal::status connect_to_server(socket_config p_config,
                                              deadline p_timeout);
  [[nodiscard]] hal::result<bool> is_connected_to_server(deadline p_timeout);
  [[nodiscard]] hal::result<write_t> server_write(
    std::span<const hal::byte> p_data,
    deadline p_timeout);
  [[nodiscard]] hal::result<read_t> server_read(std::span<hal::byte> p_data);
  [[nodiscard]] hal::status disconnect_from_server(deadline p_timeout);

private:
  class packet_manager
  {
  public:
    packet_manager();
    void find(hal::serial& p_serial);
    bool is_complete_header();
    std::uint16_t packet_length();
    hal::result<std::span<hal::byte>> read_packet(
      hal::serial& p_serial,
      std::span<hal::byte> p_buffer);
    void reset();
    void set_state(std::uint8_t p_state);

  private:
    void update_state(hal::byte p_byte);
    std::uint8_t m_state;
    std::uint16_t m_length;
  };

  /**
   * @param p_serial the serial port connected to the wlan_client
   *
   */
  at(hal::serial& p_serial);

  hal::serial* m_serial;
  packet_manager m_packet_manager;
};
}  // namespace hal::esp8266
