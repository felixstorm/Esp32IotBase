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
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

#include "CompressedData.hpp"
#include "Configuration.hpp"
#include "WebInterface.hpp"

#ifdef ESP32IOTBASE_USEDNS
#ifdef DNSServer_h
#include "CaptiveRequestHandler.hpp"
#endif
#endif

class WebServer {
    public:
        WebServer();

        void Begin(Configuration &configuration, std::function<void()> submitFunc = 0);

        void UiAddElement(const String &elementId, const String &elementName, const String &content, const String &parent = "#configform", const String &configVariable = "");
        void UiAddFormInput(const ConfigurationKey &configVariable, const String &content);
        void UiSetElementAttribute(const String &elementId, const String &attributeKey, const String &attributeValue);
        void UiSetLastEleAttr(const String &attributeKey, const String &attributeValue);

    private:
        void debugPrintRequest_(AsyncWebServerRequest *request);

        AsyncWebServer server_;

        std::vector<InterfaceElement> interfaceElements_;
};
