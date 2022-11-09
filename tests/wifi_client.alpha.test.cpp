#include <libesp8266/wifi_client.alpha.hpp>

#include <boost/ut.hpp>

namespace hal::esp8266::alpha {
namespace {
struct mock_serial : public hal::serial
{

  status driver_configure(
    [[maybe_unused]] const settings& p_settings) noexcept override
  {
    return hal::success();
  }

  result<write_t> driver_write(
    std::span<const hal::byte> p_data) noexcept override
  {
    for (const auto& byte : p_data) {
      putchar(static_cast<char>(byte));
    }

    return write_t{
      .transmitted = p_data,
      .remaining = std::span<const hal::byte>(),
    };
  }

  result<read_t> driver_read(
    [[maybe_unused]] std::span<hal::byte> p_data) noexcept override
  {
    // static constexpr std::string_view ok = "OK\r\n";
    // static constexpr std::string_view blank = "";
    // static constexpr std::string_view ready = "\r\nready\r\n";
    // static constexpr std::string_view connected = "WIFI GOT
    // IP\r\n\r\nOK\r\n";

    // std::array outputs{ blank, ok, blank, ready, blank, blank, connected };
    // m_rotation++;
    // auto select = m_rotation % outputs.size();
    // printf("rotation = %d :: select = %lu\n", m_rotation, select);
    return read_t{
      .received = std::span<hal::byte>(),
      .remaining = std::span<hal::byte>(),
      .available = 0,
      .capacity = 1024,
    };
  }
  status driver_flush() noexcept override { return hal::success(); }

  int m_rotation = 0;
};
} // namespace

boost::ut::suite wifi_client_alpha_test = []() {
  using namespace boost::ut;

  "wifi_client()"_test = []() {
    // Setup
    mock_serial mock;
    std::array<hal::byte, 1024> response_buffer;
    [[maybe_unused]] auto wifi_client =
      wifi_client::create(mock, "ssid", "password", response_buffer).value();

    // Exercise

    // Verify
  };
};
} // namespace hal::rmd