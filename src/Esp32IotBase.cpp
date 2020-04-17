/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   by Felix Storm (http://github.com/felixstorm)
   Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include <iomanip>
#include "Esp32IotBase.hpp"
#include "lwip/apps/sntp.h"

namespace {
    const constexpr char* kLoggingTag = "IotBase";

    const ulong kSystemInfoInterval = 1000UL * 60 * 5;   // 5 minutes

    std::vector<TaskHandle_t> tasksToWatch;
}

#ifndef ESP32IOTBASE_NO_SYSLOG
WiFiUDP Esp32ExtLogUdpClient;
Esp32ExtendedLogging Esp32ExtLog;
#endif


Esp32IotBase::Esp32IotBase(SetupModeWifiEncryption setupModeWifiEncryption, ConfigurationUI configurationUi) : 
    setupModeWifiEncryption_(setupModeWifiEncryption), 
    configurationUi_(configurationUi)
{
}

void Esp32IotBase::Begin(String fixedWiFiApEncryptionPassword)
{
    // unfortunately we cannot use wildcards here
    // esp_log_level_set("IotBase", ESP_LOG_DEBUG);
    // esp_log_level_set("IotBaseConfig", ESP_LOG_DEBUG);
    // esp_log_level_set("IotBaseNetwork", ESP_LOG_DEBUG);
    // esp_log_level_set("IotBaseMqtt", ESP_LOG_DEBUG);

    ESP_LOGI(kLoggingTag, "*** Esp32IotBase Startup ***");

    Config.Begin();

    // Get a cleaned version of the device name, used for DHCP and ArduinoOTA
    Hostname = getCleanHostnameFromDeviceName_();
    IsConfigured = Config.Get(ConfigKey::DeviceName).length() > 0;

    // Have checkIfConfigNeedsReset() control if the device configuration should be reset or not.
    handleQuickRebootsToResetConfig_();

    // Start network
    Network.Begin(Config, Hostname, setupModeWifiEncryption_ == SetupModeWifiEncryption::secured, fixedWiFiApEncryptionPassword);

    // Get MAC (WiFi STA or Ethernet)
    Mac = Network.GetMacAddress(":");

    ESP_LOGW(kLoggingTag, "***********************************************");
    ESP_LOGW(kLoggingTag, "Esp32IotBase System Information");
    ESP_LOGW(kLoggingTag, "***********************************************");
    ESP_LOGW(kLoggingTag, "DeviceName:      %s", Config.Get(ConfigKey::DeviceName).c_str());
    ESP_LOGW(kLoggingTag, "Hostname:        %s", Hostname.c_str());
    ESP_LOGW(kLoggingTag, "MAC-Address:     %s", Mac.c_str());
    ESP_LOGW(kLoggingTag, "IP-Address:      %s", Network.GetIp().toString().c_str());
    ESP_LOGW(kLoggingTag, "AP IP-Address:   %s", Network.GetSoftApIp().toString().c_str());
    ESP_LOGW(kLoggingTag, "SetupWifiEncr:   %d", static_cast<int>(setupModeWifiEncryption_));
    ESP_LOGW(kLoggingTag, "ConfigurationUI: %d", static_cast<int>(configurationUi_));
    ESP_LOGW(kLoggingTag, "WifiSsid:        %s", Config.Get(ConfigKey::WifiSsid).c_str());
    ESP_LOGW(kLoggingTag, "WifiPassword:    %s", Config.Get(ConfigKey::WifiPassword).c_str());
    ESP_LOGW(kLoggingTag, "SntpServer:      %s", Config.Get(ConfigKey::SntpServer).c_str());
    ESP_LOGW(kLoggingTag, "SntpTz:          %s", Config.Get(ConfigKey::SntpTz).c_str());
    ESP_LOGW(kLoggingTag, "MqttHost:        %s", Config.Get(ConfigKey::MqttHost).c_str());
    ESP_LOGW(kLoggingTag, "MqttUser:        %s", Config.Get(ConfigKey::MqttUser).c_str());
    ESP_LOGW(kLoggingTag, "MqttPassword:    %s", Config.Get(ConfigKey::MqttPassword).c_str());
    ESP_LOGW(kLoggingTag, "MqttTopicPrefix: %s", Config.Get(ConfigKey::MqttTopicPrefix).c_str());
    ESP_LOGW(kLoggingTag, "MqttHaDiscPref:  %s", Config.Get(ConfigKey::MqttHaDiscPref).c_str());
    ESP_LOGW(kLoggingTag, "OtaActive:       %s", Config.Get(ConfigKey::OtaActive).c_str());
    ESP_LOGW(kLoggingTag, "OtaPassword:     %s", Config.Get(ConfigKey::OtaPassword).c_str());
    ESP_LOGW(kLoggingTag, "SyslogServer:    %s", Config.Get(ConfigKey::SyslogServer).c_str());
    ESP_LOGW(kLoggingTag, "IsConfigured:    %d", IsConfigured);


    if (Config.Get(ConfigKey::ApSecret).length()) {
        ESP_LOGW(kLoggingTag, "***********************************************");
        ESP_LOGW(kLoggingTag, "*** ACCESS POINT PASSWORD: %-16s ***", Config.Get(ConfigKey::ApSecret).c_str());
    }
    ESP_LOGW(kLoggingTag, "***********************************************");

    if (IsConfigured) {
        checkConfigureSyslog_();
        checkConfigureSntp_();
        checkConfigureMqtt_();
        checkConfigureOta_();
    }
    checkConfigureWebserver_();

}

/**
 * This is the background task function for the Esp32IotBase class. To be called from Arduino loop.
 */
void Esp32IotBase::Handle()
{
    #ifndef ESP32IOTBASE_NO_OTA
        ArduinoOTA.handle();
    #endif

    if (IsConfigured) {
        // periodically dump some system information
        static ulong lastSystemInfo = 0;
        if (lastSystemInfo == 0 || millis() - lastSystemInfo > kSystemInfoInterval)
        {
            ESP_LOG_SYSINFO(ESP_LOG_INFO, lastSystemInfo == 0);

            // when CONFIG_FREERTOS_USE_TRACE_FACILITY is defined, ESP_LOG_SYSINFO will already have included this
            #ifndef CONFIG_FREERTOS_USE_TRACE_FACILITY
                ESP_LOGI("SysInfo", "*** FreeRTOS Task Stack High Water Marks ***");
                for(TaskHandle_t t : tasksToWatch) {
                    ESP_LOGI("SysInfo", "%-15s (%p)  %u bytes", pcTaskGetTaskName(t), t, uxTaskGetStackHighWaterMark(t));
                }
            #endif

            lastSystemInfo = millis();
        }
    }

    // be nice and let other tasks do stuff as well (although it does not seem to be strictly neccessary on core 1)
    vTaskDelay(100 / portTICK_PERIOD_MS);

}

// create tasks in a standardized fashion and add them to a watch list to be able to dump it's stack high water mark later on
void Esp32IotBase::xTaskCreateMonitored(TaskFunction_t pvTaskCode, const char * const pcName, const uint32_t usStackDepth, void * const pvParameters, UBaseType_t uxPriority)
{
    TaskHandle_t taskHandle;
    int taskCreateResult;
    // all our tasks (including AsyncTCP) run on core 1 for now (otherwise async_tcp task might sometimes block idle task on 0 causing watchdog exception)
    if ((taskCreateResult = xTaskCreatePinnedToCore(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, &taskHandle, CONFIG_ARDUINO_RUNNING_CORE)) != pdPASS)
    {
        ESP_LOGE(pcName, "ERROR calling xTaskCreate: %i, stopping.", taskCreateResult);
        for(;;);
    }

    tasksToWatch.push_back(taskHandle);
}

String Esp32IotBase::getCleanHostnameFromDeviceName_()
{
    ESP_LOGD(kLoggingTag, "Entered function");
    String clean_hostname =	Config.Get(ConfigKey::DeviceName, "Esp32IotBase-Device"); // Get device name from configuration

    clean_hostname.toLowerCase();
    for (int i = 0; i <= clean_hostname.length(); i++) {
        if (!isalnum(clean_hostname.charAt(i))) {
            clean_hostname.setCharAt(i,'-');
        };
    };
    ESP_LOGD(kLoggingTag, "clean_hostname: %s", clean_hostname.c_str());

    return clean_hostname;
};

void Esp32IotBase::handleQuickRebootsToResetConfig_()
{
    // Esp32IotBase configuration can be reset by quickling restarting the ESP32 for five times in a row
    ESP_LOGD(kLoggingTag, "Entered function");

    // create timer to reset quick reboot counter after a few seconds
    TimerHandle_t resetQuickRebootCounterTimerHandle_ = xTimerCreate("ResQubootCnt", pdMS_TO_TICKS(5 * 1000), pdFALSE, (void*)this, resetquickRebootCounterTimer_);
    xTimerStart(resetQuickRebootCounterTimerHandle_, 0);

    // esp_reset_reason() returns ESP_RST_WDT (which can be any watchdog) for the reset button,
    // so we use rtc_get_reset_reason(0) instead, which returns RTCWDT_RTC_RESET - which is at least limited to the RTC watchdog
    RESET_REASON resetReasonRtc = rtc_get_reset_reason(0);
    ESP_LOGI(kLoggingTag, "Reset reason (rtc): %d", resetReasonRtc);
    // reset button only pulls down CHIP_PU, so power on and reset button are the same here
    if (resetReasonRtc == RESET_REASON::POWERON_RESET || resetReasonRtc == RESET_REASON::RTCWDT_RTC_RESET) {
        int quickRebootCounter = Config.GetInt(ConfigKey::QuickBootCount);
        ESP_LOGI(kLoggingTag, "Quick reboot counter: %d", quickRebootCounter);
        quickRebootCounter++;

        if (quickRebootCounter > 5){
            ESP_LOGW(kLoggingTag, "Enough quick reboots - resetting configuration and restarting.");
            Config.Reset();
            ESP.restart();
        } else {
            Config.SetInt(ConfigKey::QuickBootCount, quickRebootCounter);
            Config.Save();
        };
    }
};

void Esp32IotBase::resetquickRebootCounterTimer_(TimerHandle_t xTimer)
{
    ESP_LOGD(kLoggingTag, "Entered function");
    Esp32IotBase* iotBase = (Esp32IotBase*) pvTimerGetTimerID(xTimer);
    iotBase->Config.SetInt(ConfigKey::QuickBootCount, 0);
    iotBase->Config.Save();
}

void Esp32IotBase::checkConfigureSyslog_()
{
#ifndef ESP32IOTBASE_NO_SYSLOG

        auto &syslogServer = Config.Get(ConfigKey::SyslogServer);
        if (!syslogServer.isEmpty())
        {
            ESP_LOGI(kLoggingTag, "* Syslog: Configuring to host %s ...", syslogServer.c_str());
            Esp32ExtLog.deviceHostname(Hostname.c_str()).doSyslog(Esp32ExtLogUdpClient, syslogServer.c_str()).begin();
            ESP_LOGI(kLoggingTag, "* Syslog: -> Configuration completed.");
        }
        else
        {
            ESP_LOGI(kLoggingTag, "* Syslog: Not configured.");
        }

#endif
}

void Esp32IotBase::checkConfigureSntp_()
{
#ifndef ESP32IOTBASE_NO_SNTP

    auto &sntpServer = Config.Get(ConfigKey::SntpServer);
    auto &sntpTz = Config.Get(ConfigKey::SntpTz);
    ESP_LOGI(kLoggingTag, "* SNTP: Configuring with server %s and TZ %s ...", sntpServer.c_str(), sntpTz.c_str());

    // sntp_setservername won't take const, so we need to copy it to a buffer first
    char sntpServerBuffer[sntpServer.length() + 1];
    sntpServer.toCharArray(sntpServerBuffer, sizeof(sntpServerBuffer));

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, sntpServerBuffer);
    sntp_init();
    setenv("TZ", sntpTz.c_str(), 1);
    tzset();

    ESP_LOGI(kLoggingTag, "* SNTP: -> Configuration completed.");

#endif
}

