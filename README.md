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

## Supported Firmware Version

This library supports the esp8266 AT commands for version 2.2.0 and above.
The installation procedure described by the Espressif documentation doesn't seem
to work. What worked was following this article
[Installing the AT Firmware on an ESP-01S](https://www.sigmdel.ca/michel/ha/esp8266/ESP01_AT_Firmware_en.html).

You'll probably want to pick up an esp8266 programmer to make programming the
device easy. There are various ones out there. One suggestion is this one:
[ESP8266 Adapter Programmer Downloader](https://www.amazon.com/dp/B097SZMK2W).

The shortened steps to install version 2.2.0 on your esp-01 is as follows:

1. Install esptool.py

```bash
pip install esptool
```

2. Download build 2.2.0 firmware: [ESP8266-1MB-tx1rx3-AT_V2.2.zip](https://github.com/jandrassy/UnoWiFiDevEdSerial1/wiki/files/ESP8266-1MB-tx1rx3-AT_V2.2.zip)
3. Unzip the package and enter directory in a terminal
4. Flash Firmware on device

```bash
esptool.py -p PORT_NAME -b 115200 write_flash -e @download.config
```

Replace `PORT_NAME` with the port name OR path to the port. Something like
`COM3` for Windows, `/dev/tty.usbserial-10` for MacOS, or `/dev/ttyUSB0` for
Linux.

NOTE: that the binaries were built by JAndrassy. The binaries have been mirrored
on this repo in the event that the other repo goes away. Full path to where
these can be found is here:

https://github.com/JAndrassy/UnoWiFiDevEdSerial1/wiki/Firmware#preparing-for-flashing

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details.

## License

Apache 2.0; see [`LICENSE`](LICENSE) for details.

## Disclaimer

This project is not an official Google project. It is not supported by
Google and Google specifically disclaims all warranties as to its quality,
merchantability, or fitness for a particular purpose.
