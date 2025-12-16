#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "config.h"

/* =========================
   TCP clients
   ========================= */

WiFiClient dataClient;
WiFiClient cfgClient;

/**
 * @brief Converts individual data_bits, parity, and stop_bits into the 
 * combined ESP8266 SerialConfig enum value (e.g., SERIAL_8N1).
 * * @param data_bits The number of data bits (5, 6, 7, or 8).
 * @param stop_bits The number of stop bits (1 or 2).
 * @param parity Parity setting (0=None, 1=Even, 2=Odd).
 * @return SerialConfig The combined configuration enum value.
 */
SerialConfig getSerialConfig(uint8_t data_bits, uint8_t stop_bits, uint8_t parity) {
    
  // 1. Determine the base data/stop bits config (5N1, 6N1, etc.)
  SerialConfig base_config;
  
  // For simplicity, we are using a switch based on the core values.
  // The UART_xxx macros are usually defined internally by the core.
  
  if (data_bits == 8 && stop_bits == 1) {
      base_config = SERIAL_8N1;
  } else if (data_bits == 7 && stop_bits == 1) {
      base_config = SERIAL_7N1;
  } else if (data_bits == 6 && stop_bits == 1) {
      base_config = SERIAL_6N1;
  } else if (data_bits == 5 && stop_bits == 1) {
      base_config = SERIAL_5N1;
  } else if (data_bits == 8 && stop_bits == 2) {
      base_config = SERIAL_8N2;
  } else if (data_bits == 7 && stop_bits == 2) {
      base_config = SERIAL_7N2;
  } else if (data_bits == 6 && stop_bits == 2) {
      base_config = SERIAL_6N2;
  } else if (data_bits == 5 && stop_bits == 2) {
      base_config = SERIAL_5N2;
  } else {
      // Fallback to a safe default if configuration is not standard
      return SERIAL_8N1;
  }

  // 2. Adjust the configuration for Parity
  switch (parity) {
    case 0: // No Parity (N)
        // The base_config already assumes No Parity (e.g., 8N1)
        return base_config;
    
    case 1: // Even Parity (E)
        // Add the difference between Even and No Parity to the base.
        // This relies on the fact that UART_xEx is usually UART_xNx + a fixed offset.
        return (SerialConfig)((int)base_config + ((int)SERIAL_8E1 - (int)SERIAL_8N1));
    
    case 2: // Odd Parity (O)
        // Add the difference between Odd and No Parity to the base.
        // This relies on the fact that UART_xOx is usually UART_xNx + a fixed offset.
        return (SerialConfig)((int)base_config + ((int)SERIAL_8O1 - (int)SERIAL_8N1));
    
    default:
        return base_config; // Default to No Parity
  }
}

/* =========================
   Server connection helpers
   ========================= */

bool connectToDataServer()
{
  if (!dataClient.connected()) {
    if (dataClient.connect(WiFi.gatewayIP(), TCP_DATA_PORT)) {
      dataClient.setNoDelay(true);
    }
  }
  return dataClient.connected();
}

bool connectToCfgServer()
{
  if (!cfgClient.connected()) {
    if (cfgClient.connect(WiFi.gatewayIP(), TCP_CFG_PORT)) {
      cfgClient.setNoDelay(true);
    }
  }
  return cfgClient.connected();
}


/* =========================
   JSON parsing
   ========================= */

bool parseConfigJson(const String &json)
{
  StaticJsonDocument<256> doc;

  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    return false;
  }

  uint32_t bit_rate = doc["bit_rate"]  | 0;
  uint8_t data_bits = doc["data_bits"] | 8;
  uint8_t stop_bits = doc["stop_bits"] | 1;
  uint8_t parity    = doc["parity"]    | 0;
  bool dtr          = doc["dtr"]       | false;
  bool rts          = doc["rts"]       | false;

  if (bit_rate == 0) {
    return false;
  }

  SerialConfig newConfig = getSerialConfig(data_bits, stop_bits, parity);
  Serial.flush();  
  Serial.begin(bit_rate, newConfig);
  
  return true;
}

/* =========================
   Public API
   ========================= */

void TCP_CLIENTS_init()
{
  connectToDataServer();
  connectToCfgServer();
}

void TCP_CLIENTS_process()
{
  /* Reconnect if lost */
  if (!dataClient.connected()) connectToDataServer();
  if (!cfgClient.connected())  connectToCfgServer();

  /* Data channel: TCP → UART */
  while (dataClient.available()) {
    uint8_t b = dataClient.read();
    Serial.write(b);
  }

  /* Config channel: JSON lines */
  while (cfgClient.available()) {
    String line = cfgClient.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      continue;
    }
    parseConfigJson(line);
  }
}

/* =========================
   UART → TCP data channel
   ========================= */

void TCP_DATA_send(const uint8_t *buf, size_t len)
{
  if (dataClient.connected()) {
    dataClient.write(buf, len);
  }
}