void Esp32IotBase::checkConfigureMqtt_()
{
#ifndef ESP32IOTBASE_NO_MQTT

    const auto &mqttHost = Config.Get(ConfigKey::MqttHost);
    if (!mqttHost.isEmpty()) {
        ESP_LOGI(kLoggingTag, "* MQTT: Configuring & connecting ...");
        Mqtt.BeginWithHost(mqttHost, Config.Get(ConfigKey::MqttUser), Config.Get(ConfigKey::MqttPassword),
                           Hostname, Config.Get(ConfigKey::MqttHaDiscPref));
        ESP_LOGI(kLoggingTag, "* MQTT: -> Configuration completed.");
    } else {
        ESP_LOGI(kLoggingTag, "* MQTT: Not configured.");
    }

#endif
}

void Esp32IotBase::checkConfigureOta_()
{
#ifndef ESP32IOTBASE_NO_OTA

    // Set up Over-the-Air-Updates (OTA) if it hasn't been disabled.
    if (!Config.Get(ConfigKey::OtaActive).equalsIgnoreCase("false")) {

        String OtaPassword = Config.Get(ConfigKey::OtaPassword);

        ESP_LOGI(kLoggingTag, "* OTA: Configuring with password %s ...", OtaPassword.c_str());

        if (!OtaPassword.isEmpty()) {
            ArduinoOTA.setPassword(OtaPassword.c_str());
        }

        // Set OTA hostname
        ArduinoOTA.setHostname(Hostname.c_str());

        // The following code is copied verbatim from the ESP32 BasicOTA.ino example
        // This is the callback for the beginning of the OTA process
        ArduinoOTA
            .onStart([]() {
                    String type;
                    if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                    else // U_SPIFFS
                    type = "filesystem";

                    ESP_LOGW(kLoggingTag, "Start updating %s", type.c_str());
                    })
        // When the update ends print it to serial
        .onEnd([]() {
                ESP_LOGW(kLoggingTag, "\nEnd");
                })
        // Show the progress of the update
        .onProgress([](unsigned int progress, unsigned int total) {
                ESP_LOGI(kLoggingTag, "Progress: %u%%\r", (progress / (total / 100)));
                })
        // Error handling for the update
        .onError([](ota_error_t error) {
                ESP_LOGE(kLoggingTag, "Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) ESP_LOGW(kLoggingTag, "Auth Failed");
                else if (error == OTA_BEGIN_ERROR) ESP_LOGW(kLoggingTag, "Begin Failed");
                else if (error == OTA_CONNECT_ERROR) ESP_LOGW(kLoggingTag, "Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) ESP_LOGW(kLoggingTag, "Receive Failed");
                else if (error == OTA_END_ERROR) ESP_LOGW(kLoggingTag, "End Failed");
                });

        // Start the OTA service
        ArduinoOTA.begin();

        ESP_LOGI(kLoggingTag, "* OTA: -> Configuration completed.");
    } else {
        ESP_LOGI(kLoggingTag, "* OTA: Not configured.");
    }

#endif
}

void Esp32IotBase::checkConfigureWebserver_()
{
#ifndef ESP32IOTBASE_NO_WEB

    if (configurationUi_ == ConfigurationUI::always ||
       (configurationUi_ == ConfigurationUI::accessPoint && Network.GetWiFiOperationMode() == NetworkControlBase::Mode::accessPoint))
    {
        ESP_LOGI(kLoggingTag, "* Web Server: Configuring ...");

        Web.Config = &Config;

        String deviceName = Config.Get(ConfigKey::DeviceName);
        if (deviceName == "") {
            deviceName = "Unconfigured Esp32IotBase Device";
        }

        Web.UiAddElement("title", "title", deviceName,"head");

        // Add a webinterface element for the h1 that contains the device name. It is a child of the #wrapper-element.
        Web.UiAddElement("heading", "h1", "", "#wrapper"); Web.UiSetLastEleAttr("class", "fat-border");
        Web.UiAddElement("logo", "img", "", "#heading"); Web.UiSetLastEleAttr("src", "/logo.svg");
        Web.UiAddElement("devicename", "span", deviceName,"#heading");

        // Add the configuration form, that will include all inputs for config data
        Web.UiAddElement("configform", "form", "", "#wrapper"); Web.UiSetLastEleAttr("action", "#"); Web.UiSetLastEleAttr("onsubmit", "collectConfiguration()");
        Web.UiAddElement("infotext1", "p", "Configure your device with the following options (empty: use default value, space: override potential default with empty string):", "#configform");

        Web.UiAddFormInput(ConfigKey::DeviceName, "Device name"); Web.UiSetLastEleAttr("required", "1");

        #ifndef ESP32IOTBASE_NETWORK_ETHERNET
            // Add an input field for the WIFI data and link it to the corresponding configuration data
            Web.UiAddFormInput(ConfigKey::WifiSsid, "WIFI SSID:");
            Web.UiAddFormInput(ConfigKey::WifiPassword, "WIFI Password:"); Web.UiSetLastEleAttr("type", "password");
        #endif

        #ifndef ESP32IOTBASE_NO_SNTP
            Web.UiAddFormInput(ConfigKey::SntpServer, "SNTP Server");
            Web.UiAddFormInput(ConfigKey::SntpTz, "SNTP TZ");
        #endif

        #ifndef ESP32IOTBASE_NO_MQTT
            // Add input fields for MQTT configurations
            Web.UiAddFormInput(ConfigKey::MqttHost, "MQTT Host (format: host[:port]):");
            Web.UiAddFormInput(ConfigKey::MqttUser, "MQTT User:");
            Web.UiAddFormInput(ConfigKey::MqttPassword, "MQTT Password:"); Web.UiSetLastEleAttr("type", "password");
            Web.UiAddFormInput(ConfigKey::MqttTopicPrefix, "MQTT Topic Prefix (suggested 'esp32-iotbase'):");
            Web.UiAddFormInput(ConfigKey::MqttHaDiscPref, "Home Assistant MQTT Discovery Topic Prefix (suggested 'homeassistant', empty to disable):");
        #endif

        #ifndef ESP32IOTBASE_NO_OTA
            Web.UiAddFormInput(ConfigKey::OtaActive, "OTA Active:");
            Web.UiAddFormInput(ConfigKey::OtaPassword, "OTA Password:"); Web.UiSetLastEleAttr("type", "password");
        #endif

        #ifndef ESP32IOTBASE_NO_SYSLOG
            Web.UiAddFormInput(ConfigKey::SyslogServer, "Syslog Server (space/empty to disable):");
        #endif

        // Add a save button that calls the JavaScript function collectConfiguration() on click
        Web.UiAddElement("saveform", "button", "Save", "#configform"); Web.UiSetLastEleAttr("type", "submit");

        // Show the devices MAC in the Webinterface
        Web.UiAddElement("infotext2", "p", "This device has the MAC-Address: " + Mac, "#wrapper");

        Web.UiAddElement("footer", "footer", "Powered by ", "body");
        Web.UiAddElement("footerlink", "a", "Esp32IotBase", "footer"); Web.UiSetLastEleAttr("href", "https://github.com/felixstorm/Esp32IotBase"); Web.UiSetLastEleAttr("target", "_blank");

        #ifndef ESP32IOTBASE_NO_MDNS
                ESP_LOGI(kLoggingTag, "* MDNS responder: Configuring ...");
                if (MDNS.begin(Hostname.c_str())) {
                    MDNS.addService("http", "tcp", 80);
                    ESP_LOGI(kLoggingTag, "* MDNS responder: -> Configuration completed.");
                } else {
                    ESP_LOGE(kLoggingTag, "* MDNS responder: -> ERROR: Configuration failed!");
                }
        #endif

        #ifndef ESP32IOTBASE_NO_CAPTIVE_PORTAL
            if (Network.GetWiFiOperationMode() == NetworkControlBase::Mode::accessPoint) {
                ESP_LOGI(kLoggingTag, "* Initializing captive portal (web request handler & wildcard DNS server)");
                IPAddress softApIp = Network.GetSoftApIp();
                Web.AddCaptiveRequestHandler(softApIp);
                dnsServer_.start(53, "*", softApIp);
                xTaskCreatePinnedToCore(&dnsHandlerTask_, "IotBaseDns", 4096, (void*) &dnsServer_, 5, NULL, CONFIG_ARDUINO_RUNNING_CORE);
            }
        #endif

        // Start webserver and pass the configuration object to it
        // Also pass a function that restarts the device after the configuration has been saved.
        std::function<void()> restartAfterSubmit = [](){
            delay(2000);
            ESP.restart();
        };
        Web.Begin(Config, restartAfterSubmit);

        ESP_LOGI(kLoggingTag, "* Web Server: -> Configuration completed.");
    } else {
        ESP_LOGI(kLoggingTag, "* Web Server: Not configured.");
    }

#endif
}

// handles DNS requests from clients for the captive portal
void Esp32IotBase::dnsHandlerTask_(void* dnsServerPointer)
{
    #ifndef ESP32IOTBASE_NO_CAPTIVE_PORTAL
        DNSServer* DnsServer = (DNSServer*) dnsServerPointer;
        while(1) {
            DnsServer->processNextRequest();
            vTaskDelay(100);
        }
    #endif
};

void IotBase_ResetNetworkConnectedWatchdog()
{
    IotBase.Network.ResetNetworkConnectedWatchdog();
}
