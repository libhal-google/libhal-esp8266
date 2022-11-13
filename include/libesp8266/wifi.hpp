#pragma once

namespace hal::esp8266 {
/// The type of password security used for the access point.
enum class access_point_security
{
  open,
  wep,
  wpa_psk,
  wpa2_psk,
  wpa_wpa2_psk,
};
} // namespace hal::esp8266
