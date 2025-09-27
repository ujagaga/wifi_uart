# WiFi UART

The purpose of this project is to create an easy to use wireless UART. I often have a USB device that is detected 
by the OS as a CDC device (Serial port). The wires can sometimes be long and dragging them accross the room is inconvenient. Mostly I use Rx and Tx lines and ignore the other controll lines. This should be simple to pass between two devices via TCP connection.

The idea is to use an Orange Pi Zero, Raspberry Pi Zero W or any other credit card sized single board computer
running linux: 
1. A python script will detects any newly registered /dev/ttyUSBx or /dev/ttyACMx device and connects automatically.
2. This script provides a TCP client to connect to a server to exchange UART data.
3. An ESP8266 device hosts a WiFi Access Point, an HTTP server to enable BAUD rate configuration and a TCP Server to which the python client will connect and transfer needed data.

This way the two devices would be connected and exchanging data transparently without a physical cable.

## NOTE 
You can also use an ESP32-S2 as a CDC device. Then you can set the BAUD rate from the host computer and transfer the control signals as well, but that is in planning.

## Status
All done, tested and working fine

