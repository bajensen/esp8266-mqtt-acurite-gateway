# ESP8266 MQTT Acurite Gateway
Receives 433 Mhz Acurite sensor reports and relays them to an MQTT server

## Required Hardware
1. An ESP8266 board, such as the NodeMCU
2. A 433 Mhz Receiver (such as https://www.amazon.com/gp/product/B00HEDRHG6)
3. Some dupont cables, one of which will be cut into a poorly tuned antenna

## Required Software
1. Arduino IDE
2. ESP8266 Arduino support (https://github.com/esp8266/Arduino)
3. Circular Buffer by AgileWare
4. PubSubClient by Nick O'Leary

## Wiring
- Gnd and VCC as usual
- Connect radio DATA pin to GPIO pin 13 (D7 on the NodeMCU) OR update code with IRQ capable GPIO pin
- A dupont wire dangling from the radio ANT pin as a horrible antenna

## Configuration
See the following files for variables that need to be configured:
1. wificli.h to set WiFi SSID and password
2. mqtt.h to set MQTT hostname, credentials, topic, etc

## Wants
- Organize/modularize things better
- Use a web confiration interface to set up MQTT and WiFi credentials