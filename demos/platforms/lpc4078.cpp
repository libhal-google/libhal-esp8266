// Copyright 2024 Khalil Estell
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

#include <libhal-armcortex/dwt_counter.hpp>
#include <libhal-armcortex/startup.hpp>
#include <libhal-armcortex/system_control.hpp>
#include <libhal-exceptions/control.hpp>
#include <libhal-lpc40/clock.hpp>
#include <libhal-lpc40/constants.hpp>
#include <libhal-lpc40/output_pin.hpp>
#include <libhal-lpc40/uart.hpp>
#include <libhal-util/steady_clock.hpp>

#include "../hardware_map.hpp"

[[noreturn]] void terminate_handler() noexcept
{
  hal::cortex_m::dwt_counter steady_clock(
    hal::lpc40::get_frequency(hal::lpc40::peripheral::cpu));

  hal::lpc40::output_pin led(1, 10);

  while (true) {
    using namespace std::chrono_literals;
    led.level(false);
    hal::delay(steady_clock, 100ms);
    led.level(true);
    hal::delay(steady_clock, 100ms);
    led.level(false);
    hal::delay(steady_clock, 100ms);
    led.level(true);
    hal::delay(steady_clock, 1000ms);
  }
}

hardware_map_t initialize_platform()
{
  using namespace hal::literals;

  // Set the MCU to the maximum clock speed
  hal::lpc40::maximum(10.0_MHz);

  hal::set_terminate(terminate_handler);

  // Create a hardware counter
  auto cpu_frequency = hal::lpc40::get_frequency(hal::lpc40::peripheral::cpu);
  static hal::cortex_m::dwt_counter counter(cpu_frequency);

  static std::array<hal::byte, 64> uart0_buffer{};

  // Get and initialize UART0 for UART based logging
  static hal::lpc40::uart uart0(0,
                                uart0_buffer,
                                hal::serial::settings{
                                  .baud_rate = 115200,
                                });

  static std::array<hal::byte, 8192> uart3_buffer{};
  // Get and initialize UART3 with a 8kB receive buffer
  static hal::lpc40::uart uart3(3,
                                uart3_buffer,
                                hal::serial::settings{
                                  .baud_rate = 115200,
                                });

  return {
    .console = &uart0,
    .serial = &uart3,
    .counter = &counter,
    .reset = []() { hal::cortex_m::reset(); },
  };
}
