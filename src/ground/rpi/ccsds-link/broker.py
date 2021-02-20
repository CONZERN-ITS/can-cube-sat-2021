import zmq
import logging
import os


# XSUB_ENDPOINT = "tcp://0.0.0.0:7777"
# XPUB_ENDPOINT = "tcp://0.0.0.0:7778"


_log = logging.getLogger(__name__)


def main():
    ctx = zmq.Context()

    bus_sub = os.environ["ITS_GBUS_SUB_ENDPOINT"]
    bus_pub = os.environ["ITS_GBUS_PUB_ENDPOINT"]

    sub_socket, pub_socket = ctx.socket(zmq.XSUB), ctx.socket(zmq.XPUB)

    _log.info("binding xsub to %s", bus_sub)
    _log.info("binding xpub to %s", bus_pub)
    sub_socket.bind(bus_sub)
    pub_socket.bind(bus_pub)

    poller = zmq.Poller()
    poller.register(pub_socket, zmq.POLLIN)
    poller.register(sub_socket, zmq.POLLIN)
    while True:
        events = dict(poller.poll(1000))
        if pub_socket in events:
            msgs = pub_socket.recv_multipart()
            _log.info("pub mesages %r", msgs)
            sub_socket.send_multipart(msgs)
        if sub_socket in events:
            msgs = sub_socket.recv_multipart()
            _log.info("sub messages: %r", msgs)
            pub_socket.send_multipart(msgs)

    del ctx
    return 0


if __name__ == "__main__":
    logging.basicConfig(
        format='%(asctime)-15s %(message)s',
        level=logging.INFO
    )

    exit(main())
