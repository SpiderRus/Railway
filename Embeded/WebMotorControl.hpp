#ifndef INC_WEB_MOTOR_CONTROL_HPP
#define INC_WEB_MOTOR_CONTROL_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "Motor.hpp"
#include "configuration.hpp"
#include "speedmeter.hpp"
#include "pid.hpp"

class WebMotorControl {
  public:
    WebMotorControl(AsyncWebServer& server, Configuration& config, Speedmeter& speedmater, Pid<double, double>& pid) noexcept;

  private:
    AsyncWebServer& webServer;
    Configuration& config;
    Speedmeter& speedmeter;
    Pid<double, double>& pid;
};

#endif


