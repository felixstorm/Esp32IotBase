/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   by Felix Storm (http://github.com/felixstorm)
   Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "WebServer.hpp"


namespace {
    const constexpr char* kLoggingTag = "IotBaseWeb";
}

WebServer::WebServer()
    : server_(80)
{
}

void WebServer::AddCaptiveRequestHandler(IPAddress localIpAddress)
{
    #ifndef ESP32IOTBASE_NO_CAPTIVE_PORTAL
        server_.addHandler(new CaptiveRequestHandler(localIpAddress)).setFilter(ON_AP_FILTER);
    #endif
}

void WebServer::Begin(Configuration &configuration, std::function<void()> submitFunc) {

    server_.addHandler(new InternalGzippedFilesHandler());

    server_.on("/data.json" , HTTP_GET, [&configuration, this](AsyncWebServerRequest * request)
    {
            AsyncJsonResponse *response = new AsyncJsonResponse(false, 8192);
            
            JsonObject _jsonData = response->getRoot();
            JsonArray elements = _jsonData.createNestedArray("elements");

            for (const auto &interfaceElement : interfaceElements_)
            {
                JsonObject element = elements.createNestedObject();
                JsonObject attributes = element.createNestedObject("attributes");
                element["element"] = interfaceElement.element;
                element["id"] = interfaceElement.id;
                element["content"] = interfaceElement.content;
                element["parent"] = interfaceElement.parent;

                for (const auto &attribute : interfaceElement.attributes)
                {
                    attributes[attribute.first] = String{attribute.second};
                }

                if (interfaceElement.getAttribute("data-config").length() != 0)
                {
                    if (interfaceElement.getAttribute("type")=="password")
                    {
                        attributes["placeholder"] = "Password unchanged";
                        attributes["value"] = "";
                    } else {
                        attributes["value"] = configuration.Get(interfaceElement.getAttribute("data-config"));
                    }
                }
            }

            if (LOG_LOCAL_LEVEL >= ESP_LOG_VERBOSE) {
                std::ostringstream output;
                serializeJsonPretty(_jsonData, output);
                ESP_LOGV(kLoggingTag, "\r\n%s", output.str().c_str());
            }

            response->setLength();

            // NOTE: AsyncServer.send(ptr* foo) deletes `response` after async send.
            request->send(response);
    });

    server_.on("/submitconfig", HTTP_POST, [&configuration, submitFunc, this](AsyncWebServerRequest *request)
    {
            ESP_LOG_WEBREQUEST(ESP_LOG_VERBOSE, kLoggingTag, request);

            if (request->params() == 0) {
                ESP_LOGW(kLoggingTag, "Refusing to take over an empty configuration submission.");
                request->send(500);
                return;
            }

            for (int i = 0; i < request->params(); i++)
            {
                AsyncWebParameter *webParameter = request->getParam(i);
                if (webParameter->isPost() && webParameter->value().length() != 0)
                {
                        // allow to clear value by entering spaces
                        String valueTrimmed = String(webParameter->value());
                        valueTrimmed.trim();
                        configuration.Set(webParameter->name(), valueTrimmed);
                }
            }
            configuration.Save();

            request->send(201);

            // call submitFunc if it has been set
            if (submitFunc)
                submitFunc();
    });

    server_.onNotFound([this](AsyncWebServerRequest *request)
    {
        ESP_LOG_WEBREQUEST(ESP_LOG_VERBOSE, kLoggingTag, request);
        ESP_LOGD(kLoggingTag, "Request not found: %s", request->url().c_str());
        request->send(404);
    });
    
    server_.begin();
}

// Remark: The server should be stopped before any changes to the interface elements are done to avoid inconsistent results if a request comes in at that very moment.
// However, ESPAsyncWebServer does not support any kind of end() function or something like that in the moment.
void WebServer::UiAddElement(const String &elementId, const String &elementName, const String &content, const String &parent, const String &configVariable)
{
    interfaceElements_.emplace_back(elementId, std::move(elementName), std::move(content), std::move(parent));
    if (configVariable.length() != 0) {
        UiSetElementAttribute(elementId, "data-config", std::move(configVariable));
    }
}

void WebServer::UiAddFormInput(const ConfigurationKey &configVariable, const String &content)
{
    String elementIdAndConfigVariable = configVariable._to_string();
    UiAddElement(elementIdAndConfigVariable, "input", content, "#configform", elementIdAndConfigVariable);
}

void WebServer::UiSetElementAttribute(const String &elementId, const String &attributeKey, const String &attributeValue)
{
    for (auto &element : interfaceElements_) {
        if (element.getId() == elementId) {
            element.setAttribute(attributeKey, attributeValue);
            return;
        }
    }
}

void WebServer::UiSetLastEleAttr(const String &attributeKey, const String &attributeValue)
{
    interfaceElements_.back().setAttribute(attributeKey, attributeValue);
}


void WebServerDebugPrintRequestImpl(esp_log_level_t level, const char* tag, const char* logPrefix, AsyncWebServerRequest *request)
{
        // AsyncWebServer code uses some strange bit-consstructs instead of enum class. Also no const getter.
        const std::map<WebRequestMethodComposite, std::string> requestMethods{
            { HTTP_GET, "GET" },
            { HTTP_POST, "POST" },
            { HTTP_DELETE, "DELETE" },
            { HTTP_PUT, "PUT" },
            { HTTP_PATCH, "PATCH" },
            { HTTP_HEAD, "HEAD" },
            { HTTP_OPTIONS, "OPTIONS" },
        };

        std::ostringstream output;

        output << "Method: ";
        auto found = requestMethods.find(request->method());
        if (found != requestMethods.end()) {
            output << found->second;
        } else {
            output << "Unknown (" << static_cast<unsigned int>(request->method()) << ")";
        }
        output << std::endl;

        output << "Host: " << request->host().c_str() << std::endl;
        output << "URL: " << request->url().c_str() << std::endl;
        output << "Content-Length: " << request->contentLength() << std::endl;
        output << "Content-Type: " << request->contentType().c_str() << std::endl;

        output << "Headers: " << std::endl;
        for (int i = 0; i < request->headers(); i++) {
                auto *header = request->getHeader(i);
                output << "\t" << header->name().c_str() << ": " << header->value().c_str() << std::endl;
        }

        output << "Parameters: " << std::endl;
        for (int i = 0; i < request->params(); i++) {
                auto *parameter = request->getParam(i);
                output << "\t";
                if (parameter->isFile()) {
                    output << "This is a file. FileSize: " << parameter->size() << std::endl << "\t\t";
                }
                output << parameter->name().c_str() << ": " << parameter->value().c_str() << std::endl;
        }

        esp_log_write(level, tag, "%s%s", logPrefix, output.str().c_str());
}
