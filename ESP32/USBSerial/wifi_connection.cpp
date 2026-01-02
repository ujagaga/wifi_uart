#include <WiFi.h>
#include "config.h"
#include "usbcdc.h"

void WIFIC_init(void){
  WiFi.mode(WIFI_STA); // Connect as station
  WiFi.begin(SSID, PASSWORD);

#ifdef DEBUG_MSG
  USBSerial.print("Connecting to WiFi");
#endif
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
#ifdef DEBUG_MSG
      USBSerial.print(".");
#endif
    retry++;
  }

#ifdef DEBUG_MSG
  if(WiFi.status() == WL_CONNECTED){
    USBSerial.println("\nWiFi connected!");
    USBSerial.print("IP: ");
    USBSerial.println(WiFi.localIP());
  } else {
    USBSerial.println("\nFailed to connect to WiFi");
  }
#endif
}

