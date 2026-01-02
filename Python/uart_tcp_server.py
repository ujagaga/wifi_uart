#!/usr/bin/env python3
"""
DLMS-safe TCP ↔ UART bridge SERVER
Single-threaded, Python 3.5 compatible
"""

import socket
import json
import time
import glob
import select
import serial

# =========================================================
# CONFIG
# =========================================================

BIND_IP = "0.0.0.0"
TCP_DATA_PORT = 59000
TCP_CFG_PORT = 59001

UART_SCAN_INTERVAL = 1.0
SOCKET_TIMEOUT = 0.1
CFG_PUSH_INTERVAL = 5.0  # seconds

# =========================================================
# GLOBAL STATE
# =========================================================

uart_cfg = None
uart_ser = None
uart_dev = None
last_uart_check = 0.0
last_cfg_push = 0.0

cfg_conn = None
data_conn = None
cfg_buffer = b""

# =========================================================
# UART HELPERS
# =========================================================

def is_config_changed(new_cfg, old_cfg):
    """Return True if any UART config parameter changed"""
    if old_cfg is None:
        return True
    keys = ["bit_rate", "data_bits", "stop_bits", "parity", "dtr", "rts"]
    for k in keys:
        if new_cfg.get(k) != old_cfg.get(k):
            return True
    return False


def find_uart_device():
    devs = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    return devs[0] if devs else None

def open_uart(cfg):
    parity_map = {0: serial.PARITY_NONE, 1: serial.PARITY_ODD, 2: serial.PARITY_EVEN,
                  3: serial.PARITY_MARK, 4: serial.PARITY_SPACE}
    stop_bits_map = {0: serial.STOPBITS_ONE, 1: serial.STOPBITS_ONE_POINT_FIVE, 2: serial.STOPBITS_TWO}

    dev = find_uart_device()
    if not dev:
        return None, None

    ser = serial.Serial(
        port=dev,
        baudrate=cfg["bit_rate"],
        bytesize=cfg.get("data_bits", 8),
        parity=parity_map.get(cfg.get("parity", 0), serial.PARITY_NONE),
        stopbits=stop_bits_map.get(cfg.get("stop_bits", 0), serial.STOPBITS_ONE),
        timeout=0
    )
    print("[UART] {}".format(dev))
    return ser, dev

def close_uart():
    global uart_ser, uart_dev
    if uart_ser:
        try:
            uart_ser.close()
        except:
            pass
    uart_ser = None
    uart_dev = None

def uart_hotplug_check():
    global uart_ser, uart_dev, last_uart_check
    now = time.time()
    if now - last_uart_check < UART_SCAN_INTERVAL:
        return
    last_uart_check = now

    current_dev = find_uart_device()
    if uart_ser and not current_dev:
        close_uart()
    elif uart_ser is None and current_dev and uart_cfg:
        uart_ser, uart_dev = open_uart(uart_cfg)

# =========================================================
# TCP SERVER SETUP
# =========================================================

def setup_server(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((BIND_IP, port))
    s.listen(1)
    s.settimeout(SOCKET_TIMEOUT)
    return s

def handle_new_connection(server, old_conn, name):
    """Accept a new client and close old one if necessary"""
    try:
        conn, addr = server.accept()
        conn.settimeout(SOCKET_TIMEOUT)
        if old_conn:
            try:
                old_conn.close()
            except:
                pass
        print("[{}] Client: {}".format(name, addr))
        return conn
    except socket.timeout:
        return old_conn

# =========================================================
# CFG HANDLING
# =========================================================

def send_cfg(conn):
    global uart_cfg, last_cfg_push
    if conn and uart_cfg:
        try:
            doc = json.dumps(uart_cfg)
            conn.sendall(doc.encode() + b"\n")
            last_cfg_push = time.time()
        except Exception as e:
            print("[CFG] Send error:", e)
            try:
                conn.close()
            except:
                pass
            return None
    return conn

# =========================================================
# MAIN LOOP
# =========================================================

if __name__ == "__main__":
    cfg_server = setup_server(TCP_CFG_PORT)
    data_server = setup_server(TCP_DATA_PORT)
    print("[CFG] Listening on {}".format(TCP_CFG_PORT))
    print("[DATA] Listening on {}".format(TCP_DATA_PORT))

    try:
        while True:
            uart_hotplug_check()

            # Build lists for select
            read_list = [cfg_server, data_server]
            if cfg_conn:
                read_list.append(cfg_conn)
            if data_conn:
                read_list.append(data_conn)
            if uart_ser:
                read_list.append(uart_ser.fileno())

            readable, _, _ = select.select(read_list, [], [], SOCKET_TIMEOUT)

            for r in readable:
                # New CFG connection
                if r is cfg_server:
                    cfg_conn = handle_new_connection(cfg_server, cfg_conn, "CFG")

                # New DATA connection
                elif r is data_server:
                    data_conn = handle_new_connection(data_server, data_conn, "DATA")

                # CFG client data
                elif r is cfg_conn:
                    try:
                        data = cfg_conn.recv(1024)
                        if data:
                            cfg_buffer += data
                            while b"\n" in cfg_buffer:
                                line, cfg_buffer = cfg_buffer.split(b"\n", 1)
                                cfg = json.loads(line.decode())
                                if is_config_changed(cfg, uart_cfg):
                                    uart_cfg = cfg
                                    close_uart()  # Only reopen if changed
                                    print("[CFG] UART config changed, reopening UART: {}".format(cfg))
                                # always send current cfg back to client
                                cfg_conn = send_cfg(cfg_conn)

                        else:
                            print("[CFG] Client disconnected")
                            try:
                                cfg_conn.close()
                            except:
                                pass
                            cfg_conn = None
                            cfg_buffer = b""
                    except Exception as e:
                        print("[CFG] Receive error:", e)
                        try:
                            cfg_conn.close()
                        except:
                            pass
                        cfg_conn = None
                        cfg_buffer = b""

                # DATA client → UART
                elif r is data_conn:
                    try:
                        data = data_conn.recv(1024)
                        if data:
                            if uart_ser:
                                try:
                                    uart_ser.write(data)
                                except Exception as e:
                                    print("[DATA] UART write error:", e)
                            print("[DATA] RX: {}".format(data))
                        else:
                            print("[DATA] Client disconnected")
                            try:
                                data_conn.close()
                            except:
                                pass
                            data_conn = None
                    except Exception as e:
                        print("[DATA] Receive error:", e)
                        try:
                            data_conn.close()
                        except:
                            pass
                        data_conn = None

                # UART → DATA client
                elif uart_ser and r == uart_ser.fileno():
                    try:
                        data = uart_ser.read(1024)
                        if data and data_conn:
                            try:
                                data_conn.sendall(data)
                            except Exception as e:
                                print("[DATA] Send error:", e)
                                try:
                                    data_conn.close()
                                except:
                                    pass
                                data_conn = None
                    except Exception:
                        print("[DATA] UART read error")
                        close_uart()

            # Periodic CFG push
            now = time.time()
            if cfg_conn and uart_cfg and now - last_cfg_push >= CFG_PUSH_INTERVAL:
                cfg_conn = send_cfg(cfg_conn)

    except KeyboardInterrupt:
        print("Stopping...")

    finally:
        if cfg_conn:
            cfg_conn.close()
        if data_conn:
            data_conn.close()
        close_uart()
        cfg_server.close()
        data_server.close()
