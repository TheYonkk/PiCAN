#!/usr/bin/env python3

from power_api import SixfabPower
import socket # for hostname
import json

api = SixfabPower()

data = {
    "battery_temp": float(api.get_battery_temp()),
    "battery_voltage": float(api.get_battery_voltage()),
    "battery_current": float(api.get_battery_current()),
    "battery_power": float(api.get_battery_power()),
    "battery_level": float(api.get_battery_level()),
    "battery_health": float(api.get_battery_health()),
    "fan_health": float(api.get_fan_health()),
    "fan_speed": float(api.get_fan_speed()),
    "host": str(socket.gethostname())
}

print(json.dumps(data))





