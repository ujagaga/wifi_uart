#include "tcp_servers.h"
#include "config.h"

WiFiServer tcpDataServer(TCP_DATA_PORT);
WiFiServer tcpCfgServer(TCP_CFG_PORT);

WiFiClient tcpDataClient;
WiFiClient tcpCfgClient;

void TCP_SERVERS_init() {
  tcpDataServer.begin();
  tcpDataServer.setNoDelay(true);

  tcpCfgServer.begin();
  tcpCfgServer.setNoDelay(true);
}

void handleDataServer() {
  if (!tcpDataClient || !tcpDataClient.connected()) {
    WiFiClient newClient = tcpDataServer.available();
    if (newClient) {
      if (tcpDataClient) tcpDataClient.stop();
      tcpDataClient = newClient;
      Serial.println("Data client connected");
    }
  }
}

void handleCfgServer() {
  if (!tcpCfgClient || !tcpCfgClient.connected()) {
    WiFiClient newClient = tcpCfgServer.available();
    if (newClient) {
      if (tcpCfgClient) tcpCfgClient.stop();
      tcpCfgClient = newClient;
      Serial.println("Config client connected");
    }
  }

  // Just discard any incoming data to avoid TCP buffer buildup
  if (tcpCfgClient && tcpCfgClient.connected()) {
    while (tcpCfgClient.available()) {
      tcpCfgClient.read();  // read and ignore
    }
  }
}

void TCP_SERVERS_process() {
  handleDataServer();
  handleCfgServer();
}

void TCP_DATA_send(const uint8_t *buf, size_t len) {
  if (tcpDataClient && tcpDataClient.connected())
    tcpDataClient.write(buf, len);
}

void TCP_CFG_sendBaudRate(uint32_t baud) {
  if (tcpCfgClient && tcpCfgClient.connected()) {
    tcpCfgClient.printf("BAUD %u\n", baud);
  }
}
