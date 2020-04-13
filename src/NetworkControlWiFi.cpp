#include "NetworkControlWiFi.hpp"


namespace {
    const constexpr char* kLoggingTag = "IotBaseNetwork";

    // Minumum access point secret length to be generated (8 is min for ESP32)
    const constexpr unsigned minApSecretLength = 8;
}

void NetworkControlWiFi::Begin(Configuration& configuration, bool encryptAp, String fixedApPassword, String hostname)
{
    configureNetworkConnectionWdt_();

    operationMode_ = configuration.Get(ConfigurationKey::WifiSsid).length() > 0 ? Mode::client : Mode::accessPoint;

    WiFi.onEvent(wiFiEvent_);

    if (operationMode_ == Mode::accessPoint) {

        String wifiAPName = "ESP32_" + GetMacAddress();
        ESP_LOGW(kLoggingTag, "Wifi is NOT configured, starting Wifi AP '%s'", wifiAPName.c_str());

        if (fixedApPassword.length() != 0) {
            encryptAp = true;
            if (fixedApPassword.length() < minApSecretLength) {
                ESP_LOGE(kLoggingTag, "Error: Given fixed access point secret is too short. Refusing.");
                fixedApPassword = "";
            }
        }

        String apSecret;
        if (encryptAp) {
            apSecret = fixedApPassword;
            if (apSecret.length() >= minApSecretLength) {
                ESP_LOGW(kLoggingTag, "Using fixed access point secret.");
            } else {
                apSecret = configuration.Get(ConfigurationKey::ApSecret);
                if (apSecret.length() >= minApSecretLength) {
                    ESP_LOGW(kLoggingTag, "Using saved access point secret.");
                } else {
                    ESP_LOGW(kLoggingTag, "Generating random access point secret.");
                    apSecret = generateRandomSecret_(minApSecretLength);
                }
            }
        } else {
            ESP_LOGW(kLoggingTag, "Not encrypting access point.");
        }
        configuration.Set(ConfigurationKey::ApSecret, apSecret);
        configuration.Save();

        WiFi.mode(WIFI_AP_STA);
        if (apSecret.length() > 0) {
            ESP_LOGI(kLoggingTag, "Starting AP with password %s\n", apSecret.c_str());
            WiFi.softAP(wifiAPName.c_str(), apSecret.c_str());
        } else {
            WiFi.softAP(wifiAPName.c_str());
        }
    }
    else if (operationMode_ == Mode::client) {

        String wifiSsid = configuration.Get(ConfigurationKey::WifiSsid);
        String wifiPassword = configuration.Get(ConfigurationKey::WifiPassword);
        
        ESP_LOGI(kLoggingTag, "Wifi is configured, connecting to '%s'", wifiSsid.c_str());

        WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
        // TBD klären
        // https://github.com/me-no-dev/ESPAsyncWebServer/issues/437
        // https://github.com/espressif/arduino-esp32/issues/3157
        WiFi.setSleep(false);
        WiFi.setHostname(hostname.c_str());
        //WiFi.setAutoConnect ( true );
        //WiFi.setAutoReconnect ( true );

        // make sure we are fully connected when we return from Begin()
        waitForConnection_();
    }
}

String NetworkControlWiFi::GetMacAddress(const String& delimiter)
{
    uint8_t rawMac[6];
    WiFi.macAddress(rawMac);
    return format6Bytes_(rawMac, delimiter);
}

void NetworkControlWiFi::wiFiEvent_(WiFiEvent_t event)
{
    ESP_LOGD(kLoggingTag, "WiFiEvent %d", event);

    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        localIp_ = WiFi.localIP();
        ESP_LOGI(kLoggingTag, "WIFI Got IPv4 address %s", localIp_.toString().c_str());
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        localIp_ = (uint32_t)0;
        ESP_LOGI(kLoggingTag, "WIFI Lost connection");
        WiFi.reconnect();
        break;
    default:
        break;
    }
}

String NetworkControlWiFi::generateRandomSecret_(unsigned length) const
{
    // There is no "O" (Oh) to reduce confusion
    const String validChars{"abcdefghjkmnopqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ23456789.-,:/"};
    String returnValue;

    unsigned useLength = (length < minApSecretLength)?minApSecretLength:length;
    returnValue.reserve(useLength);

    for (unsigned i = 0; i < useLength; i++)
    {
        auto randomValue = validChars[(esp_random() % validChars.length())];
        returnValue += randomValue;
    }

    return returnValue;
}
