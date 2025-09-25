#include "wifi_connection.h"
#include "config.h"
#include "http_server.h"
#include "tcp_server.h"

void setup(void) {
  Serial.begin(115200);      
  delay(500);
  WIFIC_init();
  HTTP_SERVER_init();
  TCP_SERVER_init(); 
}

void loop(void) {
  HTTP_SERVER_process();
  TCP_SERVER_process();
}
