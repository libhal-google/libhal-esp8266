from conans import ConanFile


class libesp8266_conan(ConanFile):
    name = "libesp8266"
    version = "0.0.1"
    license = "Apache License Version 2.0"
    author = "Khalil Estell"
    url = "https://github.com/SJSU-Dev2/libesp8266"
    description = "Driver for controlling ESP8266 WIFI module via UART AT commands"
    topics = ("wifi", "tcp", "ip", "esp8266", "hardware", "libembeddedhal")
    exports_sources = "CMakeLists.txt", "include/*"
    no_copy_source = True

    def package(self):
        self.copy("*.hpp")

    def package_id(self):
        self.info.header_only()

    def requirements(self):
        self.requires("libembeddedhal/0.0.1")
        self.requires("ring-span-lite/0.6.0")
