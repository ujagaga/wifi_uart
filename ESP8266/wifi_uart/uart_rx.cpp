#include <Arduino.h>
#include "tcp_clients.h"   // for TCP_DATA_send()

// Optional: adjust buffer size to your needs
#define UART_BUFFER_SIZE 128

void UART_RX_init(uint32_t baud) {
  Serial.begin(baud);
  Serial.setTimeout(2);  // small timeout for readStringUntil etc., but not really used here
}

void UART_RX_process() {
  static uint8_t buf[UART_BUFFER_SIZE];

  // Read all available bytes in chunks
  int availableBytes = Serial.available();
  while (availableBytes > 0) {
    int toRead = (availableBytes > UART_BUFFER_SIZE) ? UART_BUFFER_SIZE : availableBytes;
    int bytesRead = Serial.readBytes(buf, toRead);
    if (bytesRead > 0) {
      TCP_DATA_send(buf, bytesRead);
    }
    availableBytes = Serial.available();
  }
}
