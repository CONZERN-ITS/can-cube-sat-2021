import os
import logging
import argparse
import datetime

import zmq


from its_logfile import LogfileWriter


ITS_GBUS_BPCS_ENDPOINT = "tcp://0.0.0.0:7778"
ITS_GBUS_BSCP_ENDPOINT = "tcp://0.0.0.0:7777"


_log = logging.getLogger(__name__)


def generate_logfile_name():
    now = datetime.datetime.utcnow().replace(microsecond=0)
    isostring = now.isoformat()  # строка вида 2021-04-27T23:17:31
    isostring = isostring.replace("-", "")  # Строка вида 20210427T23:17:31
    isostring = isostring.replace(":", "")  # Строка вида 20210427T231731, то что надо
    return "its-broker-log-" + isostring + ".log"


def main():
    # TODO: Сделать красивую остановку по ctrl+c
    ctx = zmq.Context()

    logfile_name = generate_logfile_name()
    _log.info("using logfile \"%s\"", logfile_name)
    logfile_writer = LogfileWriter(logfile_name)

    bus_sub = ITS_GBUS_BSCP_ENDPOINT
    bus_pub = ITS_GBUS_BPCS_ENDPOINT

    sub_socket, pub_socket = ctx.socket(zmq.SUB), ctx.socket(zmq.PUB)

    _log.info("binding sub to %s", bus_sub)
    _log.info("binding pub to %s", bus_pub)
    sub_socket.bind(bus_sub)
    pub_socket.bind(bus_pub)

    # Подписываемся на все на sub сокете
    sub_socket.setsockopt_string(zmq.SUBSCRIBE, "")

    poller = zmq.Poller()
    poller.register(sub_socket, zmq.POLLIN)
    while True:
        events = dict(poller.poll(1000))
        if sub_socket in events:
            msgs = sub_socket.recv_multipart()
            _log.info("got messages: %r", msgs)
            logfile_writer.write(msgs)
            pub_socket.send_multipart(msgs)

    del ctx
    logfile_writer.close()
    return 0


if __name__ == "__main__":
    logging.basicConfig(
        # format='%(asctime)-15s %(message)s',
        format='%(message)s', # для systemd лога таймштампы только мешают
        level=logging.INFO
    )

    exit(main())
