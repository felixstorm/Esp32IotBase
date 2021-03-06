/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   by Felix Storm (http://github.com/felixstorm)
   Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */
#include "Configuration.hpp"

namespace {
    const constexpr char* kLoggingTag = "IotBaseConfig";
    const constexpr char* kNvsNamespaceName = "Esp32IotBase";
}

Configuration::Configuration()
{
}

bool Configuration::Begin() {

    ESP_LOGD(kLoggingTag, "Initializing NVS flash");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(kLoggingTag, "Need to erase NVS flash");
        if((err = nvs_flash_erase())) {
            ESP_LOGE(kLoggingTag, "Error erasing erasing NVS flash: %#x (%s)", err, esp_err_to_name(err));
            return false;
        }
        err = nvs_flash_init();
    }
    if(err) {
        ESP_LOGE(kLoggingTag, "Error initializing NVS flash: %#x (%s)", err, esp_err_to_name(err));
        return false;
    }

    ESP_LOGD(kLoggingTag, "Opening NVS flash for namespace '%s'", kNvsNamespaceName);
    err = nvs_open(kNvsNamespaceName, NVS_READWRITE, &nvsHandle_);
    if (err) {
        ESP_LOGE(kLoggingTag, "Error initializing NVS flash: %#x (%s)", err, esp_err_to_name(err));
        return false;
    }

    return true;
}

bool Configuration::Save() {
    ESP_LOGD(kLoggingTag, "Committing NVS flash");
    esp_err_t err = nvs_commit(nvsHandle_);
    if (err) {
        ESP_LOGE(kLoggingTag, "Error committing NVS flash: %#x (%s)", err, esp_err_to_name(err));
        return false;
    }

    return true;
}

bool Configuration::Reset()
{
    ESP_LOGD(kLoggingTag, "Erasing NVS flash for namespace '%s'", kNvsNamespaceName);
    esp_err_t err = nvs_erase_all(nvsHandle_);
    if (err) {
        ESP_LOGE(kLoggingTag, "Error erasing NVS flash: %#x (%s)", err, esp_err_to_name(err));
        return false;
    }

    return Save();
}

void Configuration::Set(const ConfigKey &key, const String &value) {
    Set(key._to_string(), value);
}

void Configuration::Set(const String &key, const String &value) {
    Set(key.c_str(), value);
}

void Configuration::Set(const char* key, const String &value) {
    
    esp_err_t err;
    if (value.isEmpty()) {
        ESP_LOGD(kLoggingTag, "Erasing '%s' because it is empty (was '%s')", key, GetRaw(key).c_str());
        err = nvs_erase_key(nvsHandle_, key);
    } else {
        ESP_LOGD(kLoggingTag, "Setting '%s' to '%s' (was '%s')", key, value.c_str(), Get(key).c_str());
        err = nvs_set_str(nvsHandle_, key, value.c_str());
    }
    if (err && err != ESP_ERR_NVS_NOT_FOUND) { // ESP_ERR_NVS_NOT_FOUND is to be expected when erasing
        ESP_LOGE(kLoggingTag, "Error writing or erasing string '%s' to NVS flash: %#x (%s)", key, err, esp_err_to_name(err));
    }
}

void Configuration::SetInt(const ConfigKey &key, int value) {
    SetInt(key._to_string(), value);
}

void Configuration::SetInt(const String &key, int value) {
    SetInt(key.c_str(), value);
}

void Configuration::SetInt(const char* key, int value) {
    ESP_LOGD(kLoggingTag, "Setting '%s' to %d (was %d)", key, value, GetInt(key));

    esp_err_t err = nvs_set_i32(nvsHandle_, key, value);
    if (err) {
        ESP_LOGE(kLoggingTag, "Error writing int '%s' to NVS flash: %#x (%s)", key, err, esp_err_to_name(err));
    }
}

const String Configuration::Get(const ConfigKey &key, const String &defaultValue) const
{
    return Get(key._to_string(), defaultValue);
}

const String Configuration::Get(const String &key, const String &defaultValue) const
{
    return Get(key.c_str(), defaultValue);
}

const String Configuration::GetRaw(const char* key) const
{
    size_t required_size;
    esp_err_t err = nvs_get_str(nvsHandle_, key, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(kLoggingTag, "Tried to read string '%s' from NVS flash: not found, returning empty string", key);
        return "";
    }
    if (err) {
        ESP_LOGE(kLoggingTag, "Error determining length of string '%s' from NVS flash: %#x (%s)", key, err, esp_err_to_name(err));
    }

    char* resultBuffer = (char*) malloc(required_size);
    if ((err = nvs_get_str(nvsHandle_, key, resultBuffer, &required_size))) {
        ESP_LOGE(kLoggingTag, "Error reading string '%s' from NVS flash: %#x (%s)", key, err, esp_err_to_name(err));
    }
    ESP_LOGD(kLoggingTag, "Read string '%s' from NVS flash: '%s'", key, resultBuffer);

    String result(resultBuffer);
    free(resultBuffer);

    return result;
}

const String Configuration::Get(const char* key, const String &defaultValue) const
{
    String result = GetRaw(key);
    if (result.isEmpty()) {
        ESP_LOGD(kLoggingTag, "Result string '%s' is empty", key);
        if (!defaultValue.isEmpty()) {
            ESP_LOGD(kLoggingTag, "Returning provided default: '%s'", defaultValue.c_str());
            return defaultValue;
        } else {
            auto configDefault = StringDefaults.find(key);
            if (configDefault != StringDefaults.end()) {
                ESP_LOGD(kLoggingTag, "Returning Config default: '%s'", configDefault->second.c_str());
                return configDefault->second;
            } else {
                ESP_LOGD(kLoggingTag, "Returning empty string.");
                return "";
            }
        }
    }

    result.trim();

    return result;
}

int Configuration::GetInt(const ConfigKey &key, int defaultValue) const
{
    return GetInt(key._to_string(), defaultValue);
}

int Configuration::GetInt(const String &key, int defaultValue) const
{
    return GetInt(key.c_str(), defaultValue);
}

int Configuration::GetInt(const char* key, int defaultValue) const
{
    int result;
    esp_err_t err = nvs_get_i32(nvsHandle_, key, &result);
    if (err) {
        if (err != ESP_ERR_NVS_NOT_FOUND)
            ESP_LOGE(kLoggingTag, "Error reading int '%s' from NVS flash: %#x (%s)", key, err, esp_err_to_name(err));
        return defaultValue;
    }
    ESP_LOGD(kLoggingTag, "Read int '%s' from NVS flash: %d", key, result);

    return result;
}
