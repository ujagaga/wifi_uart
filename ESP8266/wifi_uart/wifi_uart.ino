#include "wifi_connection.h"
#include "config.h"
#include "tcp_clients.h"
#include "uart_rx.h"

void setup(void) {
  Serial.begin(115200);      
  delay(500);
  WIFIC_init();
  TCP_CLIENTS_init();
}

void loop(void) {
  TCP_CLIENTS_process();
  UART_RX_process();
}
