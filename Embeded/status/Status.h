#ifndef __STATUS_H
#define __STATUS_H

#include <config/config.h>

#ifdef USE_INTERNAL_TEMP
    #include <driver/temperature_sensor.h>
#endif

#include <Arduino.h>
#include <udp/udp.h>
#include <atomic>

#ifdef USE_MOTOR
    #include <motor/Motor.h>
#endif

#ifdef USE_SPEEDOMETER
    #include <speedometer/Speedometer.h>
#endif

#ifdef USE_SHEME_MARKER
    #include <schememarker/schememarker.h>
#endif

#ifdef USE_SWITCH
    #include <switch/Switch.h>
#endif

#ifdef USE_SEMAPHORE
    #include <semaphore/Semaphore.h>
#endif

#ifdef WITH_LEDS
    #include <led/led.h>
#endif

class Status {
public:
    Status(
        #ifdef USE_MOTOR
            Motor& motor, 
        #endif

        #ifdef USE_SPEEDOMETER
            Speedometer& speedometer, 
        #endif

        #ifdef USE_SHEME_MARKER
            SchemeMarker& schemeMarker,
        #endif

        #ifdef USE_SWITCH
            Switch& rSwitch,
        #endif

        #ifdef USE_SEMAPHORE
            Semaphores &semaphores,
        #endif

        #ifdef WITH_LEDS
            Led **leds,
            size_t ledsCount,
        #endif

        UDPSender& udpSender) noexcept;

    void begin() noexcept;

    void sendAllStatus() noexcept;

    #ifdef USE_MOTOR
        void sendPower() noexcept;
    #endif

    #ifdef USE_SPEEDOMETER
        void sendSpeed() noexcept;
    #endif

    #ifdef USE_INTERNAL_TEMP
        float getInternalTemp() const noexcept;
    #endif
private:
    #ifdef USE_MOTOR
        Motor& motor;
        volatile float power;
        volatile int direction;
    #endif

    #ifdef USE_SPEEDOMETER
        Speedometer& speedometer;
        volatile float speed;
    #endif

    #ifdef USE_SHEME_MARKER
        SchemeMarker& schemeMarker;
        volatile uint64_t lastMarkerTime;
    #endif

    #ifdef USE_SWITCH
        Switch& rSwitch;
        volatile bool switchState;
    #endif

    UDPSender& udpSender;

    #ifdef USE_INTERNAL_TEMP
        temperature_sensor_handle_t tempHandle;
        volatile float internalTemp;
    #endif

    #ifdef USE_SEMAPHORE
        Semaphores &semaphores;
        uint32_t semaphoresCount;
        uint32_t *semaphoresSerializeBuffer;

        inline bool __attribute__((__always_inline__)) semaphoresChanged() noexcept {
            bool result = false;

            for (size_t i = 0; i < semaphoresCount; i++) {
                const uint32_t color = semaphores.get(i).getValue().getValue();

                if (semaphoresSerializeBuffer[i] != color) {
                    result = true;
                    semaphoresSerializeBuffer[i] = color;
                }
            }

            return result;
        }
    #endif

    #ifdef WITH_LEDS
        Led **leds;
        uint8_t *ledColors;
        const size_t ledsCount;

        inline bool __attribute__((__always_inline__)) ledsChanged() noexcept {
            bool result = false;

            for (size_t i = 0; i < ledsCount; i++) {
                const uint8_t color = leds[i]->getValue();

                if (ledColors[i] != color) {
                    result = true;
                    ledColors[i] = color;
                }
            }

            return result;
        }
    #endif

    TaskHandle_t taskHandle;

    std::atomic_uint version;

    static uint16_t IRAM_ATTR statusSendCallback(void *args, uint8_t *buffer, size_t maxLen) noexcept;

    static void taskStatic(void *args) noexcept;

    void task() noexcept;

    #ifdef USE_SHEME_MARKER
        static void IRAM_ATTR schemeMarkerCallback(void *args) noexcept;
    #endif
};

#endif // __STATUS_H

