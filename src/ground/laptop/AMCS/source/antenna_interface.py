from PyQt5 import QtCore
import os
os.environ['MAVLINK_DIALECT'] = "its"
os.environ['its_mav0'] = "its"
from pymavlink.dialects.v20 import its as its_mav
from pymavlink import mavutil
import json

from source import RES_ROOT, LOG_FOLDER_PATH

import math
import time

class AbstractAntennaInterface(QtCore.QObject):
    send_msg = QtCore.pyqtSignal(object)

    command_sent = QtCore.pyqtSignal(str)

    antenna_pos_changed = QtCore.pyqtSignal(tuple)
    target_pos_changed = QtCore.pyqtSignal(tuple)
    lat_lon_alt_changed = QtCore.pyqtSignal(tuple)
    ecef_changed = QtCore.pyqtSignal(tuple)
    top_to_ascs_matrix_changed = QtCore.pyqtSignal(tuple)
    dec_to_top_matrix_changed = QtCore.pyqtSignal(tuple)
    control_mode_changed = QtCore.pyqtSignal(bool)
    aiming_period_changed = QtCore.pyqtSignal(float)
    motors_enable_changed = QtCore.pyqtSignal(tuple)
    motors_auto_disable_mode_changed = QtCore.pyqtSignal(bool)
    motors_timeout_changed = QtCore.pyqtSignal(float)
    rssi_changed = QtCore.pyqtSignal(float)

    def set_angle_control_mode(self, mode):
        pass

    def set_antenna_angle(self, azimuth, elevation):
        pass

    def count_angle(self, num):
        angle = 0
        if num == 1:
            angle = 1
        elif num == 2:
            angle = 10
        elif num == 3:
            angle = 50
        return angle

    def messages_reaction(self, msgs):
        pass

    def put_up(self, num):
        pass

    def put_down(self, num):
        pass

    def turn_right(self, num):
        pass

    def turn_left(self, num):
        pass

    def automatic_control_on(self):
        pass

    def automatic_control_off(self):
        pass

    def turn_motors_on(self):
        pass

    def turn_motors_off(self):
        pass

    def motors_auto_disable_on(self):
        pass

    def motors_auto_disable_off(self):
        pass

    def set_motors_timeout(self, timeout):
        pass

    def set_aiming_period(self, period):
        pass

    def setup_elevation_zero(self):
        pass


