# pragma once

#include "NetworkControlBase.hpp"
#include <ETH.h>


class NetworkControlEth : public NetworkControlBase {

    public:
        virtual void Begin(Configuration& configuration, String hostname, bool encryptAp, String fixedApPassword);
        virtual String GetMacAddress(const String& delimiter = {});

    private:
        static void wiFiEvent_(WiFiEvent_t event);
};
