#!/usr/bin/env python3

import collections
import json
import sys
import os
import argparse
import csv
import datetime

from its_logfile import LogfileReader


def timestamp_gregorian(timestamp):
    dt = datetime.datetime.fromtimestamp(timestamp)
    ts_text = dt.strftime("%Y-%m-%dT%H:%M:%S")
    return ts_text


def is_flat_dict(record_payload: dict):
    if not isinstance(record_payload, dict):
        return False

    for value in record_payload.values():
        if not isinstance(value, (int, float, str,)):
            return False

    return True


def get_record_timestamp(record):
    if "time_s" and "time_us" in record.keys():
        return float(record["time_s"]) + float(record["time_us"]) / 10**6
    else:
        return None


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
    arg_parser = argparse.ArgumentParser("broker log to mdt jsons and csvs", add_help=True)
    arg_parser.add_argument("-i", "--input", nargs='?', required=False, dest="input")
    arg_parser.add_argument("-o", "--output-dir", nargs='?', dest="output_dir")
    arg_parser.add_argument("--csv", dest="csv", action='store_true')
    arg_parser.add_argument("--json", dest="json", action='store_true')

    args = arg_parser.parse_args(argv)
    input_path = args.input
    output_dir = args.output_dir
    make_csvs = args.csv
    make_jsons = args.json

    if input_path == "-":
        input_stream = sys.stdin
    else:
        input_stream = open(input_path, mode='rb')

    if not output_dir:
        if input_path == "-":
            output_dir = os.getcwd()
        else:
            output_dir = os.path.splitext(input_path)[0] + "_mdt"

    os.makedirs(output_dir, exist_ok=True)

    gatherer = MdtGatherer()
    with LogfileReader(stream=input_stream) as log_reader:
        while True:
            record = log_reader.read()
            if not record:
                break

            gatherer.process_record(record)

    for topic, data in gatherer.accumulators.items():
        if make_jsons:
            json_file_path = os.path.join(output_dir, topic + ".json")
            with open(json_file_path, mode="w", encoding="utf-8") as stream:
                json.dump(data, stream, ensure_ascii=False, indent=4, sort_keys=True)

        print("%d records for topic %s" % (len(data), topic))

        if make_csvs:
            if not data:
                continue

            csv_file_path = os.path.join(output_dir, topic + ".csv")
            with open(csv_file_path, mode="w", encoding="utf-8") as stream:
                writer = None
                fieldnames = set([
                    "log_timestamp",
                    "log_timestamp_gregorian",
                ])

                for record in data:
                    record_payload = record["data"]
                    if is_flat_dict(record_payload):
                        fieldnames.update(record_payload.keys())
                        if get_record_timestamp(record_payload) is not None:
                            fieldnames.add("time_gregorian")

                    else:
                        fieldnames.add("unparsed")

                writer = csv.DictWriter(stream, fieldnames=list(fieldnames))
                writer.writeheader()

                for record in data:
                    record_timestamp = record["time"]
                    record_payload = record["data"]

                    row = {
                        "log_timestamp": record_timestamp,
                        "log_timestamp_gregorian": timestamp_gregorian(record_timestamp),
                    }
                    if not is_flat_dict(record_payload):
                        row["unparsed"] = record_payload
                    else:
                        row.update(record_payload)
                        record_timestamp = get_record_timestamp(record_payload)
                        if record_timestamp is not None:
                            row["time_gregorian"] = record_timestamp

                    writer.writerow(row)

if __name__ == "__main__":
    exit(main(sys.argv[1:]))
