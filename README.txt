# WiFi UART

The purpose of this project is to create an easy to use wireless UART. I often have a USB RS485 device that is detected by the OS as a CDC device (Serial port). The wires can sometimes be long and dragging them accross the room is inconvenient, so I am trying to make it wireless to access a smart DLMS meter.

## UART Server
An ESP32-S2 device with native USB support is used as:
1. WiFi client
2. CDC device to be connected to a PC via USB
3. TCP Configuration client to pass the setting to a server
4. TCP Data client to exchange UART data, received via CDC function, with a server

## Python Server
A python script is intended to run on a Linux computer (Raspberry Pi Zero W or any other SBC), provide TCP servers for the ESP32 to connect to and detect the first available UART device (/dev/ttyUSBx or /dev/ttyACMx) to configure and bridge data to the TCP data server.
The computer should be set up as WiFi hotspot for ESP32 to connect, so:
	- SSID: UART-TCP-Bridge-01
	- PASS: 12345678

## Status
Seems to work fine for now.