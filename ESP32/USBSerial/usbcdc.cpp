#include "tcp_servers.h"

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
      // No action required
      break;

    case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
      // No action required
      break;

    case ARDUINO_USB_CDC_LINE_CODING_EVENT:
      // Capture settings only; do NOT act on them here
      g_cfg = *data;
      g_cfg_valid = true;
      break;

    case ARDUINO_USB_CDC_RX_EVENT:
      // Do nothing here; data is pulled in main loop
      break;

    default:
      break;
  }
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

/* ---------------------------------------------------------
   Main CDC ↔ TCP data bridge
   --------------------------------------------------------- */
void USB_CDC_process(void)
{
  if (!tcpDataClient || !tcpDataClient.connected())
    return;

  /* ---------- USB → TCP ---------- */
  while (USBSerial.available()) {
    uint8_t buf[64];
    size_t n = USBSerial.read(buf, sizeof(buf));
    if (n > 0) {
      tcpDataClient.write(buf, n);
    }
  }

  /* ---------- TCP → USB ---------- */
  bool wrote = false;

  while (tcpDataClient.available()) {
    uint8_t buf[64];
    size_t n = tcpDataClient.read(buf, sizeof(buf));
    if (n > 0) {
      USBSerial.write(buf, n);
      wrote = true;
    }
  }

  if (wrote) {
    USBSerial.flush();   // ensure immediate USB transmission
  }
}
