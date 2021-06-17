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

    command_ststus_changed = QtCore.pyqtSignal(list)

    STATUS_UNKNOWN = 0
    STATUS_PROCESSING = 1
    STATUS_SUCCSESS = 2
    STATUS_FAILURE = 3

    def __init__(self):
        super(AbstractCommanInterface, self).__init__()

    def msg_reaction(self, msg):
        pass

    def send_command(self, msg, cookie=None):
        pass

    def generate_message(self, name, data):
        return None


class MAVITSInterface(AbstractCommanInterface):
    def __init__(self):
        super(MAVITSInterface, self).__init__()
        self.cookie = 1
        self.control_mode = True
        self.mav = its_mav.MAVLink(file=None)

    def convert_time_from_s_to_s_us(self, current_time):
        current_time = math.modf(current_time)
        return (int(current_time[1]), int(current_time[0] * 1000000))

    def convert_time_from_s_us_to_s(self, time_s, time_us):
        return time_s + time_us/1000000

    def msg_reaction(self, msg):
        if msg.get_type() == 'BCU_RADIO_CONN_STATS':
            pass #self.command_ststus_changed.emit([msg.seq, 'test', self.STATUS_UNKNOWN])

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

    def send_command(self, msg, cookie=None):
        if msg is not None:
            if cookie is not None:
                msg.seq = cookie
            else:
                msg.seq = self.cookie
                self.cookie += 1
            msg.get_header().srcSystem = 0
            msg.get_header().srcComponent = 4
            self.send_msg(msg)


class ZMQITSUSLPInterface(MAVITSInterface):
    def __init__(self):
        super(ZMQITSInterface, self).__init__()
        self.cookie = 1
        self.buf = []
        self.last_send_time = 0
        self.timeout = 0.5

    def msg_reaction(self, msg):
        if msg[0] == 'its.telecommand_event':
            data = json.loads(msg[1].decode("utf-8"))
            cookie = data.get('cookie', None)
            status = data.get('event', None)
            if (cookie is not None) and (status is not None):
                if status == 'rejected':
                    status_type = self.STATUS_FAILURE
                elif status == 'accepted':
                    status_type = self.STATUS_PROCESSING
                elif status == 'emitted':
                    status_type = self.STATUS_PROCESSING
                elif status == 'delivered':
                    status_type = self.STATUS_SUCCSESS
                elif status == 'undelivered':
                    status_type = self.STATUS_FAILURE
                else:
                    status_type = self.STATUS_UNKNOWN
                self.command_ststus_changed.emit([cookie, status, status_type])

    def send_command(self, msg, cookie=None):
        if msg is not None:
            msg.get_header().srcSystem = 0
            msg.get_header().srcComponent = 3
            if cookie is not None:
                msg.seq = cookie
            else:
                msg.seq = self.cookie
                self.cookie += 1

            multipart = ["its.telecommand_request".encode("utf-8"),
                         ('{ "cookie": %d }' % msg.seq).encode("utf-8"),
                         msg.pack(self.mav)]
            self.send_msg.emit(multipart)


class ZMQITSInterface(MAVITSInterface):
    def __init__(self):
        super(ZMQITSInterface, self).__init__()
        self.cookie = 1
        self.buf = []
        self.last_send_time = 0
        self.timeout = 0.5

    def msg_reaction(self, msg):
        print(msg)
        if msg[0] == b'radio.uplink_state':
            data = json.loads(msg[1].decode("utf-8"))
            cookie = data.get('cookie_in_wait', 0)
            if cookie is None:
                if len(self.buf) > 0:
                    self.send_msg.emit(self.buf.pop())
            cookie = data.get('cookie_in_progress', None)
            if cookie is not  None:
                self.command_ststus_changed.emit([cookie, 'in progress', self.STATUS_PROCESSING])
            cookie = data.get('cookie_sent', None)
            if cookie is not  None:
                self.command_ststus_changed.emit([cookie, 'sent', self.STATUS_SUCCSESS])
            cookie = data.get('cookie_dropped', None)
            if cookie is not  None:
                self.command_ststus_changed.emit([cookie, 'dropped', self.STATUS_FAILURE])

        if msg[0] == b'radio.downlink_frame':
            msg_buf = zmq_msg[2]
            msgs = self.mav.parse_buffer(msg_buf)
            if msgs is not  None:
                for msg in msgs:
                    super(ZMQITSInterface, self).msg_reaction(msg)

    def send_command(self, msg, cookie=None):
        if msg is not None:
            msg.get_header().srcSystem = 0
            msg.get_header().srcComponent = 3
            if cookie is not None:
                msg.seq = cookie
            else:
                msg.seq = self.cookie
                self.cookie += 1
            multipart = ["radio.uplink_frame".encode("utf-8"),
                         ('{ "cookie": %d }' % msg.seq).encode("utf-8"),
                         msg.pack(self.mav)]
            self.buf.append(multipart)
            print(self.buf)