class MAVITSInterface(AbstractAntennaInterface):
    def __init__(self, *args, **kwargs):
        super(MAVITSInterface, self).__init__(*args, **kwargs)
        self.control_mode = True
        self.mav = its_mav.MAVLink(file=None)
        self.set_angle_control_mode()

    def messages_reaction(self, msgs):
        if msgs is not None:
            for msg in msgs:
                if msg.get_type() == 'AS_STATE':
                    self.antenna_pos_changed.emit((msg.azimuth, msg.elevation, self.convert_time_from_s_us_to_s(msg.time_s, msg.time_us)))
                    self.lat_lon_alt_changed.emit(tuple(msg.lat_lon + [msg.alt]))
                    self.ecef_changed.emit(tuple(msg.ecef))
                    self.top_to_ascs_matrix_changed.emit(tuple(msg.top_to_ascs))
                    self.dec_to_top_matrix_changed.emit(tuple(msg.dec_to_top))
                    self.target_pos_changed.emit((msg.target_azimuth, msg.target_elevation, self.convert_time_from_s_us_to_s(msg.target_time_s, msg.target_time_us)))
                    self.control_mode_changed.emit(msg.mode)
                    self.aiming_period_changed.emit(msg.period)
                    self.motors_enable_changed.emit(tuple(msg.enable))
                    self.motors_auto_disable_mode_changed.emit(msg.motor_auto_disable)
                    self.motors_timeout_changed.emit(msg.motors_timeout)

                elif msg.get_type() == 'RSSI':
                    self.rssi_changed(msg.rssi)

    def convert_time_from_s_to_s_us(self, current_time):
        current_time = math.modf(current_time)
        return [int(current_time[1]), int(current_time[0] * 1000000)]

    def convert_time_from_s_us_to_s(self, time_s, time_us):
        return time_s + time_us/1000000

    def send_message(self, msg):
        msg.get_header().srcSystem = 0
        msg.get_header().srcComponent = 3
        self.send_msg.emit(msg)
        self.command_sent.emit(str(msg))

    def set_angle_control_mode(self, mode=True):
        self.angle_control_mode = mode

    def set_antenna_angle(self, azimuth, elevation):
        args = self.convert_time_from_s_to_s_us(time.time()) + [azimuth, elevation]
        if self.angle_control_mode:
            self.send_message(its_mav.MAVLink_as_soft_manual_control_message(*args))
        else:
            self.send_message(its_mav.MAVLink_as_hard_manual_control_message(*args))

    def put_up(self, num):
        angle = self.count_angle(num)
        self.set_antenna_angle(0, angle)

    def put_down(self, num):
        angle = self.count_angle(num)
        self.set_antenna_angle(0, -angle)

    def turn_right(self, num):
        angle = self.count_angle(num)
        self.set_antenna_angle(-angle, 0)

    def turn_left(self, num):
        angle = self.count_angle(num)
        self.set_antenna_angle(angle, 0)

    def automatic_control_on(self):
        self.send_message(its_mav.MAVLink_as_automatic_control_message(*(self.convert_time_from_s_to_s_us(time.time()) + [True])))

    def automatic_control_off(self):
        self.send_message(its_mav.MAVLink_as_automatic_control_message(*(self.convert_time_from_s_to_s_us(time.time()) + [False])))

    def turn_motors_on(self):
        self.send_message(its_mav.MAVLink_as_motors_enable_mode_message(*(self.convert_time_from_s_to_s_us(time.time()) + [True])))

    def turn_motors_off(self):
        self.send_message(its_mav.MAVLink_as_motors_enable_mode_message(*(self.convert_time_from_s_to_s_us(time.time()) + [False])))

    def motors_auto_disable_on(self):
        self.send_message(its_mav.MAVLink_as_motors_auto_disable_message(*(self.convert_time_from_s_to_s_us(time.time()) + [True])))

    def motors_auto_disable_off(self):
        self.send_message(its_mav.MAVLink_as_motors_auto_disable_message(*(self.convert_time_from_s_to_s_us(time.time()) + [False])))

    def set_motors_timeout(self, timeout):
        self.send_message(its_mav.MAVLink_as_set_motors_timeout_message(*(self.convert_time_from_s_to_s_us(time.time()) + [timeout])))

    def set_aiming_period(self, period):
        self.send_message(its_mav.MAVLink_as_aiming_period_message(*(self.convert_time_from_s_to_s_us(time.time()) + [period])))

    def setup_elevation_zero(self):
        self.send_message(its_mav.MAVLink_as_send_command_message(*(self.convert_time_from_s_to_s_us(time.time()) + [0])))

    def target_to_north(self):
        self.send_message(its_mav.MAVLink_as_send_command_message(*(self.convert_time_from_s_to_s_us(time.time()) + [1])))

    def setup_coord_system(self):
        self.send_message(its_mav.MAVLink_as_send_command_message(*(self.convert_time_from_s_to_s_us(time.time()) + [2])))

    def park(self):
        self.setup_elevation_zero()
        self.target_to_north()

    def state_request(self):
        self.send_message(its_mav.MAVLink_as_send_command_message(*(self.convert_time_from_s_to_s_us(time.time()) + [3])))


class ZMQITSInterface(MAVITSInterface):
    def messages_reaction(self, msgs):
        for msg in msgs:
            topic = msg[0].decode('utf-8')
            if topic == 'antenna.telemetry_packet':
                super(ZMQITSInterface, self).messages_reaction(self.mav.parse_buffer(msg[2]))
            elif topic == 'radio.rssi_instant':
                self.rssi_changed.emit(json.loads(msg[1]).get('rssi', None))

    def send_message(self, msg):
        msg.get_header().srcSystem = 0
        msg.get_header().srcComponent = 3
        multipart = ["antenna.command_packet".encode("utf-8"),
                     bytes(),
                     msg.pack(self.mav)]
        self.send_msg.emit(multipart)
        self.command_sent.emit(str(msg))
