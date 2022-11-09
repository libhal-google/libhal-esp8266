#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cinttypes>
#include <span>
#include <string_view>

#include <libhal/serial/interface.hpp>
#include <libhal/serial/util.hpp>

namespace hal::esp8266::alpha {

class read_into_buffer
{
public:
  read_into_buffer(hal::serial& p_serial)
    : m_serial{ &p_serial }
    , m_memory{}
  {
  }

  void new_buffer(std::span<hal::byte> p_memory)
  {
    m_memory = p_memory;
    m_read_index = 0;
  }

  result<bool> done()
  {
    if (m_read_index == m_memory.size()) {
      return true;
    } else {
      m_read_index += HAL_CHECK(m_serial->read(m_memory.subspan(m_read_index)))
                        .received.size();
    }

    return false;
  }

  result<bool> operator()() { return done(); }

private:
  hal::serial* m_serial;
  std::span<hal::byte> m_memory;
  size_t m_read_index = 0;
};

class command_and_find_response
{
public:
  command_and_find_response(hal::serial& p_serial)
    : m_serial(&p_serial)
    , m_command{}
    , m_sequence{}
  {
  }

  void new_search(std::span<const hal::byte> p_command,
                  std::span<const hal::byte> p_sequence)
  {
    m_search_index = 0;
    m_sent_command = false;
    m_command = p_command;
    m_sequence = p_sequence;
  }

  result<bool> done()
  {
    std::array<hal::byte, 1> buffer;

    if (m_search_index == m_sequence.size()) {
      return true;
    }

    if (!m_sent_command) {
      HAL_CHECK(m_serial->write(m_command));
      m_sent_command = true;
    }

    auto status = HAL_CHECK(m_serial->read(buffer));
    if (status.received.size() == buffer.size()) {
      if (m_sequence[m_search_index++] != status.received[0]) {
        m_search_index = 0;
      }
    }

    return false;
  }

  result<bool> operator()() { return done(); }

private:
  size_t m_search_index = 0;
  bool m_sent_command = false;
  hal::serial* m_serial;
  std::span<const hal::byte> m_command;
  std::span<const hal::byte> m_sequence;
};

class read_integer
{
public:
  read_integer(hal::serial& p_serial)
    : m_serial(&p_serial)
  {
  }

  void restart()
  {
    m_finished = false;
    m_found_digit = false;
    m_integer = 0;
  }

  result<bool> done()
  {
    std::array<hal::byte, 1> buffer;

    if (m_finished) {
      return true;
    }

    auto status = HAL_CHECK(m_serial->read(buffer));
    if (status.received.size() == buffer.size()) {
      if (std::isdigit(static_cast<char>(status.received[0]))) {
        m_integer *= 10;
        m_integer += status.received[0] - hal::byte('0');
        m_found_digit = true;
      } else if (m_found_digit) {
        m_finished = true;
      }
    }

    return m_finished;
  }

  result<bool> operator()() { return done(); }

  auto get() { return m_integer; }

private:
  bool m_finished = true;
  bool m_found_digit = false;
  uint32_t m_integer = 0;
  hal::serial* m_serial;
};

inline std::string_view to_string_view(std::span<hal::byte> byte_sequence)
{
  return std::string_view{ reinterpret_cast<const char*>(byte_sequence.data()),
                           reinterpret_cast<const char*>(
                             byte_sequence.data() + byte_sequence.size()) };
}

/**
 * @brief wifi_client AT command driver for connecting to WiFi Access points and
 * connecting to web servers.
 *
 */
class wifi_client
{
public:
  /// Default baud rate for the wifi_client AT commands
  static constexpr uint32_t default_baud_rate = 115200;
  /// Confirmation response
  static constexpr char ok_response[] = "OK\r\n";
  /// Confirmation response after a wifi successfully connected
  static constexpr char wifi_connected[] = "WIFI GOT IP\r\n\r\nOK\r\n";
  /// Confirmation response after a reset completes
  static constexpr char reset_complete[] = "ready\r\n";
  /// End of header
  static constexpr std::string_view end_of_header = "\r\n\r\n";
  /// The maximum packet size for wifi_client AT commands
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
     * as well such as "/search?query=wifi_client&price=lowest"
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
    std::span<const hal::byte> send_data = {};
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
    reset,
    disable_echo,
    configure_as_http_client,
    attempting_ap_connection,
    connected_to_ap,
    // Phase 2: HTTP request
    connecting_to_server,
    preparing_request,
    sending_request,
    get_first_packet_length,
    reading_first_packet,
    parsing_header,
    get_packet_length,
    read_packet_into_response,
    get_next_packet,
    close_connection,
    close_connection_failure,
    complete,
    failure,
  };

  enum class read_state
  {
    until_sequence,
    into_buffer,
    integer,
    complete,
  };

  static std::string_view to_string(http_method p_method);
  static std::string_view to_string(state p_method);

