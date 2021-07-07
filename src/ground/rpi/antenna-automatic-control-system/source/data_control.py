import os
os.environ['MAVLINK_DIALECT'] = "its"
os.environ['MAVLINK20'] = "its"
from pymavlink.dialects.v20 import its as its_mav
from pymavlink import mavutil
import zmq
import time

def generate_log_file_name(log_type='tlm'):
    if log_type == 'tlm':
        prefix = 'telemetry_'
    elif log_type == 'cmd':
        prefix = 'command_'
    else:
        prefix = ''
    return time.strftime(prefix + "AMCS_log_%d-%m-%Y_%H-%M-%S", time.localtime())


class AbstractDataSource():
    def start(self):
        pass

    def read(self):
        pass

    def write(self, msg):
        pass

    def stop(self):
        pass


class MAVDataSource(AbstractDataSource):
    def __init__(self, connection_str_in, connection_str_out, log_path="./log/"):
        self.connection_str_in = connection_str_in
        self.connection_str_out = connection_str_out
        self.log_path = log_path

    def start(self):
        self.connection_in = mavutil.mavlink_connection(self.connection_str_in)
        self.connection_out = mavutil.mavlink_connection(self.connection_str_out)
        self.telemetry_log = open(self.log_path + generate_log_file_name() + '.mav', 'wb')
        self.command_log = open(self.log_path + generate_log_file_name() + '.mav', 'wb')

    def read_data(self):
        msg = self.connection_in.recv_match()
        if (msg is None):
            raise RuntimeError("No Message")
        self.telemetry_log.write(struct.pack("<Q", time.time()))
        self.telemetry_log.write(msg.get_msgbuf())

        return msg

    def write_data(self, msg):
        self.connection_out.mav.send(msg, False)
        self.command_log.write(struct.pack("<Q", time.time()))
        self.command_log.write(buf.get_msgbuf())

    def stop(self):
        self.connection_in.close()
        self.connection_out.close()
        self.telemetry_log.close()
        self.command_log.close()


class ZMQDataSource(AbstractDataSource):
    def __init__(self, bus_bpcs="tcp://127.0.0.1:7778", bus_bscp="tcp://127.0.0.1:7777", topics=[], timeout=1000):
        self.bus_bpcs = bus_bpcs
        self.bus_bscp = bus_bscp
        self.topics = topics
        self.timeout = timeout

    def start(self):
        self.zmq_ctx = zmq.Context()

        self.sub_socket = self.zmq_ctx.socket(zmq.SUB)
        self.sub_socket.connect(self.bus_bpcs)
        for topic in self.topics:
            self.sub_socket.setsockopt_string(zmq.SUBSCRIBE, topic)

        self.pub_socket = self.zmq_ctx.socket(zmq.PUB)
        self.pub_socket.connect(self.bus_bscp)

        self.poller = zmq.Poller()
        self.poller.register(self.sub_socket, zmq.POLLIN)

    def read_data(self):
        events = dict(self.poller.poll(self.timeout))
        if self.sub_socket in events:
            return self.sub_socket.recv_multipart()
        else:
            raise RuntimeError("No Message")

    def write_data(self, msg):
        self.pub_socket.send_multipart(msg)

    def stop(self):
        self.sub_socket.close()
        self.pub_socket.close()