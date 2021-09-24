#!/usr/bin/env python3

import zmq
import logging
import os
import time
import json
import argparse
import io


_log = logging.getLogger(__name__)

BUS_ENDPOINT_ENVVAR = "ITS_GBUS_BSCP_ENDPOINT"


def main(args):
	logging.basicConfig(level=logging.INFO, format='%(asctime)-15s %(levelname)s %(message)s')

	parser = argparse.ArgumentParser("Pa Request Updater", add_help=True)
	parser.add_argument(
		'--bus-endpoint', nargs='?', type=str, help='zmq bus bscp endpoint',
		required=False, dest='bus_endpoint'
	)
	parser.add_argument(
		'power', type=int, nargs='?', help="pa power value in dbm. Allowed values: 10, 14, 17, 20, 22"
	)

	args = parser.parse_args(args)
	power = args.power
	bus_endpoint = args.bus_endpoint

	if not bus_endpoint:
		bus_endpoint = os.environ.get("ITS_GBUS_BSCP_ENDPOINT")

	if not bus_endpoint:
		_log.warn("no bus endpoint specified, using default")
		bus_endpoint = "tcp://127.0.0.1:7777"

	if power not in [10, 14, 17, 20, 22]:
		_log.error("invalid pa power value: \"%s\"" % power)
		stream = io.StringIO()
		parser.print_help(stream)
		_log.error(stream.getvalue())
		return 1

	ctx = zmq.Context()
	socket = ctx.socket(zmq.PUB)


	_log.info("connecting to %s" % bus_endpoint)
	socket.connect(bus_endpoint)
	time.sleep(0.200)  # пока сокет соединяется

	_log.info("sending request for power '%s'" % power)
	socket.send_multipart([
		"radio.sdr_pa_power".encode("utf-8"),
		json.dumps({"pa_power": 10}).encode("utf-8")
	])

	_log.info("done")
	socket.close()
	del ctx
	return 0


import sys
main(sys.argv[1:])
exit()
