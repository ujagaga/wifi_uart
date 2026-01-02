#include "wifi_connection.h"
#include "usbcdc.h"
#include "tcp_clients.h"

void setup() {
  USB_CDC_init();
  WIFIC_init();
}

void loop() {
  USB_CDC_process();
  TCPC_process();
}
