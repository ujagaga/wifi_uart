#!/usr/bin/env python3

"""
DLMS-safe TCP ↔ UART bridge for ESP32-S2 USB CDC tunnel
Python 3.5 compatible
"""
import socket
import json
import time
import glob
import threading
import serial
import select

ESP_IP = "192.168.4.1"
TCP_DATA_PORT = 59000
TCP_CFG_PORT = 59001

UART_SCAN_INTERVAL = 1.0
TCP_RETRY_DELAY = 1.0

uart_cfg = None
uart_ser = None
stop_event = threading.Event()


# =========================================================
# TCP helper
# =========================================================
def connect_tcp(ip, port):
    while not stop_event.is_set():
        try:
            s = socket.create_connection((ip, port), timeout=5)
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            s.settimeout(None)
            print("[TCP] Connected to {}:{}".format(ip, port))
            return s
        except OSError:
            time.sleep(TCP_RETRY_DELAY)


# =========================================================
# UART helpers
# =========================================================
def find_uart_device():
    devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    return devices[0] if devices else None


def open_uart(cfg):
    parity_map = {
        0: serial.PARITY_NONE,
        1: serial.PARITY_ODD,
        2: serial.PARITY_EVEN,
        3: serial.PARITY_MARK,
        4: serial.PARITY_SPACE,
    }

    stop_bits_map = {
        0: serial.STOPBITS_ONE,
        1: serial.STOPBITS_ONE_POINT_FIVE,
        2: serial.STOPBITS_TWO,
    }

    ser = serial.Serial(
        port=find_uart_device(),
        baudrate=cfg["bit_rate"],
        bytesize=cfg.get("data_bits", 8),
        parity=parity_map.get(cfg.get("parity", 0), serial.PARITY_NONE),
        stopbits=stop_bits_map.get(cfg.get("stop_bits", 0), serial.STOPBITS_ONE),
        timeout=0,
    )

    print("[UART] Opened {}".format(ser.port))
    return ser


# =========================================================
# Config channel thread
# =========================================================
def cfg_thread():
    global uart_cfg

    sock = connect_tcp(ESP_IP, TCP_CFG_PORT)
    buf = b""

    while not stop_event.is_set():
        try:
            data = sock.recv(1024)
            if not data:
                raise ConnectionError
            buf += data

            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                line = line.strip()
                if not line:
                    continue

                uart_cfg = json.loads(line.decode())
                print("[CFG] {}".format(uart_cfg))

        except Exception:
            try:
                sock.close()
            except:
                pass
            sock = connect_tcp(ESP_IP, TCP_CFG_PORT)


# =========================================================
# Main data bridge (single thread, select-based)
# =========================================================
def data_bridge():
    global uart_ser

    while uart_cfg is None:
        time.sleep(0.1)

    while uart_ser is None:
        port = find_uart_device()
        if not port:
            time.sleep(UART_SCAN_INTERVAL)
            continue
        try:
            uart_ser = open_uart(uart_cfg)
        except Exception as e:
            print("[UART] Open failed:", e)
            time.sleep(1)

    sock = connect_tcp(ESP_IP, TCP_DATA_PORT)

    while not stop_event.is_set():
        try:
            rlist = [sock, uart_ser.fileno()]
            ready, _, _ = select.select(rlist, [], [], 1)

            if sock in ready:
                data = sock.recv(1024)
                if not data:
                    raise ConnectionError
                uart_ser.write(data)

            if uart_ser.fileno() in ready:
                data = uart_ser.read(1024)
                if data:
                    sock.sendall(data)

        except Exception:
            try:
                sock.close()
            except:
                pass
            sock = connect_tcp(ESP_IP, TCP_DATA_PORT)


# =========================================================
# Main
# =========================================================
def main():
    threading.Thread(target=cfg_thread, daemon=True).start()

    try:
        data_bridge()
    except KeyboardInterrupt:
        stop_event.set()
        print("Stopping...")


if __name__ == "__main__":
    main()
