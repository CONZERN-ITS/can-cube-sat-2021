import zmq
import logging
import os


_log = logging.getLogger(__name__)


def main():
    ctx = zmq.Context()
    sub_socket = ctx.socket(zmq.SUB)

    bus_pub = os.environ["ITS_GBUS_PUB_ENDPOINT"]
    _log.info("connecting to %s", bus_pub)
    sub_socket.connect(bus_pub)
    sub_socket.setsockopt_string(zmq.SUBSCRIBE, "radio.uplink_state")

    while True:
        msgs = sub_socket.recv_multipart()
        _log.info("got messsages: %s", msgs)

    del ctx
    return 0


if __name__ == "__main__":
    logging.basicConfig(
        format='%(asctime)-15s %(message)s',
        level=logging.INFO
    )

    exit(main())
