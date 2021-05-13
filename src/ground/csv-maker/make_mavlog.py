#!/usr/bin/env python3

import sys
import os
import argparse
import struct
from its_logfile import LogfileReader


MAVLINK_DATA_TOPIC = b"radio.downlink_frame"


def iterate_frames(stream):
	with LogfileReader(stream=stream) as reader:
		while True:
			frame = reader.read()
			if not frame:
				break

			frame_timestamp = frame[0]
			topic = frame[1][0]
			if topic != MAVLINK_DATA_TOPIC:
				continue

			payload = frame[1][2]
			yield frame_timestamp, payload


def process_simple(input_stream, output_stream):
	for _, frame_payload in iterate_frames(input_stream):
		output_stream.write(frame_payload)


def process_with_mavlink(input_stream, output_stream, notimestamps):
	from pymavlink.dialects.v20.its import MAVLink, MAVLink_bad_data

	mav = MAVLink(file=None)
	mav.robust_parsing = True
	mavlink_timestamp_struct = struct.Struct(">Q")

	for frame_timestamp, frame_payload in iterate_frames(input_stream):
		for message in mav.parse_buffer(frame_payload) or []:
			if isinstance(message, MAVLink_bad_data):
				continue

			if notimestamps:
				timestamp_bytes = b''
			else:
				usec = int(frame_timestamp * 1.0e6) & -3
				timestamp_bytes = mavlink_timestamp_struct.pack(usec)

			output_stream.write(timestamp_bytes + message.get_msgbuf())


def main(argv):
	arg_parser = argparse.ArgumentParser("broker log to mavlog", add_help=True)
	arg_parser.add_argument("-i", "--input", nargs='?', required=True, dest="input")
	arg_parser.add_argument("-o", "--output", nargs='?', dest="output")
	arg_parser.add_argument("--use-mavlink", action='store_true', dest='use_mavlink')
	arg_parser.add_argument("--notimestamps", action='store_true', dest='notimestamps')

	args = arg_parser.parse_args(argv)
	input_path = args.input
	output_path = args.output
	use_mavlink = args.use_mavlink
	notimestamps = args.notimestamps

	if input_path == "-":
		input_stream = sys.stdin
	else:
		input_stream = open(input_path, mode='rb')

	if not output_path:
		if input_path == "-":
			output_path = "-"
		else:
			output_path = os.path.splitext(input_path)[0] + ".mav"

	if output_path == "-":
		output_stream = sys.stdout
	else:
		output_stream = open(output_path, mode='wb')

	if use_mavlink:
		process_with_mavlink(input_stream, output_stream, notimestamps)
	else:
		process_simple(input_stream, output_stream)

	return 0


if __name__ == "__main__":
	exit(main(sys.argv[1:]))
