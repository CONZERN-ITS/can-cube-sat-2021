#!/usr/bin/env python3

import zmq
import os
import json
import enum


TX_STATE_TOPIC = b"radio.uplink_state"
FRAME_SIZE = 200


def send_payload(socket, cookie: int, payload: bytes):
    meta = {"cookie": cookie}
    parts = [
        b"radio.uplink_frame",
        json.dumps(meta).encode("utf-8"),
        payload
    ]

    socket.send_multipart(parts)


def main():
    ctx = zmq.Context()
    sub_socket = ctx.socket(zmq.SUB)
    sub_ep = os.environ["ITS_GBUS_BPCS_ENDPOINT"]
    print("connecting sub to %s" % sub_ep)
    sub_socket.connect(sub_ep)
    sub_socket.setsockopt(zmq.SUBSCRIBE, TX_STATE_TOPIC)

    pub_ep = os.environ["ITS_GBUS_BSCP_ENDPOINT"]
    print("connecting pub to %s" % pub_ep)
    pub_socket = ctx.socket(zmq.PUB)
    pub_socket.connect(pub_ep)


    cookie = 0
    serial = 0
    while True:
        msgs = sub_socket.recv_multipart()

        meta = json.loads(msgs[1])
        in_wait = meta["cookie_in_wait"]
        in_progress = meta["cookie_in_progress"]
        sent = meta["cookie_sent"]
        dropped = meta["cookie_dropped"]

        print(
            f"in wait: %s, in in_progress: %s, sent: %s, dropped: %s"
            % (in_wait, in_progress, sent, dropped)
        )

        if in_wait is not None and int(in_wait) == cookie:
            continue

        cookie = (cookie + 1) & 0xFFFFFFFFFFFFFFFF

        packet = b''
        for counter in range(0, FRAME_SIZE):
            packet += bytes([serial])
            serial = (serial + 1) & 0xFF

        print("sending packet: %s" % cookie)
        send_payload(pub_socket, cookie, packet)

    del ctx
    return 0


if __name__ == "__main__":
    exit(main())
