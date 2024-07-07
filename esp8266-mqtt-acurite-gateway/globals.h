#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "mqtt.h"

#define CIRCULAR_BUFFER_INT_SAFE
#include "CircularBuffer.h"

#define CIRCULAR_BUFFER_SIZE  256
#define MAX_PLUGINS 3
#define MSG_BUFFER_SIZE 200

CircularBuffer<unsigned long, CIRCULAR_BUFFER_SIZE> timings;
char msgBuffer[MSG_BUFFER_SIZE] = "";

typedef struct plugin_s {
  char *name;
  bool (*detect)();
  bool (*process)();
  bool (*shouldSend)();
  uint16_t (*getId)();
  void (*encode)(char *buffer, unsigned int bufferSize);

  bool enabled;
  bool detectResult;
  bool processResult;
} plugin_t;

plugin_t plugins[MAX_PLUGINS];

ICACHE_RAM_ATTR inline bool inRange(unsigned long value, unsigned long minValue, unsigned long maxValue) {
  return (minValue <= value && value <= maxValue);
}

void plugins_init() {
  for (int i = 0; i < MAX_PLUGINS; i++) {
    memset(plugins, 0, MAX_PLUGINS * sizeof(plugin_t));
  }
}

bool plugins_register(
  char *name,
  bool (*detect)(), 
  bool (*process)(),
  bool (*shouldSend)(),
  uint16_t (*getId)(),
  void (*encode)(char *buffer, unsigned int bufferSize)
) {
  for (int i = 0; i < MAX_PLUGINS; i++) {
    if (! plugins[i].enabled) {
      plugins[i].name = name;
      plugins[i].detect  = detect;
      plugins[i].process = process;
      plugins[i].shouldSend = shouldSend;
      plugins[i].getId = getId;
      plugins[i].encode = encode;

      plugins[i].enabled = true;

      Serial.printf("plugin registered: %s\n", name);
      
      return true;
    }
  }
  
  Serial.printf("plugin FAILED: %s\n", name);

  return false;
}

ICACHE_RAM_ATTR bool plugins_detect () {
  bool hasDetect = false;
  
  for (int i = 0; i < MAX_PLUGINS; i++) {
    if (plugins[i].enabled) {
      bool res = plugins[i].detect();
      plugins[i].detectResult = res;
      hasDetect |= res;
    }
  }

  return hasDetect;
}

void plugins_process() {
  for (int i = 0; i < MAX_PLUGINS; i++) {
    if (plugins[i].enabled && plugins[i].detectResult) {
      plugins[i].processResult = plugins[i].process();
    }
  }
}

void plugins_send() {
  for (int i = 0; i < MAX_PLUGINS; i++) {
    if (plugins[i].enabled && plugins[i].processResult && plugins[i].shouldSend()) {
      plugins[i].encode(mqtt_getBuffer(), mqtt_getBufferSize());
      mqtt_send(plugins[i].name, plugins[i].getId());

      snprintf(msgBuffer, MSG_BUFFER_SIZE, "%s: %x, %s\n", plugins[i].name, plugins[i].getId(), mqtt_getBuffer());
      wifi_log(msgBuffer);
    }
  }
}

void plugins_clearDetectFlags() {
  for (int i = 0; i < MAX_PLUGINS; i++) {
    plugins[i].detectResult = 0;
  }
}

void plugins_clearProcessFlags() {
  for (int i = 0; i < MAX_PLUGINS; i++) {
    plugins[i].processResult = 0;
  }
}

void plugins_clearFlags() {
  for (int i = 0; i < MAX_PLUGINS; i++) {
    plugins[i].detectResult = 0;
    plugins[i].processResult = 0;
  }
}

float c_to_f (float c) {
  return (c * 1.8) + 32;
}

#endif
