/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */
#include "Configuration.hpp"

namespace {
	const constexpr char* kLoggingTag = "BasecampConfig";
}

Configuration::Configuration()
	: _memOnlyConfig( true ),
	_jsonFile()
{
}

Configuration::Configuration(String filename)
	: _memOnlyConfig( false ),
	_jsonFile(std::move(filename))
{
}

void Configuration::setMemOnly() {
	_memOnlyConfig = true;
	_jsonFile = "";
}

void Configuration::setFileName(const String& filename) {
	_memOnlyConfig = false;
	_jsonFile = filename;
}

bool Configuration::load() {
	ESP_LOGD(kLoggingTag, "Loading config file");
	
	if (_memOnlyConfig) {
		ESP_LOGD(kLoggingTag, "Memory-only configuration: Nothing loaded!");
		return false;
	}
	
	ESP_LOGD(kLoggingTag, "JSON File: %s", _jsonFile.c_str());
	if (!SPIFFS.begin(true)) {
		ESP_LOGE(kLoggingTag, "Could not access SPIFFS.");
		return false;
	}

	File configFile = SPIFFS.open(_jsonFile, "r");

	if (!configFile || configFile.isDirectory()) {
		ESP_LOGE(kLoggingTag, "Failed to open config file");
		return false;
	}

	DynamicJsonDocument _jsonDoc(2048);
	auto error = deserializeJson(_jsonDoc, configFile);
	JsonObject _jsonData = _jsonDoc.as<JsonObject>();

	if (error) {
		ESP_LOGE(kLoggingTag, "Failed to parse config file.");
		return false;
	}

	for (const auto& configItem: _jsonData) {
		set(String(configItem.key().c_str()), String(configItem.value().as<char*>()));
	}

	configFile.close();
	return true;
}

bool Configuration::save() {
	ESP_LOGD(kLoggingTag, "Saving config file");
	
	if (_memOnlyConfig) {
		ESP_LOGD(kLoggingTag, "Memory-only configuration: Nothing saved!");
		return false;
	}

	File configFile = SPIFFS.open(_jsonFile, "w");
	if (!configFile) {
		ESP_LOGE(kLoggingTag, "Failed to open config file for writing");
		return false;
	}

	if (configuration.empty())
	{
		ESP_LOGI(kLoggingTag, "Configuration empty");
	}

	DynamicJsonDocument _jsonDoc(2048);
	
	for (const auto& x : configuration)
	{
		_jsonDoc[String(x.first)] = (String(x.second));
	}

	serializeJson(_jsonDoc, configFile);
#ifdef DEBUG
	serializeJsonPretty(_jsonDoc, Serial);
#endif
	configFile.close();
	_configurationTainted = false;
	return true;
}

void Configuration::set(String key, String value) {
	ESP_LOGD(kLoggingTag, "Setting %s to %s (was %s)", key.c_str(), value.c_str(), get(key).c_str());

	if (get(key) != value) {
		_configurationTainted = true;
		configuration[key] = value;
	} else {
		ESP_LOGD(kLoggingTag, "Cowardly refusing to overwrite existing key with the same value");
	}
}

void Configuration::set(ConfigurationKey key, String value)
{
	set(getKeyName(key), std::move(value));
}

const String &Configuration::get(String key) const
{
	auto found = configuration.find(key);
	if (found != configuration.end()) {
		ESP_LOGD(kLoggingTag, "Config value for %s: %s", key.c_str(), found->second.c_str());
		return found->second;
	}

	// Default: if not set, we just return an empty String. TODO: Throw?
	return noResult_;
}

const String &Configuration::get(ConfigurationKey key) const
{
	return get(getKeyName(key));
}

// return a char* instead of a Arduino String to maintain backwards compatibility
// with printed examples
[[deprecated("getCString() is deprecated. Use get() instead")]]
char* Configuration::getCString(String key)
{
	char *newCString = (char*) malloc(configuration[key].length()+1);
	strcpy(newCString,get(key).c_str());
	return newCString;
}

bool Configuration::keyExists(const String& key) const
{
	return (configuration.find(key) != configuration.end());
}

bool Configuration::keyExists(ConfigurationKey key) const
{
	return (configuration.find(getKeyName(key)) != configuration.end());
}

bool Configuration::isKeySet(ConfigurationKey key) const
{
	auto found = configuration.find(getKeyName(key));
	if (found == configuration.end())
	{
		return false;
	}

	return (found->second.length() > 0);
}

void Configuration::reset()
{
	configuration.clear();
	this->save();
	this->load();
}

void Configuration::resetExcept(const std::list<ConfigurationKey> &keysToPreserve)
{
	std::map<ConfigurationKey, String> preservedKeys;
	for (const auto &key : keysToPreserve) {
		if (keyExists(key)) {
			// Make a copy of the old value
			preservedKeys[key] = get(key);
		}
	}

	configuration.clear();

	for (const auto &key : preservedKeys) {
		set(key.first, key.second);
	}

	this->save();
	this->load();
}

void Configuration::dump() {
#ifndef DEBUG
	for (const auto &p : configuration) {
		ESP_LOGD(kLoggingTag, "configuration[%s] = %s", p.first.c_str(), p.second.c_str());
	}
#endif
}