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

namespace hal::esp8266 {
extern void http_response_test();
}  // namespace hal::esp8266

namespace hal::esp8266::http {
extern void util_test();
}  // namespace hal::esp8266::http

namespace hal::esp8266::at {
extern void socket_test();
extern void wlan_client_test();
}  // namespace hal::esp8266::at

int main()
{
  hal::esp8266::http_response_test();
  hal::esp8266::http::util_test();
  hal::esp8266::at::socket_test();
  hal::esp8266::at::wlan_client_test();
}