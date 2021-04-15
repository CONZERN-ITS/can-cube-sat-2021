import os
os.environ['MAVLINK_DIALECT'] = "its"
os.environ['MAVLINK20'] = "its"
from pymavlink.dialects.v20 import its as its_mav
from pymavlink import mavutil
import time
import re
import math
import numpy as NumPy
import struct
import zmq

from source.functions.wgs84 import wgs84_xyz_to_latlonh as wgs84_conv

def generate_log_file_name(log_type='tlm'):
    if log_type == 'tlm':
        prefix = 'telemetry_'
    elif log_type == 'cmd':
        prefix = 'command_'
    else:
        prefix = ''
    return time.strftime(prefix + "AMCS_log_%d-%m-%Y_%H-%M-%S", time.localtime())


class MAVDataSource():
    def __init__(self, connection_str_in, connection_str_out, log_path="./"):
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
        self.log.write(struct.pack("<Q", time.time()))
        self.log.write(msg.get_msgbuf())

        return msg

    def write_data(self, msg):
        self.connection_out.mav.send(msg, False)

    def stop(self):
        self.connection.close()
        self.log.close()


class ZMQDataSource():
    def __init__(self, bus_pub="tcp://127.0.0.1:7778", bus_sub="tcp://127.0.0.1:7777", topics=[], log_path="./"):
        self.bus_pub = bus_pub
        self.bus_sub = bus_sub
        self.topics = topics
        self.log_path = log_path

    def start(self):
        self.zmq_ctx = zmq.Context()

        self.sub_socket = self.zmq_ctx.socket(zmq.SUB)
        self.sub_socket.connect(self.bus_pub)
        for topic in self.topics:
            self.sub_socket.setsockopt_string(zmq.SUBSCRIBE, topic)


        self.pub_socket = self.zmq_ctx.socket(zmq.PUB)
        self.pub_socket.connect(self.bus_sub)

        self.poller = zmq.Poller()
        self.poller.register(self.sub_socket, zmq.POLLIN)

    def read_data(self):
        events = dict(self.poller.poll(1000))
        if self.sub_socket in events:
            return self.sub_socket.recv_multipart()
        else:
            raise RuntimeError("No Message")

    def write_data(self, msg):
        self.pub_socket.send_multipart(msg)


    def stop(self):
        self.sub_socket.close()
        self.pub_socket.close()
