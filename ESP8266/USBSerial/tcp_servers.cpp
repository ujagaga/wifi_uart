#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

#include "tcp_servers.h"
#include "config.h"

/* ================= TCP ================= */

WiFiServer tcpDataServer(TCP_DATA_PORT);
WiFiServer tcpCfgServer(TCP_CFG_PORT);

WiFiClient tcpDataClient;
WiFiClient tcpCfgClient;

/* ================= UART CONFIG ================= */

uint32_t g_baud      = 9600;
uint8_t  g_data_bits = 8;
uint8_t  g_parity    = 0;   // 0=N, 1=O, 2=E
uint8_t  g_stop_bits = 0;   // 0=1, 2=2
bool     g_cfg_valid = false;

/* ================= HELPERS ================= */

static SerialConfig makeSerialConfig() {
  if (g_parity == 1 && g_stop_bits == 0) return SERIAL_8O1;
  if (g_parity == 2 && g_stop_bits == 0) return SERIAL_8E1;
  if (g_parity == 0 && g_stop_bits == 2) return SERIAL_8N2;
  return SERIAL_8N1;
}

static void applyUart() {
  static uint32_t lastBaud = 0;
  static SerialConfig lastCfg = SERIAL_8N1;

  SerialConfig cfg = makeSerialConfig();

  if (g_baud != lastBaud || cfg != lastCfg) {
    Serial.flush();
    Serial.begin(g_baud, cfg);
    lastBaud = g_baud;
    lastCfg  = cfg;
  }
}

/* ================= INIT ================= */

void TCP_SERVERS_init() {
  tcpDataServer.begin();
  tcpCfgServer.begin();
}

/* ================= DATA ================= */

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

/* ================= CFG ================= */

void TCP_CFG_sendConfig() {
  if (!tcpCfgClient || !tcpCfgClient.connected() || !g_cfg_valid)
    return;

  StaticJsonDocument<256> doc;
  doc["bit_rate"]  = g_baud;
  doc["data_bits"] = g_data_bits;
  doc["stop_bits"] = g_stop_bits;
  doc["parity"]    = g_parity;
  doc["dtr"]       = true;
  doc["rts"]       = true;

  serializeJson(doc, tcpCfgClient);
  tcpCfgClient.println();
}

void handleCfgServer() {
  static String rx;

  if (!tcpCfgClient || !tcpCfgClient.connected()) {
    WiFiClient newClient = tcpCfgServer.available();
    if (newClient) {
      if (tcpCfgClient) tcpCfgClient.stop();
      tcpCfgClient = newClient;
      rx = "";
      TCP_CFG_sendConfig();
    }
    return;
  }

  while (tcpCfgClient.available()) {
    char c = tcpCfgClient.read();
    if (c == '\n') {
      StaticJsonDocument<256> doc;
      if (deserializeJson(doc, rx) == DeserializationError::Ok) {
        g_baud      = doc["bit_rate"]  | g_baud;
        g_data_bits = doc["data_bits"] | g_data_bits;
        g_stop_bits = doc["stop_bits"] | g_stop_bits;
        g_parity    = doc["parity"]    | g_parity;
        g_cfg_valid = true;
        applyUart();
        TCP_CFG_sendConfig();
      }
      rx = "";
    } else {
      rx += c;
    }
  }
}

/* ================= PROCESS ================= */

void TCP_SERVERS_process() {
  handleDataServer();
  handleCfgServer();
}
