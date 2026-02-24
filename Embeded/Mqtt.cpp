#include "Mqtt.hpp"
#include "Utils.hpp"
#include "logger.hpp"

#define MQTT_PORT 8883

const char MQTT_LOG[] PROGMEM = "MQTT";

static const int32_t defaultDelayMs = 1000 / portTICK_PERIOD_MS;

Mqtt::Mqtt(Configuration& conf) noexcept : config(conf), delayMs(200), numCallbacks(0), numConCallbacks(0) {
  mqttClient.onConnect([this] (bool sessionPresent) { onConnect(sessionPresent); });

  mqttClient.onDisconnect([this] (AsyncMqttClientDisconnectReason reason) { onDisconnect(reason); });

  mqttClient.onMessage([this] (char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    onMessage(topic, payload, properties, len, index, total);
  });
}

bool Mqtt::begin(IPAddress addr) noexcept {
  if (!mqttClient.connected()) {
    brokerAddress = addr;

    mqttClient.setServer(addr, MQTT_PORT);

    mqttClient.connect();

    return true;
  }

  return false;
}

void Mqtt::end() noexcept {
  mqttClient.disconnect(true);
}

uint16_t Mqtt::publish(const char *topic, const void *data, size_t length, uint8_t qos, bool retain, uint16_t message_id) noexcept {
  if (mqttClient.connected()) {
    char fullTopic[128];
  
    copy_str(copy_str(copy_str(fullTopic, topic), "/"), config.getHostname());

    return mqttClient.publish(fullTopic, qos, retain, (const char*)data, length, false, message_id);
  }

  return 0;
}

uint16_t Mqtt::subscribe(const char *topic, uint8_t qos) noexcept {
  return mqttClient.subscribe(topic, qos);
}

void Mqtt::onConnect(bool sessionPresent) noexcept {
  SP_LOGI(MQTT_LOG, "Connected to broker: %s, session=%s", brokerAddress.toString().c_str(), sessionPresent ? "true" : "false");
  MQTT_CON_CALLBACK_INFO copy[MAX_MQTT_CALLBACKS];
  conCallbacksCritical.enter();

  const size_t n = numConCallbacks;
  for (auto i = 0; i < n; i++)
    copy[i] = conCallbacks[i];

  conCallbacksCritical.leave();

  for (auto i = 0; i < n; i++)
    copy[i].callback(sessionPresent, copy[i].args);
}

void Mqtt::onDisconnect(AsyncMqttClientDisconnectReason reason) noexcept {
  SP_LOGI(MQTT_LOG, "Disconnect from broker: %d", (int)reason);
}

void Mqtt::onMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) noexcept {
  SP_LOGI(MQTT_LOG, "Receive: %s", topic);

  if (len != total) {
    SP_LOGW(MQTT_LOG, "Unsupported chunks. Topic: %s", topic);
    return;
  }

  MQTT_CALLBACK_INFO copy[MAX_MQTT_CALLBACKS];
  callbacksCritical.enter();

  const size_t n = numCallbacks;
  for (auto i = 0; i < n; i++)
    copy[i] = callbacks[i];

  callbacksCritical.leave();

  for (auto i = 0; i < n; i++)
    copy[i].callback(topic, payload, properties, len, copy[i].args);
}
