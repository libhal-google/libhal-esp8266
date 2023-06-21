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

#include <libhal-esp8266/at/wlan_client.hpp>

#include "../helpers.hpp"

#include <boost/ut.hpp>

namespace hal::esp8266::at {
void wlan_client_test()
{
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