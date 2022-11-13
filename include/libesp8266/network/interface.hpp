#pragma once

#include <libhal/error.hpp>

namespace hal::esp8266 {
class tcp_socket_client
{
public:
  struct send_t
  {
    std::span<const hal::byte> sent;
  };

  struct receive_t
  {
    std::span<hal::byte> received;
  };

  [[nodiscard]] hal::result<send_t> send(std::span<const hal::byte> p_data)
  {
    return driver_send(p_data);
  }

  [[nodiscard]] hal::result<receive_t> receive(std::span<hal::byte> p_data)
  {
    return driver_receive(p_data);
  }

  virtual ~tcp_socket_client() = default;

private:
  virtual hal::result<send_t> driver_send(
    std::span<const hal::byte> p_data) = 0;
  virtual hal::result<receive_t> driver_receive(
    std::span<hal::byte> p_data) = 0;
};
}  // namespace hal::esp8266
