#include <EEPROM.h>
#include <string.h>
#include <SPIFFS.h>
#include "configuration.hpp"
#include "logger.hpp"

#define START_EEPROM_ADDRESS 0

#define SIGNATURE 0x1E0BC75C

static const char TAG[] PROGMEM = "CONFIG";

static const char defSsid[] PROGMEM = "SPDC";
static const char defPassword[] PROGMEM = "743855341921";

static const char defApSsid[] PROGMEM = "ESP_32";
static const char defApPassword[] PROGMEM = "spiderru";

static const char wifi_json_template[] PROGMEM = 
  "{ \"hostname\": \"%hostname%\", \"ap\": { \"ssid\": \"%ap.ssid%\", \"password\": \"%ap.password%\" }, \"selfap\": { \"ssid\": \"%selfap.ssid%\", \"password\": \"%selfap.password%\" } }";

static const size_t wifi_json_template_length = strlen(wifi_json_template);

static inline void __attribute__((__always_inline__)) copy(char *dst, const String& src, size_t maxLen) noexcept {
  strncpy(dst, src.c_str(), maxLen);
  dst[maxLen - 1] = '\0';
}

#define copyToArray(dst, src) copy(dst, src, sizeof(dst))

static void readConfig(CONFIGURATION_DATA *config) noexcept {
  EEPROM.readBytes(START_EEPROM_ADDRESS, config, sizeof(CONFIGURATION_DATA));

  if (config->signature != SIGNATURE) {
    memset(config, 0, sizeof(CONFIGURATION_DATA));

    uint32_t macAddress[2];
    if (esp_efuse_mac_get_default((uint8_t*)macAddress) != ESP_OK)
      macAddress[0] = macAddress[1] = 0xFFFFFFFF;

    sprintf(config->hostname, "TRAIN-%08x", macAddress[0] ^ macAddress[1]);

    strcpy(config->wifi_configuration.connection.ssid, defSsid);
    strcpy(config->wifi_configuration.connection.password, defPassword);

    strcpy(config->wifi_configuration.ap_connection.ssid, defApSsid);
    strcpy(config->wifi_configuration.ap_connection.password, defApPassword);

    config->motor_config.minPower = 542;
    config->motor_config.plusPin = 32;
    config->motor_config.minusPin = 33;

    config->motor_pid_config.Kd = 0.01;
    config->motor_pid_config.Kp = 0.05;
    config->motor_pid_config.Ki = 0.01;
    config->motor_pid_config.approvedErr = 0.5;
    config->motor_pid_config.delayMs = 100;

    config->speedmeter_config.maxRps = 10;
    config->speedmeter_config.mmPerTurn = 0;
    config->speedmeter_config.pulsePerTurn = 2;
    config->speedmeter_config.sensorBackwardMs = 200;
    config->speedmeter_config.sensorPin = 26;
  }
}

void Configuration::saveConfig(CONFIGURATION_DATA *config) noexcept {
  config->signature = SIGNATURE;

  size_t written = EEPROM.writeBytes(START_EEPROM_ADDRESS, config, sizeof(CONFIGURATION_DATA));

  if (written == sizeof(CONFIGURATION_DATA))
    EEPROM.commit();
  else
    SP_LOGE(TAG, "EEPROM write error. Expected=%d, Result=%d", (int)sizeof(CONFIGURATION_DATA), (int)written);
}

Configuration::Configuration(AsyncWebServer& server) noexcept :  webServer(server) {
  webServer.on("/wifi-config.html", WebRequestMethod::HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/wifi-config.html");
  });

  webServer.on("/wifi/configuration", WebRequestMethod::HTTP_GET, [this] (AsyncWebServerRequest *req) {
    AsyncWebServerRequest& request = *req;
    CONFIGURATION_DATA& config = this->configData;

    request.send_P(200, applicationJson, (const uint8_t*)wifi_json_template, wifi_json_template_length, [config] (const String& v) -> String {
      if (v.equals(F("hostname")))
        return config.hostname;
      if (v.equals(F("ap.ssid")))
        return config.wifi_configuration.connection.ssid;
      if (v.equals(F("ap.password")))
        return config.wifi_configuration.connection.password;
      if (v.equals(F("selfap.ssid")))
        return config.wifi_configuration.ap_connection.ssid;
      if (v.equals(F("selfap.password")))
        return config.wifi_configuration.ap_connection.password;

      return EMPTY_STR;  
    });
  });

  webServer.on("/wifi/configuration", WebRequestMethod::HTTP_POST, [this] (AsyncWebServerRequest *req) {
      ESP_LOGI(TAG, "Request update");

      CONFIGURATION_DATA& config = this->configData;

      if (req->hasArg("hostname")) {
        String hostname = req->arg("hostname");
        hostname.trim();

        if (hostname.length() > 0)
          copyToArray(config.hostname, hostname);
      }

      if (req->hasArg("ap.ssid") && req->hasArg("ap.password")) {
        String ssid = req->arg("ap.ssid");
        String password = req->arg("ap.password");
        ssid.trim();
        password.trim();

        if (ssid.length() > 0 && password.length() > 0) {
          copyToArray(config.wifi_configuration.connection.ssid, ssid);
          copyToArray(config.wifi_configuration.connection.password, password);
        }
      }

      if (req->hasArg("selfap.ssid") && req->hasArg("selfap.password")) {
        String ssid = req->arg("selfap.ssid");
        String password = req->arg("selfap.password");
        ssid.trim();
        password.trim();

        if (ssid.length() > 0 && password.length() > 0) {
          copyToArray(config.wifi_configuration.ap_connection.ssid, ssid);
          copyToArray(config.wifi_configuration.ap_connection.password, password);
        }
      }

      saveConfig(&config);

      AsyncWebServerResponse *response = req->beginResponse(302, textPlain, F("OK"));
      response->addHeader(F("Location"), F("/wifi-config.html"));
      response->addHeader(F("Connection"), F("close"));
      response->addHeader(F("Access-Control-Allow-Origin"), F("*"));
      req->send(response);

      SP_LOGE(TAG, "Reboot...");

      delay(2000);

      ESP.restart();
    });
}

void Configuration::begin(void) noexcept {
  EEPROM.begin(sizeof(CONFIGURATION_DATA));

  readConfig(&configData);
}
