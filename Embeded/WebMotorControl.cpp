#include "WebMotorControl.hpp"

#include <SPIFFS.h>

#include "logger.hpp"

static const char WEB_MOTOR_CONTROL_LOG[] = "MOTOR_C";

static const char motor_json_template[] PROGMEM =
  "{"\
   "\"motor\": {\"minPower\": %motor.minPower%, \"plusPin\": { \"value\": %motor.plusPin.value%, \"pins\": [%motor.plusPin.pins%] }, "\
      "\"minusPin\": { \"value\": %motor.minusPin.value%, \"pins\": [%motor.minusPin.pins%] } }, "\
  "\"pid\": { \"Kp\": %pid.Kp%, \"Kd\": %pid.Kd%, \"Ki\": %pid.Ki%, \"maxError\": %pid.maxError%, \"delay\": %pid.delay% }, "\
  "\"speedometer\": { \"maxRps\": %speedometer.maxRps%, \"mmPerTurn\": %speedometer.mmPerTurn%, \"pulsePerTurn\": %speedometer.pulsePerTurn%, "\
      "\"sensorPin\": { \"value\": %speedometer.sensorPin.value%, \"pins\": [%speedometer.sensorPin.pins%] }, \"sensorBackwardMs\": %speedometer.sensorBackwardMs% } "\
  "}";
        
static const size_t motor_json_template_length = strlen(motor_json_template);

static String motorPins(F("32, 33, 34, 35"));
static String sensorPins(F("25, 26, 32, 33, 34, 35"));

