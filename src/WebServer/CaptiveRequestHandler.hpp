#pragma once

#include <ESPAsyncWebServer.h>


class CaptiveRequestHandler : public AsyncWebHandler {

    private:
        const char* kLoggingTag = "IotBaseWebCaptiveHandler";
        String localIpAddressString_;

    public:
        CaptiveRequestHandler(IPAddress localIpAddress)
            : localIpAddressString_(localIpAddress.toString())
        {
        }

        bool canHandle(AsyncWebServerRequest *request) {
            // handle all requests not directed directly to us
            bool result = request->host() != localIpAddressString_;

            ESP_LOG_WEBREQUEST(ESP_LOG_VERBOSE, kLoggingTag, request);
            ESP_LOGV(kLoggingTag, "result: %d", result);

            return result;
        }

        void handleRequest(AsyncWebServerRequest *request) {
            // redirect to our IP and root
            // cannot use request->redirect() as we also want to set Cache-Control: no-store to ensure it's temporary only
            AsyncWebServerResponse * response = request->beginResponse(302);
            response->addHeader("Location", "http://" + localIpAddressString_ + "/");
            response->addHeader("Cache-Control", "no-store");
            request->send(response);
        }
};
