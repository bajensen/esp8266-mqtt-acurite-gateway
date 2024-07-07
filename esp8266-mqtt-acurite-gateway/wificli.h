#ifndef _WIFI_CLI_H
#define _WIFI_CLI_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define WIFI_SSID "MyWiFiSSID"
#define WIFI_PASS "MyWifiPassword"
IPAddress syslogServer(192, 168, 7, 77);

extern char msgBuffer[];
#define MSG_BUFFER_SIZE 200

WiFiClient wifi_client;
WiFiUDP udp;
int wifi_state = WL_IDLE_STATUS;

void wifi_init();
char *wifi_getStatusName(int statusId);
void wifi_loop();
void wifi_log(char *str);

void wifi_init() {
  // We start by connecting to a WiFi network
  Serial.print("WiFi: Connecting to: ");
  Serial.println(WIFI_SSID);

  WiFi.enableAP(0);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void wifi_loop() {
  int newStatus = WiFi.status();

  if (wifi_state != newStatus) {
    Serial.print("WiFi status: ");
    Serial.println(wifi_getStatusName(newStatus));

    if (newStatus == WL_CONNECTED) {
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      snprintf(msgBuffer, MSG_BUFFER_SIZE, "Connected\n");
      wifi_log(msgBuffer);
    }
  }

  wifi_state = newStatus;
}

bool wifi_connected() {
  return (wifi_state == WL_CONNECTED);
}

char *wifi_getStatusName(int statusId) {
  switch (statusId) {
    case WL_CONNECTED: return "Connected";
    case WL_NO_SHIELD: return "No Shield";
    case WL_IDLE_STATUS: return "Idle";
    case WL_NO_SSID_AVAIL: return "NO SSID Avail";
    case WL_SCAN_COMPLETED: return "Scan Completed";
    case WL_CONNECT_FAILED: return "Connect Failed";
    case WL_CONNECTION_LOST: return "Connection Lost";
    case WL_DISCONNECTED: return "Disconnected";
    default: return "Unknown";
  }
}

void wifi_log(char *str) {
  // Print to serial
  Serial.print(str);

  // Write to syslog server
  udp.beginPacket(syslogServer, 514);
  udp.write("acurite-gw ");
  udp.write(str, strlen(str));
  udp.endPacket();
}

#endif
