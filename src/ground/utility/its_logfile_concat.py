#!/usr/bin/env python3

from argparse import ArgumentParser
from its_logfile import LogfileWriter, LogfileReader
from contextlib import ExitStack

def concat(output_stream, *input_streams):
    writer = LogfileWriter(stream=output_stream)
    readers = [LogfileReader(stream=stream) for stream in input_streams]
    for reader in readers:
        for record in reader.read_all():
            timestamp, zmq_multipart = record
            writer.write(zmq_multipart, timestamp=timestamp)



def main(argv):
    parser = ArgumentParser("its logfile concatenator", add_help=True)
    parser.add_argument("-i,--input", nargs='+', type=str, dest="inputs")
    parser.add_argument("-o,--output", nargs='?', type=str, dest="output")

    args = parser.parse_args(argv)
    
    input_files = args.inputs
    output_file = args.output

    with ExitStack() as stack:
        output_stream = stack.enter_context(open(output_file, mode="wb"))
        input_streams = [stack.enter_context(open(f, mode="rb")) for f in input_files]

        concat(output_stream, *input_streams)


if __name__ == "__main__":
    import sys
    argv = sys.argv[1:]
    main(argv)
