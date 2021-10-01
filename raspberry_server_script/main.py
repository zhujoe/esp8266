#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import paho.mqtt.client as mqtt
import os
import psutil
import json
import socket
import time
import random

mqttServerIP = "175.24.88.178"  #"192.168.31.127"
mqttServerPort = 1883
mqttKeepAlive = 600
mqttTopic = "cube/dashboard"
clinet_id = 'python-mqtt-pc-' + str(random.randint(0, 1000))
alertCPUTempMax = 70
alertDiskUsagePercentMax = 95
alertCPUpercent = 95.0
sleeptime = 2

def on_disconnect(client, userdata, rc):
    print("断开连接，状态码" + str(rc))
    client.reconnect_delay_set(min_delay=1, max_delay=10)

def on_connect(client, userdata, flags, rc):
    print("已连接，状态码 " + str(rc))

def on_message(client, userdata, msg):
    data = msg.payload.decode()
    if (data == "reboot"):
        # print("reboot")
        os.popen("sudo reboot")
    elif (data == "shutdown"):
        # print("shutdown")
        os.popen("sudo shutdown now")
    # else:
        # print(data)
    
client = mqtt.Client(clinet_id)
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect
client.connect(mqttServerIP, mqttServerPort, mqttKeepAlive)
client.subscribe(mqttTopic)
client.loop_start()

time.sleep(2)

data_old = {
    "net_io_counters": {
        "bytes_sent": psutil.net_io_counters().bytes_sent,
        "bytes_recv": psutil.net_io_counters().bytes_recv
    }
}
while True:
    cpu_temperature = -128
    ip = "127.0.0.1"
    host_name = "localhost"
    try:
        ip = os.popen('ifconfig wlan0| grep "inet "').read().split()[1]
        # ip = socket.getaddrinfo()
        host_name = socket.gethostname()
        cpu_percent = psutil.cpu_percent()
        cpu_temperature = (int(os.popen('cat /sys/class/thermal/thermal_zone0/temp').read())) / 1000
    except:
        ip = os.popen('ifconfig eth0| grep "inet "').read().split()[1]
        # ip = socket.getaddrinfo()
        host_name = socket.gethostname()
        cpu_percent = psutil.cpu_percent()
        cpu_temperature = (int(os.popen('cat /sys/class/thermal/thermal_zone0/temp').read())) / 1000
    alert = 0
    if cpu_temperature > alertCPUTempMax:
        alert = 1
    if psutil.disk_usage("/").percent > alertDiskUsagePercentMax:
        alert = 1
    if cpu_percent > alertCPUpercent:
        alert = 1
    data = {
        "cpu_percent": cpu_percent,
        # "cpu_times_percent": {
        #     "user": psutil.cpu_times_percent().user,
        #     "nice": psutil.cpu_times_percent().nice,
        #     "system": psutil.cpu_times_percent().system,
        #     "idle": psutil.cpu_times_percent().idle
        # },
        "cpu_count": psutil.cpu_count(),
        "cpu_freq": {
            "current": psutil.cpu_freq().current,
            # "min": psutil.cpu_freq().min,
            # "max": psutil.cpu_freq().max
        },
        "virtual_memory": {
            "total": psutil.virtual_memory().total/1024/1024,
            # "available": psutil.virtual_memory().available/1024/1024,
            "percent": psutil.virtual_memory().percent,
            # "used": psutil.virtual_memory().used/1024/1024,
            "free": psutil.virtual_memory().free/1024/1024,
            # "active": psutil.virtual_memory().active/1024/1024,
            # "inactive": psutil.virtual_memory().inactive/1024/1024,
        },
        "swap_memory": {
            "total": psutil.swap_memory().total/1024/1024,
            # "used": psutil.swap_memory().used/1024/1024,
            "free": psutil.swap_memory().free/1024/1024,
            "percent": psutil.swap_memory().percent,
            # "sin": psutil.swap_memory().sin/1024/1024,
            # "sout": psutil.swap_memory().sout/1024/1024
        },
        "disk_usage": {
            "total": psutil.disk_usage("/").total/1024/1024,
            # "used": psutil.disk_usage("/").used/1024/1024,
            "free": psutil.disk_usage("/").free/1024/1024,
            "percent": psutil.disk_usage("/").percent
        },
        "net_io_counters": {
            "bytes_sent": psutil.net_io_counters().bytes_sent/1024/1024,
            "bytes_recv": psutil.net_io_counters().bytes_recv/1024/1024,
            # "packets_sent": psutil.net_io_counters().packets_sent,
            # "packets_recv": psutil.net_io_counters().packets_recv,
            # "errin": psutil.net_io_counters().errin,
            # "errout": psutil.net_io_counters().errout,
            # "dropin": psutil.net_io_counters().dropin,
            # "dropout": psutil.net_io_counters().dropout,
            # "sent": psutil.net_io_counters().bytes_sent/1024/1024,
            # "recv": psutil.net_io_counters().bytes_recv/1024/1024,
        },
        "up_time": int(time.time() - psutil.boot_time()),
        "ip": ip,
        "cpu_temperature": cpu_temperature,
        "host_name": host_name,
        "net_io_speed": {
            "recv": (psutil.net_io_counters().bytes_recv/1024/1024 - data_old.get("net_io_counters").get("bytes_recv")) / sleeptime,
            "sent": (psutil.net_io_counters().bytes_sent/1024/1024 - data_old.get("net_io_counters").get("bytes_sent")) / sleeptime
        },
        "time": {
            # "timestamp": int(time.time()),
            "timestamp_cst": int(time.time()) + 3600 * 8,
            "tm_year": time.localtime().tm_year,
            "tm_mon": time.localtime().tm_mon,
            "tm_mday": time.localtime().tm_mday,
            "tm_hour": time.localtime().tm_hour,
            "tm_min": time.localtime().tm_min,
            "tm_sec": time.localtime().tm_sec,
            "tm_wday": time.localtime().tm_wday,
            "tm_yday": time.localtime().tm_yday,
            # "tm_isdst": time.localtime().tm_isdst
        },
        "alert": alert
    }
    data_old = data
    client.publish(mqttTopic, payload = json.dumps(data), qos = 0)
    # print(json.dumps(data, sort_keys = False, indent = 4, separators = (',', ': ')))   
    time.sleep(sleeptime)
    # client.loop()
