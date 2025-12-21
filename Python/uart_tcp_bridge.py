#!/usr/bin/env python3
"""
Python 3.5 bridge between ESP32 TCP servers and UART
Dependencies: pyserial
    pip3 install pyserial
"""

import socket
import json
import time
import glob
import threading
import serial

ESP_IP = "192.168.4.1"
TCP_DATA_PORT = 59000
TCP_CFG_PORT = 59001

UART_SCAN_INTERVAL = 1.0
TCP_RETRY_DELAY = 1.0

uart_cfg = None
uart_port = None
uart_ser = None
stop_event = threading.Event()


# =========================
# TCP connect helper
# =========================
def connect_tcp(ip, port):
    while not stop_event.is_set():
        try:
            s = socket.create_connection((ip, port), timeout=5)
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            s.settimeout(0.1)
            print("[TCP] Connected to {}:{}".format(ip, port))
            return s
        except OSError:
            time.sleep(TCP_RETRY_DELAY)


# =========================
# UART helper
# =========================
def find_uart_device():
    devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    return devices[0] if devices else None

def open_uart(cfg):
    global uart_ser

    parity_map = {0: serial.PARITY_NONE, 1: serial.PARITY_ODD,
                  2: serial.PARITY_EVEN, 3: serial.PARITY_MARK, 4: serial.PARITY_SPACE}
    stop_bits_map = {0: serial.STOPBITS_ONE, 1: serial.STOPBITS_ONE_POINT_FIVE,
                     2: serial.STOPBITS_TWO}

    uart_ser = serial.Serial(
        port=uart_port,
        baudrate=cfg["bit_rate"],
        bytesize=cfg.get("data_bits", 8),
        parity=parity_map.get(cfg.get("parity", 0), serial.PARITY_NONE),
        stopbits=stop_bits_map.get(cfg.get("stop_bits", 0), serial.STOPBITS_ONE),
        timeout=0.1  # important for Python 3.5
    )


# =========================
# Config channel thread
# =========================
def cfg_thread():
    global uart_cfg

    sock = connect_tcp(ESP_IP, TCP_CFG_PORT)
    buf = b""

    while not stop_event.is_set():
        try:
            try:
                data = sock.recv(1024)
                if not data:
                    raise ConnectionError
                buf += data
            except socket.timeout:
                continue

            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                line = line.strip()
                if not line:
                    continue

                cfg = json.loads(line.decode())
                uart_cfg = cfg
                print("[CFG] Received: {}".format(cfg))

        except Exception:
            try: sock.close()
            except: pass
            sock = connect_tcp(ESP_IP, TCP_CFG_PORT)


# =========================
# UART management thread
# =========================
def uart_manager_thread():
    global uart_port, uart_ser

    while not stop_event.is_set():
        if uart_ser and uart_ser.is_open:
            time.sleep(0.5)
            continue

        if not uart_cfg:
            time.sleep(0.5)
            continue

        uart_port = find_uart_device()
        if not uart_port:
            time.sleep(UART_SCAN_INTERVAL)
            continue

        try:
            open_uart(uart_cfg)
            print("[UART] Opened {}".format(uart_port))
        except Exception as e:
            print("[UART] Failed to open {}: {}".format(uart_port, e))
            uart_ser = None
            time.sleep(1)


# =========================
# TCP → UART thread
# =========================
def tcp_to_uart(sock):
    global uart_ser
    while not stop_event.is_set():
        if not uart_ser or not uart_ser.is_open:
            time.sleep(0.01)
            continue
        try:
            try:
                data = sock.recv(1024)
                if data:
                    uart_ser.write(data)
                    print(data)
            except socket.timeout:
                continue
        except Exception:
            try: sock.close()
            except: pass
            sock = connect_tcp(ESP_IP, TCP_DATA_PORT)


# =========================
# UART → TCP thread
# =========================
def uart_to_tcp(sock):
    global uart_ser
    while not stop_event.is_set():
        if not uart_ser or not uart_ser.is_open:
            time.sleep(0.01)
            continue
        try:
            data = uart_ser.read(256)  # blocking with small timeout
            if data:
                sock.sendall(data)
        except Exception:
            try: sock.close()
            except: pass
            sock = connect_tcp(ESP_IP, TCP_DATA_PORT)


# =========================
# Main
# =========================
def main():
    # Start threads
    threading.Thread(target=cfg_thread, daemon=True).start()
    threading.Thread(target=uart_manager_thread, daemon=True).start()

    # Wait until UART is ready
    while not uart_cfg:
        time.sleep(0.1)
    while not uart_port:
        time.sleep(0.1)

    # Connect to data server and start bridge threads
    sock = connect_tcp(ESP_IP, TCP_DATA_PORT)
    threading.Thread(target=tcp_to_uart, args=(sock,), daemon=True).start()
    threading.Thread(target=uart_to_tcp, args=(sock,), daemon=True).start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        stop_event.set()
        print("Stopping...")


if __name__ == "__main__":
    main()
