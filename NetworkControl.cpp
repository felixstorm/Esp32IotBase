/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "NetworkControl.hpp"
#ifdef BASECAMP_NETWORK_ETHERNET
#include <ETH.h>
#endif

namespace {
	const constexpr char* kLoggingTag = "BasecampNetwork";

	// Minumum access point secret length to be generated (8 is min for ESP32)
	const constexpr unsigned minApSecretLength = 8;
#ifdef BASECAMP_NETWORK_ETHERNET
	static bool eth_connected = false;
#endif
}

void NetworkControl::begin(String essid, String password, String configured,
												String hostname, String apSecret)
{
#ifdef BASECAMP_NETWORK_ETHERNET
	ESP_LOGI(kLoggingTag, "Connecting to Ethernet");
	operationMode_ = Mode::client;
	WiFi.onEvent(WiFiEvent);
	ETH.begin() ;
	ETH.setHostname(hostname.c_str());
	ESP_LOGD(kLoggingTag, "Ethernet initialized") ;
	ESP_LOGI(kLoggingTag, "Waiting for connection") ;
	while (!eth_connected) {
		Serial.print(".") ;
		delay(100) ;
	}
#else
	ESP_LOGI(kLoggingTag, "Connecting to Wifi");
	String _wifiConfigured = std::move(configured);
	_wifiEssid = std::move(essid);
	_wifiPassword = std::move(password);
	if (_wifiAPName.length() == 0) {
		_wifiAPName = "ESP32_" + getHardwareMacAddress();
	}

	WiFi.onEvent(WiFiEvent);
	if (_wifiConfigured.equalsIgnoreCase("true")) {
		operationMode_ = Mode::client;
		ESP_LOGI(kLoggingTag, "Wifi is configured, connecting to '%s'", _wifiEssid.c_str());

		WiFi.begin(_wifiEssid.c_str(), _wifiPassword.c_str());
		// TBD klären
		// https://github.com/me-no-dev/ESPAsyncWebServer/issues/437
		// https://github.com/espressif/arduino-esp32/issues/3157
		WiFi.setSleep(false);
		WiFi.setHostname(hostname.c_str());
		//WiFi.setAutoConnect ( true );
		//WiFi.setAutoReconnect ( true );
	} else {
		operationMode_ = Mode::accessPoint;
		ESP_LOGW(kLoggingTag, "Wifi is NOT configured, starting Wifi AP '%s'", _wifiAPName.c_str());

		WiFi.mode(WIFI_AP_STA);
		if (apSecret.length() > 0) {
			// Start with password protection
			ESP_LOGD(kLoggingTag, "Starting AP with password %s\n", apSecret.c_str());
			WiFi.softAP(_wifiAPName.c_str(), apSecret.c_str());
		} else {
			// Start without password protection
			WiFi.softAP(_wifiAPName.c_str());
		}
	}
#endif

}


bool NetworkControl::isConnected()
{
#ifdef BASECAMP_NETWORK_ETHERNET
	return eth_connected ;
#else
	return WiFi.isConnected() ;
#endif
}

NetworkControl::Mode NetworkControl::getOperationMode() const
{
	return operationMode_;
}

int NetworkControl::status() {
	return WiFi.status();

}
IPAddress NetworkControl::getIP() {
#ifdef BASECAMP_NETWORK_ETHERNET
	return ETH.localIP() ;
#else
	return WiFi.localIP();
#endif
}
IPAddress NetworkControl::getSoftAPIP() {
	return WiFi.softAPIP();
}

void NetworkControl::setAPName(const String &name) {
	_wifiAPName = name;
}

String NetworkControl::getAPName() {
	return _wifiAPName;
}

void NetworkControl::WiFiEvent(WiFiEvent_t event)
{
	Preferences preferences;
	preferences.begin("basecamp", false);
	unsigned int __attribute__((unused)) bootCounter = preferences.getUInt("bootcounter", 0);
	// In case somebody wants to know this..
	ESP_LOGD(kLoggingTag, "WiFiEvent %d, Bootcounter is %d", event, bootCounter);
#ifdef BASECAMP_NETWORK_ETHERNET
	switch (event) {
    case SYSTEM_EVENT_ETH_START:
      ESP_LOGI(kLoggingTag, "ETH Started");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      ESP_LOGI(kLoggingTag, "ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
	  ESP_LOGI(kLoggingTag, "ETH Got IPv4 %s (%d Mbps, full duplex: %d, MAC %s)", ETH.localIP().toString().c_str(), ETH.linkSpeed(), ETH.fullDuplex(), ETH.macAddress().c_str());
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      ESP_LOGI(kLoggingTag, "ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      ESP_LOGI(kLoggingTag, "ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
#else
	IPAddress ip __attribute__((unused));
	switch(event) {
		case SYSTEM_EVENT_STA_GOT_IP:
			ip = WiFi.localIP();
			ESP_LOGI(kLoggingTag, "WIFI Got IPv4 address %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
			preferences.putUInt("bootcounter", 0);
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
			ESP_LOGI(kLoggingTag, "WIFI Lost connection");
			WiFi.reconnect();
			break;
		default:
			// INFO: Default = do nothing
			break;
	}
#endif
}

namespace {
	template <typename BYTES>
	String format6Bytes(const BYTES &bytes, const String& delimiter)
	{
		std::ostringstream stream;
		for (unsigned int i = 0; i < 6; i++) {
			if (i != 0 && delimiter.length() > 0) {
				stream << delimiter.c_str();
			}
			stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(bytes[i]);
		}

		String mac{stream.str().c_str()};
		return mac;
	}
}

// TODO: This will return the default mac, not a manually set one
// See https://github.com/espressif/esp-idf/blob/master/components/esp32/include/esp_system.h
String NetworkControl::getHardwareMacAddress(const String& delimiter)
{
#ifdef BASECAMP_NETWORK_ETHERNET
	return ETH.macAddress() ;
#else
	uint8_t rawMac[6];
	esp_efuse_mac_get_default(rawMac);
	return format6Bytes(rawMac, delimiter);
#endif
}

String NetworkControl::getSoftwareMacAddress(const String& delimiter)
{
#ifdef BASECAMP_NETWORK_ETHERNET
	return ETH.macAddress() ;
#else
	uint8_t rawMac[6];
	WiFi.macAddress(rawMac);
	return format6Bytes(rawMac, delimiter);
#endif
	
}

String NetworkControl::getBaseMacAddress(const String& delimiter)
{
	uint8_t rawMac[6];
	esp_read_mac(rawMac, (esp_mac_type_t)0);
	return format6Bytes(rawMac, delimiter);
}

unsigned NetworkControl::getMinimumSecretLength() const
{
	return minApSecretLength;
}

String NetworkControl::generateRandomSecret(unsigned length) const
{
	// There is no "O" (Oh) to reduce confusion
	const String validChars{"abcdefghjkmnopqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ23456789.-,:$/"};
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
