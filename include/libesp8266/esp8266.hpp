#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <memory_resource>
#include <span>
#include <string>

#include <libembeddedhal/driver.hpp>
#include <libembeddedhal/serial.hpp>

namespace embed {
using namespace std::chrono_literals;

/**
 * @brief esp8266 AT command driver for connecting to WiFi Access points and
 * connecting to web servers.
 *
 */
class esp8266 : public embed::driver<>
{
public:
  /// Default baud rate for the esp8266 AT commands
  static constexpr uint32_t default_baud_rate = 115200;
  /// Confirmation response
  static constexpr char ok_response[] = "\r\nOK\r\n";
  /// Confirmation response after a wifi successfully connected
  static constexpr char wifi_connected[] = "WIFI GOT IP\r\n\r\nOK\r\n";
  /// The maximum packet size for esp8266 AT commands
  static constexpr size_t maximum_packet_size = 1460;

  /// The type of password security used for the access point.
  enum class access_point_security
  {
    open,
    wep,
    wpa_psk,
    wpa2_psk,
    wpa_wpa2_psk,
  };

  enum class method
  {
    /// The GET method requests a representation of the specified resource.
    /// Requests using GET should only retrieve data.
    GET,
    /// The HEAD method asks for a response identical to a GET request, but
    /// without the response body.
    HEAD,
    /// The POST method submits an entity to the specified resource, often
    /// causing a change in state or side effects on the server.
    POST,
    /// The PUT method replaces all current representations of the target
    /// resource with the request payload.
    PUT,
    /// The DELETE method deletes the specified resource.
    DELETE,
    /// The CONNECT method establishes a tunnel to the server identified by the
    /// target resource.
    CONNECT,
    /// The OPTIONS method describes the communication options for the target
    /// resource.
    OPTIONS,
    /// The TRACE method performs a message loop-back test along the path to the
    /// target resource.
    TRACE,
    /// The PATCH method applies partial modifications to a resource.
    PATCH,
  };

  struct request_t
  {
    /**
     * @brief Domain name of the server to connect to. This should not include
     * stuff like "http://" or "www". An example would be `google.com`,
     * `example.com`, or `developer.mozilla.org`.
     *
     */
    std::string_view domain;
    /**
     * @brief path to the resource within the domain url. To get the root page,
     * use "/" (or "/index.html"). URL parameters can also be placed in the path
     * as well such as "/search?query=esp8266&price=lowest"
     *
     */
    std::string_view path = "/";
    /**
     * @brief which http method to use for this request. Most web servers use
     * GET and POST and tend to ignore the others.
     *
     */
    method method = method::GET;
    /**
     * @brief Data to transmit to web server. This field is typically used (or
     * non-null) when performing POST requests. Typically this field will be
     * ignored if the method choosen is HEAD or GET. Set this to an empty span
     * if there is no data to be sent.
     *
     */
    std::span<const std::byte> send_data = {};
    /**
     * @brief which server port number to connect to.
     *
     */
    std::string_view port = "80";
  };

  enum class state
  {
    initialized,
    connecting_to_server,
    sending_request,
    waiting_for_response,
    complete,
    failure_404,
    corrupted_response,
  };

  /**
   * @param p_serial the serial port connected to the esp8266
   * @param p_baud_rate the operating baud rate for the esp8266
   *
   */
  esp8266(embed::serial& p_serial,
          std::span<std::byte> p_request_span,
          std::span<std::byte> p_response_span,
          uint32_t p_baud_rate = default_baud_rate)
    : m_serial(p_serial)
    , m_request_span(p_request_span)
    , m_response_span(p_response_span)
    , m_state(state::initialized)
    , m_baud_rate(p_baud_rate)

  {}

