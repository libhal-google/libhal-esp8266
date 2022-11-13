#include <libesp8266/at/wlan_client.hpp>

#include <boost/ut.hpp>

namespace hal::esp8266::at {
namespace {

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
      .received = p_buffer.subspan(0, size),
      .remaining = p_buffer.subspan(size),
      .available = 0,
      .capacity = 1024,
    };
  }

private:
  std::span<const hal::byte> m_output{};
};

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
    return m_stream_out(p_data);
  }

  status driver_flush() noexcept override
  {
    return hal::success();
  }

  size_t rotation = 0;
  stream_out m_stream_out;
};
}  // namespace

boost::ut::suite wlan_client_test = []() {
  using namespace boost::ut;

  "wlan_client()"_test = []() {
    using namespace std::literals;
    // Setup
    mock_serial mock;
    mock.m_stream_out =
      stream_out("ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n"sv);
    [[maybe_unused]] auto wlan_client =
      wlan_client::create(mock, "ssid", "password", hal::never_timeout())
        .value();
    // Exercise
    // Verify
  };

  "wlan_client::socket_connect()"_test = []() {
    using namespace std::literals;
    // Setup
    mock_serial mock;
    mock.m_stream_out =
      stream_out("ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n"sv);
    auto wlan_client =
      wlan_client::create(mock, "ssid", "password", hal::never_timeout())
        .value();
    auto tcp =
      wlan_client.socket_connect("example.com", "80", hal::never_timeout())
        .value();
    tcp.send(hal::as_bytes("Hello, World\r\n\r\n"sv)).value();

    mock.m_stream_out = stream_out("+IPD,20:response from server"sv);

    std::array<hal::byte, 1024> buffer;
    auto actual_response = tcp.receive(buffer).value().received;

    // Exercise

    // Verify
    printf("Response: %.*s\n",
           static_cast<int>(actual_response.size()),
           actual_response.data());
  };
};
}  // namespace hal::esp8266::at