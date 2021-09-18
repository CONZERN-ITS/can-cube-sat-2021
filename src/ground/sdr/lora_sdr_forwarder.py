import socket
import struct
import enum
import datetime
import dataclasses

import json
import zmq
import time


INTERFACE = "127.0.0.1"
PORT = 40868

bus_endpoint = "tcp://192.168.1.223:7777"


def generate_logfile_name():
    now = datetime.datetime.utcnow().replace(microsecond=0)
    isostring = now.isoformat()  # строка вида 2021-04-27T23:17:31
    isostring = isostring.replace("-", "")  # Строка вида 20210427T23:17:31
    isostring = isostring.replace(":", "")  # Строка вида 20210427T231731, то что надо
    return "sdr-lora-log-" + isostring + ".bin"


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((INTERFACE, PORT,))

ctx = zmq.Context()
socket = ctx.socket(zmq.PUB)
print("connecting to %s" % bus_endpoint)
socket.connect(bus_endpoint)

log_file = generate_logfile_name()
log_stream = open(log_file, mode="wb")

cookie = 1

while True:
    data, port = sock.recvfrom(4096)
    log_stream.write(struct.pack("<L", len(data)) + data)
    log_stream.flush()

    lora_tap_len = 13 # 13 байт на непонятные нули
    lora_mac_len = 5  # пять байт на что-то, что постоянно меняется
    crc_len = 2 # 2 байта, на что-то типо контрольной суммы?

    tap_bytes = data[:lora_tap_len]
    rssi_bytes = data[lora_tap_len:][:lora_mac_len]
    payload = data[lora_tap_len + lora_mac_len:][:-crc_len]
    crc_bytes = data[-crc_len:]
    print(
        "tap: %s, mac: %s, payload: %s, crc: %s"
        % (tap_bytes.hex(), rssi_bytes.hex(), payload.hex(), crc_bytes.hex())
    )

    frame_no = payload[:2]
    frame_no, = struct.unpack("<H", frame_no)
    payload = payload[2:]

    now = time.time()
    packet = {
        'time_s': int(now),
        'time_us': int((now - int(now)) * 1000_0000),
        'cookie': cookie,
        'snr' : rssi_bytes[0],
        'frame_no': frame_no,

        'tap_bytes': tap_bytes.hex(),
        'rssi_bytes': rssi_bytes.hex(),
        'crc_bytes': crc_bytes.hex(),
    }

    parts = [
        b"sdr.downlink_frame", 
        json.dumps(packet).encode("utf-8"),
        payload
    ]

    socket.send_multipart(parts)
    cookie += 1
    cookie &= 0xFFFF_FFFF_FFFF_FFFF
    if cookie == 0:
        cookie = 1

