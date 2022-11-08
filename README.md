# libeps8266

Library for controlling esp8266 WiFi modules via serial AT commands using the
libhal interfaces.

## 🏗️ WARNING: Work in progress! 🚧
- Only supports GET requests
- GET requests must be below 2048 bytes (multi packet transmission not possible
yet).

## Costly Dependencies
- Utilizes snprintf & sscanf (decimal only and planned to be removed with the
  less bulky std::to_chars & std::from_chars)
