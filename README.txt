# WiFi UART

The purpose of this project is to create an easy to use wireless UART. I often have a USB device that is detected 
by the OS as a CDC device (Serial port). The wires can sometimes be long and dragging them accross the room is inconvenient. Mostly I use Rx and Tx lines and ignore the other controll lines. This is simple to pass between two devices via TCP connection.

## UART Server
An ESP32-S2 device with native USB support is used as:
1. WiFi access point
2. CDC device to be connected to a PC via USB
3. TCP Configuration server to pass the setting to a client
4. TCP Data server to exchange UART data, received via CDC function, with a client

## UART Client
An ESP8266 device is used as:
1. WiFi client to connect to ESP32-S2
2. TCP configuration client to receive uart configuration 
3. TCP data client to exchange data with server and pass it to/from UART.

## Python Client
A python script is intended to run on a Linux computer (Raspberry Pi Zero W or any other SBC), connect to the TCP servers running on the ESP32 and detect the first available UART device (/dev/ttyUSBx or /dev/ttyACMx) to configure and bridge data to the TCP data server.
The computer should be set up to connect to ESP32 WiFi, so:
- SSID: USB_Serial_ESP32_WIFI_1
- PASS: 12345678

This way the two devices (ESP8266 or Python as client and ESP32 as server) are connected and exchanging data transparently without a physical cable.

## NOTE 
I tried connecting an RS485 module so I can communicate with DLMS devices like a smart electricity meter, but I could never get it working, so I made a new device based on RaspberryPi Zero W to act as a USB host for an RS485 USB adapter I already have been working with.

## Status
All done, tested and working fine

As a bonus, there is also `uart_tcp_bridge_server.py` which monitors uart and connects to it using hardcoded parameters. It opens a TCP server at port 4059 and exchanges data between a connected client and uart. It accepts only one client.