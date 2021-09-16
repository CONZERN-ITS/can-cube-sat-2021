import socket
import struct
import enum
import dataclasses

import json
import zmq


INTERFACE = "127.0.0.1"
PORT = 40868


bus_endpoint = "tcp://192.168.1.223:7777"

def twos_comp(val, bits=8):
    """compute the 2's complement of int value val"""
    if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
        val = val - (1 << bits)        # compute negative value
    return val                         # return positive value as is


class LoraSF(enum.IntEnum):
    SF7: 7
    SF8: 8
    SF9: 9
    SF10: 10
    SF11: 11
    SF12: 12


@dataclasses.dataclass
class LoraTAPHeader:

    lt_version: int
    lt_length: int
    frequency: int
    bandwidth: int
    sf: LoraSF 
    packet_rssi: int
    max_rssi: int
    current_rssi: int
    snr: int

    data: bytes

    def bandwidth_hz(self):
        return self.bandwidth * 125*1000

    def packet_rssi_dbm(self):
        if self.snr_db() > 0:
            return -139 + self.packet_rssi
        else:
            return -139 + (self.packet_rssi / 4)

    def max_rssi_dbm(self):
        return -139 + self.max_rssi

    def current_rssi_dbm(self):
        return -139 + self.current_rssi

    def snr_db(self):
        return self.snr / 4




class LoraTapHeaderParser:

    def __init__(self):
        self.primary_struct = struct.Struct(
            "<"  # little-endian
            "B"  # uint8_t lt_version
            "x"  # uint8_t lt_padding
            "H"  # uin16_t lt_length
        )

        self.channel_struct = struct.Struct(
            "<"
            "L"  # uint32_t frequency
            "B"  # uint8_t bandwidth
            "B"  # uint8_t sf
        )

        self.rssi_struct = struct.Struct(
            "<"
            "4B"  # packet_rssi, max_rssi, current_rssi, snr
        )

        self.syncword_struct = struct.Struct(
            "<"
            "B"  # uint8_t LoRa radio sync word [0x34 = LoRaWAN]
        )

    def parse(self, data: bytes) -> LoraTAPHeader:
        retval = {}

        portion, leftovers = data[0:self.primary_struct.size], data[self.primary_struct.size:]
        version, length = self.primary_struct.unpack(portion)
        retval["lt_version"] = version
        retval["lt_length"] = length

        portion, leftovers = leftovers[0:self.channel_struct.size], data[self.channel_struct.size:]
        frequency, bandwidth, sf = self.channel_struct.unpack(portion)
        retval["frequency"] = frequency 
        retval["bandwidth"] = bandwidth
        retval["sf"] = sf

        portion, leftovers = leftovers[0:self.rssi_struct.size], data[self.channel_struct.size:]
        print(f"portion = {portion}")
        values = self.rssi_struct.unpack(portion)
        
        retval["snr"] = twos_comp(values[3])/4
        if retval["snr"] > 0:
            retval["packet_rssi"] = -139 + values[0]
        else:
            retval["packet_rssi"] = -139 + values[0]/4

        retval["max_rssi"] = -139 + values[1]
        retval["current_rssi"] = -139 + values[2]

        retval["data"] = leftovers

        return LoraTAPHeader(**retval)



sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((INTERFACE, PORT,))

ctx = zmq.Context()
socket = ctx.socket(zmq.PUB)
print("connecting to %s" % bus_endpoint)
socket.connect(bus_endpoint)

parser = LoraTapHeaderParser()

template = b'loraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloraloralora'
good_ones = 0
bad_ones = 0

log_file = "lora_log.bin"
log_stream = open(log_file, mode="wb")

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
    crc = data[-crc_len:]
    print(
        "tap: %s, mac: %s, payload: %s, crc: %s"
        % (tap_bytes.hex(), rssi_bytes.hex(), payload, crc.hex())
    )
    
    if payload == template:
        good_ones += 1
    else:
        bad_ones += 1
    print("good: %s, bad: %s" % (good_ones, bad_ones))


    packet = {'snr' : rssi_bytes[-1], 'cookie': cookie}

    parts = [
        b"sdr.payload", 
        json.dumps(packet).encode("utf-8"),
        payload
    ]

    socket.send_multipart(parts)

