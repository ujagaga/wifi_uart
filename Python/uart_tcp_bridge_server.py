#!/usr/bin/env python3

import socket
import time
import glob
import os
import threading
import serial

TCP_PORT = 4059
UART_BAUD = 115200

UART_SCAN_INTERVAL = 1.0
UART_IDLE_TIMEOUT  = 3.0
SOCKET_TIMEOUT     = 0.1

uart_ser = None
uart_port = None
last_uart_rx = 0

client_sock = None


# ---------------- UART helpers ----------------

def find_uart():
    if os.path.exists("/dev/uart_bridge"):
        return "/dev/uart_bridge"
    devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    return devices[0] if devices else None


def open_uart(port):
    ser = serial.Serial(
        port=port,
        baudrate=UART_BAUD,
        timeout=0.1
    )
    print("[UART] Opened {}".format(port))
    return ser


def close_uart():
    global uart_ser
    if uart_ser:
        try:
            uart_ser.close()
        except:
            pass
    uart_ser = None


# ---------------- TCP helpers ----------------

def close_client():
    global client_sock
    if client_sock:
        try:
            client_sock.close()
        except:
            pass
    client_sock = None


# ---------------- UART manager thread ----------------

def uart_manager():
    global uart_ser, uart_port, last_uart_rx

    while True:
        if uart_ser:
            if not os.path.exists(uart_ser.port):
                print("[UART] Device disappeared")
                close_uart()

        if not uart_ser:
            port = find_uart()
            if port:
                try:
                    uart_ser = open_uart(port)
                    uart_port = port
                    last_uart_rx = time.time()
                except Exception as e:
                    print("[UART] Open failed:", e)
                    uart_ser = None

        time.sleep(UART_SCAN_INTERVAL)


# ---------------- TCP server thread ----------------

def tcp_server():
    global client_sock

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("", TCP_PORT))
    server.listen(1)
    server.settimeout(0.5)

    print("[TCP] Listening on port {}".format(TCP_PORT))

    while True:
        if client_sock:
            time.sleep(0.5)
            continue

        try:
            client, addr = server.accept()
            client.settimeout(SOCKET_TIMEOUT)
            client_sock = client
            print("[TCP] Client connected:", addr)
        except socket.timeout:
            pass


# ---------------- Main thread: data bridge ----------------

def bridge_loop():
    global last_uart_rx

    while True:
        if not uart_ser or not client_sock:
            time.sleep(0.1)
            continue

        # TCP → UART
        try:
            data = client_sock.recv(4096)
            if data:
                uart_ser.write(data)
            else:
                raise ConnectionError
        except socket.timeout:
            pass
        except Exception:
            print("[TCP] Client disconnected")
            close_client()
            continue

        # UART → TCP
        try:
            data = uart_ser.read(256)
            if data:
                last_uart_rx = time.time()
                client_sock.sendall(data)
            elif time.time() - last_uart_rx > UART_IDLE_TIMEOUT:
                raise IOError("UART stalled")
        except Exception as e:
            print("[UART] Reset:", e)
            close_uart()

        time.sleep(0.01)


# ---------------- Main ----------------

def main():
    threading.Thread(target=uart_manager, daemon=True).start()
    threading.Thread(target=tcp_server, daemon=True).start()

    # Run bridge in main thread
    bridge_loop()


if __name__ == "__main__":
    main()
