import os
os.environ['MAVLINK_DIALECT'] = "its"
os.environ['MAVLINK20'] = "its"
from pymavlink.dialects.v20 import its as its_mav
from pymavlink import mavutil

import math
import time
import numpy as NumPy
from source import auto_guidance_math
from config import *


class AbstractControlInterface():
    def __init__(self, drive_object, guidance_math, aiming_period=1, min_rotation_angle=1):
        # Subsystems
        self.drive_object = drive_object
        self.guidance_math = guidance_math

        # Autoguidance parameters
        self.aiming_period = aiming_period
        self.min_rotation_angle = min_rotation_angle

    def set_aiming_period(self, period):
        self.aiming_period = period

    def set_min_rotation_angle(self, angle=1):
        self.min_rotation_angle = angle

    def get_aiming_period(self):
        return self.aiming_period

    def get_min_rotation_angle(self):
        return self.min_rotation_angle

    def target_to_north(self):
        pass

    def setup_elevation_zero(self):
        pass

    def messages_reaction(self, msgs):
        return []

    def generate_state_message(self):
        pass

    def update_target_position_WGS84_DEC(self, position):
        pass

    def update_target_position_WGS84_VEC(self, vector):
        pass

    def update_target_position_GCSCS_VEC(self, vector):
        pass

    def update_target_position(self, azimuth, elevation):
        pass


