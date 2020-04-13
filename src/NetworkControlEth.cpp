#include "NetworkControlEth.hpp"


namespace {
    const constexpr char* kLoggingTag = "IotBaseNetwork";
}

void NetworkControlEth::Begin(Configuration& configuration, bool encryptAp, String fixedApPassword, String hostname)
{
    configureNetworkConnectionWdt_();

    ESP_LOGI(kLoggingTag, "Connecting to Ethernet");
    operationMode_ = Mode::client;

    WiFi.onEvent(wiFiEvent_);

    ETH.begin() ;
    ETH.setHostname(hostname.c_str());
    ESP_LOGD(kLoggingTag, "Ethernet initialized") ;

    // make sure we are fully connected when we return from Begin()
    waitForConnection_();
}

String NetworkControlEth::GetMacAddress(const String &delimiter)
{
    uint8_t rawMac[6];
    ETH.macAddress(rawMac);
    return format6Bytes_(rawMac, delimiter);
}

void NetworkControlEth::wiFiEvent_(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_ETH_CONNECTED:
        ESP_LOGI(kLoggingTag, "ETH Connected");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        localIp_ = ETH.localIP();
        ESP_LOGI(kLoggingTag, "ETH Got IPv4 %s (%d Mbps, full duplex: %d, MAC %s)",
                 localIp_.toString().c_str(), ETH.linkSpeed(), ETH.fullDuplex(), ETH.macAddress().c_str());
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        localIp_ = (uint32_t)0;
        ESP_LOGI(kLoggingTag, "ETH Disconnected");
        break;
    default:
        break;
    }
}
