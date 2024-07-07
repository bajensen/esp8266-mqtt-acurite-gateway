#include "globals.h"
#include "temp606a.h"
#include "temphum609a.h"
#include "temphum592txr.h"
#include "wificli.h"
#include "mqtt.h"

// Inspired by: http://rayshobby.net/?p=8827

// Requires libraries:
// Circular Buffer by AgileWare
// PubSubClient by Nick O'Leary

#define DATAPIN 13    // PIN 13 = D7

#define MIN_LENGTH   50

byte interruptNumber;
unsigned long previousMs = 0;
volatile byte received = false;
volatile unsigned long interruptCount = 0;
unsigned long heartbeatTs = 0;
bool interruptsEnabled = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Started.");
  Serial.println("Temperature/Humidity RX Gateway");
  Serial.println("Version 0.2");

  pinMode(DATAPIN, INPUT);
  interruptNumber = digitalPinToInterrupt(DATAPIN);

//  attachInterrupt(interruptNumber, handler, CHANGE);

  plugins_init();
  temp606a_init();
  temphum609a_init();
  temphum592txr_init();
  wifi_init();
  mqtt_init();
}

void attachInt() {
  interruptsEnabled = true;
  attachInterrupt(interruptNumber, handler, CHANGE);
}

void detachInt() {
  detachInterrupt(interruptNumber);
  interruptsEnabled = false;
}

ICACHE_RAM_ATTR void handler() {
  interruptCount++;

  // Bail out if waiting for processing
  if (received) return;

  // Get number of microseconds since start
  unsigned long nowMs = micros();

  // If the timer overflowed, bail out
  if (nowMs < previousMs) {
    // Update the previous so that the next interrupt works properly
    previousMs = nowMs;
    return;
  }

  // Find difference
  unsigned long diff = nowMs - previousMs;

  // Ignore changes that happen too quickly
  if (diff < MIN_LENGTH) return;

  // Push onto timing circular buffer
  timings.push(diff);

  // See if we've hit the end mark
  if (plugins_detect()) {
    received = true;
  }

  // Store previous microsecond timestamp
  previousMs = nowMs;
}

void loop() {
  if (received) {
    detachInt();

    plugins_process();
    plugins_clearDetectFlags();

    received = false;

    attachInt();
  }

  plugins_send();
  plugins_clearProcessFlags();

  wifi_loop();
  mqtt_loop();

  if (! interruptsEnabled && wifi_connected() && mqtt_connected()) {
    attachInt();
  }
  else if (interruptsEnabled && (!wifi_connected() || !mqtt_connected())) {
    detachInt();
  }

  if (heartbeatTs < millis()) {

    snprintf(msgBuffer, MSG_BUFFER_SIZE, "Interrupt count is now %lu\n", interruptCount);
    wifi_log(msgBuffer);
    heartbeatTs = millis() + 60*1000;
  }
}
