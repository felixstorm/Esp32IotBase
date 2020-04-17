/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   by Felix Storm (http://github.com/felixstorm)
   Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#pragma once

#include <Esp32Logging.hpp>

#include <map>
#include <vector>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

#include "Configuration.hpp"
#include "WebInterface.hpp"

// declared up here since the handlers (may) need it
void WebServerDebugPrintRequestImpl(esp_log_level_t level, const char* tag, const char* logPrefix, AsyncWebServerRequest *request);
#define ESP_LOG_WEBREQUEST(level, tag, request) do {                                                \
        if (LOG_LOCAL_LEVEL >= level) {                                                             \
            char logPrefixBuffer[255];                                                              \
            snprintf(logPrefixBuffer, sizeof(logPrefixBuffer), LOG_FORMAT(-, tag, "Web request:")); \
            WebServerDebugPrintRequestImpl(level, tag, logPrefixBuffer, request);                   \
        };                                                                                          \
    } while(0);

#include "InternalGzippedFilesHandler.hpp"
#include "CaptiveRequestHandler.hpp"


class WebServer {
    public:
        WebServer();

        void Begin(Configuration &configuration, std::function<void()> submitFunc = 0);
        void AddCaptiveRequestHandler(IPAddress localIpAddress);

        Configuration* Config;

        void UiAddElement(const String &elementId, const String &elementName, const String &content, const String &parent = "#configform", const String &configVariable = "");
        void UiAddFormInput(const ConfigKey &configVariable, const String &content);
        void UiSetElementAttribute(const String &elementId, const String &attributeKey, const String &attributeValue);
        void UiSetLastEleAttr(const String &attributeKey, const String &attributeValue);

    private:
        AsyncWebServer server_;

        std::vector<InterfaceElement> interfaceElements_;
};
