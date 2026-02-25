#include "NetworkEvents.h"
#include "HardwareSerial.h"
#include "IPAddress.h"
#include <SPWiFi.hpp>
#include <logger/logger.h>

static const char WIFI_LOG[] PROGMEM = "WIFI";

static IPAddress apIPAddress(192, 168, 4, 1);
static IPAddress apSubnet(255, 255, 255, 0);

void SPWiFi::begin(
    const char *ssid,
    const char *password,
    const char *apSsid,
    const char *apPassword,
    const char *hostname,
    void (*connected)(IPAddress&) noexcept,
    void (*disconnected)() noexcept
) noexcept {
    WiFi.onEvent([connected](arduino_event_id_t event, arduino_event_info_t info) {
        IPAddress ipAddress = WiFi.localIP();
        SP_LOGI(WIFI_LOG, "Started in station mode. SSID=%s, IP=%s", WiFi.SSID().c_str(), ipAddress.toString().c_str());

        connected(ipAddress);
    }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

    WiFi.onEvent([disconnected](arduino_event_id_t event, arduino_event_info_t info) {
        disconnected();
    }, ARDUINO_EVENT_WIFI_STA_LOST_IP);

    WiFi.onEvent([connected](arduino_event_id_t event, arduino_event_info_t info) {
        IPAddress ipAddress = WiFi.softAPIP();
        SP_LOGI(WIFI_LOG, "Started in AP mode. SSID: '%s', IP: %s", WiFi.softAPSSID().c_str(), ipAddress.toString().c_str());

        connected(ipAddress);
    }, ARDUINO_EVENT_WIFI_AP_START);

    if (*ssid != '\0') {
        WiFi.setAutoReconnect(true);

        if (*hostname != '\0')
            WiFi.setHostname(hostname);

        WiFi.begin(ssid, password);

        const long end = millis() + 15000;

        do {
            if (WiFi.waitForConnectResult() == WL_CONNECTED)
                return;

            SP_LOGW(WIFI_LOG, "Retry connect...");
            delay(1000);
        }  while (millis() <= end);
    }    

    WiFi.setAutoReconnect(false);

    if (WiFi.softAPConfig(apIPAddress, apIPAddress, apSubnet)) {
        if (*hostname != '\0')
            WiFi.softAPsetHostname(hostname);

        if (WiFi.softAP(apSsid, apPassword))
            return;
            
        SP_LOGE(WIFI_LOG, "Failed to enter AP-mode");
    } else
        SP_LOGE(WIFI_LOG, "Can't configure AP.");

    delay(10000);

    ESP.restart();
}

