/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   by Felix Storm (http://github.com/felixstorm)
   Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#pragma once

#include <Esp32Logging.hpp>
#include "Configuration.hpp"

#include <iomanip>
#include <sstream>


class NetworkControlBase {
    public:
        enum class Mode {
            unconfigured,
            accessPoint,
            client,
        };

        NetworkControlBase(){};
        virtual void Begin(Configuration& configuration, bool encryptAp, String fixedApPassword, String hostname = "Esp32IotDevice") = 0;
        virtual String GetMacAddress(const String& delimiter = {}) = 0;

        virtual bool IsConnected();
        virtual IPAddress GetIP();

        virtual Mode GetWiFiOperationMode() const;

        void ResetNetworkConnectedWatchdog();

    protected:
        Mode operationMode_ = Mode::unconfigured;
        static IPAddress localIp_;

        void configureNetworkConnectionWdt_();
        void waitForConnection_();

        template <typename BYTES>
        String format6Bytes_(const BYTES &bytes, const String& delimiter)
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

    private:
        TimerHandle_t networkConnectedWdtHandle_ = 0;
        static void networkConnectedWdtElapsed_(TimerHandle_t xTimer);
};
