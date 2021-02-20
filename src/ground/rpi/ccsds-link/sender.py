import zmq
import logging
import os
import time
import json


_log = logging.getLogger(__name__)


bus_sub = os.environ["ITS_GBUS_SUB_ENDPOINT"]
bus_pub = os.environ["ITS_GBUS_PUB_ENDPOINT"]

ctx = zmq.Context()
socket = ctx.socket(zmq.PUB)
socket.connect(bus_sub)

time.sleep(1)

socket.send_multipart([
	"radio.uplink_frame".encode("utf-8"),
	json.dumps({"cookie": 55}).encode("utf-8"),
	bytes([0x00]*140)
])
