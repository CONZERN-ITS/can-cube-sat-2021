from PyQt5 import QtCore
import os
os.environ['MAVLINK_DIALECT'] = "its"
os.environ['MAVLINK20'] = "its"
from pymavlink.dialects.v20 import its as its_mav
from pymavlink import mavutil
import json

from source import RES_ROOT, LOG_FOLDER_PATH

import math
import time

class AbstractCommanInterface(QtCore.QObject):
    send_msg = QtCore.pyqtSignal(object)

    def __init__(self):
        super(AbstractCommanInterface, self).__init__()

    def msg_reaction(self, msg):
        pass

    def send_command(self, msg):
        pass

    def generate_message(self, name, data):
        return None


class MAVITSInterface(AbstractCommanInterface):
    def __init__(self):
        super(MAVITSInterface, self).__init__()
        self.control_mode = True
        self.mav = its_mav.MAVLink(file=None)

    def convert_time_from_s_to_s_us(self, current_time):
        current_time = math.modf(current_time)
        return (int(current_time[1]), int(current_time[0] * 1000000))

    def convert_time_from_s_us_to_s(self, time_s, time_us):
        return time_s + time_us/1000000

    def generate_message(self, name, data):
        command = None
        for msg_def in its_mav.mavlink_map.values():
            if msg_def.name == name:
                command = msg_def
                break
        if command is not None:
            try:
                msg = msg_def(**data)
            except Exception as e:
                print(e)
                msg = None
        else:
            msg = None
        return msg

    def send_command(self, msg):
        if msg is not None:
            msg.get_header().srcSystem = 0
            msg.get_header().srcComponent = 4
            self.send_msg(msg)


class ZMQITSInterface(MAVITSInterface):
    def __init__(self):
        super(ZMQITSInterface, self).__init__()
        self.cookie = (time.time() // 1000) % 1000 

    def msg_reaction(self, msg):
        pass

    def send_command(self, msg):
        if msg is not None:
            msg.get_header().srcSystem = 0
            msg.get_header().srcComponent = 3
            multipart = ["antenna.command_packet".encode("utf-8"),
                         ("{ cookie: %d }" % self.cookie).encode("utf-8"),
                         msg.pack(self.mav)]
            self.cookie += 1
            self.send_msg.emit(multipart)