  static result<wifi_client> create(hal::serial& p_serial,
                                    std::string_view p_ssid,
                                    std::string_view p_password,
                                    std::span<hal::byte> p_response_span)
  {
    HAL_CHECK(p_serial.configure({
      .baud_rate = 115200,
      .stop = hal::serial::settings::stop_bits::one,
      .parity = hal::serial::settings::parity::none,
    }));

    HAL_CHECK(p_serial.flush());
    return wifi_client(p_serial, p_ssid, p_password, p_response_span);
  }

  /**
   * @brief Change the access point to connect to. Running work() after
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
  result<state> work();
  /**
   * @brief Returns a const reference to the response buffer. This function
   * should not be called unless the progress() function returns "completed",
   * otherwise the contents of the buffer are undefined.
   *
   * @return std::span<hal::byte> a span that points to p_response_buffer with a
   * size equal to the number of bytes retrieved from the response buffer.
   */
  std::span<const hal::byte> response() { return m_response; }

private:
  /**
   * @param p_serial the serial port connected to the wifi_client
   * @param p_baud_rate the operating baud rate for the wifi_client
   *
   */
  wifi_client(hal::serial& p_serial,
              std::string_view p_ssid,
              std::string_view p_password,
              std::span<hal::byte> p_response_span)
    : m_serial{ &p_serial }
    , m_response{ p_response_span }
    , m_ssid{ p_ssid }
    , m_password{ p_password }
    , m_commander{ p_serial }
    , m_reader{ p_serial }
    , m_integer_reader{ p_serial }
    , m_packet{}
    , m_request{}
    , m_header{}
  {
  }

  result<void> write(std::string_view p_string)
  {
    return hal::write(*m_serial, p_string);
  }

  void transition_state();

  header_t response_header_from_string(std::span<hal::byte> p_header_info)
  {
    std::string_view header_info = to_string_view(p_header_info);

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

    count = sscanf(
      &header_info[index], "Content-Length: %zu", &new_header.content_length);
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

  serial* m_serial;
  std::span<hal::byte> m_response;
  std::string_view m_ssid;
  std::string_view m_password;
  command_and_find_response m_commander;
  read_into_buffer m_reader;
  read_integer m_integer_reader;
  std::array<hal::byte, maximum_response_packet_size> m_packet;
  request_t m_request;
  header_t m_header;
  state m_state = state::reset;
  state m_next_state = state::reset;
  read_state m_read_state = read_state::complete;
  size_t m_request_length = 0;
  size_t m_response_position = 0;
};
} // namespace hal

