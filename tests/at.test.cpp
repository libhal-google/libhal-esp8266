#include <libhal-esp8266/at.hpp>

#include "helpers.hpp"

#include <boost/ut.hpp>

namespace hal::esp8266 {
void at_test()
{
  using namespace boost::ut;

  "at::create() + ::reset()"_test = []() {
    using namespace std::literals;
    // Setup
    mock_serial mock;
    mock.m_stream_out = stream_out("ready\r\n OK\r\n OK\r\n"sv);

    // Exercise
    // Verify
    [[maybe_unused]] at my_at(mock, hal::never_timeout());
  };
}
}  // namespace hal::esp8266
