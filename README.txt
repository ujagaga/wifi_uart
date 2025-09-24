# WiFi UART

The purpose of this project is to create an easy to use wireless UART. I often have a USB device that is detected 
by the OS as a CDC device (Serial port). The wires can sometimes be long and dragging them accross the room is inconvenient. Mostly I use Rx and Tx lines and ignore the other controll lines. This should be simple to pass between two devices via TCp connection.

The idea is to use an Orange Pi Zero, Raspberry Pi Zero W or any other credit card sized single board computer
running linux: 
1. A python script will detect any newly registered /dev/tty device and connect automatically.
2. This script will provide a TCP server and a UDP becon so it can be located on the network.
3. An ESP8266 device will host an UDP client to listen to UDP broadcast and detect the IP address of the TCP server.
4. The ESP8266 will also implement a TCP client to connect to the server and transfer needed data.
5. There would also be an HTTP server to configure BAUD rate and WiFi parameters to connect to network

This way the two devices would be connected and exchanging data transparently without a physical cable.

You can also use an ESP32-S2 as a CDC device. Then you can set the BAUD rate from the host computer and transfer the control signals as well.

NOTE: Just starting the project so it is not yet stabile.