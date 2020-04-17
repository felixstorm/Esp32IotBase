/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   by Felix Storm (http://github.com/felixstorm)
   Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef Configuration_h
#define Configuration_h

#include <Esp32Logging.hpp>
#include <nvs.h>
#include <nvs_flash.h>
#include <map>
#include "enum.h"

// 15 chars max due to NVS limit
BETTER_ENUM(ConfigKey, int, 
    QuickBootCount,
    DeviceName,
    ApSecret,
    WifiSsid,
    WifiPassword,
    SntpServer,
    SntpTz,
    MqttHost,
    MqttUser,
    MqttPassword,
    MqttTopicPrefix,
    MqttHaDiscPref,
    OtaActive,
    OtaPassword,
    SyslogServer
)

class Configuration {
    public:
        Configuration();
        ~Configuration() = default;

        bool Begin();
        bool Save();
        bool Reset();

        void Set(const ConfigKey &key, const String &value);
        void Set(const String &key, const String &value);
        void Set(const char* key, const String &value);
        void SetInt(const ConfigKey &key, const int value);
        void SetInt(const String &key, const int value);
        void SetInt(const char* key, const int value);

        const String Get(const ConfigKey &key, const String &defaultValue = {}) const;
        const String Get(const String &key, const String &defaultValue = {}) const;
        const String GetRaw(const char* key) const;
        const String Get(const char* key, const String &defaultValue = {}) const;
        int GetInt(const ConfigKey &key, const int defaultValue = 0) const;
        int GetInt(const String &key, const int defaultValue = 0) const;
        int GetInt(const char* key, const int defaultValue = 0) const;

        // unordered_map does not seem to work with String keys for whatever reason
        std::map<String, String> StringDefaults{
            {(+ConfigKey::SntpServer)._to_string(), "pool.ntp.org"},
            {(+ConfigKey::SntpTz)._to_string(), "CET-1CEST,M3.5.0/2:00,M10.5.0/3:00:"}};

    private:
        nvs_handle nvsHandle_;
};

#endif