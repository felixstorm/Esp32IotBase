#pragma once

#include <ESPAsyncWebServer.h>
#include "InternalGzippedFilesContent.hpp"


class InternalGzippedFilesHandler : public AsyncWebHandler {
    public:
        bool canHandle(AsyncWebServerRequest *request) {
            return (request->method() == HTTP_GET && (request->url() == "/" ||
                                                      request->url() == "/esp32iotbase.css" ||
                                                      request->url() == "/esp32iotbase.js" ||
                                                      request->url() == "/logo.svg"));
        }

        void handleRequest(AsyncWebServerRequest *request) {
            
            String contentType;
            size_t contentLength;
            const uint8_t * content;
            if (request->url() == "/") {
                contentType = "text/html";
                contentLength = k_index_htm_gz_len;
                content = k_index_htm_gz;
            } else if (request->url() == "/esp32iotbase.css") {
                contentType = "text/css";
                contentLength = k_esp32iotbase_css_gz_len;
                content = k_esp32iotbase_css_gz;
            } else if (request->url() == "/esp32iotbase.js") {
                contentType = "text/javascript";
                contentLength = k_esp32iotbase_js_gz_len;
                content = k_esp32iotbase_js_gz;
            } else if (request->url() == "/logo.svg") {
                contentType = "image/svg+xml";
                contentLength = k_logo_svg_gz_len;
                content = k_logo_svg_gz;
            } else {
                // should not happen
                request->send(404);
                return;
            }
            
            AsyncWebServerResponse *response = request->beginResponse_P(200, contentType, content, contentLength);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        }
};
