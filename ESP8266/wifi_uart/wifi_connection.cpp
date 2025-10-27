#include <ESP8266WiFi.h>
#include "config.h"

void WIFIC_init(void) {
  WiFi.mode(WIFI_STA); // set as station (client)
  WiFi.begin(SSID, PASSWORD);

  // Serial.print("Connecting to ");
  // Serial.println(SSID);

  unsigned long startAttemptTime = millis();

  // Try to connect for up to 10 seconds
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    // Serial.print(".");
    delay(500);
  }

  // if (WiFi.status() == WL_CONNECTED) {
  //   Serial.println();
  //   Serial.println("WiFi connected!");
  //   Serial.print("IP address: ");
  //   Serial.println(WiFi.localIP());
  // } else {
  //   Serial.println();
  //   Serial.println("WiFi connection failed!");
  // }
}
