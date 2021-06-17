#!/usr/bin/env python3

import json
import sys
import argparse
import datetime
from its_logfile import LogfileReader


def format_time(timestamp: int) -> str:
	dt = datetime.datetime.fromtimestamp(timestamp)
	return dt.isoformat()


class ChronoProcessor:

	def __init__(self):
		self.record_no = 0

		self.cookie_in_wait = None
		self.cookie_in_progress = None
		self.cookie_sent = None
		self.cookie_dropped = None

	def process_uplink_request(self, log_record):
		time_of_rcv = log_record[0]
		meta = log_record[1][1]
		parsed_meta = json.loads(meta)

		cookie = parsed_meta["cookie"]
		print(f"{format_time(time_of_rcv)}: {self.record_no:08d}: uplink request; cookie {cookie}")

	def process_uplink_state(self, log_record):
		time_of_rcv = log_record[0]
		meta = log_record[1][1]
		parsed_meta = json.loads(meta)

		cookie_in_wait = parsed_meta["cookie_in_wait"]
		cookie_in_progress = parsed_meta["cookie_in_progress"]
		cookie_sent = parsed_meta["cookie_sent"]
		cookie_dropped = parsed_meta["cookie_dropped"]

		if self.cookie_in_wait != cookie_in_wait:
			print(
				f"{format_time(time_of_rcv)}: "
				f"{self.record_no:08d} :"
				f"cookie in wait changed from {self.cookie_in_wait} to {cookie_in_wait}"
			)

		if self.cookie_in_progress != cookie_in_progress:
			print(
				f"{format_time(time_of_rcv)}: "
				f"{self.record_no:08d} :"
				f"cookie in progress changed from {self.cookie_in_progress} to {cookie_in_progress}"
			)

		if self.cookie_sent != cookie_sent:
			print(
				f"{format_time(time_of_rcv)}: "
				f"{self.record_no:08d} :"
				f"cookie sent changed from {self.cookie_sent} to {cookie_sent}"
			)

		if self.cookie_dropped != cookie_dropped:
			print(
				f"{format_time(time_of_rcv)}: "
				f"{self.record_no:08d} :"
				f"cookie dropped changed from {self.cookie_dropped} to {cookie_dropped}"
			)

		self.cookie_in_wait = cookie_in_wait
		self.cookie_in_progress = cookie_in_progress
		self.cookie_sent = cookie_sent
		self.cookie_dropped =cookie_dropped

	def process_record(self, log_record):
		topic = log_record[1][0].decode("utf-8")
		if topic == "radio.uplink_frame":
			self.process_uplink_request(log_record)
		elif topic == "radio.uplink_state":
			self.process_uplink_state(log_record)

		self.record_no += 1


def main(argv):
	arg_parser = argparse.ArgumentParser("broker log to mdt jsons", add_help=True)
	arg_parser.add_argument("-i", "--input", nargs='?', required=False, dest="input")

	args = arg_parser.parse_args(argv)
	input_path = args.input

	if input_path == "-":
		input_stream = sys.stdin
	else:
		input_stream = open(input_path, mode='rb')

	processor = ChronoProcessor()
	with LogfileReader(stream=input_stream) as log_reader:
		while True:
			record = log_reader.read()
			if not record:
				break

			processor.process_record(record)


if __name__ == "__main__":
	exit(main(sys.argv[1:]))
