#!/usr/bin/env python3

import zmq
import logging
import os
import time
import json


_log = logging.getLogger(__name__)


# pub_ep = os.environ["ITS_GBUS_BSCP_ENDPOINT"]
bus_endpoint = "tcp://192.168.1.223:7777"

ctx = zmq.Context()
socket = ctx.socket(zmq.PUB)
print("connecting to %s" % bus_endpoint)
socket.connect(bus_endpoint)

time.sleep(1)  # пока сокет соединяется

socket.send_multipart([
	"radio.sdr_pa_power".encode("utf-8"),
	json.dumps({"pa_power": 10}).encode("utf-8")
])

time.sleep(1)