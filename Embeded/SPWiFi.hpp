#include "IPAddress.h"
#ifndef INC_SPWIFI_HPP
#define INC_SPWIFI_HPP

#include <WiFi.h>

class SPWiFi {
public:
    SPWiFi() noexcept {};

    void begin(
        const char *ssid,
        const char *password,
        const char *apSsid,
        const char *apPassword,
        const char *hostname,
        void (*)(IPAddress&) noexcept,
        void (*)(void) noexcept
    ) noexcept;

    inline void __attribute__((__always_inline__)) begin(
        String ssid,
        String password,
        String apSsid,
        String apPassword,
        String hostname,
        void (*connected)(IPAddress&) noexcept,
        void (*disconnected)() noexcept
    ) noexcept {
        begin(ssid.c_str(), password.c_str(), apSsid.c_str(), apPassword.c_str(), hostname.c_str(), connected, disconnected);
    }

    inline int8_t __attribute__((__always_inline__)) getRSSI(void) const noexcept {
        return WiFi.RSSI();
    }

    inline IPAddress __attribute__((__always_inline__)) getIP(void) const noexcept {
        return WiFi.localIP();
    }
};

#endif