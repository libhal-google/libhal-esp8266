#pragma once

#include <string_view>

#include <libhal-esp8266/util.hpp>
#include <libhal-util/serial.hpp>

/**
 * @brief Debug serial data on one serial port via another
 *
 * This class takes two serial ports, a "primary" and a "mirror". When the write
 * and read APIs are used, data is passed directly through to the primary. On
 * write operations, mirror will also write the same data. On read, the mirror
 * will report the read data by also writing it to its port. This way, a user
 * using a serial to USB debugger or some other debugging device can inspect
 * what is being written to and read from the primary serial port.
 *
 */
class serial_mirror : public hal::serial
{
public:
  serial_mirror(hal::serial& p_primary, hal::serial& p_mirror)
    : m_primary(&p_primary)
    , m_mirror(&p_mirror)
  {
  }

  hal::status driver_configure(const settings& p_settings) override
  {
    return m_primary->configure(p_settings);
  }

  hal::result<write_t> driver_write(std::span<const hal::byte> p_data) override
  {
    hal::print(*m_mirror, "WRITE:[");
    HAL_CHECK(m_mirror->write(p_data));
    hal::print(*m_mirror, "]\n");
    return m_primary->write(p_data);
  }

  hal::result<read_t> driver_read(std::span<hal::byte> p_data) override
  {
    auto result = m_primary->read(p_data);

    if (result.has_value() && result.value().data.size() != 0) {
      HAL_CHECK(m_mirror->write(p_data));
    }

    return result;
  }

  hal::status driver_flush() override
  {
    return m_primary->flush();
  }

private:
  hal::serial* m_primary;
  hal::serial* m_mirror;
};

inline std::string_view to_string_view(std::span<const hal::byte> p_span)
{
  return std::string_view(reinterpret_cast<const char*>(p_span.data()),
                          p_span.size());
}

// namespace hal {
// inline void write(serial& p_serial, std::integral auto p_integer)
// {
//   auto integer_string = esp8266::integer_string::create(p_integer).value();
//   (void)write(p_serial, integer_string.str());
// }
// }  // namespace hal

inline hal::status print_http_response_info(hal::serial& p_serial,
                                            std::string_view p_http_response)
{
  // Print out the results from the TCP socket
  hal::print(p_serial, "=============== Full Response! ==========\n");
  hal::print(p_serial, p_http_response);
  hal::print(p_serial, "\n\n");

  // Print out the parsed HTTP meta data from TCP socket string
  hal::print(p_serial, "================ Meta Data! ===========\n");
  // Print HTTP Status Code
  hal::print<64>(
    p_serial, "HTTP Status: %zu\n", hal::esp8266::http_status(p_http_response));
  // Print out the content type
  hal::print(p_serial, "Content-Type: ");
  hal::print(p_serial,
             hal::esp8266::http_header("Content-Type", p_http_response));
  hal::print(p_serial, "\n");
  // Print out the Date header flag
  hal::print(p_serial, "Date: ");
  hal::print(p_serial, hal::esp8266::http_header("Date", p_http_response));
  hal::print(p_serial, "\n");
  // Print out the length of the body of the response
  hal::print(p_serial, "Content-Length: ");
  hal::print(p_serial,
             hal::esp8266::http_header("Content-Length", p_http_response));
  hal::print(p_serial, "\n");

  // Print out the body of the response
  hal::print(p_serial, "================ Body! ===========\n");
  hal::print(p_serial, hal::esp8266::http_body(p_http_response));
  hal::print(p_serial, "\n");
  hal::print(p_serial, "=================== /end ================\n");

  return hal::success();
}
