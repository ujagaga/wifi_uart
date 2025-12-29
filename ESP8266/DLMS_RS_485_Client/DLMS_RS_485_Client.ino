#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

/* ================= CONFIG ================= */

const char* WIFI_SSID = "USB_Serial_ESP32_WIFI_1";
const char* WIFI_PASS = "12345678";

IPAddress ESP32_IP(192, 168, 4, 1);
const uint16_t TCP_DATA_PORT = 59000;
const uint16_t TCP_CFG_PORT  = 59001;

/* ========================================= */

WiFiClient dataClient;
WiFiClient cfgClient;

/* UART config from ESP32 */
uint32_t uart_baud = 9600;
SerialConfig uart_cfg = SERIAL_8N1;

/* ================= WIFI ================= */

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }
}

/* ================= TCP ================= */

void connectData() {
  while (!dataClient.connect(ESP32_IP, TCP_DATA_PORT)) {
    delay(500);
  }
  dataClient.setNoDelay(true);
}

void connectCfg() {
  while (!cfgClient.connect(ESP32_IP, TCP_CFG_PORT)) {
    delay(500);
  }
  cfgClient.setNoDelay(true);
}

/* ================= CFG ================= */

void handleCfg() {
  static String line;

  while (cfgClient.available()) {
    char c = cfgClient.read();
    if (c == '\n') {
      StaticJsonDocument<256> doc;
      if (deserializeJson(doc, line) == DeserializationError::Ok) {
        uart_baud = doc["bit_rate"] | uart_baud;
        uint8_t parity    = doc["parity"] | 0;
        uint8_t stop_bits = doc["stop_bits"] | 0;

        /* Default */
        SerialConfig new_cfg = SERIAL_8N1;

        if (parity == 0 && stop_bits == 0)
          new_cfg = SERIAL_8N1;
        else if (parity == 2 && stop_bits == 0)
          new_cfg = SERIAL_8E1;
        else if (parity == 1 && stop_bits == 0)
          new_cfg = SERIAL_8O1;
        else if (parity == 0 && stop_bits == 2)
          new_cfg = SERIAL_8N2;

        /* Apply only if changed */
        if (new_cfg != uart_cfg || uart_baud != Serial.baudRate()) {
          uart_cfg = new_cfg;
          Serial.flush();
          Serial.begin(uart_baud, uart_cfg);
        }

      }
      line = "";
    } else {
      line += c;
    }
  }
}

/* ================= SETUP ================= */

void setup() {
  Serial.begin(uart_baud);

  connectWiFi();
  connectData();
  connectCfg();
}

/* ================= LOOP ================= */

void loop() {
  /* ---- CFG channel ---- */
  if (!cfgClient.connected()) {
    cfgClient.stop();
    connectCfg();
  }
  handleCfg();

  /* ---- DATA channel ---- */
  if (!dataClient.connected()) {
    dataClient.stop();
    connectData();
  }

  /* TCP → UART */
  while (dataClient.available()) {
    uint8_t b = dataClient.read();
    Serial.write(b);
    Serial.flush();   // critical for auto-direction adapters
  }

  /* UART → TCP */
  while (Serial.available()) {
    uint8_t b = Serial.read();
    dataClient.write(b);
  }
}
