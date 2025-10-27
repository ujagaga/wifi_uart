#include <WiFi.h>
#include "config.h"

void WIFIC_init(void){
  IPAddress local_IP(192,168,4,1);
  IPAddress gateway(192,168,4,1);
  IPAddress subnet(255,255,255,0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  bool success = WiFi.softAP(SSID, PASSWORD);

  if (!success) {
    Serial.println("AP OK");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("AP Fail!");
  }
}