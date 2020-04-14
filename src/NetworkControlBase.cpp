/*
   Esp32IotBase - ESP32 library to simplify the basics of IoT projects
   by Felix Storm (http://github.com/felixstorm)
   Heavily based on Basecamp (https://github.com/ct-Open-Source/Basecamp) by Merlin Schumacher (mls@ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "NetworkControlBase.hpp"

namespace {
    const constexpr char* kLoggingTag = "IotBaseNetwork";
}

bool NetworkControlBase::IsConnected()
{
    return localIp_ != 0;
}

IPAddress NetworkControlBase::GetIp() {
    return localIp_;
}

IPAddress NetworkControlBase::GetSoftApIp() {
    return {};
}

NetworkControlBase::Mode NetworkControlBase::GetWiFiOperationMode() const
{
    return operationMode_;
}

void NetworkControlBase::ResetNetworkConnectedWatchdog()
{
    if (networkConnectedWdtHandle_)
        xTimerReset(networkConnectedWdtHandle_, (TickType_t)0);
}

IPAddress NetworkControlBase::localIp_;

void NetworkControlBase::configureNetworkConnectionWdt_()
{
    // prepare Network Connected WDT
    networkConnectedWdtHandle_ = xTimerCreate("NetConnWdt", pdMS_TO_TICKS(60 * 60 * 1000), pdFALSE, (void *)0, networkConnectedWdtElapsed_);
    xTimerStart(networkConnectedWdtHandle_, 0);
}

void NetworkControlBase::waitForConnection_()
{
    // make sure we are fully connected when we return from begin()
    while (!IsConnected()) {
        ESP_LOGI(kLoggingTag, "Waiting for connection...");
        delay(1000);
    }
    delay(1000);
    ESP_LOGI(kLoggingTag, "Successfully connected.");
}

void NetworkControlBase::networkConnectedWdtElapsed_(TimerHandle_t xTimer)
{
    // apparently something is broken with the network, so we reset and hope that this will solve it...
    ESP.restart();
}
