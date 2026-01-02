#include <WiFi.h>
#include <ArduinoJson.h>
#include "tcp_clients.h"
#include "config.h"
#include "usbcdc.h"

WiFiClient tcpDataClient;
WiFiClient tcpCfgClient;

arduino_usb_cdc_event_data_t g_cfg;
bool g_cfg_valid;
bool cfg_dirty = false;
static unsigned long lastCfgPushTime = 0;
const unsigned long CFG_PUSH_INTERVAL = 5000;

// ---------------------------------------------------------
// Connect TCP client to server with auto-reconnect
// ---------------------------------------------------------
void connectClients() {
  if (!tcpDataClient.connected()) {    
    tcpDataClient.connect(SERVER_IP, TCP_DATA_PORT);
#ifdef DEBUG_MSG
    if (tcpDataClient.connected()) USBSerial.println("[DATA] Connected!");
#endif
  }

  if (!tcpCfgClient.connected()) {
    cfg_dirty = true;

    tcpCfgClient.connect(SERVER_IP, TCP_CFG_PORT);
    if (tcpCfgClient.connected()) {
#ifdef DEBUG_MSG
      USBSerial.println("[CFG] Connected!");
#endif
      TCPC_CFG_sendConfig();  // push current config on connect
    }
  }
}

// ---------------------------------------------------------
// Send USB CDC line coding as JSON
// ---------------------------------------------------------
void TCPC_CFG_sendConfig() {
    if (!tcpCfgClient.connected() || !g_cfg_valid) return;

    StaticJsonDocument<256> doc;

    doc["bit_rate"]  = g_cfg.line_coding.bit_rate;
    doc["data_bits"] = g_cfg.line_coding.data_bits;
    doc["stop_bits"] = g_cfg.line_coding.stop_bits;
    doc["parity"]    = g_cfg.line_coding.parity;
    doc["dtr"]       = g_cfg.line_state.dtr;
    doc["rts"]       = g_cfg.line_state.rts;

    serializeJson(doc, tcpCfgClient);
    tcpCfgClient.println();  // newline-delimited JSON
    tcpCfgClient.clear();
    cfg_dirty = false;
}

// ---------------------------------------------------------
// Called from USB CDC event callback
// ---------------------------------------------------------
void TCPC_CFG_setConfig(const arduino_usb_cdc_event_data_t *data) {
    g_cfg = *data;
    g_cfg_valid = true;
    cfg_dirty = true;
}

// ---------------------------------------------------------
// USB ↔ TCP DATA
// ---------------------------------------------------------
void TCPC_DATA_send(uint8_t* buf, size_t len){
  if (tcpDataClient && tcpDataClient.connected() && (len > 0)){
    tcpDataClient.write(buf, len);
    tcpDataClient.clear();
  }
}

// ---------------------------------------------------------
// Main process called from loop()
// ---------------------------------------------------------
void TCPC_process() {
    connectClients();

    unsigned long now = millis();

    if (g_cfg_valid && tcpCfgClient.connected() &&
        (cfg_dirty || now - lastCfgPushTime >= CFG_PUSH_INTERVAL)) {
        
        TCPC_CFG_sendConfig();
        lastCfgPushTime = now;  // update timestamp
    }

    if (tcpDataClient.connected()) {
      while (tcpDataClient.available()) {
          uint8_t buf[1024];
          size_t n = tcpDataClient.read(buf, sizeof(buf));
          if (n > 0) USB_CDC_send(buf, n);
      }
    }
}
