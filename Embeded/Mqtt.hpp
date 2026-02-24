#ifndef INC_MQTT_HPP
#define INC_MQTT_HPP

#include <AsyncMqttClient.h>
#include <AsyncMqttClient.hpp>
#include "configuration.hpp"
#include "logger.hpp"
#include "Udp.hpp"

//typedef std::function<void(char* topic, void* payload, AsyncMqttClientMessageProperties properties, size_t len)> MqttMessageCallback;

//typedef void (*MqttMessageCallback)(char* topic, void* payload, AsyncMqttClientMessageProperties properties, size_t len, void *args);

typedef void (*MqttMessageCallback)(char* topic, void* payload, AsyncMqttClientMessageProperties properties, size_t len, void *args);

typedef void (*MqttConnectCallback)(bool sessionPresent, void *args);

typedef struct _MQTT_CALLBACK_INFO {
  MqttMessageCallback callback;
  void *args;
} MQTT_CALLBACK_INFO;

typedef struct _MQTT_CON_CALLBACK_INFO {
  MqttConnectCallback callback;
  void *args;
} MQTT_CON_CALLBACK_INFO;

#define MAX_MQTT_CALLBACKS 16

extern const char MQTT_LOG[];

class Mqtt {
  public:
    Mqtt(Configuration& conf) noexcept;

    bool begin(IPAddress addr) noexcept;

    void end() noexcept;

    uint16_t publish(const char *topic, const void *data, size_t length, uint8_t qos = 1, bool retain = false, uint16_t message_id = 0) noexcept;

    uint16_t subscribe(const char *topic, uint8_t qos) noexcept;

    inline IPAddress __attribute__((__always_inline__)) brokerIp() noexcept {
      return brokerAddress;
    }

    inline void __attribute__((__always_inline__)) addCallback(MqttMessageCallback callback, void *args) noexcept {
      callbacksCritical.enter();

      const size_t n = numCallbacks;
      if (n < MAX_MQTT_CALLBACKS) {
        for (auto i = 0; i < n; i++)
          if (callbacks[n].callback == callback) {
            SP_LOGW(MQTT_LOG, "Callback already added");

            callbacksCritical.leave();
            return;
          }

        callbacks[n].callback = callback;
        callbacks[n].args = args;
        numCallbacks++;
      }
      else
        SP_LOGW(MQTT_LOG, "Max callbacks!!!");

      callbacksCritical.leave();
    }

    inline void __attribute__((__always_inline__)) removeCallback(MqttMessageCallback callback) noexcept {
      callbacksCritical.enter();

      const size_t n = numCallbacks;
      for (auto i = 0; i < n; i++)
        if (callbacks[n].callback == callback) {
          if (i < n - 1)
            callbacks[i] = callbacks[n - 1];

          numCallbacks--;
          break;
        }

      callbacksCritical.leave();
    }

    inline void __attribute__((__always_inline__)) addConCallback(MqttConnectCallback callback, void *args) noexcept {
      conCallbacksCritical.enter();

      const size_t n = numConCallbacks;
      if (n < MAX_MQTT_CALLBACKS) {
        for (auto i = 0; i < n; i++)
          if (conCallbacks[n].callback == callback) {
            SP_LOGW(MQTT_LOG, "Callback already added");

            conCallbacksCritical.leave();
            return;
          }

        conCallbacks[n].callback = callback;
        conCallbacks[n].args = args;
        numConCallbacks++;
      }
      else
        SP_LOGW(MQTT_LOG, "Max callbacks!!!");

      conCallbacksCritical.leave();
    }

    inline void __attribute__((__always_inline__)) removeConCallback(MqttConnectCallback callback) noexcept {
      conCallbacksCritical.enter();

      const size_t n = numConCallbacks;
      for (auto i = 0; i < n; i++)
        if (conCallbacks[n].callback == callback) {
          if (i < n - 1)
            conCallbacks[i] = conCallbacks[n - 1];

          numConCallbacks--;
          break;
        }

      conCallbacksCritical.leave();
    }

  private:
    static void speedTask(void *args) noexcept;

    void onConnect(bool sessionPresent) noexcept;

    void onDisconnect(AsyncMqttClientDisconnectReason reason) noexcept;

    void onMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) noexcept;

    AsyncMqttClient mqttClient;
    Configuration& config;
    IPAddress brokerAddress;
    const uint32_t delayMs;

    CriticalSection callbacksCritical;
    size_t numCallbacks;
    MQTT_CALLBACK_INFO callbacks[MAX_MQTT_CALLBACKS];

    CriticalSection conCallbacksCritical;
    size_t numConCallbacks;
    MQTT_CON_CALLBACK_INFO conCallbacks[MAX_MQTT_CALLBACKS];
};


#endif

