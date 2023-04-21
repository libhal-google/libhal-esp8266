<div align="center">

[![✅CI](https://github.com/libhal/libhal-esp8266/actions/workflows/ci.yml/badge.svg)](https://github.com/libhal/libhal-esp8266/actions/workflows/ci.yml)
[![coverage](https://libhal.github.io/libhal-esp8266/coverage/coverage.svg)](https://libhal.github.io/libhal-esp8266/coverage/)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/b084e6d5962d49a9afcb275d62cd6586)](https://www.codacy.com/gh/libhal/libhal/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=libhal/libhal&amp;utm_campaign=Badge_Grade)
[![GitHub stars](https://img.shields.io/github/stars/libhal/libhal-esp8266.svg)](https://github.com/libhal/libhal-esp8266/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/libhal/libhal-esp8266.svg)](https://github.com/libhal/libhal-esp8266/network)
[![GitHub issues](https://img.shields.io/github/issues/libhal/libhal-esp8266.svg)](https://github.com/libhal/libhal-esp8266/issues)
[![Latest Version](https://libhal.github.io/libhal-esp8266/latest_version.svg)](https://github.com/libhal/libhal-esp8266/blob/main/conanfile.py)
[![ConanCenter Version](https://repology.org/badge/version-for-repo/conancenter/libhal-esp8266.svg)](https://conan.io/center/libhal-esp8266)

</div>

# libhal-esp8266

Library for controlling esp8266 WiFi modules via serial AT commands using the
libhal interfaces.

## 🏗️ WARNING: Work in progress! 🚧
- Only supports GET requests
- GET requests must be below 2048 bytes (multi packet transmission not possible
yet).

## Costly Dependencies
- Utilizes snprintf & sscanf (decimal only and planned to be removed with the
  less bulky std::to_chars & std::from_chars)

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details.

## License

Apache 2.0; see [`LICENSE`](LICENSE) for details.

## Disclaimer

This project is not an official Google project. It is not supported by
Google and Google specifically disclaims all warranties as to its quality,
merchantability, or fitness for a particular purpose.

