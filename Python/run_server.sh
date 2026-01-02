#!/usr/bin/env bash

# Get the path of the script as it was called (might be a symlink)
SCRIPT_PATH="$BASH_SOURCE"
# Resolve the symlink, if it is one, to get the actual file path
while [ -h "$SCRIPT_PATH" ]; do
  SCRIPT_PATH=$(readlink "$SCRIPT_PATH")
done
# Get the directory of the resolved script path
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

# Delete any existing WiFi hotspots. I failed to persist the AP settings, so creating it each time at startup.
for conn in $(nmcli -t -f NAME connection show | grep '^Hotspot'); do
    nmcli connection delete "$conn"
done

# Create the hotspot
nmcli dev wifi hotspot \
  ifname wlan0 \
  ssid UART-TCP-Bridge-01 \
  password 12345678

# Run the server script located in this same folder
/usr/bin/python3 $SCRIPT_DIR/uart_tcp_server.py
