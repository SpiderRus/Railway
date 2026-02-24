#ifndef INC_PID_HPP
#define INC_PID_HPP

#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "logger.hpp"
#include "configuration.hpp"

#define abs_db(v) ((v) < 0 ? (-(v)) : (v))

extern const char PID_LOG[];

template<typename FEEDBACK_TYPE, typename ACTION_TYPE>
class Pid {
  public:
    Pid(std::function<FEEDBACK_TYPE(void*)> feedback, void *feedbackArgs,
              std::function<void(ACTION_TYPE, void*)> action, void *actionArgs,
              std::function<ACTION_TYPE(void*)> minA, void *minAargs,
              std::function<ACTION_TYPE(void*)> maxA, void *maxAargs,
              std::function<ACTION_TYPE(void*)> minF, void *minFargs,
              std::function<ACTION_TYPE(void*)> maxF, void *maxFargs,
              Configuration& conf) 
          : feedback(feedback), 
            feedbackArgs(feedbackArgs),
            action(action),
            actionArgs(actionArgs),

            config(conf),

            minAction(minA),
            minActionArgs(minAargs),
            maxAction(maxA),
            maxActionArgs(maxAargs),

            minFeedback(minF),
            minFeedbackArgs(minFargs),
            maxFeedback(maxF),
            maxFeedbackArgs(maxFargs),

            integral(0),
            prevErr(0),
            target(0),
            mux(portMUX_INITIALIZER_UNLOCKED) {};

  private:
    inline double __attribute__((__always_inline__)) constrainOut(double value) noexcept {
      const ACTION_TYPE min = minAction(minActionArgs);
      const ACTION_TYPE max = maxAction(maxActionArgs);
      return value > max ? max : (value < min ? min : value);
    };

    static void timerTask(void *args) noexcept {
      SP_LOGI(PID_LOG, "PID task started");

      Pid<FEEDBACK_TYPE, ACTION_TYPE>& self = *(Pid<FEEDBACK_TYPE, ACTION_TYPE>*)args;
      double value, dtsec, i, d;
      FEEDBACK_TYPE err;
      uint32_t cnt = 0;

      do {
        const uint64_t currentTime = esp_timer_get_time();
        bool needAction = false;
        const FEEDBACK_TYPE feed = self.feedback(self.feedbackArgs);
        const double kProp = ((self.maxAction(self.maxActionArgs) - self.minAction(self.minActionArgs)) / 
                        (self.maxFeedback(self.maxFeedbackArgs) - self.minFeedback(self.minFeedbackArgs))) * self.config.getMotorPidConfig().Kp;

        portENTER_CRITICAL(&self.mux);
        err = self.target - feed;
        const double approvedErr = self.config.getMotorPidConfig().approvedErr;
        if (abs(err) > approvedErr) {
          dtsec = (currentTime - self.prevTime) / 1000000.0;

          if (dtsec > 0) {
            self.integral = i = self.constrainOut(self.integral + err * self.config.getMotorPidConfig().Ki * dtsec);
            d = (((double)(err - self.prevErr)) * self.config.getMotorPidConfig().Kd) / dtsec;
            value = err * kProp + i + d;
            needAction = true;

            if (cnt++ & 7 == 0)
              SP_LOGI(PID_LOG, "dts=%f, err=%f, prevErr=%f, i=%f, d=%f, v=%f, deltaErr=%f, current=%f, Kp=%f", dtsec, (double)err, 
                                       (double)self.prevErr, i, d, value, approvedErr, (double)feed, kProp);

            self.prevErr = err;
            self.prevTime = currentTime;
          }
        }
        portEXIT_CRITICAL(&self.mux);

        if (needAction) 
          self.action((ACTION_TYPE)value, self.actionArgs);

        const int32_t dt = (((int32_t)self.config.getMotorPidConfig().delayMs) - ((int32_t)((esp_timer_get_time() - currentTime) / 1000))) / portTICK_PERIOD_MS;

        if (dt > 0)
          vTaskDelay(dt);
      } while (true);  
    }

    const std::function<FEEDBACK_TYPE(void*)> feedback;
    void * const feedbackArgs;

    const std::function<void(ACTION_TYPE, void*)> action;
    void * const actionArgs;

    const std::function<ACTION_TYPE(void*)> minAction;
    void * const minActionArgs;

    const std::function<ACTION_TYPE(void*)> maxAction;
    void * const maxActionArgs;

    const std::function<FEEDBACK_TYPE(void*)> minFeedback;
    void * const minFeedbackArgs;

    const std::function<FEEDBACK_TYPE(void*)> maxFeedback;
    void * const maxFeedbackArgs;

    Configuration& config;

    double integral; 

    TaskHandle_t xHandle;

    FEEDBACK_TYPE prevErr;
    volatile FEEDBACK_TYPE target;

    uint64_t prevTime;
  
    portMUX_TYPE mux;

  public:
    void begin() noexcept {
      prevTime = esp_timer_get_time();

      xTaskCreate(timerTask, "PID", 4096, this, 1000, &xHandle);
    }

    void setValue(FEEDBACK_TYPE value) noexcept {
      if (value != target) {
        SP_LOGI(PID_LOG, "Pid value=%f", value);
        
        portENTER_CRITICAL(&mux);
        if (value != target) {
          target = value;
          prevTime = esp_timer_get_time();
          prevErr = value - feedback(feedbackArgs);
          integral = 0;
        }
        portEXIT_CRITICAL(&mux);
      }
    }
};

#endif