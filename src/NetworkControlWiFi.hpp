# pragma once

#include "NetworkControlBase.hpp"
#include "Configuration.hpp"
#include <WiFi.h>


class NetworkControlWiFi : public NetworkControlBase {

    public:
        virtual void Begin(Configuration& configuration, String hostname, bool encryptAp, String fixedApPassword);
        virtual String GetMacAddress(const String& delimiter = {});
        virtual IPAddress GetSoftApIp();

    private:
        static void wiFiEvent_(WiFiEvent_t event);
        String generateRandomSecret_(unsigned length) const;
};
