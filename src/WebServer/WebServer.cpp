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
#ifdef ESP32IOTBASE_USEDNS
#ifdef DNSServer_h
    server_.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
#endif
#endif
}

void WebServer::Begin(Configuration &configuration, std::function<void()> submitFunc) {

    server_.on("/" , HTTP_GET, [](AsyncWebServerRequest * request)
    {
            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", k_index_htm_gz, k_index_htm_gz_len);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
    });

    server_.on("/esp32iotbase.css" , HTTP_GET, [](AsyncWebServerRequest * request)
    {
            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", k_esp32iotbase_css_gz, k_esp32iotbase_css_gz_len);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
    });

    server_.on("/esp32iotbase.js" , HTTP_GET, [](AsyncWebServerRequest * request)
    {
            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", k_esp32iotbase_js_gz, k_esp32iotbase_js_gz_len);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
    });
    server_.on("/logo.svg" , HTTP_GET, [](AsyncWebServerRequest * request)
    {
            AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg+xml", k_logo_svg_gz, k_logo_svg_gz_len);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
    });

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
#ifdef DEBUG
            serializeJsonPretty(_jsonData, Serial);
#endif
            response->setLength();

            // NOTE: AsyncServer.send(ptr* foo) deletes `response` after async send.
            request->send(response);
    });

    server_.on("/submitconfig", HTTP_POST, [&configuration, submitFunc, this](AsyncWebServerRequest *request)
    {
            if (request->params() == 0) {
                ESP_LOGW(kLoggingTag, "Refusing to take over an empty configuration submission.");
                request->send(500);
                return;
            }
            debugPrintRequest_(request);

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
        ESP_LOGD(kLoggingTag, "Request not found:");
        debugPrintRequest_(request);
        request->send(404);
    });
    
    server_.begin();
}

void WebServer::debugPrintRequest_(AsyncWebServerRequest *request)
{
#ifdef DEBUG
        /**
         That AsyncWebServer code uses some strange bit-consstructs instead of enum
         class. Also no const getter. As I refuse to bring that code to 21st century,
         we have to live with it until someone brave fixes it.
        */
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

        Serial.println(output.str().c_str());
#endif
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