  bool driver_initialize() override;
  /**
   * @brief Connect to WiFi access point. The device must be disconnected first
   * before attempting to connect to another access point.
   *
   * @param ssid name of the access point
   * @param password the password for the access point
   */
  void connect(std::string_view ssid, std::string_view password);
  /**
   * @brief checks the connection state to an access point.
   *
   * @return true is connected to access point
   * @return false is NOT connected access point
   */
  bool connected();
  /**
   * @brief Disconnect from the access point.
   *
   */
  void disconnect();
  /**
   * @brief Starts a http request. This function will issue a command to connect
   * with a server and will return immediately. This function will also abort
   * any ongoing requests if they are in progress. This function is non-blocking
   * and as such, to progress the request further the `progress()` function must
   * be called, until it returns "complete" or an error condition has occurred.
   *
   */
  void request(request_t p_request);
  /**
   * @brief After issuing a request, this function must be called in order to
   * progress the http request. This function manages, connecting to the server,
   * sending the request to server and receiving data from the server.
   *
   * @return state is the state of the current transaction. This value
   * can be checked to determine if a certain stage is taking too long.
   */
  state progress();
  /**
   * @brief Returns a const reference to the response buffer. This function
   * should not be called unless the progress() function returns "completed",
   * otherwise the contents of the buffer are undefined.
   *
   * @return std::span<std::byte> a span that points to p_response_buffer with a
   * size equal to the number of bytes retrieved from the response buffer.
   */
  std::span<const std::byte> response() { return m_response_span; }

private:
  void write(std::string_view p_string)
  {
    m_serial.flush();
    std::span<const char> string_span(p_string.begin(), p_string.end());
    m_serial.write(std::as_bytes(string_span));
  }

  void read(std::string_view p_string) {}

  serial& m_serial;
  std::span<std::byte> m_request_span;
  std::span<std::byte> m_response_span;
  std::atomic<state> m_state;
  request_t m_request;
  const uint32_t m_baud_rate;
};

template<size_t RequestBufferSize = esp8266::maximum_packet_size,
         size_t ResponseBufferSize = esp8266::maximum_packet_size * 2>
class static_esp8266 : public esp8266
{
public:
  static_esp8266(embed::serial& p_serial,
                 uint32_t p_baud_rate = esp8266::default_baud_rate)
    : esp8266(p_serial, m_request_buffer, m_response_buffer, p_baud_rate)
  {}

private:
  std::array<std::byte, RequestBufferSize> m_request_buffer;
  std::array<std::byte, ResponseBufferSize> m_response_buffer;
};
} // namespace embed

namespace embed {
bool esp8266::driver_initialize()
{
  m_serial.settings().baud_rate = m_baud_rate;
  m_serial.settings().frame_size = 8;
  m_serial.settings().parity = serial_settings::parity::none;
  m_serial.settings().stop = serial_settings::stop_bits::one;

  if (!m_serial.initialize()) {
    return false;
  }

  // Force device out of transparent wifi mode if it was previous in that mode
  write("+++");

  // Bring device back to default settings, aborting all transactions and
  // disconnecting from any access points.
  write("AT+RST\r\n");
  read("\r\nready\r\n");

  // Disable echo
  write("ATE0\r\n");
  read(ok_response);

  // Enable simple IP client mode
  write("AT+CWMODE=1\r\n");
  read(ok_response);

  write("AT\r\n");
  read(ok_response);
}
void esp8266::connect(std::string_view ssid, std::string_view password)
{
  std::string payload;
  payload += "AT+CWJAP_CUR=\"";
  payload += ssid;
  payload += "\",\"";
  payload += password;
  payload += "\"\r\n";

  write(payload);

  return read(wifi_connected);
}
bool esp8266::connected()
{
  return true;
}
void esp8266::disconnect()
{
  write("AT+CWQAP\r\n");
  read(ok_response);
}
void esp8266::request(request_t p_request)
{
  // Save request information
  m_request = p_request;

  // Create stack allocated buffer
  std::array<char, 256> buffer{ 0 };
  std::pmr::monotonic_buffer_resource buffer_resource{ buffer.data(),
                                                       buffer.size() };
  std::pmr::string payload(&buffer_resource);

  // Create command for connection to server
  payload += "AT+CIPSTART=\"TCP\",\"";
  payload += m_request.domain;
  payload += "\",\"";
  payload += m_request.port;
  payload += "\"\r\n";

  // Send payload to esp8266
  write(payload);

  // Move to connecting to server
  m_state = state::connecting_to_server;
}
auto esp8266::progress() -> state
{
  switch (m_state) {
    case state::connecting_to_server:
      break;
    case state::sending_request:
      break;
    case state::waiting_for_response:
      break;
    case state::complete:
      break;
    default:
      break;
  }
}
} // namespace embed
