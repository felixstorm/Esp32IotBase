#include "EspIdfMqttClient.hpp"


EspIdfMqttClient& EspIdfMqttClient::Begin(const String& mqttUri, const String& deviceName, const String& haDiscoveryTopicPrefix, const String& baseTopic)
{
    ESP_LOGD("MQTT", "mqttUri: %s, deviceName: %s, haDiscoveryTopicPrefix: %s, baseTopic: %s", 
             mqttUri.c_str(), deviceName.c_str(), haDiscoveryTopicPrefix.c_str(), baseTopic.c_str());

    if (!mqttUri.isEmpty()) {
        uint8_t rawMac[6];
        char macString[sizeof(rawMac) * 2 + 1];
        esp_read_mac(rawMac, ESP_MAC_WIFI_STA);
        sprintf(macString, "%02x%02x%02x%02x%02x%02x", rawMac[0], rawMac[1], rawMac[2], rawMac[3], rawMac[4], rawMac[5]);
        this->macAddress = macString;

        this->deviceName = !deviceName.isEmpty() ? deviceName : String("esp32-" + this->macAddress);
        this->baseTopic = !baseTopic.isEmpty() ? baseTopic : String("esp-basecamp/" + this->deviceName);
        this->haDiscoveryTopicPrefix = haDiscoveryTopicPrefix;
        String clientId = this->deviceName + (this->deviceName.indexOf(this->macAddress) < 0 ? ("-" + this->macAddress) : "");
        ESP_LOGD("MQTT", "macAddress: %s, deviceName: %s, baseTopic: %s, clientId: %s", 
                 this->macAddress.c_str(), this->deviceName.c_str(), this->baseTopic.c_str(), clientId.c_str());

        esp_mqtt_client_config_t mqtt_cfg = {};
        mqtt_cfg.uri = mqttUri.c_str();
        mqtt_cfg.event_handle = StaticEventHandler;
        mqtt_cfg.user_context = this;
        mqtt_cfg.client_id = clientId.c_str();
        mqttClient = esp_mqtt_client_init(&mqtt_cfg);
        
        esp_mqtt_client_start(mqttClient);
    }

    return *this;
}

EspIdfMqttClient& EspIdfMqttClient::OnConnect(OnConnectUserCallback callback) {
  // TBD: fire immediately if alreay connected?
  _onConnectUserCallbacks.push_back(callback);
  return *this;
}

esp_err_t EspIdfMqttClient::StaticEventHandler(esp_mqtt_event_handle_t event)
{
    ESP_LOGD("MQTT", "Event received: %i", (int)event->event_id);

    return reinterpret_cast<EspIdfMqttClient*>(event->user_context)->EventHandler(event);
}

esp_err_t EspIdfMqttClient::EventHandler(esp_mqtt_event_handle_t event)
{
    if (event->event_id == MQTT_EVENT_CONNECTED)
    {
        ESP_LOGI("MQTT", "Connected");
        for (auto callback : _onConnectUserCallbacks)
            callback();
    }

    return ESP_OK;
}

void EspIdfMqttClient::Publish(const String& message, bool retain /* = false */, const String& topicSuffix /* = {} */, const String& topic /* = {} */)
{
    String topicInt = !topic.isEmpty() ? topic : baseTopic;
    if (!topicSuffix.isEmpty())
        topicInt += "/" + topicSuffix;

    ESP_LOGI("MQTT", "topic: %s, retain: %u, message: %s", topicInt.c_str(), retain, message.c_str());

    int publishResult = -1;
    if (mqttClient)
        publishResult = esp_mqtt_client_publish(mqttClient, topicInt.c_str(),  message.c_str(), 0, 0, retain);

    ESP_LOGI("MQTT", "publish result: %i", publishResult);
}

void EspIdfMqttClient::Publish(JsonDocument message, bool retain /* = false */, const String& topicSuffix /* = {} */, const String& topic /* = {} */)
{
    ESP_LOGD("MQTT", "Entered function");

    String stringMessage;
    serializeJson(message, stringMessage);
#if DEBUG
    Serial.println("message:");
    serializeJsonPretty(message, Serial);
    Serial.println();
#endif

    Publish(stringMessage, retain, topicSuffix, topic);
}

void EspIdfMqttClient::PublishHaDiscoveryInformation(bool isBinary, const String &unitOfMeasurement, const String &deviceClass, int expireAfter, const String &valueTemplate,
                                         bool forceUpdate, bool setJsonAttributesTopic, const String &entitySuffix, const String &stateTopicSuffix)
{
    if (!haDiscoveryTopicPrefix.isEmpty())
    {

        ESP_LOGI("MQTT", "stateTopicSuffix: %s, entityIdSuffix: %s", stateTopicSuffix.c_str(), entitySuffix.c_str());

        String entityIdSuffixInt;
        if (!stateTopicSuffix.isEmpty())
            entityIdSuffixInt += "_" + stateTopicSuffix;
        if (!entitySuffix.isEmpty())
            entityIdSuffixInt += "_" + entitySuffix;
        
        // TBD slash vs. underscore
        String uniqueDeviceId = "esp32_" + macAddress;
        String uniqueEntityId = uniqueDeviceId + entityIdSuffixInt;
        String entityName = deviceName + entityIdSuffixInt;
        String entityStateTopic = baseTopic;
        if (!stateTopicSuffix.isEmpty())
            entityStateTopic += "/" + stateTopicSuffix;
        String haEntityType = isBinary ? "binary_sensor" : "sensor";

        ESP_LOGD("MQTT", "haDiscoveryTopicPrefix: %s, uniqueDeviceId: %s, deviceName: %s, uniqueEntityId: %s, entityName: %s, entityStateTopic: %s",
                 haDiscoveryTopicPrefix.c_str(), uniqueDeviceId.c_str(), deviceName.c_str(), uniqueEntityId.c_str(), entityName.c_str(), entityStateTopic.c_str());

        DynamicJsonDocument haDiscovery(2048);
        haDiscovery["unique_id"] = uniqueEntityId;
        haDiscovery["name"] = entityName;
        haDiscovery["state_topic"] = entityStateTopic;
        if (unitOfMeasurement && !unitOfMeasurement.isEmpty())
            haDiscovery["unit_of_measurement"] = unitOfMeasurement;
        if (deviceClass && !deviceClass.isEmpty())
            haDiscovery["device_class"] = deviceClass;
        if (expireAfter)
            haDiscovery["expire_after"] = expireAfter;
        if (valueTemplate && !valueTemplate.isEmpty())
        {
            haDiscovery["value_template"] = valueTemplate;
            if (isBinary)
            {
                // DynamicJsonDocument will render true/false as true/false
                haDiscovery["payload_on"] = true;
                haDiscovery["payload_off"] = false;
            }
        }
        else
        {
            if (isBinary)
            {
                // String(boolean) will render true/false as 1/0
                haDiscovery["payload_on"] = 1;
                haDiscovery["payload_off"] = 0;
            }
        }
        if (forceUpdate)
            haDiscovery["force_update"] = "true";
        if (setJsonAttributesTopic)
            haDiscovery["json_attributes_topic"] = entityStateTopic;

        JsonObject deviceObject = haDiscovery.createNestedObject("device");
        deviceObject.createNestedArray("identifiers").add(uniqueDeviceId);
        deviceObject["name"] = deviceName;

        Publish(haDiscovery, true, "", haDiscoveryTopicPrefix + "/" + haEntityType + "/" + uniqueEntityId + "/config");
    }
}
