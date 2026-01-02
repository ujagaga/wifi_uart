#ifndef TCP_CLIENTS_H
#define TCP_CLIENTS_H

#include <USB.h>

extern void TCPC_process(void);
extern void TCPC_DATA_send(uint8_t *buf, size_t len);
extern void TCPC_CFG_setConfig(const arduino_usb_cdc_event_data_t *data);
extern void TCPC_CFG_sendConfig(void);

#endif