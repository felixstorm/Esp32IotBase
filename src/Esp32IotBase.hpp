/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#pragma once

#include <Esp32ExtendedLogging.hpp>
#include "Configuration.hpp"
#include <rom/rtc.h>

#ifndef ESP32IOTBASE_NETWORK_ETHERNET
// WiFi
#include "NetworkControlWiFi.hpp"
#else
// Ethernet
#include "NetworkControlEth.hpp"
#endif

#ifndef ESP32IOTBASE_NO_MQTT
#include "EspIdfMqttClient.hpp"
#endif

#ifndef ESP32IOTBASE_NO_OTA
#include <ArduinoOTA.h>
#endif

#ifndef ESP32IOTBASE_NO_WEB
#include "WebServer/WebServer.hpp"
#endif

#if !(defined(ESP32IOTBASE_NO_WEB) || defined(ESP32IOTBASE_NO_CAPTIVE_PORTAL))
#include <DNSServer.h>
#endif

class Esp32IotBase
{
    public:
        // How to handle encryption in setup mode (AP mode)
        enum class SetupModeWifiEncryption
        {
            none,       ///< Do not use WiFi encryption (open network)
            secured,    ///< Use ESP32 default encryption (WPA2 at this time)
        };

        // When to enable the Configuration UI (setup via local webserver)
        enum class ConfigurationUI
        {
            always,             ///< Always start the configuration-ui webserver
            accessPoint,        ///< Only start the server if acting as an access  (first setup mode)
        };

        explicit Esp32IotBase(Esp32IotBase::SetupModeWifiEncryption setupModeWifiEncryption =
            Esp32IotBase::SetupModeWifiEncryption::none,
            Esp32IotBase::ConfigurationUI configurationUi = Esp32IotBase::ConfigurationUI::always);

        ~Esp32IotBase() = default;

        /** Initialize.
         * Give a fixex ap secret here to override the one-time secret
         * password generation. If a password is given, the ctor given
         * SetupModeWifiEncryption will be overriden to SetupModeWifiEncryption::secure.
        */
        void Begin(String fixedWiFiApEncryptionPassword = {});
        void Handle();

        Configuration Config;
        String Hostname;
        bool IsConfigured;
#ifndef ESP32IOTBASE_NETWORK_ETHERNET
// WiFi
        NetworkControlWiFi Network;
#else
// Ethernet
        NetworkControlEth Network;
#endif
        String Mac;

#ifndef ESP32IOTBASE_NO_MQTT
        EspIdfMqttClient Mqtt;
#endif

#ifndef ESP32IOTBASE_NO_WEB
        WebServer Web;
#endif

    private:
        SetupModeWifiEncryption setupModeWifiEncryption_;
        ConfigurationUI configurationUi_;

        String getCleanHostnameFromDeviceName_();

        void handleQuickRebootsToResetConfig_();
        static void resetquickRebootCounterTimer_(TimerHandle_t xTimer);

        void checkConfigureSyslog_();
        void checkConfigureSntp_();
        void checkConfigureMqtt_();
        void checkConfigureOta_();
        void checkConfigureWebserver_();

#if !(defined(ESP32IOTBASE_NO_WEB) || defined(ESP32IOTBASE_NO_CAPTIVE_PORTAL))
        DNSServer dnsServer_;
        static void dnsHandlerTask_(void* dnsServerPointer);
#endif
};
