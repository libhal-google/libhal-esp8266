#pragma once

#include <array>
#include <atomic>
#include <charconv>
#include <memory_resource>
#include <span>
#include <string>

#include <libembeddedhal/driver.hpp>
#include <libembeddedhal/serial.hpp>
#include <nonstd/ring_span.hpp>

namespace embed {

template<typename T>
class read_until_buffer
{
public:
  read_until_buffer(embed::serial& p_serial,
                    std::span<const T> p_sequence,
                    std::span<T> p_memory)
    : m_serial(p_serial)
    , m_memory(p_memory)
    , m_sequence(p_sequence)
  {}

  int search()
  {
    constexpr size_t single_byte_length = 1;
    std::array<std::byte, single_byte_length> buffer;

    auto received_byte = m_serial.read(buffer);
    if (received_byte.size() == single_byte_length) {
      m_memory[m_memory_index] = received_byte[0];
      if (m_sequence[m_search_index++] == received_byte[0]) {
        if (m_search_index == m_sequence.size()) {
          m_location = m_memory_index - m_sequence.size();
        }
      } else {
        m_search_index = 0;
      }
      m_memory_index++;
    }

    return m_location;
  }

private:
  int m_location = -1;
  int m_memory_index = 0;
  int m_search_index = 0;
  embed::serial& m_serial;
  std::span<T> m_memory;
  std::span<const T> m_sequence;
};

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
  static constexpr char ok_response[] = "OK\r\n";
  /// Confirmation response after a wifi successfully connected
  static constexpr char wifi_connected[] = "WIFI GOT IP\r\n\r\nOK\r\n";
  /// Confirmation response after a reset complets
  static constexpr char reset_complete[] = "ready\r\n";
  /// End of header
  static constexpr std::string_view end_of_header = "\r\n\r\n";
  /// The maximum packet size for esp8266 AT commands
  static constexpr size_t maximum_response_packet_size = 1460;
  static constexpr size_t maximum_transmit_packet_size = 2048;

  /// The type of password security used for the access point.
  enum class access_point_security
  {
    open,
    wep,
    wpa_psk,
    wpa2_psk,
    wpa_wpa2_psk,
  };

  enum class http_method
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
    http_method method = http_method::GET;
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

  struct header_t
  {
    uint32_t status_code = 0;
    size_t content_length = 0;
    size_t header_length = 0;
    bool is_valid()
    {
      return status_code != 0 && content_length != 0 && header_length != 0;
    }
  };

  enum class state
  {
    // Phase 1: Connecting to Wifi access point
    start,
    reset,
    disable_echo,
    configure_as_http_client,
    attempting_ap_connection,
    connected_to_ap,
    // Phase 2: HTTP request
    connecting_to_server,
    sending_request,
    gathering_response,
    complete,
    failure,
  };

  static std::string_view to_string(http_method p_method);

  /**
   * @param p_serial the serial port connected to the esp8266
   * @param p_baud_rate the operating baud rate for the esp8266
   *
   */
  esp8266(embed::serial& p_serial,
          std::string_view p_ssid,
          std::string_view p_password,
          std::span<std::byte> p_response_span)
    : m_serial(p_serial)
    , m_response_span(p_response_span)
    , m_state(state::start)
    , m_ssid(p_ssid)
    , m_password(p_password)
  {}

