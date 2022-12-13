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