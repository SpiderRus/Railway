#ifndef INC_SPEEDMETER_HPP
#define INC_SPEEDMETER_HPP

#include "Motor.hpp"
#include "Udp.hpp"
#include "configuration.hpp"

#define SPEED_NUM_PROBES 64

// in millimeters per second
class Speedmeter {
  public:
    Speedmeter(Motor& motor, Udp& udp, Configuration& conf, AsyncWebServer& server) noexcept;

    void begin(void) noexcept;

    inline double __attribute__((__always_inline__)) getSpeed(void) const noexcept { return currentSpeed; };

    inline int32_t __attribute__((__always_inline__)) getMinSpeed(void) const noexcept { 
      auto conf = config.getSpeedmeterConfig();
      return sign(motor.getMinValue()) * conf.maxRps * conf.mmPerTurn; 
    }

    inline int32_t __attribute__((__always_inline__)) getMaxSpeed(void) const noexcept { 
      auto conf = config.getSpeedmeterConfig();
      return sign(motor.getMaxValue()) * conf.maxRps * conf.mmPerTurn; 
    }

    inline uint32_t __attribute__((__always_inline__)) rps() const noexcept {
      const uint32_t backMs = getBackMs();
      return (1000 * getInterrupts(backMs)) / (std::max<uint32_t>(config.getSpeedmeterConfig().pulsePerTurn, 1) * backMs);
    }

    inline double __attribute__((__always_inline__)) rpsAsDouble() const noexcept {
      const uint32_t backMs = getBackMs();
      return (1000.0 * getInterrupts(backMs)) / (std::max<uint32_t>(config.getSpeedmeterConfig().pulsePerTurn, 1) * backMs);
    }

  private:
    inline uint32_t __attribute__((__always_inline__)) getBackMs(void) const noexcept { return std::max<uint32_t>(config.getSpeedmeterConfig().sensorBackwardMs, 50); };

    inline uint32_t __attribute__((__always_inline__)) getInterrupts(uint32_t backMs) const noexcept {
      const uint32_t current = (uint32_t)(esp_timer_get_time() >> 4);
      const uint32_t th = current - ((backMs * 1000) >> 4);
      uint32_t counter = 0;
  
      for (size_t i = 0; i < SPEED_NUM_PROBES; i++) {
        const uint32_t value = interruptTimes[i];

        if (value >= th)
          counter++;
      }

      return counter;
    }

    static void IRAM_ATTR sensorInterrupt(void *arg);

    static void timerFunc(void*) noexcept;

    static void speedTask(void *args) noexcept;

    Motor& motor;
    Udp& udp;
    Configuration& config;
    AsyncWebServer& server;
    esp_timer_handle_t timerHandle;

    TaskHandle_t speedTaskHandle;
    QueueHandle_t speedQueue;

    volatile double currentSpeed;

    uint32_t interruptTimesArray[SPEED_NUM_PROBES];
    volatile uint32_t * const interruptTimes = interruptTimesArray;
    volatile size_t interruptCounter;
};

#endif


