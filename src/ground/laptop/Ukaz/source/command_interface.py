from PyQt5 import QtCore
import typing
import os
os.environ['MAVLINK_DIALECT'] = "its"
os.environ['MAVLINK20'] = "its"
from pymavlink.dialects.v20 import its as its_mav
from pymavlink import mavutil
import json

from source import RES_ROOT, LOG_FOLDER_PATH

import math
import time


class DataMessage():
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


class CommandStatusMessage():
    STATUS_UNKNOWN = 0
    STATUS_PROCESSING = 1
    STATUS_SUCCSESS = 2
    STATUS_FAILURE = 3

    def __init__(self, cookie, 
                       status='Undefined',
                       status_type=STATUS_UNKNOWN,
                       stage_id=0,
                       name='Undefined'):

        self.cookie = cookie
        self.set_status(status)
        self.set_status_type(status_type)
        self.set_stage_id(stage_id)
        self.set_name(name)

    def get_cookie(self):
        return self.cookie

    def get_status(self):
        return self.status

    def get_status_type(self):
        return self.status_type

    def get_stage_id(self):
        return self.stage_id

    def get_name(self):
        return self.name

    def set_status(self, status='Undefined'):
        self.status = status

    def set_status_type(self, status_type=STATUS_UNKNOWN):
        self.status_type = status_type

    def set_stage_id(self, stage_id=0):
        self.stage_id = stage_id

    def set_name(self, name='Undefined'):
        self.name = name


class AbstractCommanInterface(QtCore.QObject):
    send_msg = QtCore.pyqtSignal(object)
    command_ststus_changed = QtCore.pyqtSignal(object)
    data_changed = QtCore.pyqtSignal(object)

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
        self.mav.robust_parsing = True

    def convert_time_from_s_to_s_us(self, current_time):
        current_time = math.modf(current_time)
        return (int(current_time[1]), int(current_time[0] * 1000000))

    def convert_time_from_s_us_to_s(self, time_s, time_us):
        return time_s + time_us/1000000

    def msg_reaction(self, msg):
        if msg.get_type() == 'BCU_RADIO_CONN_STATS':
            if msg.last_executed_cmd_seq > 0:
                self.command_ststus_changed.emit(CommandStatusMessage(cookie=msg.last_executed_cmd_seq, 
                                                                      status='Executed',
                                                                      status_type=self.STATUS_SUCCSESS,
                                                                      stage_id=2))
        if msg.get_type() != "BAD_DATA":
            header = msg.get_header()
            data = msg.to_dict()
            data.pop('mavpackettype')
            msg_time = data.pop("time_s", time.time()) + data.pop("time_us", 0)/1000000
            self.data_changed.emit(DataMessage(message_id=msg.get_type(),
                                               source_id=(str(header.srcSystem) + '_' + str(header.srcComponent)),
                                               msg_time=msg_time,
                                               msg_data=data))



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
                    status_type = CommandStatusMessage.STATUS_FAILURE
                elif status == 'accepted':
                    status_type = CommandStatusMessage.STATUS_PROCESSING
                elif status == 'emitted':
                    status_type = CommandStatusMessage.STATUS_PROCESSING
                elif status == 'delivered':
                    status_type = CommandStatusMessage.STATUS_SUCCSESS
                elif status == 'undelivered':
                    status_type = CommandStatusMessage.STATUS_FAILURE
                else:
                    status_type = CommandStatusMessage.STATUS_UNKNOWN
                self.command_ststus_changed.emit(CommandStatusMessage(cookie=cookie, 
                                                                      status=status,
                                                                      status_type=status_type,
                                                                      stage_id=1))

    def send_command(self, msg, cookie=None):
        if msg is not None:
            msg.get_header().srcSystem = 0
            msg.get_header().srcComponent = 3
            if cookie is not None:
                seq = cookie
            else:
                seq = self.cookie
                self.cookie += 1
            self.mav.seq = int(seq)
            multipart = ["its.telecommand_request".encode("utf-8"),
                         ('{ "cookie": %d }' % msg.seq).encode("utf-8"),
                         msg.pack(self.mav)]
            self.send_msg.emit(multipart)


