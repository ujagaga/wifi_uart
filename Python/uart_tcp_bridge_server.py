#!/usr/bin/env python3

import socket
import serial
import select
import time
import os
import glob

# ---------------- Configuration ----------------

TCP_PORT = 4059

UART_BAUD = 9600
UART_TURNAROUND = 0.02      # 20 ms DLMS silent gap
UART_IDLE_TIMEOUT = 30

# Adjust if needed (DLMS is usually 7E1)
UART_BYTESIZE = serial.EIGHTBITS
UART_PARITY   = serial.PARITY_NONE
UART_STOPBITS = serial.STOPBITS_ONE

# ------------------------------------------------


def find_uart():
    if os.path.exists("/dev/uart_bridge"):
        return "/dev/uart_bridge"
    devs = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    return devs[0] if devs else None


def open_uart(port):
    uart = serial.Serial(
        port=port,
        baudrate=UART_BAUD,
        bytesize=UART_BYTESIZE,
        parity=UART_PARITY,
        stopbits=UART_STOPBITS,
        timeout=0,
        write_timeout=0,
        rtscts=False,
        dsrdtr=False
    )

    # Default to RX mode
    uart.setRTS(False)

    print(f"[UART] Opened {port}")
    return uart


def main():
    uart = None
    last_uart_rx = time.time()

    # TCP server
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("", TCP_PORT))
    server.listen(1)
    server.setblocking(False)

    print(f"[TCP] Listening on {TCP_PORT}")

    client = None

    while True:
        # Ensure UART is present
        if not uart:
            port = find_uart()
            if port:
                try:
                    uart = open_uart(port)
                    last_uart_rx = time.time()
                except Exception as e:
                    print("[UART] Open failed:", e)
            time.sleep(0.5)
            continue

        # Build select list
        rlist = [server, uart.fileno()]
        if client:
            rlist.append(client)

        readable, _, _ = select.select(rlist, [], [], 1)

        for r in readable:

            # New TCP client
            if r is server:
                client, addr = server.accept()
                client.setblocking(False)
                client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                print("[TCP] Client connected:", addr)

            # TCP → UART (FRAME WRITE)
            elif client and r is client:
                data = client.recv(4096)
                if not data:
                    print("[TCP] Client disconnected")
                    client.close()
                    client = None
                    continue

                # ---- DLMS TX PHASE ----
                uart.setRTS(True)           # enable TX
                uart.write(data)
                uart.flush()

                time.sleep(UART_TURNAROUND) # mandatory silent gap

                uart.setRTS(False)          # enable RX
                last_uart_rx = time.time()

            # UART → TCP (STREAM RX)
            elif r == uart.fileno():
                data = uart.read(4096)
                if data:
                    last_uart_rx = time.time()
                    if client:
                        client.sendall(data)

        # UART removal detection
        if uart and not os.path.exists(uart.port):
            print("[UART] Device removed")
            uart.close()
            uart = None

        # UART idle reset (optional safety)
        if uart and time.time() - last_uart_rx > UART_IDLE_TIMEOUT:
            print("[UART] Idle timeout")
            uart.close()
            uart = None


if __name__ == "__main__":
    main()
