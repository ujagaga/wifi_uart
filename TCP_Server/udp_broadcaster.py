#!/usr/bin/env python3
import socket
import threading
import time

class UDPBroadcaster:
    """
    Periodically send a UDP broadcast message so clients
    can discover the server on the local network.
    """
    def __init__(self, message, port, interval):
        """
        :param message:  Bytes or string to broadcast
        :param port:     UDP port for broadcast
        :param interval: Seconds between broadcasts
        """
        self.message = message.encode() if isinstance(message, str) else message
        self.port = port
        self.interval = interval
        self.stop_event = threading.Event()
        self.thread = threading.Thread(target=self._broadcast_loop, daemon=True)

    def start(self):
        """Start broadcasting in a background thread."""
        self.thread.start()

    def stop(self):
        """Stop broadcasting."""
        self.stop_event.set()
        self.thread.join(timeout=1)

    def _broadcast_loop(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            while not self.stop_event.is_set():
                try:
                    s.sendto(self.message, ("<broadcast>", self.port))
                except Exception as e:
                    print(f"[UDP] Broadcast error: {e}")
                time.sleep(self.interval)
