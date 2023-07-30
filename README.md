# libhal-esp8266

[![✅ Checks](https://github.com/libhal/libhal-esp8266/actions/workflows/ci.yml/badge.svg)](https://github.com/libhal/libhal-esp8266/actions/workflows/ci.yml)
[![Coverage](https://libhal.github.io/libhal-esp8266/coverage/coverage.svg)](https://libhal.github.io/libhal-esp8266/coverage/)
[![GitHub stars](https://img.shields.io/github/stars/libhal/libhal-esp8266.svg)](https://github.com/libhal/libhal-esp8266/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/libhal/libhal-esp8266.svg)](https://github.com/libhal/libhal-esp8266/network)
[![GitHub issues](https://img.shields.io/github/issues/libhal/libhal-esp8266.svg)](https://github.com/libhal/libhal-esp8266/issues)

libhal device library for the esp8266 wifi module/soc from
[espressif](https://www.espressif.com/en/products/socs/esp8266).

## 📚 Software APIs & Usage

To learn about the available drivers and APIs see the
[Doxygen](https://libhal.github.io/libhal-esp8266/api)
documentation page or look at the
[`include/libhal-esp8266`](https://github.com/libhal/libhal-esp8266/tree/main/include/libhal-esp8266)
directory.

To see how each driver is used see the
[`demos/`](https://github.com/libhal/libhal-esp8266/tree/main/demos) directory.

## 🧰 Setup

Following the
[🚀 Getting Started](https://libhal.github.io/2.1/getting_started/)
instructions.

## 📡 Installing Profiles

The `libhal-lpc40` profiles used for demos. To install them use the following
commands.

```bash
conan config install -sf conan/profiles/ -tf profiles https://github.com/libhal/libhal-armcortex.git
conan config install -sf conan/profiles/ -tf profiles https://github.com/libhal/libhal-lpc40.git
```

## 🏗️ Building Demos

To build demos, start at the root of the repo and execute the following command:

```bash
conan build demos -pr lpc4078 -s build_type=Debug
```

or for the `lpc4074`

```bash
conan build demos -pr lpc4074 -s build_type=Debug
```

## 🔌 Device Wiring & Hookup guide (ESP-01 variant)

1. Locate the TX (UART Transmit Data) and RX (UART Receive Data) pins on
   your microcontroller port.
2. Connect the microcontroller's TX pin to the RX pin of the ESP-01 pin.
3. Connect the microcontroller's RX pin to the TX pin of the ESP-01 pin.
4. Supply adequate power to the ESP-01 with 3v3 at the VCC line.

## 📦 Adding `libhal-esp8266` to your project

Add the following to your `requirements()` method:

```python
    def requirements(self):
        self.requires("libhal-esp8266/[^2.0.0]")
```

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details.

## License

Apache 2.0; see [`LICENSE`](LICENSE) for details.

## Disclaimer

This project is not an official Google project. It is not supported by
Google and Google specifically disclaims all warranties as to its quality,
merchantability, or fitness for a particular purpose.
