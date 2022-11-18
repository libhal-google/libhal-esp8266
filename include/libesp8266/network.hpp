#pragma once

#include <span>

#include <libhal/error.hpp>

namespace hal::esp8266 {
class socket_client
{
public:
  struct write_t
  {
    std::span<const hal::byte> data;
  };

  struct read_t
  {
    std::span<hal::byte> data;
  };

  [[nodiscard]] hal::result<write_t> write(std::span<const hal::byte> p_data)
  {
    return driver_write(p_data);
  }

  [[nodiscard]] hal::result<read_t> read(std::span<hal::byte> p_data)
  {
    return driver_read(p_data);
  }

  virtual ~socket_client() = default;

private:
  virtual hal::result<write_t> driver_write(
    std::span<const hal::byte> p_data) = 0;
  virtual hal::result<read_t> driver_read(std::span<hal::byte> p_data) = 0;
};
}  // namespace hal::esp8266
