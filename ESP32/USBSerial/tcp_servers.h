#ifndef TCP_SERVERS_H
#define TCP_SERVERS_H

#include <WiFi.h>
#include <USB.h>

extern WiFiClient tcpDataClient;
extern WiFiClient tcpCfgClient;

extern void TCP_SERVERS_init(void);
extern void TCP_SERVERS_process(void);
extern void TCP_DATA_send(const uint8_t *buf, size_t len);
extern void TCP_CFG_setConfig(const arduino_usb_cdc_event_data_t *data);
extern void TCP_CFG_sendConfig(void);

#endif