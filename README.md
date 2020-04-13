# Esp32IotBase

Esp32IotBase - ESP32 library to simplify the basics of IoT projects  
by Felix Storm (http://github.com/felixstorm)  
Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)

Modifications include:
* Enhanced ethernet support
* Dropped async-mqtt-client in favor of EspIdfMqttClient (due to stability issues)
* Support for Home Assistant MQTT Discovery
* Switched logging to use https://github.com/felixstorm/Esp32Logging (including logging to syslog)
* Added SNTP support
* Moved configuration from JSON file to NVS
* Major code restructuring and cleanup

Licensed under GPLv3. See LICENSE for details.
