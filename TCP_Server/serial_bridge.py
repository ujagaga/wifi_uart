#!/usr/bin/env python3
import os
import glob
import time
import threading
import serial
from tcp_server import TCPServer
import config
from udp_broadcaster import UDPBroadcaster

current_device = None
serial_conn = None
stop_event = threading.Event()
tcp = TCPServer(port=config.TCP_PORT)

udp = UDPBroadcaster(
    message=f"WIFI_UART:{config.TCP_PORT}",
    port=config.UDP_PORT,
    interval=config.SCAN_INTERVAL
)

def scan_for_serial():
    return sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))

def serial_reader():
    global serial_conn
    while not stop_event.is_set():
        if serial_conn and serial_conn.is_open:
            try:
                data = serial_conn.read(serial_conn.in_waiting or 1)
                if data:
                    tcp.send(data)
            except Exception as e:
                print(f"[Serial] Error: {e}")
                close_serial()
        time.sleep(0.01)

def close_serial():
    global serial_conn
    if serial_conn:
        try:
            serial_conn.close()
        except:
            pass
        serial_conn = None

def serial_monitor():
    global current_device, serial_conn
    while not stop_event.is_set():
        devices = scan_for_serial()
        if devices:
            newest = max(devices, key=os.path.getctime)
            if newest != current_device:
                print(f"[Serial] New device: {newest}")
                close_serial()
                try:
                    serial_conn = serial.Serial(newest, config.BAUDRATE, timeout=0)
                    current_device = newest
                    print(f"[Serial] Connected to {newest} @ {config.BAUDRATE} baud")
                except Exception as e:
                    print(f"[Serial] Failed to open {newest}: {e}")
        else:
            if current_device:
                print("[Serial] No devices, closing")
                close_serial()
                current_device = None
        time.sleep(config.SCAN_INTERVAL)

if __name__ == "__main__":
    try:
        tcp.start()
        udp.start()
        t1 = threading.Thread(target=serial_monitor, daemon=True)
        t2 = threading.Thread(target=serial_reader, daemon=True)
        t1.start()
        t2.start()
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[Main] Stopping...")
        stop_event.set()
        close_serial()
        tcp.stop()
        udp.stop()
