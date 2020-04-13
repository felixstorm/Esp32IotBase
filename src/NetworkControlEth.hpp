# pragma once

#include "NetworkControlBase.hpp"
#include <ETH.h>


class NetworkControlEth : public NetworkControlBase {

    public:
        virtual void Begin(Configuration& configuration, bool encryptAp, String fixedApPassword, String hostname = "Esp32IotDevice");
        virtual String GetMacAddress(const String& delimiter = {});

    private:
        static void wiFiEvent_(WiFiEvent_t event);
};