class ZMQITSInterface(MAVITSInterface):

    def _extract_mdt_time(self, zmq_message: typing.List[bytes]) -> float:
        """ Извлекает из сообщения на ZMQ шине таймштамп если он там есть
            Если не удалось - возвращает time.time() """
        mdt = zmq_message[1]
        parsed = json.loads(mdt)  # type: typing.Dict

        timestamp = None
        if "time_s" in parsed and "time_us" in parsed:
            try:
                timestamp = int(parsed["time_s"]) + float(parsed["time_us"])/1000/1000
            except:
                pass

        return timestamp if timestamp is not None else time.time()

    def __init__(self):
        super(ZMQITSInterface, self).__init__()
        self.cookie = 1
        self.buf = []
        self.last_send_time = 0
        self.timeout = 0.5

    def msg_reaction(self, msg):
        if msg[0] == b'radio.uplink_state':
            data = json.loads(msg[1].decode("utf-8"))
            cookie = data.get('cookie_in_wait', 0)
            if cookie is None:
                if len(self.buf) > 0:
                    self.send_msg.emit(self.buf.pop())
            cookie = data.get('cookie_in_progress', None)
            if cookie is not  None:
                self.command_ststus_changed.emit(CommandStatusMessage(cookie=cookie, 
                                                                      status='in progress',
                                                                      status_type=CommandStatusMessage.STATUS_PROCESSING,
                                                                      stage_id=1))
            cookie = data.get('cookie_sent', None)
            if cookie is not  None:
                self.command_ststus_changed.emit(CommandStatusMessage(cookie=cookie, 
                                                                      status='sent',
                                                                      status_type=CommandStatusMessage.STATUS_PROCESSING,
                                                                      stage_id=1))
            cookie = data.get('cookie_dropped', None)
            if cookie is not  None:
                self.command_ststus_changed.emit(CommandStatusMessage(cookie=cookie, 
                                                                      status='dropped',
                                                                      status_type=CommandStatusMessage.STATUS_FAILURE,
                                                                      stage_id=1))
        elif msg[0] == b'radio.downlink_frame':
            msg_buf = msg[2]
            mav_msgs = self.mav.parse_buffer(msg_buf)
            if mav_msgs is not  None:
                for mav_msg in mav_msgs:
                    super(ZMQITSInterface, self).msg_reaction(mav_msg)

        elif msg[0] == b'radio.rssi_instant':
            self.data_changed.emit(DataMessage(message_id='radio.rssi_instant',
                                               source_id=('1_0'),
                                               msg_time=self._extract_mdt_time(msg),
                                               msg_data=json.loads(msg[1].decode("utf-8"))))
        elif msg[0] == b'radio.rssi_packet':
            self.data_changed.emit(DataMessage(message_id='radio.rssi_packet',
                                               source_id=('1_0'),
                                               msg_time=self._extract_mdt_time(msg),
                                               msg_data=json.loads(msg[1].decode("utf-8"))))
        elif msg[0] == b'radio.stats':
            self.data_changed.emit(DataMessage(message_id='radio.stats',
                                               source_id=('1_0'),
                                               msg_time=self._extract_mdt_time(msg),
                                               msg_data=json.loads(msg[1].decode("utf-8"))))

    def send_command(self, msg, cookie=None):
        if msg is not None:
            msg.get_header().srcSystem = 0
            msg.get_header().srcComponent = 3
            if cookie is not None:
                seq = cookie
            else:
                seq = self.cookie
                self.cookie += 1
            self.mav.seq = int(seq)
            multipart = ["radio.uplink_frame".encode("utf-8"),
                         ('{ "cookie": %d }' % int(seq)).encode("utf-8"),
                         msg.pack(self.mav)]
            self.buf.append(multipart)