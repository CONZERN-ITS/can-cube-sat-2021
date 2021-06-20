import os
import argparse
import time

import zmq

from its_logfile import LogfileReader


bus_endpoint = os.environ["ITS_GBUS_BSCP_ENDPOINT"]  # мы публикуем, брокер подписывается


def main(argv):
	parser = argparse.ArgumentParser("its log player")
	parser.add_argument("-i,--input,--log-file", nargs="?", type=str, dest="log_file", required=True)
	parser.add_argument("--speed", nargs="?", type=float, dest="speed", default=1.0)

	args = parser.parse_args(argv)
	filepath = args.log_file
	speed_factor = 1/args.speed

	ctx = zmq.Context()
	socket = ctx.socket(zmq.PUB)
	print("connecting to %s" % bus_endpoint)
	socket.connect(bus_endpoint)
	time.sleep(0.1) # нужно чуточку поспать пока сокеты соединяются

	print("playing file \"%s\"" % filepath)
	last_msg_time = None
	with LogfileReader(filepath) as reader:
		while True:
			time_and_msg = reader.read()
			if not time_and_msg:
				break

			msg_time, msg = time_and_msg
			if last_msg_time is not None:
				# Спим, чтобы отправлять сообщения в том же темпе как и отправитель
				to_sleep = msg_time - last_msg_time
				to_sleep = to_sleep * speed_factor
				print("sleeping for %s seconds" % to_sleep)
				time.sleep(to_sleep)

			print("sending %s: %s" % (msg_time, msg))
			socket.send_multipart(msg)
			last_msg_time = msg_time


if __name__ == "__main__":
	import sys
	exit(main(sys.argv[1:]))
