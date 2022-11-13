#include <libesp8266/http_request.hpp>

#include <boost/ut.hpp>
#include <libhal/comparison.hpp>

template<typename T, size_t size>
constexpr bool operator==(const std::array<T, size>& p_array,
                          const std::span<const T>& p_span) noexcept
{
  if (p_span.size() != size) {
    return false;
  }

  return std::equal(p_array.begin(), p_array.end(), p_span.begin());
}

template<typename T, size_t size>
constexpr bool operator!=(const std::array<T, size>& p_array,
                          const std::span<const T>& p_span) noexcept
{
  return !(p_array == p_span);
}

namespace hal::esp8266::at {
namespace {
struct mock_serial : public hal::serial
{
  static constexpr std::string_view ipd = "OK\r\n+IPD,14:Hello, World\r\n";

  status driver_configure(
    [[maybe_unused]] const settings& p_settings) noexcept override
  {
    return hal::success();
  }

  result<write_t> driver_write(
    std::span<const hal::byte> p_data) noexcept override
  {
    std::array<hal::byte, 2> end_of_header{ '\r', '\n' };
    for (const auto& byte : p_data) {
      putchar(static_cast<char>(byte));
    }
    if (p_data == end_of_header) {
      m_end_of_header_counter++;
    } else {
      m_end_of_header_counter = 0;
    }

    if (m_end_of_header_counter == 2) {
      m_packet_sending = true;
      m_packet = std::span<const char>{ ipd.begin(), ipd.size() };
    }

    return write_t{
      .transmitted = p_data,
      .remaining = std::span<const hal::byte>(),
    };
  }

  result<read_t> driver_read(std::span<hal::byte> p_data) noexcept override
  {
    if (m_packet_sending) {
      size_t smallest_size = std::min(p_data.size(), m_packet.size());
      std::copy_n(m_packet.begin(), smallest_size, p_data.begin());
      m_packet = m_packet.subspan(smallest_size);

      if (m_packet.empty()) {
        m_packet_sending = false;
        m_end_of_header_counter = 0;
      }

      return read_t{
        .received = p_data.subspan(0, smallest_size),
        .remaining = p_data.subspan(smallest_size),
        .available = 0,
        .capacity = 1024,
      };
    } else {
      std::string_view ok_response = " OK\r\n";
      size_t index = m_rotation++ % ok_response.size();
      p_data[0] = static_cast<hal::byte>(ok_response[index]);

      return read_t{
        .received = p_data.subspan(0, 1),
        .remaining = p_data.subspan(1),
        .available = 0,
        .capacity = 1024,
      };
    }
  }

  status driver_flush() noexcept override
  {
    return hal::success();
  }

  size_t m_rotation = 0;
  int m_end_of_header_counter = 0;
  bool m_packet_sending = false;
  std::span<const char> m_packet{ ipd.begin(), ipd.size() };
};

auto counting_timeout(unsigned p_counts)
{
  return [p_counts]() mutable -> status {
    if (p_counts == 0) {
      return new_error(std::errc::timed_out);
    }
    p_counts--;
    return success();
  };
}

}  // namespace

boost::ut::suite http_client_test = []() {
  using namespace boost::ut;

  "http_request()"_test = []() {
    // Setup
    mock_serial mock;
    auto wlan_client =
      wlan_client::create(mock, "ssid", "password", counting_timeout(1000))
        .value();

    // Exercise
    auto response0 = http_request<4096>(wlan_client,
                                        request_t{
                                          .method = http_method::GET,
                                          .domain = "example.com",
                                        },
                                        counting_timeout(1000));

    auto response1 = http_request<4096>(wlan_client,
                                        request_t{
                                          .method = http_method::POST,
                                          .domain = "example.com",
                                          .body = "Hello, World!\n",
                                        },
                                        counting_timeout(1000));

    auto response2 =
      http_request<4096>(wlan_client,
                         request_t{
                           .method = http_method::CONNECT,
                           .domain = "example.com",
                           .body = "username=slim,password=shaddy\n",
                         },
                         counting_timeout(1000));

    // Verify
    expect(response0.has_value());
    expect(response1.has_value());
    expect(response2.has_value());
  };
};
}  // namespace hal::esp8266::at