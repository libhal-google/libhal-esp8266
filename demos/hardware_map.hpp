#pragma once

#include <libhal/alias.hpp>
#include <libhal/serial.hpp>
#include <libhal/steady_clock.hpp>

namespace hal::esp8266 {
struct hardware_map
{
  hal::serial* debug;
  hal::serial* esp;
  hal::steady_clock* counter;
  hal::function_ref<void()> reset;
};
}  // namespace hal::esp8266

// Application function must be implemented by one of the compilation units
// (.cpp) files.
hal::status application(hal::esp8266::hardware_map& p_map);
hal::result<hal::esp8266::hardware_map> initialize_target();
