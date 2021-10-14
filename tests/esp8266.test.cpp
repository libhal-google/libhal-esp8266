#include <cstdio>

#include "../include/libesp8266/esp8266.hpp"

class mock_serial : public embed::serial
{
  bool driver_initialize() override { return true; }

  [[nodiscard]] bool busy() override { return false; }
  void write(std::span<const std::byte> p_data) override
  {
    for (const auto& byte : p_data) {
      putchar(std::to_integer<char>(byte));
    }
  }

  [[nodiscard]] size_t bytes_available() override { return 1; }
  std::span<const std::byte> read(std::span<std::byte> p_data) override
  {
    static constexpr std::string_view ok = "OK\r\n";
    static constexpr std::string_view blank = "";
    static constexpr std::string_view ready = "\r\nready\r\n";
    static constexpr std::string_view connected = "WIFI GOT IP\r\n\r\nOK\r\n";

    std::array outputs{ blank, ok, blank, ready, blank, blank, connected };
    m_rotation++;
    auto select = m_rotation % outputs.size();
    printf("rotation = %d :: select = %d\n", m_rotation, select);
    return std::as_bytes(std::span<const char>(outputs.at(select).begin(),
                                               outputs.at(select).end()));
  }
  void flush() override {}
  int m_rotation = 0;
};

int main()
{
  mock_serial test;
  embed::static_esp8266 esp(test, "SSID", "PASSWORD");
  bool success = esp.initialize();
  if (!success) {
    return 1;
  }

  printf("sizeof(esp) = %zu\n", sizeof(esp));

  return 0;

  for (auto state = esp.get_status();
       state != embed::esp8266::state::connected_to_ap;
       state = esp.get_status()) {
    printf("\u001b[41m== state == %d ==\u001b[0m\n", static_cast<int>(state));
    // printf("state == %d", embed::esp8266::to_string(state).data());
  }

  return 0;
}