#include <ESP8266WiFi.h>
#include "config.h"

WiFiClient dataClient;
WiFiClient cfgClient;

bool connectToDataServer() {
  if (!dataClient.connected()) {
    // Serial.print("Connecting to data server ");
    // Serial.print(WiFi.gatewayIP()); // ESP32 AP’s IP is usually 192.168.4.1
    // Serial.print(":");
    // Serial.println(TCP_DATA_PORT);

    if (dataClient.connect(WiFi.gatewayIP(), TCP_DATA_PORT)) {
      dataClient.setNoDelay(true);
    } else {
      // Serial.println("Failed to connect to data server!");
    }
  }
  return dataClient.connected();
}

bool connectToCfgServer() {
  if (!cfgClient.connected()) {
    // Serial.print("Connecting to config server ");
    // Serial.print(WiFi.gatewayIP());
    // Serial.print(":");
    // Serial.println(TCP_CFG_PORT);

    if (cfgClient.connect(WiFi.gatewayIP(), TCP_CFG_PORT)) {
      cfgClient.setNoDelay(true);
    } else {
      // Serial.println("Failed to connect to config server!");
    }
  }
  return cfgClient.connected();
}

void TCP_CLIENTS_init() {
  connectToDataServer();
  connectToCfgServer();
}

void TCP_CLIENTS_process() {
  // Reconnect if lost
  if (!dataClient.connected()) connectToDataServer();
  if (!cfgClient.connected())  connectToCfgServer();

  // Handle incoming serial data from ESP32 data server → UART
  while (dataClient.available()) {
    uint8_t b = dataClient.read();
    Serial.write(b);
  }

  // Handle config messages from ESP32 config server
  while (cfgClient.available()) {
    String msg = cfgClient.readStringUntil('\n');
    msg.trim();
    if (msg.startsWith("BAUD ")) {
      uint32_t newBaud = msg.substring(5).toInt();
      // Serial.printf("Changing baud to %u\n", newBaud);
      // Serial.flush();
      Serial.begin(newBaud);  // apply new baud rate
    } else {
      // Serial.printf("Unknown config message: %s\n", msg.c_str());
    }
  }
}

// Send data to the ESP32 data server
void TCP_DATA_send(const uint8_t *buf, size_t len) {
  if (dataClient.connected()) {
    dataClient.write(buf, len);
  }
}

