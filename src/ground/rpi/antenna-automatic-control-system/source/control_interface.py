import os
os.environ['MAVLINK_DIALECT'] = "its"
os.environ['MAVLINK20'] = "its"
from pymavlink.dialects.v20 import its as its_mav
from pymavlink import mavutil

import math
import time


class AbstractControlInterface():
    def __init__(self, drive_object, auto_guidance_math, aiming_period=1, min_rotation_angle=1):
        # Subsystems
        self.drive_object = drive_object
        self.auto_guidance_math = auto_guidance_math

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
        pass

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

    def messages_reaction(self, msgs):
        if msgs is not None:
            for msg in msgs:
                if msg.get_type() == "AS_AUTOMATIC_CONTROL":
                    self.auto_control_mode = bool(msg.mode)
                elif msg.get_type() == "AS_HARD_MANUAL_CONTROL":
                    self.update_target_position(msg.azimuth, msg.elevation)
                    self.elevation_delta -= msg.elevation
                    self.azimuth_delta -= msg.azimuth
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
                    pass
                elif msg.get_type() == "AS_MOTORS_AUTO_DISABLE":
                    self.drive_object.set_drive_auto_disable_mode(msg.mode)
                    pass
                elif msg.get_type() == "AS_SEND_COMMAND":
                    enum = mavutil.mavlink.enums['AS_COMMANDS']
                    if enum[msg.command_id].name == 'AS_SETUP_ELEVATION_ZERO':
                        self.setup_elevation_zero()
                    elif enum[msg.command_id].name == 'AS_TARGET_TO_NORTH':
                        self.target_to_north()
                        pass
                    elif enum[msg.command_id].name == 'SETUP_COORD_SYSTEM':
                        try:
                            self.auto_guidance_math.setup_coord_system()
                        except Exception as e:
                            print(e)
                            pass
                        pass
                elif msg.get_type() == "GPS_UBX_NAV_SOL":
                    if (msg.gpsFix > 0) and (msg.gpsFix < 4):
                        self.target_last_time = convert_time_from_s_to_s_us(time.time())
                        position = [msg.ecefX / 100, msg.ecefY / 100, msg.ecefZ / 100]
                        self.update_target_position_WGS84_DEC(position)
                    pass

    def convert_time_from_s_to_s_us(self, current_time):
        current_time = math.modf(current_time)
        return (int(current_time[1]), int(current_time[0] * 1000000))

    def convert_time_from_s_us_to_s(self, time_s, time_us):
        return time_s + time_us/1000000

    def generate_state_message(self):
        current_time = self.convert_time_from_s_to_s_us(time.time())
        enable = [self.drive_object.get_vertical_enable_state(), self.drive_object.get_horizontal_enable_state()]
        enable = [False if state is None else state for state in enable]
        msg = its_mav.MAVLink_as_state_message(time_s=current_time[0],
                                               time_us=current_time[1],
                                               azimuth=self.azimuth,
                                               elevation=self.elevation,
                                               ecef=list(self.auto_guidance_math.get_x_y_z()),
                                               lat_lon=list(self.auto_guidance_math.get_lat_lon()),
                                               alt=self.auto_guidance_math.get_alt(),
                                               top_to_ascs=list(self.auto_guidance_math.get_top_to_gcs().reshape(9)),
                                               dec_to_top=list(self.auto_guidance_math.get_dec_to_top().reshape(9)),
                                               target_time_s=self.target_last_time[0],
                                               target_time_us=self.target_last_time[1],
                                               target_azimuth=self.target_alpha,
                                               target_elevation=self.target_phi,
                                               mode=int(self.auto_control_mode),
                                               period=self.aiming_period,
                                               enable=[int(mode) for mode in enable],
                                               motor_auto_disable=self.drive_object.get_drive_auto_disable_mode(),
                                               motors_timeout=self.drive_object.get_drive_timeout())
        msg.get_header().srcSystem = 0
        msg.get_header().srcComponent = 3
        return msg

    def target_to_north(self):
        vector = self.auto_guidance_math.get_top_to_gcs()[:,0]
        self.update_target_position_GCSCS_VEC(vector)

    def setup_elevation_zero(self):
        self.update_target_position(0, -360)
        trigger = self.drive_object.get_last_vertical_limit()
        if trigger is not None:
            self.update_target_position(0, self.drive_object.get_vertical_limits().get(trigger, 0))

    def update_target_position_WGS84_DEC(self, position):
        vector = NumPy.array(position).reshape((3, 1))
        self.update_target_position_WGS84_VEC(vector)

    def update_target_position_WGS84_VEC(self, vector):
        self.update_target_position_GCSCS_VEC(self.auto_guidance_math.count_vector(vector))

    def update_target_position_GCSCS_VEC(self, vector):
        if self.auto_guidance_math.get_coord_system_sucsess_flag():
            (self.target_alpha, self.target_phi) = self.auto_guidance_math.count_target_angles(vector)
            self.update_target_position(azimuth=(self.target_alpha - self.azimuth_delta - self.azimuth),
                                        elevation=(self.target_phi - self.elevation_delta - self.elevation))

    def update_target_position(self, azimuth, elevation):
        if ((self.auto_control_mode) and ((time.time() + self.aiming_period) > self.last_rotation_time)):
            if azimuth >= self.min_rotation_angle:
                self.drive_object.horizontal_rotation(azimuth)
                self.azimuth = self.drive_object.get_horizontal_position() + self.azimuth_delta
            if elevation >= self.min_rotation_angle:
                self.drive_object.vertical_rotation(elevation)
                self.elevation = self.drive_object.get_vertical_position() + self.elevation_delta
            if ((azimuth >= self.min_rotation_angle) or (elevation >= self.min_rotation_angle)):
                self.last_rotation_time = time.time()

class ZMQITSControlInterface(MAVITSControlInterface):
    def __init__(self, *args, **kwargs):
        super(ZMQITSControlInterface, self).__init__(*args, **kwargs)
        self.mav = its_mav.MAVLink(file=None)

    def messages_reaction(self, msgs):
        for msg in msgs:
            topic = msg[0].decode('utf-8')
            if topic == 'antenna.command_packet':
                super(ZMQITSControlInterface, self).messages_reaction(self.mav.parse_buffer(msg[2]))

    def generate_state_message(self):
        msg = super(ZMQITSControlInterface, self).generate_state_message()
        multipart = ["antenna.telemetry_packet".encode("utf-8"),
                     bytes(),
                     msg.pack(self.mav)]
        return multipart
