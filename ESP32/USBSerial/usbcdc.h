#ifndef USB_CDC_H
#define USB_CDC_H

#include <USB.h>

extern USBCDC USBSerial;

extern void USB_CDC_init(void);
extern void USB_CDC_send(uint8_t* buf, size_t len);
extern void USB_CDC_process(void);

#endif
