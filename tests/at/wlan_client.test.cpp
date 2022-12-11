#include <libesp8266/at/wlan_client.hpp>

#include "../helpers.hpp"

#include <boost/ut.hpp>

namespace hal::esp8266::at {
boost::ut::suite wlan_client_test = []() {
  using namespace boost::ut;

  "wlan_client()"_test = []() {
    using namespace std::literals;
    // Setup
    mock_serial mock;
    mock.m_stream_out =
      stream_out("ready\r\n OK\r\n OK\r\n OK\r\n OK\r\n OK\r\n"sv);
    // Exercise
    // Verify
    [[maybe_unused]] auto wlan_client =
      wlan_client::create(mock, "ssid", "password", hal::never_timeout())
        .value();
  };
};
}  // namespace hal::esp8266::at