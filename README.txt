# WiFi UART

The purpose of this project is to create an easy to use wireless UART. I often have a USB RS485 device that is detected by the OS as a CDC device (Serial port). The wires can sometimes be long and dragging them accross the room is inconvenient, so I am trying to make it wireless to access a smart DLMS meter.

## UART Server
An ESP32-S2 device with native USB support is used as:
1. WiFi access point:
	- SSID: USB_Serial_ESP32_WIFI_1
	- PASS: 12345678
2. CDC device to be connected to a PC via USB
3. TCP Configuration server to pass the setting to a client
4. TCP Data server to exchange UART data, received via CDC function, with a client

## Python Client
A python script is intended to run on a Linux computer (Raspberry Pi Zero W or any other SBC), connect to the TCP servers running on the ESP32 and detect the first available UART device (/dev/ttyUSBx or /dev/ttyACMx) to configure and bridge data to the TCP data server.
The computer should be set up to connect to ESP32 WiFi, so:
	- SSID: USB_Serial_ESP32_WIFI_1
	- PASS: 12345678

## ESP8266
Instead of a python client on a linux machine and a USB RS485 dongle, I am trying an ESP8266 client with an RS485 adapter module.

This way the two devices (Python or ESP8266, as client and ESP32 as server) are connected and exchanging data transparently without a physical cable. To connect:
- Plug a smart meter connected RS485 adapter to the linux computer running the python script or use the ESP8266 device connected to a smart meter
- Plug the ESP32-S2 device into a remote computer running the GuruX DLMS director
- Connect from DlmsDirector to ESP32 device as if it was wired to the smart meter.

## Status
ESP32 => Python client works. 
ESP32 => ESP8266 does not work.

Tries to replace ESP32 with ESP8266, but failed, so giving up. Will create a linux box to use my python script and a proven USB RS485 adapter.
