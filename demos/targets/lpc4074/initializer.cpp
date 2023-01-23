#include <libhal-armcortex/dwt_counter.hpp>
#include <libhal-armcortex/startup.hpp>
#include <libhal-armcortex/system_control.hpp>

#include <libhal-lpc40xx/constants.hpp>
#include <libhal-lpc40xx/system_controller.hpp>
#include <libhal-lpc40xx/uart.hpp>

#include "../../hardware_map.hpp"

hal::result<hal::esp8266::hardware_map> initialize_target()
{
  using namespace hal::literals;
  hal::cortex_m::initialize_data_section();

  // Set the MCU to the maximum clock speed
  HAL_CHECK(hal::lpc40xx::clock::maximum(10.0_MHz));

  // Create a hardware counter
  auto& clock = hal::lpc40xx::clock::get();
  auto cpu_frequency = clock.get_frequency(hal::lpc40xx::peripheral::cpu);
  static hal::cortex_m::dwt_counter counter(cpu_frequency);

  // Get and initialize UART0 for UART based logging
  auto& uart0 = HAL_CHECK((hal::lpc40xx::uart::get<0, 64>(hal::serial::settings{
    .baud_rate = 38400,
  })));

  // Get and initialize UART3 with a 8kB receive buffer
  auto& uart3 =
    HAL_CHECK((hal::lpc40xx::uart::get<3, 8192>(hal::serial::settings{
      .baud_rate = 115200,
    })));

  return hal::esp8266::hardware_map{
    .debug = &uart0,
    .esp = &uart3,
    .counter = &counter,
    .reset = []() { hal::cortex_m::system_control::reset(); },
  };
}
