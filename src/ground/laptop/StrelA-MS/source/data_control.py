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

def generate_log_file_name():
  return time.strftime("StrelA-MS_log_%d-%m-%Y_%H-%M-%S", time.localtime())


class Message():
    def __init__(self, message_id, source_id, msg_time, msg_data):
        self.message_id = message_id
        self.source_id = source_id
        self.data = msg_data
        self.time = msg_time
        self.creation_time = time.time()

    def get_message_id(self):
        return self.message_id

    def get_source_id(self):
        return self.source_id

    def get_time(self):
        return self.time

    def get_data_dict(self):
        return self.data

    def get_creation_time(self):
        return self.creation_time


class MAVDataSource():
    def __init__(self, connection_str, log_path="./"):
        self.connection_str = connection_str
        self.log_path = log_path

    def start(self):
        self.connection = mavutil.mavlink_connection(self.connection_str)
        self.log = open(self.log_path + generate_log_file_name + '.mav', 'wb')

    def read_data(self):
        msg = self.connection.recv_match()
        if (msg is None):
            raise RuntimeError("No Message")
        self.log.write(struct.pack("<Q", time.time()))
        self.log.write(msg.get_msgbuf())

        if msg.get_type() == "BAD_DATA":
          raise TypeError("BAD_DATA message received")

        data = self.get_data([msg])
        if data is None:
            raise TypeError("Message type not supported")

        return data

    def get_data(self, msgs):
        msg_list = []
        for msg in msgs:
            if msg.get_type() == "BAD_DATA":
                continue
            data = msg.to_dict()
            data.pop('mavpackettype', None)
            msg_time = data.pop("time_s", time.time()) + data.pop("time_us", 0)/1000000

            for item in list(data.items()):
                if isinstance(item[1], list):
                    data.pop(item[0])
                    for i in range(len(item[1])):
                        data.update([[item[0] + '[' + str(i) + ']', item[1][i]]])

            if msg.get_type() == "GPS_UBX_NAV_SOL":
                gps = wgs84_conv(msg.ecefX / 100, msg.ecefY / 100, msg.ecefZ / 100)
                data.update([['lat', gps[0]], ['lon', gps[1]], ['alt', gps[2]]])
                data['ecefX'] /= 100
                data['ecefY'] /= 100
                data['ecefZ'] /= 100

            header = msg.get_header()
            msg_list.append(Message(message_id=msg.get_type(),
                                source_id=(str(header.srcSystem) + '_' + str(header.srcComponent)),
                                msg_time=msg_time,
                                msg_data=data))
        return msg_list

    def stop(self):
        self.connection.close()
        self.log.close()


class MAVLogDataSource():
    def __init__(self, log_path, real_time=0, time_delay=0.01):
        self.log_path = log_path
        self.real_time = real_time
        self.time_delay = time_delay

    def start(self):
        self.connection = mavutil.mavlogfile(self.log_path, notimestamps=False)
        self.time_shift = None
        self.time_start = None

    def read_data(self):
        msg = self.connection.recv_match()
        if (msg is None):
            raise RuntimeError("No Message")

        data = MAVDataSource.get_data(MAVDataSource, [msg])
        if data is None:
            raise TypeError("Message type not supported")

        if self.real_time:
          if self.time_shift is None:
            self.time_shift = datetime.fromtimestamp(msg._timestamp)
            if self.time_start is None:
                self.time_start = time.time()
            while (time.time() - self.time_start) < (datetime.fromtimestamp(msg._timestamp) - self.time_shift):
                time.sleep(0.001)
        else:
            time.sleep(self.time_delay)
        return data

    def stop(self):
        self.connection.close()


class ZMQDataSource():
    def __init__(self, bus_pub="tcp://127.0.0.1:7778", topics=["its.telemetry_packet"], log_path="./"):
        self.bus_pub = bus_pub
        self.topics = topics
        self.log_path = log_path

    def start(self):
        self.zmq_ctx = zmq.Context()
        self.sub_socket = self.zmq_ctx.socket(zmq.SUB)
        self.sub_socket.connect(self.bus_pub)
        for topic in self.topics:
            self.sub_socket.setsockopt_string(zmq.SUBSCRIBE, topic)

        self.poller = zmq.Poller()
        self.poller.register(self.sub_socket, zmq.POLLIN)

        self.log = open(self.log_path + generate_log_file_name() + '.mav', 'wb')

        self.mav = its_mav.MAVLink(file=None)
        self.mav.robust_parsing = True

    def read_data(self):
        events = dict(self.poller.poll(1000))
        if self.sub_socket in events:
            zmq_msg = self.sub_socket.recv_multipart()
            msg_buf = self.get_data(zmq_msg)

            self.log.write(struct.pack("<Q", int(time.time() * 1000)))
            self.log.write(msg_buf)

            msg = self.mav.parse_buffer(msg_buf)
            if msg is None:
                raise RuntimeError("No Message")
            data = MAVDataSource.get_data(MAVDataSource, msg)
            return data
        else:
            raise RuntimeError("No Message")

    def get_data(self, msgs):
        return msgs[2]

    def stop(self):
        self.log
