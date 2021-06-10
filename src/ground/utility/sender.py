#!/usr/bin/env python3

import zmq
import logging
import os
import time
import json


_log = logging.getLogger(__name__)


pub_ep = os.environ["ITS_GBUS_BSCP_ENDPOINT"]

ctx = zmq.Context()
socket = ctx.socket(zmq.PUB)
socket.connect(pub_ep)

time.sleep(1)  # пока сокет соединяется

socket.send_multipart([
	"radio.downlink_frame".encode("utf-8"),
	json.dumps({"cookie": 55}).encode("utf-8"),
	bytes([0x00]*140)
])
