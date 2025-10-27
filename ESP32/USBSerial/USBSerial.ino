#include "wifi_connection.h"
#include "usbcdc.h"
#include "tcp_servers.h"
#include "USB.h"

void setup() {
  Serial.begin(115200);
  USB_CDC_init();
  WIFIC_init();
  TCP_SERVERS_init();
}

void loop() {
  USB_CDC_process();
  TCP_SERVERS_process();
}
