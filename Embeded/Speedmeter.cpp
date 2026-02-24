#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include <Arduino.h>
#include <SPIFFS.h>

#include "Speedmeter.hpp"
#include "logger.hpp"

static const char SPEEDMETER_LOG[] = "SPEEDMETER";

static const char speed_json_template[] PROGMEM = "{ \"minSpeed\": %minSpeed%, \"maxSpeed\": %maxSpeed%, \"speed\": %speed%, \"maxRps\": %maxRps%, \"rps\": %rps%, "\
                                                    "\"power\": %power% }";
static const size_t speed_json_template_length = strlen(speed_json_template);

Speedmeter::Speedmeter(Motor& motor, Udp& udp, Configuration& conf, AsyncWebServer& server) noexcept : motor(motor), udp(udp), config(conf), server(server), currentSpeed(0) {
  speedQueue = xQueueCreate(1, sizeof(int32_t));
  xTaskCreate(speedTask, "SPEEDSEND", 2048, this, tskIDLE_PRIORITY + 1, &speedTaskHandle);

  server.on("/speedometer.html", WebRequestMethod::HTTP_GET, [] (AsyncWebServerRequest *req) { req->send(SPIFFS, "/speedometer.html"); });
  server.on("/speedometer/assets/icons.svg", WebRequestMethod::HTTP_GET, [] (AsyncWebServerRequest *req) { req->send(SPIFFS, "/speedometer/assets/icons.svg"); });
  server.on("/speedometer/css/styles.css", WebRequestMethod::HTTP_GET, [] (AsyncWebServerRequest *req) { req->send(SPIFFS, "/speedometer/css/styles.css"); });
  server.on("/speedometer/js/speedometer.js", WebRequestMethod::HTTP_GET, [] (AsyncWebServerRequest *req) { req->send(SPIFFS, "/speedometer/js/speedometer.js"); });
  server.on("/speedometer/js/fraction.min.js", WebRequestMethod::HTTP_GET, [] (AsyncWebServerRequest *req) { req->send(SPIFFS, "/speedometer/js/fraction.min.js"); });

  server.on("/speedometer/speed", WebRequestMethod::HTTP_GET, [this] (AsyncWebServerRequest *req) {
    AsyncWebServerResponse *response = req->beginResponse_P(200, applicationJson, (uint8_t*)speed_json_template, speed_json_template_length, 
        [this] (const String& v) -> String {
          if (v.equals(F("minSpeed")))
            return String(this->getMinSpeed());
          if (v.equals(F("maxSpeed")))
            return String(this->getMaxSpeed());
          if (v.equals(F("speed")))
            return String(this->getSpeed());

          if (v.equals(F("maxRps")))
            return String(this->config.getSpeedmeterConfig().maxRps);
          if (v.equals(F("rps")))
            return String(this->rpsAsDouble());

          if (v.equals(F("power")))
            return String(abs(this->motor.getCurrentValue() / std::max(abs(this->motor.getMaxValue()), std::abs(this->motor.getMinValue()))));

          return EMPTY_STR;
        });

    //response->addHeader(F("Connection"), F("Keep-Alive"));
    req->send(response);
  });

}

void IRAM_ATTR Speedmeter::sensorInterrupt(void *arg) {
  const uint32_t currentTime = (uint32_t) (esp_timer_get_time() >> 4);

  Speedmeter& self = *(Speedmeter*)arg;

  const auto index = self.interruptCounter;
  self.interruptTimes[index & (SPEED_NUM_PROBES - 1)] = currentTime;
  self.interruptCounter = index + 1;
}

void Speedmeter::timerFunc(void *data) noexcept {
  static uint32_t s;
  
  Speedmeter& self = *(Speedmeter*)data;

  const double rps = self.rpsAsDouble();
  const double speed = sign(self.motor.getCurrentValue()) * rps * self.config.getSpeedmeterConfig().mmPerTurn;

  if (abs(self.currentSpeed - speed) > 0.001) {
    self.currentSpeed = speed;

    xQueueOverwrite(self.speedQueue, &s);

    //SP_LOGI(SPEEDMETER_LOG, "Current speed: %f : %f", speed, rps);
  }
}

void Speedmeter::speedTask(void *args) noexcept {
  Speedmeter& self = *(Speedmeter*)args;
  int32_t speed;

  while(true) {
    xQueueReceive(self.speedQueue, &speed, 2000 / portTICK_PERIOD_MS);

    self.udp.writeTo(sizeof(SPEED_MESSAGE), [self] (void *data, size_t maxLen) {
      SPEED_MESSAGE& msg = *(SPEED_MESSAGE*)data;

      msg.code = UDP_MESSAGE_SPEED;
      msg.hostnameLen = sizeof(SPEED_MESSAGE::hostname);
      strcpy(msg.hostname, self.config.getHostname());
      msg.stamp = currentTimeMillis();

      msg.minPower = self.motor.getMinValue();
      msg.maxPower = self.motor.getMaxValue();
      msg.power = self.motor.getCurrentValue();

      msg.maxRps = self.config.getSpeedmeterConfig().maxRps;
      msg.rps = (uint16_t)self.rps();

      msg.minSpeed = (int16_t)self.getMinSpeed();
      msg.maxSpeed = (int16_t)self.getMaxSpeed();
      msg.speed = (int16_t)self.getSpeed();
    });
  }
}

void Speedmeter::begin(void) noexcept {
  esp_timer_create_args_t timer_args;
  timer_args.callback = &timerFunc;
  timer_args.name = "speedmeter";
  timer_args.arg = this;
  timer_args.dispatch_method = ESP_TIMER_TASK;

  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timerHandle));

  ESP_ERROR_CHECK(esp_timer_start_periodic(timerHandle, getBackMs() * 1000));

  memset(interruptTimesArray, 0, sizeof(interruptTimesArray));
  interruptCounter = 0;

  const uint8_t sensorPin = config.getSpeedmeterConfig().sensorPin;
  pinMode(sensorPin, INPUT_PULLUP);
  attachInterruptArg(sensorPin, sensorInterrupt, this, CHANGE);
}
