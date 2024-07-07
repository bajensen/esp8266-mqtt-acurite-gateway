#ifndef _MQTT_H
#define _MQTT_H

#include "wificli.h"

#define MQTT_TOPIC "my_mqtt_topic"
#define MQTT_CLIENT_ID "my_mqtt_client_id"
#define MQTT_USER "my_mqtt_user"
#define MQTT_PASS "my_mqtt_pass"
#define MQTT_HOST "mqtt.example.com"
#define MQTT_PORT 1883
#define MQTT_STATUS_TOPIC "acurite/status"
#define MQTT_STATUS_ONLINE "online"
#define MQTT_STATUS_OFFLINE "offline"

#define MQTT_RETRY_MS 2000
#define MQTT_TOPIC_BUFFER_SIZE 50
#define MQTT_MSG_BUFFER_SIZE 100

#include <PubSubClient.h>

PubSubClient mqtt_client(wifi_client);
unsigned long mqtt_nextTryMs = 0;
int mqtt_state = MQTT_DISCONNECTED;

char mqtt_topicBuffer[MQTT_TOPIC_BUFFER_SIZE];
char mqtt_msgBuffer[MQTT_MSG_BUFFER_SIZE];

void mqtt_init();
void mqtt_send(char *name, uint16_t id);
char *mqtt_getBuffer();
unsigned int mqtt_getBufferSize();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void mqtt_loop();
void mqtt_reconnect();
char *mqtt_getStateName (int statusId);

void mqtt_init() {
  mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
}

void mqtt_send(char *name, uint16_t id) {
  if (! mqtt_client.connected()) return;

  snprintf(mqtt_topicBuffer, MQTT_TOPIC_BUFFER_SIZE, "%s/%s/x%03x", MQTT_TOPIC, name, id);
  mqtt_client.publish(mqtt_topicBuffer, mqtt_msgBuffer);
}

char *mqtt_getBuffer() {
  return mqtt_msgBuffer;
}

unsigned int mqtt_getBufferSize() {
  return MQTT_MSG_BUFFER_SIZE;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
//  Serial.print("Message arrived [");
//  Serial.print(topic);
//  Serial.print("] ");
//
//  Serial.print(" (");
//  Serial.print(length);
//  Serial.print(") ");
//
//  for (int i = 0; i < length; i++) {
//    Serial.print((char) payload[i]);
//  }
}

void mqtt_loop() {
  if (! mqtt_client.connected()) {
    mqtt_reconnect();
  }

  if (mqtt_client.connected()) {
    mqtt_client.loop();
  }

  int newState = mqtt_client.state();

  if (mqtt_state != newState) {
    Serial.print("MQTT state changed: ");
    Serial.println(mqtt_getStateName(newState));
  }

  mqtt_state = newState;
}

void mqtt_reconnect() {
  if (mqtt_nextTryMs > millis()) return;
  mqtt_nextTryMs = millis() + MQTT_RETRY_MS;

  // Attempt to connect
  mqtt_client.connect(
    MQTT_CLIENT_ID,
    MQTT_USER, MQTT_PASS,
    MQTT_STATUS_TOPIC, 1, true, MQTT_STATUS_OFFLINE // will: topic, qos, retain, msg)
  );

  if (mqtt_client.connected()) {
    mqtt_client.publish(MQTT_STATUS_TOPIC, (uint8_t*) MQTT_STATUS_ONLINE, strlen(MQTT_STATUS_ONLINE), true);
  }
}

bool mqtt_connected() {
  return (mqtt_state == MQTT_CONNECTED);
}

char *mqtt_getStateName (int statusId) {
  switch (statusId) {
    case MQTT_CONNECTION_TIMEOUT: return "Connection Timeout";
    case MQTT_CONNECTION_LOST: return "Connection Lost";
    case MQTT_CONNECT_FAILED: return "Connect Failed";
    case MQTT_DISCONNECTED: return "Disconnected";
    case MQTT_CONNECTED: return "Connected";
    case MQTT_CONNECT_BAD_PROTOCOL: return "Bad Protocol";
    case MQTT_CONNECT_BAD_CLIENT_ID: return "Bad Client ID";
    case MQTT_CONNECT_UNAVAILABLE: return "Unavailable";
    case MQTT_CONNECT_BAD_CREDENTIALS: return "Bad Credentials";
    case MQTT_CONNECT_UNAUTHORIZED: return "Unauthorized";
    default: return "Unknown";
  }
}

#endif
