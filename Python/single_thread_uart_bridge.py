#!/usr/bin/env python3

import socket
import json
import time
import glob
import os
import serial

ESP_IP = "192.168.4.1"
TCP_DATA_PORT = 59000
TCP_CFG_PORT  = 59001

UART_SCAN_INTERVAL = 1.0
UART_IDLE_TIMEOUT  = 3.0
SOCKET_TIMEOUT     = 0.1

uart_cfg = None
uart_ser = None
uart_port = None
last_uart_rx = 0


# ---------------- TCP helpers ----------------

def connect_tcp(ip, port):
    while True:
        try:
            s = socket.create_connection((ip, port), timeout=5)
            s.settimeout(SOCKET_TIMEOUT)
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            print("[TCP] Connected to {}:{}".format(ip, port))
            return s
        except OSError:
            time.sleep(1)


# ---------------- UART helpers ----------------

def find_uart_device():
    # Prefer stable udev name if you add one
    if os.path.exists("/dev/uart_bridge"):
        return "/dev/uart_bridge"

    devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    return devices[0] if devices else None


def open_uart(cfg, port):
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
        port=port,
        baudrate=cfg["bit_rate"],
        bytesize=cfg.get("data_bits", 8),
        parity=parity_map.get(cfg.get("parity", 0), serial.PARITY_NONE),
        stopbits=stop_bits_map.get(cfg.get("stop_bits", 0), serial.STOPBITS_ONE),
        timeout=0.1
    )

    ser.dtr = cfg.get("dtr", True)
    ser.rts = cfg.get("rts", True)

    print("[UART] Opened {} @ {}".format(port, cfg["bit_rate"]))
    return ser


def close_uart():
    global uart_ser
    if uart_ser:
        try:
            uart_ser.close()
        except:
            pass
    uart_ser = None


# ---------------- Main loop ----------------

def main():
    global uart_cfg, uart_ser, uart_port, last_uart_rx

    cfg_sock  = connect_tcp(ESP_IP, TCP_CFG_PORT)
    data_sock = connect_tcp(ESP_IP, TCP_DATA_PORT)

    cfg_buf = b""

    while True:
        # ---------- Handle config socket ----------
        try:
            data = cfg_sock.recv(1024)
            if data:
                cfg_buf += data
                while b"\n" in cfg_buf:
                    line, cfg_buf = cfg_buf.split(b"\n", 1)
                    cfg = json.loads(line.decode())
                    uart_cfg = cfg
                    print("[CFG]", cfg)

                    # Force UART reconfigure
                    close_uart()
            else:
                raise ConnectionError
        except socket.timeout:
            pass
        except Exception:
            try: cfg_sock.close()
            except: pass
            cfg_sock = connect_tcp(ESP_IP, TCP_CFG_PORT)

        # ---------- Ensure UART ----------
        if uart_cfg:
            if uart_ser:
                # Detect disappeared device
                if not os.path.exists(uart_ser.port):
                    print("[UART] Device disappeared")
                    close_uart()

            if not uart_ser:
                port = find_uart_device()
                if port:
                    try:
                        uart_ser = open_uart(uart_cfg, port)
                        uart_port = port
                        last_uart_rx = time.time()
                    except Exception as e:
                        print("[UART] Open failed:", e)
                        time.sleep(1)

        # ---------- Data bridge ----------
        if uart_ser:
            # TCP -> UART
            try:
                data = data_sock.recv(4096)
                if data:
                    uart_ser.write(data)
                else:
                    raise ConnectionError
            except socket.timeout:
                pass
            except Exception:
                try: data_sock.close()
                except: pass
                data_sock = connect_tcp(ESP_IP, TCP_DATA_PORT)

            # UART -> TCP
            try:
                data = uart_ser.read(256)
                if data:
                    last_uart_rx = time.time()
                    data_sock.sendall(data)
                elif time.time() - last_uart_rx > UART_IDLE_TIMEOUT:
                    raise IOError("UART stalled")
            except Exception as e:
                print("[UART] Reset:", e)
                close_uart()

        time.sleep(0.01)


if __name__ == "__main__":
    main()
