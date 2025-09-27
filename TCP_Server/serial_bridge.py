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
serial_lock   = threading.Lock()   # NEW: protect serial access

def dbg_print(msg):
    if config.PRINT_DBG:
        print(msg)

# --- Serial Helpers ---------------------------------------------------------
def scan_serial():
    return sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))

def close_serial():
    global serial_conn, current_dev
    with serial_lock:
        if serial_conn:
            try:
                serial_conn.close()
            except:
                pass
            serial_conn = None
            current_dev = None

def open_serial(dev, baud):
    global serial_conn, current_dev
    with serial_lock:
        try:
            serial_conn = serial.Serial(dev, baud, timeout=0)
            current_dev = dev
            dbg_print(f"[Serial] Connected {dev} @ {baud}")
            return True
        except Exception as e:
            dbg_print(f"[Serial] Open failed {dev}: {e}")
            serial_conn = None
            return False

def serial_monitor():
    global current_dev, esp_baud, last_esp_baud
    while not stop_event.is_set():
        devices = scan_serial()

        if devices:
            newest = max(devices, key=os.path.getctime)
            if newest != current_dev:
                dbg_print(f"[Serial] New device: {newest}, Reconnecting")
                close_serial()
                time.sleep(0.05)  # give OS time to release
                if open_serial(newest, esp_baud):
                    last_esp_baud = esp_baud
        else:
            if current_dev:
                dbg_print("[Serial] Device gone")
                close_serial()

        time.sleep(config.SCAN_INTERVAL)

def serial_reader():
    global tcp_sock
    while not stop_event.is_set():
        data = None
        with serial_lock:
            if serial_conn and serial_conn.is_open:
                try:
                    n = serial_conn.in_waiting or 0
                    if n:
                        data = serial_conn.read(n)
                except Exception as e:
                    dbg_print(f"[Serial→TCP] read error: {e}")
                    close_serial()
        if data and tcp_sock:
            try:
                tcp_sock.sendall(data)
                dbg_print(f"[Serial→TCP] {data}")
            except Exception as e:
                dbg_print(f"[Serial→TCP] send error: {e}")
                try: tcp_sock.close()
                except: pass
                tcp_sock = None
        time.sleep(0.01)

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
            with serial_lock:
                if serial_conn and serial_conn.is_open:
                    serial_conn.write(data)
                    dbg_print(f"[TCP→Serial] {data}")
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
    global esp_baud, last_esp_baud

    url = f"http://{config.TCP_SERVER_IP}/get"
    while not stop_event.is_set():
        try:
            r = requests.get(url, timeout=3)
            if r.status_code == 200:
                new_baud = int(r.text.strip())
                if new_baud != esp_baud:
                    dbg_print(f"[HTTP] Baud change {esp_baud} → {new_baud}")
                    esp_baud = new_baud   # monitor thread will reopen
                    dbg_print("[Serial] Reconnecting")
                    close_serial()
                    time.sleep(0.05)  # give OS time to release
                    if open_serial(current_dev, esp_baud):
                        last_esp_baud = esp_baud

        except requests.RequestException:
            pass  # silent if ESP is unreachable
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
        for t in threads:
            t.start()

        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        dbg_print("\n[Main] Stopping...")
        stop_event.set()
        close_serial()
        if tcp_sock:
            try: tcp_sock.close()
            except: pass