class MAVITSControlInterface(AbstractControlInterface):
    def __init__(self, *args, **kwargs):
        super(MAVITSControlInterface, self).__init__(*args, **kwargs)

        # Variables default value
        self.last_rotation_time = 0
        self.elevation = 0
        self.azimuth = 0
        self.elevation_delta = 0
        self.azimuth_delta = 0
        self.auto_control_mode = False
        self.target_alpha = 0
        self.target_phi = 0
        self.target_last_time = (0, 0)
        self.setup_gps_filter(DEFAULT_GPS_FILTER)

    def messages_reaction(self, msgs):
        response = []
        if msgs is not None:
            for msg in msgs:
                if msg.get_type() == "AS_AUTOMATIC_CONTROL":
                    self.auto_control_mode = bool(msg.mode)
                elif msg.get_type() == "AS_HARD_MANUAL_CONTROL":
                    self.update_target_position_hard(msg.azimuth, msg.elevation)
                elif msg.get_type() == "AS_SOFT_MANUAL_CONTROL":
                    self.update_target_position(msg.azimuth, msg.elevation)
                elif msg.get_type() == "AS_MOTORS_ENABLE_MODE":
                    self.drive_object.set_vertical_motor_enabled(bool(msg.mode))
                    self.drive_object.set_horizontal_motor_enabled(bool(msg.mode))
                elif msg.get_type() == "AS_AIMING_PERIOD":
                    if msg.period <= 0:
                        self.aiming_period = DEFAULT_ANTENNA_AIMING_PERIOD
                    else:
                        self.aiming_period = msg.period
                elif msg.get_type() == "AS_SET_MOTORS_TIMEOUT":
                    if msg.timeout <= 0:
                        self.drive_object.setup_drive_timeout(MOTORS_TIMEOUT)
                    else:
                        self.drive_object.setup_drive_timeout(msg.timeout)
                elif msg.get_type() == "AS_MOTORS_AUTO_DISABLE":
                    self.drive_object.set_drive_auto_disable_mode(msg.mode)
                elif msg.get_type() == "AS_SEND_COMMAND":
                    enum = mavutil.mavlink.enums['AS_COMMANDS']
                    if enum[msg.command_id].name == 'AS_SETUP_ELEVATION_ZERO':
                        self.setup_elevation_zero()
                    elif enum[msg.command_id].name == 'AS_TARGET_TO_NORTH':
                        self.target_to_north()
                    elif enum[msg.command_id].name == 'SETUP_COORD_SYSTEM':
                        try:
                            self.guidance_math.setup_coord_system()
                        except Exception as e:
                            print(e)
                    elif enum[msg.command_id].name == "STATE_REQUEST":
                        response.append(self._generate_as_state_message())
                elif msg.get_type() == "AS_SETUP_GPS_FILTER":
                    self.setup_gps_filter(mavutil.mavlink.enums['AS_GPS_FILTER'][msg.filter_id].name)
                elif msg.get_type() == "GPS_UBX_NAV_SOL":
                    if (msg.gpsFix > 0) and (msg.gpsFix < 4):
                        self.target_last_time = (msg.time_s, msg.time_us)
                        position = [msg.ecefX / 100, msg.ecefY / 100, msg.ecefZ / 100]
                        position = self.gps_filter.filter_out(coords=NumPy.array(position).reshape((3, 1)),
                                                              coords_time=self.target_last_time)
                        if self.guidance_math.get_coord_system_sucsess_flag():
                            vector = self.guidance_math.count_vector(NumPy.array(position).reshape((3, 1)))
                            (self.target_alpha, self.target_phi) = self.guidance_math.count_target_angles(vector)
                            if ((self.auto_control_mode) and (position is not None) and ((time.time() + self.aiming_period) > self.last_rotation_time)):
                                self.update_target_position_GCSCS_ANG(self.target_alpha, self.target_phi)
        return response

    def setup_gps_filter(self, gps_filter='NO_FILTER'):
        if gps_filter == 'VELOCITY_FILTER':
            self.gps_filter = auto_guidance_math.VelocityGPSFilter(GPS_FILTER_MAX_VELOCITY)
            self.gps_filter_id = 1
        else:
            self.gps_filter = auto_guidance_math.AbstractGPSFilter()
            self.gps_filter_id = 0

    def convert_time_from_s_to_s_us(self, current_time):
        current_time = math.modf(current_time)
        return (int(current_time[1]), int(current_time[0] * 1000000))

    def convert_time_from_s_us_to_s(self, time_s, time_us):
        return time_s + time_us/1000000

    def generate_state_message(self):
        return self._generate_as_state_message()

    def _generate_as_state_message(self):
        current_time = self.convert_time_from_s_to_s_us(time.time())
        enable = [self.drive_object.get_vertical_enable_state(), self.drive_object.get_horizontal_enable_state()]
        enable = [False if state is None else state for state in enable]
        msg = its_mav.MAVLink_as_state_message(time_s=current_time[0],
                                               time_us=current_time[1],
                                               azimuth=self.azimuth,
                                               elevation=self.elevation,
                                               ecef=list(self.guidance_math.get_x_y_z()),
                                               lat_lon=list(self.guidance_math.get_lat_lon()),
                                               alt=self.guidance_math.get_alt(),
                                               top_to_ascs=list(self.guidance_math.get_top_to_gcs().reshape(9)),
                                               dec_to_top=list(self.guidance_math.get_dec_to_top().reshape(9)),
                                               target_time_s=self.target_last_time[0],
                                               target_time_us=self.target_last_time[1],
                                               target_azimuth=self.target_alpha,
                                               target_elevation=self.target_phi,
                                               mode=int(self.auto_control_mode),
                                               period=self.aiming_period,
                                               enable=[int(mode) for mode in enable],
                                               motor_auto_disable=self.drive_object.get_drive_auto_disable_mode(),
                                               motors_timeout=self.drive_object.get_drive_timeout(),
                                               filter_id=int(self.gps_filter_id))
        msg.get_header().srcSystem = 0
        msg.get_header().srcComponent = 3
        return msg

    def target_to_north(self):
        vector = self.guidance_math.get_top_to_gcs()[:,0]
        self.update_target_position_GCSCS_VEC(vector)

    def setup_elevation_zero(self):
        self.update_target_position(0, -360)
        trigger = self.drive_object.get_last_vertical_limit()
        if trigger is not None:
            self.update_target_position(0, -self.drive_object.get_vertical_limits().get(trigger, 0))

    def update_target_position_WGS84_DEC(self, position):
        vector = NumPy.array(position).reshape((3, 1))
        self.update_target_position_WGS84_VEC(vector)

    def update_target_position_WGS84_VEC(self, vector):
        if self.guidance_math.get_coord_system_sucsess_flag():
            self.update_target_position_GCSCS_VEC(self.guidance_math.count_vector(vector))

    def update_target_position_GCSCS_VEC(self, vector):
        if self.guidance_math.get_coord_system_sucsess_flag():
            self.update_target_position_GCSCS_ANG(*self.guidance_math.count_target_angles(vector))

    def update_target_position_GCSCS_ANG(self, target_alpha, target_phi):
        if (target_alpha - self.azimuth) > 180:
            self.update_target_position(azimuth=(target_alpha - self.azimuth - 360),
                                        elevation=(target_phi - self.elevation))
        elif (target_alpha - self.azimuth) < -180:
            self.update_target_position(azimuth=(target_alpha - self.azimuth + 360),
                                        elevation=(target_phi - self.elevation))
        else:
            self.update_target_position(azimuth=(target_alpha - self.azimuth),
                                        elevation=(target_phi - self.elevation))

    def recount_angle(self, angle):
        if angle < -180:
            angle += 360 * int(-(angle - 180) / 360)
        elif angle > 180:
            angle -= 360 * int((angle + 180) / 360)
        return angle

    def update_target_position(self, azimuth, elevation):
            if (azimuth)**2 >= (self.min_rotation_angle)**2:
                self.drive_object.horizontal_rotation(azimuth)
                self.azimuth = self.recount_angle(self.drive_object.get_horizontal_position() + self.azimuth_delta)
            if (elevation)**2 >= (self.min_rotation_angle)**2:
                self.drive_object.vertical_rotation(elevation)
                self.elevation = self.recount_angle(self.drive_object.get_vertical_position() + self.elevation_delta)
            if (((azimuth)**2 >= (self.min_rotation_angle)**2) or ((elevation)**2 >= (self.min_rotation_angle)**2)):
                self.last_rotation_time = time.time()

    def update_target_position_hard(self, azimuth, elevation):
            if (azimuth)**2 >= (self.min_rotation_angle)**2:
                self.azimuth_delta -= self.drive_object.horizontal_rotation(azimuth)
            if (elevation)**2 >= (self.min_rotation_angle)**2:
                self.elevation -= self.drive_object.vertical_rotation(elevation)


class ZMQITSControlInterface(MAVITSControlInterface):
    def __init__(self, *args, **kwargs):
        super(ZMQITSControlInterface, self).__init__(*args, **kwargs)
        self.mav = its_mav.MAVLink(file=None)
        self.mav.robust_parsing = True

    def messages_reaction(self, msgs):
        response = []
        for msg in msgs:
            topic = msg[0].decode('utf-8')
            if (topic == 'antenna.command_packet') or (topic == "radio.downlink_frame"):
                for mav_msg in super(ZMQITSControlInterface, self).messages_reaction(self.mav.parse_buffer(msg[2])):
                    response.append(self._wrap_in_antenna_telemetry_packet(mav_msg))
        return response

    def generate_state_message(self):
        return self._wrap_in_antenna_telemetry_packet(super(ZMQITSControlInterface, self).generate_state_message())

    def _wrap_in_antenna_telemetry_packet(self, mav_msg):
        return ["antenna.telemetry_packet".encode("utf-8"),
                bytes(),
                mav_msg.pack(self.mav)]
