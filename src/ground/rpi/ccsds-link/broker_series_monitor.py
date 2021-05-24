#!/usr/bin/env python3

import zmq
import os
import json


MAVLINK_DATA_TOPIC = b"radio.downlink_frame"
RSSI_TOPIC = b"radio.packet_rssi"


def main():
    ctx = zmq.Context()
    sub_socket = ctx.socket(zmq.SUB)
    
    sub_ep = os.environ["ITS_GBUS_BPCS_ENDPOINT"]
    print("connecting to %s" % sub_ep)
    
    sub_socket.connect(sub_ep)
    sub_socket.setsockopt(zmq.SUBSCRIBE, MAVLINK_DATA_TOPIC)
    sub_socket.setsockopt(zmq.SUBSCRIBE, RSSI_TOPIC)

    prev_byte = 0x00
    while True:
        msgs = sub_socket.recv_multipart()

        if msgs[0] == MAVLINK_DATA_TOPIC:
            packet_data = msgs[2]
            
            packet_good = True
            string = ""
            for byte in packet_data:
                if byte == (prev_byte + 1) & 0xFF:
                    string += "%02X" % byte
                else:
                    string += "!%02X" % byte
                    packet_good = False

                prev_byte = byte
            meta = json.loads(msgs[1])
            crc_valid = meta["checksum_valid"]
            packet_rssi = meta["rssi_pkt"]
            snr = meta["snr_pkt"]
            signal_rssi = meta["rssi_signal"]

            print(
                "got_packet: crc: %s, seq: %s, p_rssi: %s, snr: %s, srssi: %s -> %s" 
                % 
                (crc_valid, packet_good, packet_rssi, snr, signal_rssi, string)
            )

    del ctx
    return 0


if __name__ == "__main__":
    exit(main())
