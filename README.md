# Basecamp

Basecamp - ESP32 library to simplify the basics of IoT projects

Originally written by Merlin Schumacher (https://github.com/ct-Open-Source/Basecamp)

**Heavily modified with breaking changes**, e.g.:
* Enhanced ethernet support
* Dropped async-mqtt-client in favor of EspIdfMqttClient (due to stability issues)
* Support for Home Assistant MQTT Discovery
* Switched logging to use https://github.com/felixstorm/Esp32Logging (including logging to syslog)
* Added SNTP support

Licensed under GPLv3. See LICENSE for details.
