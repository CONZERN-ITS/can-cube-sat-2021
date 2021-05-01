import zmq
import logging
import os
import time
import json


_log = logging.getLogger(__name__)


pub_ep = os.environ["ITS_GBUS_BSCP_ENDPOINT"]
sub_ep = os.environ["ITS_GBUS_BPCS_ENDPOINT"]

ctx = zmq.Context()
socket = ctx.socket(zmq.PUB)
socket.connect(sub_ep)

time.sleep(1)

socket.send_multipart([
	"radio.uplink_frame".encode("utf-8"),
	json.dumps({"cookie": 55}).encode("utf-8"),
	bytes([0x00]*140)
])
