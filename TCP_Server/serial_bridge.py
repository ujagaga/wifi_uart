#!/usr/bin/env python3
import os, glob, time, socket, threading, serial, requests
import config

# === Globals ===
serial_conn   = None
tcp_sock      = None
current_dev   = None
esp_baud      = config.BAUDRATE
last_esp_baud = esp_baud
stop_event    = threading.Event()

def dbg_print(msg):
    if config.PRINT_DBG:
        print(msg)

# --- Serial Helpers ---------------------------------------------------------
def scan_serial():
    return sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))

def close_serial():
    global serial_conn
    if serial_conn:
        try: serial_conn.close()
        except: pass
        serial_conn = None

def open_serial(dev, baud):
    global serial_conn, current_dev
    try:
        serial_conn = serial.Serial(dev, baud, timeout=0)
        current_dev = dev
        dbg_print(f"[Serial] Connected {dev} @ {baud}")
    except Exception as e:
        dbg_print(f"[Serial] Open failed {dev}: {e}")
        serial_conn = None

def serial_monitor():
    global current_dev, serial_conn, esp_baud, last_esp_baud
    while not stop_event.is_set():
        devices = scan_serial()

        # Need reconnect if: no device, new device, or baud changed
        need_reconnect = False
        if devices:
            newest = max(devices, key=os.path.getctime)
            if newest != current_dev or esp_baud != last_esp_baud:
                need_reconnect = True
        else:
            if current_dev:
                dbg_print("[Serial] Device gone")
                close_serial()
                current_dev = None

        if need_reconnect:
            close_serial()
            open_serial(newest, esp_baud)
            last_esp_baud = esp_baud

        time.sleep(config.SCAN_INTERVAL)

def serial_reader():
    global tcp_sock
    while not stop_event.is_set():
        if serial_conn and serial_conn.is_open and tcp_sock:
            try:
                data = serial_conn.read(serial_conn.in_waiting or 1)
                if data:
                    tcp_sock.sendall(data)
            except Exception as e:
                dbg_print(f"[Serial→TCP] {e}")
                time.sleep(1)
        else:
            time.sleep(0.2)

# --- TCP Client -------------------------------------------------------------
def tcp_client():
    global tcp_sock
    while not stop_event.is_set():
        if not tcp_sock:
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(5)
                dbg_print(f"[TCP] Connecting {config.TCP_SERVER_IP}:{config.TCP_PORT}...")
                s.connect((config.TCP_SERVER_IP, config.TCP_PORT))
                s.settimeout(None)
                tcp_sock = s
                dbg_print("[TCP] Connected.")
            except Exception as e:
                dbg_print(f"[TCP] Connect failed: {e}")
                tcp_sock = None
                time.sleep(3)
                continue

        try:
            data = tcp_sock.recv(1024)
            if not data:
                raise ConnectionError("No data (likely disconnected)")
            if serial_conn and serial_conn.is_open:
                serial_conn.write(data)
        except (ConnectionError, OSError) as e:
            dbg_print(f"[TCP] Lost: {e}")
            try: tcp_sock.close()
            except: pass
            tcp_sock = None
            time.sleep(3)
        except Exception as e:
            dbg_print(f"[TCP] Error: {e}")
            time.sleep(1)

# --- Baud Poller ------------------------------------------------------------
def baud_poller():
    global esp_baud
    url = f"http://{config.TCP_SERVER_IP}/get"
    while not stop_event.is_set():
        try:
            r = requests.get(url, timeout=3)
            if r.status_code == 200:
                new_baud = int(r.text.strip())
                if new_baud != esp_baud:
                    dbg_print(f"[HTTP] Baud change {esp_baud} → {new_baud}")
                    esp_baud = new_baud
        except requests.RequestException:
            # Silent when ESP is unreachable
            pass
        except Exception as e:
            dbg_print(f"[HTTP] Poll error: {e}")
        time.sleep(config.SCAN_INTERVAL)

# --- Main -------------------------------------------------------------------
if __name__ == "__main__":
    try:
        threads = [
            threading.Thread(target=serial_monitor, daemon=True),
            threading.Thread(target=serial_reader, daemon=True),
            threading.Thread(target=tcp_client, daemon=True),
            threading.Thread(target=baud_poller, daemon=True)
        ]
        for t in threads: t.start()

        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        dbg_print("\n[Main] Stopping...")
        stop_event.set()
        close_serial()
        if tcp_sock:
            try: tcp_sock.close()
            except: pass
