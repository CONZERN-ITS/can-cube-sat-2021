#!/usr/bin/env python3

import collections
import json
import sys
import os
import argparse
from its_logfile import LogfileReader


class MdtGatherer:
	def __init__(self):
		self.accumulators = collections.defaultdict(lambda: [])

	def process_record(self, log_record):
		time_of_rcv = log_record[0]
		topic = log_record[1][0].decode("utf-8")
		meta = log_record[1][1]

		accumulator = self.accumulators[topic]
		# Попытаемся разобрать это сообщение как валидный джсон
		# Теоретически они все должны быть валидными джсонами.....
		# однако на практике... не всегда
		try:
			parsed_meta = json.loads(meta)
		except (ValueError, Exception):
			parsed_meta = meta.decode("utf-8")

		accum_record = {
			"time": time_of_rcv,
			"data": parsed_meta,
		}
		accumulator.append(accum_record)


def main(argv):
	arg_parser = argparse.ArgumentParser("broker log to mdt jsons", add_help=True)
	arg_parser.add_argument("-i", "--input", nargs='?', required=False, dest="input", default="-")
	arg_parser.add_argument("-o", "--output-dir", nargs='?', dest="output_dir")

	args = arg_parser.parse_args(argv)
	input_path = args.input
	output_dir = args.output_dir

	if input_path == "-":
		input_stream = sys.stdin
	else:
		input_stream = open(input_path, mode='rb')

	if not output_dir:
		if input_path == "-":
			output_dir = os.getcwd()
		else:
			output_dir = os.path.splitext(input_path)[0] + "_mdt_csv"

	os.makedirs(output_dir, exist_ok=True)

	gatherer = MdtGatherer()
	with LogfileReader(stream=input_stream) as log_reader:
		while True:
			record = log_reader.read()
			if not record:
				break

			gatherer.process_record(record)

	for topic, data in gatherer.accumulators.items():
		file_path = os.path.join(output_dir, topic + ".json")
		with open(file_path, mode="w", encoding="utf-8") as stream:
			json.dump(data, stream, ensure_ascii=False, indent=4, sort_keys=True)

		print("%d records for topic %s" % (len(data), topic))


if __name__ == "__main__":
	exit(main(sys.argv[1:]))
