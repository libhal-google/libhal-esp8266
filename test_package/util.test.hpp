#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

#include <libhal-util/as_bytes.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/streams.hpp>
#include <libhal/serial.hpp>

namespace hal::esp8266 {

class stream_out
{
public:
  stream_out(std::span<const hal::byte> p_output)
    : m_output(p_output)
  {
  }

  stream_out(std::string_view p_output)
    : m_output(hal::as_bytes(p_output))
  {
  }

  stream_out()
  {
  }

  hal::serial::read_t operator()(std::span<hal::byte> p_buffer)
  {
    auto size = std::min(p_buffer.size(), m_output.size());
    std::copy_n(m_output.begin(), size, p_buffer.begin());
    m_output = m_output.subspan(size);

    return hal::serial::read_t{
      .data = p_buffer.subspan(0, size),
      .available = 0,
      .capacity = 1024,
    };
  }

private:
  std::span<const hal::byte> m_output{};
};

struct mock_serial : public hal::serial
{
  hal::status driver_configure(
    [[maybe_unused]] const settings& p_settings) override
  {
    return hal::success();
  }

  hal::result<write_t> driver_write(std::span<const hal::byte> p_data) override
  {
    for (const auto& byte : p_data) {
      putchar(static_cast<char>(byte));
    }

    return write_t{ .data = p_data };
  }

  result<read_t> driver_read(
    [[maybe_unused]] std::span<hal::byte> p_data) override
  {
    return m_stream_out(p_data);
  }

  hal::status driver_flush() override
  {
    return hal::success();
  }

  size_t rotation = 0;
  stream_out m_stream_out;
};
}  // namespace hal::esp8266