  bool driver_initialize() override;
  /**
   * @brief Change the access point to connect to. Running get_status() after
   * calling this will disconnect from the previous access point and attempt to
   * connect to the new one.
   *
   * @param p_ssid name of the access point
   * @param p_password the password for the access point
   */
  void change_access_point(std::string_view p_ssid,
                           std::string_view p_password);
  /**
   * @brief checks the connection state to an access point.
   *
   * @return true is connected to access point
   * @return false is NOT connected access point
   */
  bool connected();
  /**
   * @brief Starts a http request. This function will issue a command to connect
   * with a server and will return immediately. This function will also abort
   * any ongoing requests if they are in progress. This function is non-blocking
   * and as such, to progress the request further the `progress()` function must
   * be called, until it returns "complete" or an error condition has occurred.
   *
   * @note Only currently support GET requests
   *
   * @param true if the request is valid and can be transmitted
   * @param false if the request is invalid and cannot be transmitted. This
   * typically occurs if the packet size if greater than 2048 bytes.
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
  state get_status();
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
    while (m_serial.busy()) {
      continue;
    }
  }

  void read(std::string_view p_string);
  inline std::span<std::byte> read_packet();

  header_t get_header(std::span<std::byte> p_header_info)
  {
    std::string_view header_info(
      reinterpret_cast<const char*>(p_header_info.data()),
      reinterpret_cast<const char*>(p_header_info.data() +
                                    p_header_info.size()));

    constexpr header_t failure_header{};

    header_t new_header;
    size_t index = 0;
    int count = 0;

    index = header_info.find("HTTP/1.1 ");
    if (index == std::string_view::npos) {
      return failure_header;
    }

    count = sscanf(
      &header_info[index], "HTTP/1.1 %" PRIu32 " OK", &new_header.status_code);
    if (count != 1) {
      return failure_header;
    }

    index = header_info.find("Content-Length: ");
    if (index == std::string_view::npos) {
      return failure_header;
    }

    count = sscanf(&header_info[index],
                   "Content-Length: %" PRIu32,
                   &new_header.content_length);
    if (count != 1) {
      return failure_header;
    }

    index = header_info.find(end_of_header);
    if (index == std::string_view::npos) {
      return failure_header;
    }

    new_header.header_length = index + end_of_header.size();

    return new_header;
  }

  serial& m_serial;
  std::span<std::byte> m_response_span;
  std::atomic<state> m_state;
  std::string_view m_ssid;
  std::string_view m_password;
  request_t m_request;
  header_t m_header;
  std::array<std::byte, maximum_response_packet_size> m_packet;
};

template<size_t ResponseBufferSize = esp8266::maximum_response_packet_size * 2>
class static_esp8266 : public esp8266
{
public:
  static_esp8266(embed::serial& p_serial,
                 std::string_view p_ssid,
                 std::string_view p_password)
    : esp8266(p_serial, p_ssid, p_password, m_response_buffer)
  {}

  static_assert(ResponseBufferSize >=
                esp8266::maximum_response_packet_size * 2);

private:
  std::array<std::byte, ResponseBufferSize> m_response_buffer;
};
} // namespace embed

namespace embed {
inline bool esp8266::driver_initialize()
{
  m_serial.settings().baud_rate = 115200;
  m_serial.settings().frame_size = 8;
  m_serial.settings().parity = serial_settings::parity::none;
  m_serial.settings().stop = serial_settings::stop_bits::one;
  if (!m_serial.initialize()) {
    return false;
  }
  m_serial.flush();
  m_state = state::reset;
  return true;
}
inline void esp8266::change_access_point(std::string_view p_ssid,
                                         std::string_view p_password)
{
  m_ssid = p_ssid;
  m_password = p_password;
  m_state = state::connected_to_ap;
}
inline bool esp8266::connected()
{
  return m_state >= state::connected_to_ap;
}
inline void esp8266::request(request_t p_request)
{
  m_request = p_request;
  m_state = state::connecting_to_server;
}
inline void esp8266::read(std::string_view p_sequence)
{
  std::array<std::byte, 1024 * 2> response_buffer;
  std::ranges::fill(response_buffer, std::byte{ 0 });

  auto sequence_span =
    std::span<const char>(p_sequence.begin(), p_sequence.end());
  auto sequence_bytes = std::as_bytes(sequence_span);

  read_until_buffer<std::byte> reader(
    m_serial, sequence_bytes, response_buffer);

  while (reader.search() == -1) {
    continue;
  }
}
inline std::span<std::byte> esp8266::read_packet()
{
  std::ranges::fill(m_packet, std::byte{ 0 });

  std::array<std::byte, 1> colon = { std::byte{ ':' } };
  read_until_buffer<std::byte> reader(m_serial, colon, m_packet);

  while (reader.search() == -1) {
    continue;
  }

  size_t packet_length = 0;
  int count = sscanf(
    reinterpret_cast<char*>(m_packet.data()), "\r\n+IPD,%zu:", &packet_length);

  if (count != 1 || packet_length > m_packet.size()) {
    return {};
  }

  size_t read_index = 0;
  std::span<std::byte> frame_span(m_packet.begin(), packet_length);
  while (frame_span.size() != 0) {
    auto received_bytes = m_serial.read(frame_span);
    frame_span = frame_span.subspan(received_bytes.size());
  }

  return { m_packet.begin(), packet_length };
}
inline auto esp8266::get_status() -> state
{
  switch (m_state) {
    case state::start:
    case state::reset:
      // // Force device out of transparent wifi mode if it was previous in that
      // // mode
      // write("+++");
      // // Bring device back to default settings, aborting all transactions and
      // // disconnecting from any access points.
      // write("AT+RST\r\n");
      // read(reset_complete);
      m_state = state::disable_echo;
      break;
    case state::disable_echo:
      // Disable echo
      write("ATE0\r\n");
      read(ok_response);
      m_state = state::configure_as_http_client;
      break;
    case state::configure_as_http_client:
      // Enable simple IP client mode
      write("AT+CWMODE=1\r\n");
      read(ok_response);
      m_state = state::attempting_ap_connection;
      break;
    case state::attempting_ap_connection:
      write("AT+CWJAP_CUR=\"");
      write(m_ssid);
      write("\",\"");
      write(m_password);
      write("\"\r\n");
      read(wifi_connected);
      m_state = state::connected_to_ap;
      break;
    case state::connected_to_ap:
      break;
    case state::connecting_to_server:
      write("AT+CIPSTART=\"TCP\",\"");
      write(m_request.domain);
      write("\",");
      write(m_request.port);
      write("\r\n");
      read(ok_response);
      m_state = state::sending_request;
      break;
    case state::sending_request: {
      int payload_length =
        snprintf(reinterpret_cast<char*>(m_response_span.data()),
                 m_response_span.size(),
                 // Request
                 "GET %s HTTP/1.1\r\n"
                 // Host Field
                 "Host: %s:%s\r\n"
                 // End of header
                 "\r\n\r\n",
                 m_request.path.data(),
                 m_request.domain.data(),
                 m_request.port.data());

      if (payload_length < 0) {
        m_state = state::failure;
        break;
      }

      std::array<char, 64> buffer;
      int cipsend_length = snprintf(
        buffer.data(), buffer.size(), "AT+CIPSEND=%d\r\n", payload_length);

      if (cipsend_length < 0) {
        m_state = state::failure;
        break;
      }

      write(std::string_view(buffer.data(), cipsend_length));
      read(ok_response);

      // send payload
      m_serial.write(m_response_span.subspan(0, payload_length));
      read(ok_response);

      m_state = state::gathering_response;
      break;
    }
    case state::gathering_response: {
      // Storing response
      std::ranges::fill(m_response_span, std::byte{ 0 });

      auto header_packet = read_packet();
      m_header = get_header(header_packet);
      if (!m_header.is_valid()) {
        m_state = state::failure;
        break;
      } else if (m_header.content_length > m_response_span.size()) {
        m_state = state::failure;
        break;
      } else if (header_packet.size() < maximum_response_packet_size) {
        std::copy_n(header_packet.begin() + m_header.header_length,
                    m_header.content_length,
                    m_response_span.begin());
      } else {
        // Pull out contents of body from header packet
        size_t bytes_retrieved = header_packet.size() - m_header.header_length;
        auto response_iterator =
          std::copy_n(header_packet.begin() + m_header.header_length,
                      bytes_retrieved,
                      m_response_span.begin());
        while (bytes_retrieved < m_header.content_length) {
          auto new_packet = read_packet();
          response_iterator = std::copy_n(
            new_packet.begin(), new_packet.size(), response_iterator);
          bytes_retrieved += header_packet.size();
        }
      }

      m_state = state::complete;
      break;
    }
    case state::complete:
      break;
    case state::failure:
      break;
  }

  return m_state;
}
inline std::string_view esp8266::to_string(http_method p_method)
{
  switch (p_method) {
    case http_method::GET:
      return "GET";
    case http_method::HEAD:
      return "HEAD";
    case http_method::POST:
      return "POST";
    case http_method::PUT:
      return "PUT";
    case http_method::DELETE:
      return "DELETE";
    case http_method::CONNECT:
      return "CONNECT";
    case http_method::OPTIONS:
      return "OPTIONS";
    case http_method::TRACE:
      return "TRACE";
    case http_method::PATCH:
      return "PATCH";
  }
}
} // namespace embed