namespace hal::esp8266::alpha {
inline void wifi_client::change_access_point(std::string_view p_ssid,
                                             std::string_view p_password)
{
  m_ssid = p_ssid;
  m_password = p_password;
  if (connected()) {
    m_next_state = state::connected_to_ap;
  }
}

inline bool wifi_client::connected()
{
  return static_cast<int>(m_state) >= static_cast<int>(state::connected_to_ap);
}

inline void wifi_client::request(request_t p_request)
{
  m_request = p_request;
  m_next_state = state::connecting_to_server;
  transition_state();
}

inline result<wifi_client::state> wifi_client::work()
{
  if (m_state == state::reset) {
    transition_state();
  }

  switch (m_read_state) {
    case read_state::until_sequence:
      if (HAL_CHECK(m_commander())) {
        m_read_state = read_state::complete;
      }
      break;
    case read_state::into_buffer:
      if (HAL_CHECK(m_reader())) {
        m_read_state = read_state::complete;
      }
      break;
    case read_state::integer:
      if (HAL_CHECK(m_integer_reader())) {
        m_read_state = read_state::complete;
      }
      break;
    case read_state::complete:
      m_state = m_next_state;
      transition_state();
      break;
  }

  return m_state;
}

inline void wifi_client::transition_state()
{
  switch (m_state) {
    case state::reset:
      m_next_state = state::disable_echo;
      break;
    case state::disable_echo:
      m_commander.new_search(hal::as_bytes("ATE0\r\n"),
                             hal::as_bytes(ok_response));
      m_next_state = state::configure_as_http_client;
      m_read_state = read_state::until_sequence;
      break;
    case state::configure_as_http_client:
      m_commander.new_search(hal::as_bytes("AT+CWMODE=1\r\n"),
                             hal::as_bytes(ok_response));
      m_next_state = state::attempting_ap_connection;
      m_read_state = read_state::until_sequence;
      break;
    case state::attempting_ap_connection:
      write("AT+CWJAP_CUR=\"");
      write(m_ssid);
      write("\",\"");
      write(m_password);
      m_commander.new_search(hal::as_bytes("\"\r\n"),
                             hal::as_bytes(ok_response));
      m_next_state = state::connected_to_ap;
      m_read_state = read_state::until_sequence;
      break;
    case state::connected_to_ap:
      break;
    case state::connecting_to_server:
      write("AT+CIPSTART=\"TCP\",\"");
      write(m_request.domain);
      write("\",");
      write(m_request.port);
      m_commander.new_search(hal::as_bytes("\r\n"), hal::as_bytes(ok_response));
      m_next_state = state::preparing_request;
      m_read_state = read_state::until_sequence;
      break;
    case state::preparing_request: {
      int length = snprintf(reinterpret_cast<char*>(m_response.data()),
                            m_response.size(),
                            // Request
                            "GET %s HTTP/1.1\r\n"
                            // Host Field
                            "Host: %s:%s\r\n"
                            // End of header
                            "\r\n\r\n",
                            m_request.path.data(),
                            m_request.domain.data(),
                            m_request.port.data());

      if (length >= 0) {
        m_request_length = static_cast<size_t>(length);
      } else {
        m_next_state = state::close_connection_failure;
        break;
      }

      std::array<char, 64> buffer;
      int cip_send_command_length = snprintf(
        buffer.data(), buffer.size(), "AT+CIPSEND=%zu\r\n", m_request_length);

      if (cip_send_command_length < 0) {
        m_next_state = state::close_connection_failure;
        break;
      }

      write(std::string_view(buffer.data(),
                             static_cast<size_t>(cip_send_command_length)));

      m_commander.new_search(std::span<hal::byte>{},
                             hal::as_bytes(ok_response));
      m_next_state = state::sending_request;
      m_read_state = read_state::until_sequence;
    }
    case state::sending_request:
      m_commander.new_search(m_response.subspan(0, m_request_length),
                             hal::as_bytes("+IPD,"));
      m_next_state = state::get_first_packet_length;
      m_read_state = read_state::until_sequence;
      break;
    case state::get_first_packet_length:
      m_integer_reader.restart();
      m_next_state = state::reading_first_packet;
      m_read_state = read_state::integer;
      break;
    case state::reading_first_packet:
      m_reader.new_buffer(
        std::span{ m_packet.begin(), m_integer_reader.get() });
      m_next_state = state::parsing_header;
      m_read_state = read_state::into_buffer;
      break;
    case state::parsing_header:
      m_header = response_header_from_string(m_packet);
      if (!m_header.is_valid()) {
        m_next_state = state::close_connection_failure;
        break;
      } else if (m_header.content_length > m_response.size()) {
        m_next_state = state::close_connection_failure;
        break;
      } else if (m_integer_reader.get() < maximum_response_packet_size) {
        std::copy_n(m_packet.begin() + m_header.header_length,
                    m_header.content_length,
                    m_response.begin());
        m_next_state = state::close_connection;
      } else {
        // Pull out contents of body from header packet
        size_t bytes_retrieved =
          m_integer_reader.get() - m_header.header_length;
        std::copy_n(m_packet.begin() + m_header.header_length,
                    bytes_retrieved,
                    m_response.begin());
        m_response_position = bytes_retrieved;
        m_next_state = state::get_packet_length;
      }
      break;
    case state::get_packet_length:
      m_integer_reader.restart();
      m_next_state = state::read_packet_into_response;
      m_read_state = read_state::integer;
      break;
    case state::read_packet_into_response:
      m_reader.new_buffer(
        std::span(m_response)
          .subspan(m_response_position, m_integer_reader.get()));
      m_next_state = state::get_next_packet;
      m_read_state = read_state::into_buffer;
      break;
    case state::get_next_packet:
      m_response_position += m_integer_reader.get();
      if (m_response_position >= m_header.content_length) {
        m_next_state = state::close_connection;
      } else {
        m_next_state = state::get_packet_length;
      }
      break;
    case state::close_connection:
      m_commander.new_search(hal::as_bytes("AT+CIPCLOSE\r\n"),
                             hal::as_bytes(ok_response));
      m_next_state = state::complete;
      m_read_state = read_state::until_sequence;
      break;
    case state::close_connection_failure:
      m_commander.new_search(hal::as_bytes("AT+CIPCLOSE\r\n"),
                             hal::as_bytes(ok_response));
      m_next_state = state::failure;
      m_read_state = read_state::until_sequence;
      break;
    case state::complete:
      break;
    case state::failure:
      break;
  }
}

inline std::string_view wifi_client::to_string(http_method p_method)
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

inline std::string_view wifi_client::to_string(state p_method)
{
  switch (p_method) {
    case state::reset:
      return "reset";
    case state::disable_echo:
      return "disable_echo";
    case state::configure_as_http_client:
      return "configure_as_http_client";
    case state::attempting_ap_connection:
      return "attempting_ap_connection";
    case state::connected_to_ap:
      return "connected_to_ap";
    case state::connecting_to_server:
      return "connecting_to_server";
    case state::preparing_request:
      return "preparing_request";
    case state::sending_request:
      return "sending_request";
    case state::get_first_packet_length:
      return "get_first_packet_length";
    case state::reading_first_packet:
      return "reading_first_packet";
    case state::parsing_header:
      return "parsing_header";
    case state::get_packet_length:
      return "get_packet_length";
    case state::read_packet_into_response:
      return "read_packet_into_response";
    case state::get_next_packet:
      return "get_next_packet";
    case state::close_connection:
      return "close_connection";
    case state::close_connection_failure:
      return "close_connection_failure";
    case state::complete:
      return "complete";
    case state::failure:
      return "failure";
  }
}
} // namespace hal
