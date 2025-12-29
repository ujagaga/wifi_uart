#include <USB.h>
#include <ArduinoJson.h>
#include "tcp_servers.h"
#include "config.h"

WiFiServer tcpDataServer(TCP_DATA_PORT);
WiFiServer tcpCfgServer(TCP_CFG_PORT);

WiFiClient tcpDataClient;
WiFiClient tcpCfgClient;
arduino_usb_cdc_event_data_t g_cfg;
bool g_cfg_valid;

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
    }
  }
}

void TCP_DATA_send(const uint8_t *buf, size_t len) {
  if (tcpDataClient && tcpDataClient.connected())
    tcpDataClient.write(buf, len);
}

void TCP_CFG_sendConfig()
{
  if (!tcpCfgClient || !tcpCfgClient.connected() || !g_cfg_valid)
    return;

  StaticJsonDocument<256> doc;

  doc["bit_rate"]  = g_cfg.line_coding.bit_rate;
  doc["data_bits"] = g_cfg.line_coding.data_bits;
  doc["stop_bits"] = g_cfg.line_coding.stop_bits;
  doc["parity"]    = g_cfg.line_coding.parity;
  doc["dtr"]       = g_cfg.line_state.dtr;
  doc["rts"]       = g_cfg.line_state.rts;

  serializeJson(doc, tcpCfgClient);
  tcpCfgClient.println();   // newline-delimited JSON
}

void TCP_CFG_setConfig(const arduino_usb_cdc_event_data_t *data)
{
  g_cfg = *data;
  g_cfg_valid = true; 

  TCP_CFG_sendConfig();  // push update
}

void handleCfgServer()
{
  if (!tcpCfgClient || !tcpCfgClient.connected()) {
    WiFiClient newClient = tcpCfgServer.available();
    if (newClient) {
      if (tcpCfgClient) tcpCfgClient.stop();
      tcpCfgClient = newClient;

      TCP_CFG_sendConfig();
    }
  }

  while (tcpCfgClient && tcpCfgClient.available()) {
    tcpCfgClient.read();
  }
}

void TCP_SERVERS_process() {
  static uint32_t lastCfgPush = 0;
  if (g_cfg_valid && millis() - lastCfgPush > 200) {
    TCP_CFG_sendConfig();
    lastCfgPush = millis();
  }
  handleDataServer();
  handleCfgServer();
}
