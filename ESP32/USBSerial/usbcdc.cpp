#include "tcp_clients.h"

#if !ARDUINO_USB_CDC_ON_BOOT
USBCDC USBSerial;
#endif

/* ---------------------------------------------------------
   USB CDC event callback
   --------------------------------------------------------- */
static void usbEventCallback(void *arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data)
{
    if (event_base != ARDUINO_USB_CDC_EVENTS)
        return;

    arduino_usb_cdc_event_data_t *data =
        (arduino_usb_cdc_event_data_t *)event_data;

    switch (event_id) {
        case ARDUINO_USB_CDC_CONNECTED_EVENT:
        case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
            break;

        case ARDUINO_USB_CDC_LINE_CODING_EVENT:
            TCPC_CFG_setConfig(data);  // Send JSON config to server
            break;

        case ARDUINO_USB_CDC_RX_EVENT:        
            break;

        default:
            break;
    }
}

void USB_CDC_send(uint8_t* buf, size_t len){
    USBSerial.write(buf, len);
    USBSerial.flush();
}

/* ---------------------------------------------------------
   Initialization
   --------------------------------------------------------- */
void USB_CDC_init(void)
{
    USB.onEvent(usbEventCallback);
    USBSerial.onEvent(usbEventCallback);

    USBSerial.begin();   // CDC ACM
    USB.begin();         // USB stack
}

void USB_CDC_process(void)
{    
    // USB → TCP
    while (USBSerial.available()) {
        uint8_t buf[1024];
            size_t n = USBSerial.read(buf, sizeof(buf));
            TCPC_DATA_send(buf, n);
    }    
}