#ifndef TCP_CLIENTS_H
#define TCP_CLIENTS_H

extern void TCP_CLIENTS_init(void);
extern void TCP_CLIENTS_process(void);
extern void TCP_DATA_send(const uint8_t *buf, size_t len);

#endif
