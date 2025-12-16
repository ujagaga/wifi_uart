#include <WiFi.h>
#include <USB.h>
#include "tcp_servers.h"  

#if !ARDUINO_USB_CDC_ON_BOOT
USBCDC USBSerial;
#endif

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == ARDUINO_USB_EVENTS) {
  } 
  else if (event_base == ARDUINO_USB_CDC_EVENTS) {
    arduino_usb_cdc_event_data_t *data = (arduino_usb_cdc_event_data_t *)event_data;
    switch (event_id) {
      case ARDUINO_USB_CDC_CONNECTED_EVENT:
        break;

      case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
        break;

      case ARDUINO_USB_CDC_LINE_CODING_EVENT:
      {
        TCP_CFG_setConfig(data);        
      }
        break;

      case ARDUINO_USB_CDC_RX_EVENT:
        {
          uint8_t buf[data->rx.len];
          size_t len = USBSerial.read(buf, data->rx.len);
          TCP_DATA_send(buf, len);
        }
        break;

      default:
        break;
    }
  }
}

void USB_CDC_init(void) {
  USB.onEvent(usbEventCallback);
  USBSerial.onEvent(usbEventCallback);
  USBSerial.begin();
  USB.begin();
}

void USB_CDC_process(void) {
  if (tcpDataClient && tcpDataClient.connected()) {
    while (tcpDataClient.available()) {
      USBSerial.write(tcpDataClient.read());
    }
  }
}
