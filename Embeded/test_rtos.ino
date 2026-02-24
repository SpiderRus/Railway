#include <AsyncElegantOTA.h>
#include <Hash.h>
#include <elegantWebpage.h>

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <AsyncTCP.h>

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <soc/rtc_wdt.h>

#include "logger.hpp"
#include "configuration.hpp"
#include "Motor.hpp"
#include "WebMotorControl.hpp"
#include "Speedmeter.hpp"
#include "pid.hpp"
#include "Udp.hpp"
#include "Mqtt.hpp"
#include "Utils.hpp"

#define ONBOARD_LED 2

static const char esp_all_html[] PROGMEM =
  "<!DOCTYPE HTML>\n"
  "<html>\n"
  " <head>\n"
  "  <meta charset=\"utf-8\">\n"
  "  <title>ESP32</title>\n"
  " </head>\n"
  "\n"
  " <frameset rows=\"300px,380px,420px\">\n"
  "    <frame src=\"/motor\">\n"
  "    <frame src=\"/configuration\">\n"
  "    <frame src=\"/update\">\n"
  " </frameset>\n"
  " \n"
  "</html>\n";

static const size_t esp_all_html_length = strlen(esp_all_html);

static const char WIFI_LOG[] PROGMEM = "WIFI";

static const char MAIN_LOG[] PROGMEM = "MAIN";

static IPAddress apIPAddress(192, 168, 4, 1);
static IPAddress apSubnet(255, 255, 255, 0);

static Udp groupUdp;
static Udp mainUdp;

static AsyncWebServer webServer(80);
static Configuration config(webServer);
static Motor motor(config);
static Speedmeter speedmeter(motor, mainUdp, config, webServer);

static Mqtt mqtt(config);
static const char* const SPEED_SET_TOPIC = "/train_speed/set/";
static const size_t SPEED_SET_TOPIC_LEN = s_len(SPEED_SET_TOPIC);

static Logger logger(SP_LOG_CONSOLE | SP_LOG_UDP, mainUdp, config);

static Pid<double, double> pid(
  [](void*) -> double { return speedmeter.getSpeed(); }, NULL,

  [](double value, void*) { motor.setValue(motor.getCurrentValue() + value); }, NULL,

  [](void*) { return motor.getMinValue(); }, NULL,
  [](void*) { return motor.getMaxValue();  }, NULL,
  [](void*) { return speedmeter.getMinSpeed(); }, NULL,
  [](void*) { return speedmeter.getMaxSpeed(); }, NULL,

  config
);

static WebMotorControl motorControl(webServer, config, speedmeter, pid);

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    SP_LOGI(MAIN_LOG, "Client: %s %s", request->client()->remoteIP().toString().c_str(), request->url().c_str());

    String fullName = (request->hasArg("dir") ? request->arg("dir") + "/" : EMPTY_STR) + filename;

    if (fullName.charAt(0) != '/')
      fullName = String("/") + fullName;

    if (SPIFFS.exists(fullName))
      SPIFFS.remove(fullName);
      

    // open the file on first call and store the file handle in the request object
    request->_tempFile = SPIFFS.open(fullName, "w", true);

    SP_LOGI(MAIN_LOG, "Upload file: %s", filename.c_str());
  }

  if (len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    //SP_LOGI(MAIN_LOG, "Writing file: %s, index=%d, len=%d", filename.c_str(), (int)index, (int)len);
  }

  if (final) {
    // close the file handle as the upload is now done
    request->_tempFile.close();
    //request->redirect("/");
    //SP_LOGI(MAIN_LOG, "Upload complete: %s, size=%d", filename.c_str(), (int)(index + len));
  }
}

static void setSpeed(int32_t speed) {
  if (speed < speedmeter.getMinSpeed()) {
    SP_LOGW(MAIN_LOG, "Min speed: Expected=%d, but=%d", speedmeter.getMinSpeed(), speed);
    speed = speedmeter.getMinSpeed();
  }
  else if (speed > speedmeter.getMaxSpeed()) {
    SP_LOGW(MAIN_LOG, "Max speed: Expected=%d, but=%d", speedmeter.getMaxSpeed(), speed);
    speed = speedmeter.getMaxSpeed();
  }

  pid.setValue(speed);

  SP_LOGI(MAIN_LOG, "Set speed: %d", speed);
}

