#include <ESP8266WiFi.h>
#include "config.h"

WiFiServer tcpServer(TCP_PORT);    
WiFiClient tcpClient;

void TCP_SERVER_init() {
  tcpServer.begin();
  tcpServer.setNoDelay(true); // send data immediately
  Serial.println("TCP server started on port 23");
}

void TCP_SERVER_process() {
  // Accept a new client if none is connected
  if (!tcpClient || !tcpClient.connected()) {
    WiFiClient newClient = tcpServer.available();
    if (newClient) {
      if (tcpClient) tcpClient.stop();  // drop any old connection
      tcpClient = newClient;
#ifdef DBG
      Serial.println("TCP client connected: " + tcpClient.remoteIP().toString());
#endif
    }
  }

  // If a client is connected, handle data exchange
  if (tcpClient && tcpClient.connected()) {
    // Client -> Serial
    while (tcpClient.available()) {
      Serial.write(tcpClient.read());
    }

    // Serial -> Client
    while (Serial.available()) {
      tcpClient.write(Serial.read());
    }
  }
}
