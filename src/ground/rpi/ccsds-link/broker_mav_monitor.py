#!/usr/bin/env python3

import zmq
import os

from pymavlink.dialects.v20.its import MAVLink, MAVLink_bad_data


MAVLINK_DATA_TOPIC = b"radio.downlink_frame"


def main():
    ctx = zmq.Context()
    sub_socket = ctx.socket(zmq.SUB)
    
    sub_ep = os.environ["ITS_GBUS_BPCS_ENDPOINT"]
    print("connecting to %s" % sub_ep)
    
    sub_socket.connect(sub_ep)
    sub_socket.setsockopt(zmq.SUBSCRIBE, MAVLINK_DATA_TOPIC)

    mav = MAVLink(file=None)
    mav.robust_parsing = True

    while True:
        msgs = sub_socket.recv_multipart()

        mavlink_payload = msgs[2]
        packets = mav.parse_buffer(mavlink_payload)
        for p in packets or []:
            print(p)

    del ctx
    return 0


if __name__ == "__main__":
    exit(main())
