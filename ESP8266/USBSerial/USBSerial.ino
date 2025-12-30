#include <Arduino.h>
#include "wifi_connection.h"
#include "tcp_servers.h"

/*
  ESP8266 UART pins:
  TX = GPIO1
  RX = GPIO3
*/

void setup() {
  /* Initial UART — will be reconfigured by CFG channel */
  Serial.begin(9600, SERIAL_8N1);
  Serial.setRxBufferSize(256);
  Serial.setTimeout(1);

  WIFIC_init();
  TCP_SERVERS_init();
}

void loop() {
  /* Handle TCP servers (accept clients, config, etc.) */
  TCP_SERVERS_process();

  /* ================= UART → TCP ================= */
  if (tcpDataClient && tcpDataClient.connected()) {
    while (Serial.available()) {
      uint8_t buf[128];
      size_t n = Serial.readBytes(buf, sizeof(buf));
      if (n) {
        tcpDataClient.write(buf, n);
      }
    }
  }

  /* ================= TCP → UART ================= */
  if (tcpDataClient && tcpDataClient.connected()) {
    while (tcpDataClient.available()) {
      uint8_t buf[128];
      size_t n = tcpDataClient.read(buf, sizeof(buf));
      if (n) {
        Serial.write(buf, n);
        Serial.flush();   // CRITICAL for DLMS timing
      }
    }
  }
}
