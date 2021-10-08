#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <span>

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

  enum class request_status
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
    , m_status(request_status::initialized)
    , m_baud_rate(p_baud_rate)

  {}

  bool driver_initialize() override;

  /**
   * @brief Tests that the esp8266 can respond to commands.
   *
   */
  void test_module();
  /**
   * @brief Resets esp8266 using serial command.
   *
   */
  void software_reset();
  /**
   * @brief Connect to WiFi access point. The device must be disconnected first
   * before attempting to connect to another access point.
   *
   * @param ssid name of the access point
   * @param password the password for the access point
   */
  void connect(std::string_view ssid, std::string_view password);
  /**
   * @brief checks the connection status to an access point.
   *
   * @return true is connected to access point
   * @return false is NOT connected access point
   */
  bool connect();
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
   * @param p_domain domain name of the server to connect to. This should not
   * include stuff like "http://" or "www". An example would be `google.com`,
   * `example.com`, or `developer.mozilla.org`.
   * @param p_path path to the resource within the domain url. To get the root
   * page, use "/" (or "/index.html"). URL parameters can also be placed in the
   * path as well such as "/search?query=esp8266&price=lowest"
   * @param p_method which http method to use for this request. Most web servers
   * use GET and POST and tend to ignore the others.
   * @param p_send_data data to transmit to web server. This field is typically
   * used (or non-null) when performing POST requests. Typically this field will
   * be ignored if the method choosen is HEAD or GET. Set this to an empty span
   * if there is no data to be sent.
   * @param p_port which server port number to connect to.
   */
  void request(std::string_view p_domain,
               std::string_view p_path = "/",
               method p_method = method::GET,
               std::span<const std::byte> p_send_data = {},
               uint16_t p_port = 80);
  /**
   * @brief Returns a const reference to the response buffer. This function
   * should not be called unless the progress() function returns "completed",
   * otherwise the contents of the buffer are undefined.
   *
   * @return std::span<std::byte> a span that points to p_response_buffer with a
   * size equal to the number of bytes retrieved from the response buffer.
   */
  std::span<const std::byte> response() { return m_response_span; }
  /**
   * @brief After issuing a request, this function must be called in order to
   * progress the http request. This function manages, connecting to the server,
   * sending the request to server and receiving data from the server.
   *
   * @return request_status is the state of the current transaction. This value
   * can be checked to determine if a certain stage is taking too long.
   */
  request_status progress();

private:
  serial& m_serial;
  std::span<std::byte> m_request_span;
  std::span<std::byte> m_response_span;
  std::atomic<request_status> m_status;
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
