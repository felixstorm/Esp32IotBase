#pragma once

#include <Esp32Logging.hpp>
#include <ArduinoJson.h>
#include <functional>
#include <mqtt_client.h>

typedef std::function<void()> OnConnectUserCallback;

class EspIdfMqttClient {
    public:
        EspIdfMqttClient& BeginWithHost(const String& mqttHost, const String& mqttUser, const String& mqttPassword, const String& deviceName = {}, const String& haDiscoveryTopicPrefix = {}, const String& baseTopic = {});
        EspIdfMqttClient& BeginWithUri(const String& mqttUri, const String& deviceName = {}, const String& haDiscoveryTopicPrefix = {}, const String& baseTopic = {});
        EspIdfMqttClient& OnConnect(OnConnectUserCallback callback);
        void Publish(const String& message, bool retain = false, const String& topicSuffix = {}, const String& topic = {});
        void Publish(JsonDocument message, bool retain = false, const String& topicSuffix = {}, const String& topic = {});
        void PublishHaDiscoveryInformation(bool isBinary, const String &unitOfMeasurement, const String &deviceClass, int expireAfter, const String &valueTemplate,
                                           bool forceUpdate, bool setJsonAttributesTopic, const String &entityIdSuffix, const String &stateTopicSuffix);
    private:
        String macAddress;
        String deviceName;
        String baseTopic;
        String haDiscoveryTopicPrefix;
        esp_mqtt_client_handle_t mqttClient;
        static esp_err_t StaticEventHandler(esp_mqtt_event_handle_t event);
        esp_err_t EventHandler(esp_mqtt_event_handle_t event);
        std::vector<OnConnectUserCallback> _onConnectUserCallbacks;
        bool isConnected_;
};

void IotBase_ResetNetworkConnectedWatchdog();
