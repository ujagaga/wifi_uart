#ifndef TCP_SERVERS_H
#define TCP_SERVERS_H
#include <WiFi.h>

extern WiFiClient tcpDataClient;
extern WiFiClient tcpCfgClient;

extern void TCP_SERVERS_init(void);
extern void TCP_SERVERS_process(void);
extern void TCP_DATA_send(const uint8_t *buf, size_t len);
extern void TCP_CFG_sendBaudRate(uint32_t baud);

#endif