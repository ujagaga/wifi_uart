# WiFi UART

The purpose of this project is to create an easy to use wireless UART. I often have a USB device that is detected 
by the OS as a CDC device (Serial port). The wires can sometimes be long and dragging them accross the room is inconvenient. Mostly I use Rx and Tx lines and ignore the other controll lines. This should be simple to pass between two devices via TCP connection.

## UART Server
An ESP32-S2 device with native USB support is used as:
1. WiFi access point
2. CDC device to be connected to a PC via USB
3. TCP Configuration server to pass the BAUD rate setting to a client
4. TCP Data server to exchange UART data, received via CDC function, with a client

## UART Client
An ESP8266 device is uses as:
1. WiFi client to connect to ESP32-S2
2. TCP configuration client to receive BAUD rate configuration 
3. TCP data client to exchange data with server and pass it to/from UART.

This way the two devices are connected and exchanging data transparently without a physical cable.

## NOTE 
I also connected an RS485 module so I can communicate with DLMS devices like a smart electricity meter.

## Status
All done, tested and working fine

