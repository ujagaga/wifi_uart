#include <ESP8266WiFi.h>
#include "config.h"

static IPAddress apIP(192,168,1,1);

void WIFIC_init(void){
  Serial.println("\nStarting AP");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  if (WiFi.softAP(SSID, PASSWORD)) {
    Serial.println("Running in AP mode. SSID: " + String(SSID) +
                   ", IP: " + apIP.toString());
  } else {
    Serial.println("Failed to start AP mode");
  }
}