void setup() {
  // rtc_wdt_protect_off();    // Turns off the automatic wdt service
  // rtc_wdt_enable();         // Turn it on manually
  // rtc_wdt_set_time(RTC_WDT_STAGE0, 3600 * 1000);  // Define how long you desire to let dog wait.

  config.begin();

  webServer.on("/favicon.ico", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(404);
  });

  webServer.on("/bootstrap/bootstrap.min.css", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/bootstrap/bootstrap.min.css");
  });

  webServer.on("/bootstrap/bootstrap.min.js", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/bootstrap/bootstrap.min.js");
  });

  webServer.on("/", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });

  webServer.on("/index.html", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });

  webServer.on("/fs/upload", WebRequestMethod::HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
  }, handleUpload);

  webServer.on("/fs/clear", WebRequestMethod::HTTP_DELETE, [](AsyncWebServerRequest *request) {
    SP_LOGI(MAIN_LOG, "Clear fs");

    File root = SPIFFS.open("/");

    for (File file = root.openNextFile(); file; file = file.openNextFile()) {
      String name = file.path();

      file.close();

      SP_LOGI(MAIN_LOG, "Remove: name=%s, result=%s", name.c_str(), SPIFFS.remove(name) ? "true" : "false");
    }

    root.close();

    request->send(200);
  });

  webServer.on("/fs/list", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    String result;

    File root = SPIFFS.open("/");

    for (File file = root.openNextFile(); file;) {
      result.concat(file.path());
      result.concat("\t");
      result.concat(file.size());
      result.concat("\n");

      file.close();
      file = root.openNextFile();
    }

    root.close();

    request->send(200, textPlain, result);
  });

  groupUdp.addHandler([] (const UDP_MESSAGE& message, size_t length) {
    if (message.code == UDP_ROOT_CODE) {
      UDP_HANDSHAKE_MESSAGE& handshake = (UDP_HANDSHAKE_MESSAGE&)message;

      char hostname[64];
      size_t hostnameLen = length - sizeof(UDP_HANDSHAKE_MESSAGE);
      memcpy(hostname, handshake.hostname, hostnameLen);
      hostname[hostnameLen] = '\0';
      IPAddress host(handshake.ip);

      //SP_LOGI(MAIN_LOG, "Received root message: code=%d, ip=%s, hostname=%s, len=%d", handshake.code, host.toString().c_str(), hostname, length);

      mainUdp.begin(host, 1234);
      mqtt.begin(host);
    }
    else if (message.code == UDP_TIME_SYNC) {
      const auto requested = ((TIME_SYNC_MESSAGE&)message).stamp;

      mainUdp.writeTo(sizeof(TIME_SYNC_MESSAGE), [requested] (void *data, size_t maxLen) {
        TIME_SYNC_MESSAGE& send = *(TIME_SYNC_MESSAGE*)data;

        send.code = UDP_TIME_SYNC;
        send.hostnameLen = sizeof(TIME_SYNC_MESSAGE::hostname);
        strcpy(send.hostname, config.getHostname());
        send.stamp = currentTimeMillis();
        send.requested = requested;
      });
    }
    else if (message.code == UDP_MESSAGE_SPEED) {
      SPEED_MESSAGE& msg = (SPEED_MESSAGE&)message;

      setSpeed(msg.speed);
    }
  });

  if (!SPIFFS.begin(true))
    SP_LOGE(MAIN_LOG, "Error init SPIFFS!!!");

  const WIFI_CONFIGURATION &wifiConfig = config.getWiFiConfig();

  AsyncElegantOTA.begin(&webServer);

  motor.begin();

  speedmeter.begin();

  pid.begin();

  mqtt.addConCallback([] (bool sessionPresent, void *args) {
    char topic[SPEED_SET_TOPIC_LEN + sizeof(CONFIGURATION_DATA::hostname)];
    copy_str(copy_str(topic, SPEED_SET_TOPIC), config.getHostname());
    SP_LOGI(MAIN_LOG, "Subscribe to: %s", topic);
    mqtt.subscribe(topic, 2);
  }, NULL);

  mqtt.addCallback([] (char* topic, void* payload, AsyncMqttClientMessageProperties properties, size_t len, void *args) {
    if (start_with(topic, SPEED_SET_TOPIC) && !strcmp(topic + SPEED_SET_TOPIC_LEN, config.getHostname())) {
      if (len == sizeof(SPEED_MESSAGE)) {
        SPEED_MESSAGE * const msg = (SPEED_MESSAGE*)payload;
        setSpeed(msg->speed);
      }
      else
        SP_LOGE(MAIN_LOG, "Unknown MQTT message length: Expected=%u, but=%u", sizeof(SPEED_MESSAGE), len);
    }
  }, NULL);

  const char *hostname = config.getHostname();
  WiFi.onEvent([hostname](arduino_event_id_t event, arduino_event_info_t info) {
    configTime(3 * 3600, 0, "europe.pool.ntp.org", "pool.ntp.org");

    SP_LOGI(WIFI_LOG, "Started in station mode. SSID=%s, IP=%s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

    webServer.begin();
    groupUdp.begin(IPAddress(224, 0, 0, 0), 1234, WiFi.localIP(), hostname);
  }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

  WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
    SP_LOGI(WIFI_LOG, "Started in AP mode. SSID: '%s', IP: %s", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());

    webServer.begin();
  }, ARDUINO_EVENT_WIFI_AP_START);

  WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
    SP_LOGI(WIFI_LOG, "Lost IP.");

    webServer.end();
    groupUdp.end();
    mainUdp.end();
    mqtt.end();
  }, ARDUINO_EVENT_WIFI_STA_LOST_IP);


  if (wifiConfig.connection.ssid[0] != '\0') {
    if (*hostname != '\0')
      WiFi.setHostname(hostname);

    WiFi.begin(wifiConfig.connection.ssid, wifiConfig.connection.password);
    //WiFi.begin("Sphone1", "spiderru");

    const long end = millis() + 15000;

    do {
      if (WiFi.waitForConnectResult() == WL_CONNECTED)
        return;

      SP_LOGW(WIFI_LOG, "Retry connect...");
      delay(1000);
    } while (millis() <= end);

    SP_LOGW(WIFI_LOG, "Can't connect to AP (SSID='%s', PASSWORD='%s')", wifiConfig.connection.ssid, wifiConfig.connection.password);
  } else
    SP_LOGW(WIFI_LOG, "WiFi station not configured");

  WiFi.setAutoReconnect(false);

  if (!WiFi.softAPConfig(apIPAddress, apIPAddress, apSubnet))
    SP_LOGW(WIFI_LOG, "Can't configure AP.");

  if (*hostname != '\0')
    WiFi.softAPsetHostname(hostname);

  if (!WiFi.softAP(wifiConfig.ap_connection.ssid, wifiConfig.ap_connection.password)) {
    SP_LOGE(WIFI_LOG, "Failed to enter AP-mode");

    delay(10000);

    ESP.restart();
  }
}

void loop() {
}