WebMotorControl::WebMotorControl(AsyncWebServer& server, Configuration& conf, Speedmeter& speedmeter, Pid<double, double>& pid) noexcept : 
                                  webServer(server), config(conf), speedmeter(speedmeter), pid(pid) {
  webServer.on("/motor-config.html", WebRequestMethod::HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/motor-config.html");
  });

  webServer.on("/motor/configuration", WebRequestMethod::HTTP_GET, [this] (AsyncWebServerRequest *request) {
    request->send_P(200, applicationJson, (const uint8_t*)motor_json_template, motor_json_template_length, [this] (const String& v) -> String {
      // motor
      if (v.equals(F("motor.minPower")))
        return String(this->config.getMotorConfig().minPower);
      if (v.equals(F("motor.plusPin.value")))
        return String(this->config.getMotorConfig().plusPin);
      if (v.equals(F("motor.plusPin.pins")))
        return motorPins;
      if (v.equals(F("motor.minusPin.value")))
        return String(this->config.getMotorConfig().minusPin);
      if (v.equals(F("motor.minusPin.pins")))
        return motorPins;

      // pid
      if (v.equals(F("pid.Kp")))
        return String(this->config.getMotorPidConfig().Kp, 3);
      if (v.equals(F("pid.Kd")))
        return String(this->config.getMotorPidConfig().Kd, 3);
      if (v.equals(F("pid.Ki")))
        return String(this->config.getMotorPidConfig().Ki, 3);
      if (v.equals(F("pid.maxError")))
        return String(this->config.getMotorPidConfig().approvedErr);
      if (v.equals(F("pid.delay")))
        return String(this->config.getMotorPidConfig().delayMs);

      // speedometer
      if (v.equals(F("speedometer.maxRps")))
        return String(this->config.getSpeedmeterConfig().maxRps);
      if (v.equals(F("speedometer.mmPerTurn")))
        return String(this->config.getSpeedmeterConfig().mmPerTurn);
      if (v.equals(F("speedometer.pulsePerTurn")))
        return String(this->config.getSpeedmeterConfig().pulsePerTurn);
      if (v.equals(F("speedometer.sensorPin.value")))
        return String(this->config.getSpeedmeterConfig().sensorPin);
      if (v.equals(F("speedometer.sensorPin.pins")))
        return sensorPins;
      if (v.equals(F("speedometer.sensorBackwardMs")))
        return String(this->config.getSpeedmeterConfig().sensorBackwardMs);

      return EMPTY_STR;
    });
  });

  webServer.on("/motor/configuration", WebRequestMethod::HTTP_POST, [this] (AsyncWebServerRequest *req) {
      MOTOR_CONFIGURATION motorConfig;
      MOTOR_PID_CONFIGURATION motorPidConfig;
      SPEEDMETER_CONFIGURATION speedConfig;

      // for (size_t i = 0; i < req->args(); i++)
      //   SP_LOGI(WEB_MOTOR_CONTROL_LOG, "Arg: %s = %s", req->argName(i).c_str(), req->arg(i).c_str());
      
      bool updated = false;

      memcpy(&motorConfig, &this->config.getMotorConfig(), sizeof(MOTOR_CONFIGURATION));
      memcpy(&motorPidConfig, &this->config.getMotorPidConfig(), sizeof(MOTOR_PID_CONFIGURATION));
      memcpy(&speedConfig, &this->config.getSpeedmeterConfig(), sizeof(SPEEDMETER_CONFIGURATION));

      // motor
      if (req->hasArg("motor.minPower")) {
        motorConfig.minPower = (uint16_t) (req->arg("motor.minPower").toInt());
        updated = true;
      }
      if (req->hasArg("motor.plusPin")) {
        motorConfig.plusPin = (int8_t) (req->arg("motor.plusPin").toInt());
        updated = true;
      }
      if (req->hasArg("motor.minusPin")) {
        motorConfig.minusPin = (int8_t) (req->arg("motor.minusPin").toInt());
        updated = true;
      }

      if (updated) {
        // SP_LOGI(WEB_MOTOR_CONTROL_LOG, "Update motor config");
        this->config.setMotorConfig(motorConfig);
        updated = false;
      }

      // pid
      if (req->hasArg("pid.Kp")) {
        motorPidConfig.Kp = req->arg("pid.Kp").toDouble();
        updated = true;
      }
      if (req->hasArg("pid.Kd")) {
        motorPidConfig.Kd = req->arg("pid.Kd").toDouble();
        updated = true;
      }
      if (req->hasArg("pid.Ki")) {
        motorPidConfig.Ki = req->arg("pid.Ki").toDouble();
        updated = true;
      }
      if (req->hasArg("pid.maxError")) {
        motorPidConfig.approvedErr = req->arg("pid.maxError").toDouble();
        updated = true;
      }
      if (req->hasArg("pid.delay")) {
        motorPidConfig.delayMs = (uint32_t) req->arg("pid.delay").toInt();
        updated = true;
      }

      if (updated) {
        // SP_LOGI(WEB_MOTOR_CONTROL_LOG, "Update pid config");
        this->config.setMotorPidConfig(motorPidConfig);
        updated = false;
      }

      // speedometer
      if (req->hasArg("speedometer.maxRps")) {
        speedConfig.maxRps = (int16_t) req->arg("speedometer.maxRps").toInt();
        updated = true;
      }
      if (req->hasArg("speedometer.mmPerTurn")) {
        speedConfig.mmPerTurn = req->arg("speedometer.mmPerTurn").toDouble();
        updated = true;
      }
      if (req->hasArg("speedometer.pulsePerTurn")) {
        speedConfig.pulsePerTurn = (uint32_t) req->arg("speedometer.pulsePerTurn").toInt();
        updated = true;
      }
      if (req->hasArg("speedometer.sensorPin")) {
        speedConfig.sensorPin = (uint8_t) req->arg("speedometer.sensorPin").toInt();
        updated = true;
      }
      if (req->hasArg("speedometer.sensorBackwardMs")) {
        speedConfig.sensorBackwardMs = (uint32_t) req->arg("speedometer.sensorBackwardMs").toInt();
        updated = true;
      }

      if (updated) {
        // SP_LOGI(WEB_MOTOR_CONTROL_LOG, "Update speedometer config");
        this->config.setSpeedmeterConfig(speedConfig);
      }

      AsyncWebServerResponse *response = req->beginResponse(302, textPlain, F("OK"));
      
      response->addHeader(F("Location"), F("/motor-config.html"));
      response->addHeader(F("Connection"), F("Keep-Alive"));
      response->addHeader(F("Access-Control-Allow-Origin"), F("*"));
      req->send(response);
  });

  webServer.on("/motor/speed", WebRequestMethod::HTTP_POST, [this] (AsyncWebServerRequest *req) {
    AsyncWebServerResponse *response;
    if (req->hasArg("value")) {
      this->pid.setValue(req->arg("value").toDouble());
       SP_LOGI(WEB_MOTOR_CONTROL_LOG, "Speed value=%f", req->arg("value").toDouble());

      response = req->beginResponse(200);
    }
    else
      response = req->beginResponse(400);

    //response->addHeader(F("Connection"), F("Keep-Alive"));
    req->send(response);
  });
}
