#ifndef UART_RX_H
#define UART_RX_H

extern void TCP_CLIENTS_init(void);
extern void TCP_CLIENTS_process(void);
extern void TCP_DATA_send(const uint8_t *buf, size_t len);

#endif
