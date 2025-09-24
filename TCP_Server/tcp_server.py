#!/usr/bin/env python3
import socket
import threading

class TCPServer:
    def __init__(self, host="0.0.0.0", port=5000):
        self.host = host
        self.port = port
        self.server_socket = None
        self.client_socket = None
        self.client_addr = None
        self.stop_event = threading.Event()
        self.lock = threading.Lock()

    def start(self):
        """Start the TCP server in a background thread."""
        t = threading.Thread(target=self._run_server, daemon=True)
        t.start()

    def _run_server(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((self.host, self.port))
            s.listen(1)
            print(f"[TCP] Listening on {self.host}:{self.port}")
            self.server_socket = s
            while not self.stop_event.is_set():
                conn, addr = s.accept()
                with self.lock:
                    self.client_socket = conn
                    self.client_addr = addr
                print(f"[TCP] Client connected: {addr}")
                try:
                    while True:
                        if not conn.recv(1024):  # wait for client disconnect
                            break
                except Exception as e:
                    print(f"[TCP] Error: {e}")
                finally:
                    print("[TCP] Client disconnected")
                    with self.lock:
                        self.client_socket = None
                    conn.close()

    def send(self, data: bytes):
        """Send bytes to the connected client (if any)."""
        with self.lock:
            if self.client_socket:
                try:
                    self.client_socket.sendall(data)
                except Exception as e:
                    print(f"[TCP] Send error: {e}")
                    self.client_socket = None

    def stop(self):
        self.stop_event.set()
        with self.lock:
            if self.client_socket:
                try:
                    self.client_socket.close()
                except:
                    pass
