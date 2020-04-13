# pragma once

#include "NetworkControlBase.hpp"
#include "Configuration.hpp"
#include <WiFi.h>


class NetworkControlWiFi : public NetworkControlBase {

    public:
        virtual void Begin(Configuration& configuration, bool encryptAp, String fixedApPassword, String hostname = "Esp32IotDevice");
        virtual String GetMacAddress(const String& delimiter = {});

    private:
        static void wiFiEvent_(WiFiEvent_t event);
        String generateRandomSecret_(unsigned length) const;
};
