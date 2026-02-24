#ifndef INC_CONFIGURATION_HPP
#define INC_CONFIGURATION_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#pragma pack(push, 1)

typedef struct WIFI_CONNECTION_INFO_ {
  char ssid[21];
  char password[21];
} WIFI_CONNECTION_INFO;

typedef struct WIFI_CONFIGURATION_ {
  WIFI_CONNECTION_INFO connection;
  WIFI_CONNECTION_INFO ap_connection;
} WIFI_CONFIGURATION;

typedef struct MOTOR_CONFIGURATION_ {
  uint16_t minPower;
  uint8_t plusPin;
  uint8_t minusPin;
} MOTOR_CONFIGURATION;

typedef struct MOTOR_PID_CONFIGURATION_ {
  double Kp;
  double Ki;
  double Kd;
  double approvedErr;
  uint16_t delayMs;
} MOTOR_PID_CONFIGURATION;

typedef struct SPEEDMETER_CONFIGURATION_ {
  int16_t maxRps;
  double mmPerTurn;
  uint32_t pulsePerTurn;
  uint8_t sensorPin;
  uint32_t sensorBackwardMs;
} SPEEDMETER_CONFIGURATION;

typedef struct CONFIGURATION_DATA_ {
  uint32_t signature;
  WIFI_CONFIGURATION wifi_configuration;
  char hostname[21];
  MOTOR_CONFIGURATION motor_config;
  MOTOR_PID_CONFIGURATION motor_pid_config;
  SPEEDMETER_CONFIGURATION speedmeter_config;
} CONFIGURATION_DATA;

#pragma pack(pop)

class Configuration {
  private:
    static void saveConfig(CONFIGURATION_DATA *config) noexcept;

  public:
    Configuration(AsyncWebServer& webServer) noexcept;

    inline const WIFI_CONFIGURATION& __attribute__((__always_inline__)) getWiFiConfig(void) noexcept { return configData.wifi_configuration; }

    inline const MOTOR_CONFIGURATION& __attribute__((__always_inline__)) getMotorConfig(void) noexcept { return configData.motor_config; };

    inline void __attribute__((__always_inline__)) setMotorConfig(const MOTOR_CONFIGURATION& config) noexcept { 
      memcpy(&configData.motor_config, &config, sizeof(MOTOR_CONFIGURATION));
      saveConfig(&configData);
    }

    inline const MOTOR_PID_CONFIGURATION& __attribute__((__always_inline__)) getMotorPidConfig(void) noexcept { return configData.motor_pid_config; };

    inline void __attribute__((__always_inline__)) setMotorPidConfig(const MOTOR_PID_CONFIGURATION& config) noexcept { 
      memcpy(&configData.motor_pid_config, &config, sizeof(MOTOR_PID_CONFIGURATION));
      saveConfig(&configData);
    }

    inline const __attribute__((__always_inline__)) SPEEDMETER_CONFIGURATION& getSpeedmeterConfig(void) noexcept { return configData.speedmeter_config; };

    inline void __attribute__((__always_inline__)) setSpeedmeterConfig(const SPEEDMETER_CONFIGURATION& config) noexcept { 
      memcpy(&configData.speedmeter_config, &config, sizeof(SPEEDMETER_CONFIGURATION));
      saveConfig(&configData);
    }

    inline const char* __attribute__((__always_inline__)) getHostname(void) noexcept { return configData.hostname; }

    void begin(void) noexcept;    

  private:
    AsyncWebServer& webServer;
    CONFIGURATION_DATA configData;
};

#endif
