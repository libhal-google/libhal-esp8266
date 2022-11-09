#include <libarmcortex/dwt_counter.hpp>
#include <libesp8266/wifi_client.alpha.hpp>
#include <libhal/serial/util.hpp>
#include <libhal/steady_clock/util.hpp>
#include <liblpc40xx/constants.hpp>
#include <liblpc40xx/system_controller.hpp>
#include <liblpc40xx/uart.hpp>

hal::status application()
{
  using namespace std::chrono_literals;
  using namespace hal::literals;

  // Do not change this, this is the max speed of the internal PLL
  static constexpr auto max_speed = 120.0_MHz;
  // If "baudrate" is above 100.0_kHz, then an external crystal must be used for
  // clock rate accuracy.
  static constexpr auto external_crystal_frequency = 10.0_MHz;
  static constexpr auto multiply = max_speed / external_crystal_frequency;

  auto& clock = hal::lpc40xx::clock::get();

  // Set CPU & peripheral clock speed
  auto& config = clock.config();
  config.oscillator_frequency = external_crystal_frequency;
  config.use_external_oscillator = true;
  config.cpu.use_pll0 = true;
  config.peripheral_divider = 1;
  config.pll[0].enabled = true;
  config.pll[0].multiply = static_cast<uint8_t>(multiply);
  HAL_CHECK(clock.reconfigure_clocks());

  static hal::cortex_m::dwt_counter counter(
    hal::lpc40xx::clock::get().get_frequency(hal::lpc40xx::peripheral::cpu));

  auto& uart0 = hal::lpc40xx::uart::get<0>(hal::serial::settings{
    .baud_rate = 38400,
  });

  HAL_CHECK(hal::write(uart0, "ESP8266 WiFi Client Application Starting...\n"));
  auto& uart3 = hal::lpc40xx::uart::get<3>();
  std::array<hal::byte, 4096> buffer{};

  auto result =
    hal::esp8266::alpha::wifi_client::create(uart3, "ssid", "password", buffer);

  if (!result) {
    HAL_CHECK(hal::write(uart0, "Failed to create wifi client!\n"));
  }

  auto client = result.value();

  auto state = HAL_CHECK(client.work());
  while (state != hal::esp8266::alpha::wifi_client::state::connected_to_ap) {
    HAL_CHECK(hal::delay(counter, 10ms));
    HAL_CHECK(
      hal::write(uart0, hal::esp8266::alpha::wifi_client::to_string(state)));
    HAL_CHECK(hal::write(uart0, "\n"));
    state = HAL_CHECK(client.work());
  }

  while (true) {
    buffer.fill('_');

    client.request(hal::esp8266::alpha::wifi_client::request_t{
      .domain = "example.com",
    });

    auto status = HAL_CHECK(client.work());

    while (status != hal::esp8266::alpha::wifi_client::state::complete) {
      HAL_CHECK(hal::delay(counter, 10ms));
      HAL_CHECK(hal::write(uart0, "."));
      if (status == hal::esp8266::alpha::wifi_client::state::failure ||
          status ==
            hal::esp8266::alpha::wifi_client::state::close_connection_failure) {
        break;
      }

      status = HAL_CHECK(client.work());
    }

    HAL_CHECK(hal::write(uart0, buffer));
    HAL_CHECK(hal::write(uart0, "\n\n"));
  }

  return hal::success();
}
