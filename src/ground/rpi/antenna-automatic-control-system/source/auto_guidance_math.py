import numpy as NumPy
from math import sin, cos, radians, acos, asin
import time

class AutoGuidanceMath():
    def __init__ (self, accel_sensor,
                        mag_sensor,
                        gps_sensor,
                        decl=0,
                        sensors_accumulation_num=1,
                        gps_accumulation_num=1,
                        sensors_timeout=1,
                        gps_timeout=1,
                        mag_recount_matrix=None,
                        mag_calibration_matrix=None,
                        mag_calibration_vector=None,
                        accel_recount_matrix=None,
                        accel_calibration_matrix=None,
                        accel_calibration_vector=None):
        # Subsystems
        self.accel_sensor = accel_sensor
        self.mag_sensor = mag_sensor
        self.gps_sensor = gps_sensor

        # Mag vector declination
        self.decl = decl

        # Data collecting settings
        self.sensors_accumulation_num = sensors_accumulation_num
        self.gps_accumulation_num = gps_accumulation_num
        self.sensors_timeout = sensors_timeout
        self.gps_timeout = gps_timeout

        # Data correction matrixes
        self.mag_recount_matrix = mag_recount_matrix
        self.mag_calibration_matrix = mag_calibration_matrix
        self.mag_calibration_vector = mag_calibration_vector
        self.accel_recount_matrix = accel_recount_matrix
        self.accel_calibration_matrix = accel_calibration_matrix
        self.accel_calibration_vector = accel_calibration_vector

        # Variables default value
        self.coord_sys_center = NumPy.zeros((3, 1))
        self.dec_to_top = NumPy.zeros((3, 3))
        self.top_to_gcs = NumPy.zeros((3, 3))
        self.mag = NumPy.zeros((3, 1))
        self.accel = NumPy.zeros((3, 1))
        self.lat_lon = NumPy.array([0.0, 0.0])
        self.alt = 0.0
        self.x_y_z = NumPy.zeros((3, 1))
        self.coord_system_sucsess_flag = False

    def set_decl(self, decl):
        self.decl = decl

    def dec_to_top_matix(self, lat, lon):
        lat = radians(lat)
        lon = radians(lon)
        return  NumPy.array([[-sin(lat)*cos(lon),  -sin(lat)*sin(lon), cos(lat)],
                             [ sin(lon),           -cos(lon),          0       ],
                             [ cos(lat)*cos(lon),   cos(lat)*sin(lon), sin(lat)]])

    def top_to_gcscs_matix(self, mag, accel, decl):
        accel = accel / NumPy.linalg.norm(accel)
        mag = mag / NumPy.linalg.norm(mag)
        decl = radians(decl)

        mag = mag - accel * NumPy.dot(accel.T, mag)
        mag = mag / NumPy.linalg.norm(mag)
        mag = mag * cos(decl) + accel * (1 - cos(decl)) * NumPy.dot(accel.T, mag) + sin(decl) * NumPy.cross(accel.T, mag.T).T

        b = NumPy.cross(accel.T, mag.T).T
        b = b / NumPy.linalg.norm(b)

        return NumPy.column_stack([mag, b, accel])

    def recount_mag(self, mag):
        if self.mag_calibration_vector is not None:
            mag = mag - self.mag_calibration_vector
        if self.mag_calibration_matrix is not None:
            mag = NumPy.dot(self.mag_calibration_matrix, mag)
        if self.mag_recount_matrix is not None:
            mag = NumPy.dot(self.mag_recount_matrix, mag)
        return mag

    def recount_accel(self, accel):
        if self.accel_calibration_vector is not None:
            accel = accel - self.accel_calibration_vector
        if self.accel_calibration_matrix is not None:
            accel = NumPy.dot(self.accel_calibration_matrix, accel)
        if self.accel_recount_matrix is not None:
            accel = NumPy.dot(self.accel_recount_matrix, accel)
        return accel

    def recount_vector(self, vector):
        vector = NumPy.dot(self.dec_to_top, vector)
        vector = NumPy.dot(self.top_to_gcs, vector)
        return vector

    def setup_coord_system_math(self, accel, mag, coord_sys_center, lat_lon):
        self.coord_system_sucsess_flag = False
        accel = self.recount_accel(accel)
        mag = self.recount_mag(mag)
        self.coord_sys_center = coord_sys_center

        self.dec_to_top = self.dec_to_top_matix(*list(lat_lon))
        self.top_to_gcs = self.top_to_gcscs_matix(mag, accel, self.decl)
        self.coord_system_sucsess_flag = True

    def setup_coord_system(self):
        self.coord_system_sucsess_flag = False
        # Collecting data from accel sensor
        start_time = time.time()
        self.accel = NumPy.zeros((3, 1))
        for i in range(self.sensors_accumulation_num):
            if (time.time() - start_time) > self.sensors_timeout:
                raise TimeoutError('Accel sensor data accumulation timeout')
            self.accel += NumPy.array(self.accel_sensor.get_data_accel()).reshape((3, 1))
        self.accel /= self.sensors_accumulation_num

        # Collecting data from mag sensor
        start_time = time.time()
        self.mag = NumPy.zeros((3, 1))
        for i in range(self.sensors_accumulation_num):
            if (time.time() - start_time) > self.sensors_timeout:
                raise TimeoutError('Mag sensor data accumulation timeout')
            self.mag += NumPy.array(self.mag_sensor.get_data_mag()).reshape((3, 1))
        self.mag /= self.sensors_accumulation_num

        # Collecting data from gps
        start_time = time.time()
        self.lat_lon = NumPy.array([0.0, 0.0])
        self.alt = 0.0
        self.x_y_z = NumPy.zeros((3, 1))
        num = 0
        while num <= self.gps_accumulation_num:
            if (time.time() - start_time) > self.gps_timeout:
                raise TimeoutError('Gps data accumulation timeout')
            data = self.gps_sensor.find_tpv_data()
            if data is None:
                continue
            self.lat_lon += NumPy.array(self.gps_sensor.get_lat_lon(data))
            self.alt += self.gps_sensor.get_alt(data)
            self.x_y_z += NumPy.array(self.gps_sensor.get_x_y_z(data)).reshape((3, 1))
        self.lat_lon /= self.gps_accumulation_num
        self.alt /= self.gps_accumulation_num
        self.x_y_z /= self.gps_accumulation_num

        # Count coord system variables
        self.setup_coord_system_math(self.accel, self.mag, self.x_y_z, self.lat_lon)

    def count_vector(self, object_coords):
        vector = object_coords - self.coord_sys_center
        vector = self.recount_vector(vector)
        return vector

    def get_top_to_gcs(self):
        return self.top_to_gcs

    def get_dec_to_top(self):
        return self.dec_to_top

    def get_accel(self):
        return self.accel

    def get_mag(self):
        return self.mag

    def get_lat_lon(self):
        return self.lat_lon

    def get_alt(self):
        return self.alt

    def get_x_y_z(self):
        return self.x_y_z

    def get_coord_system_sucsess_flag(self):
        return self.coord_system_sucsess_flag

    def count_target_angles(self, vector):
        vector = vector / NumPy.linalg.norm(vector)
        vector_phi = degrees(asin(vector[2]))
        vector_alpha = degrees(acos(vector[0]/((vector[0]^2 + vector[1]^2)^(1/2))))
        if vector[1] < 0:
            vector_alpha = 360 - vector_alpha

        return (vector_alpha, vector_phi)

    def north(self):
        return self.top_to_gcs[:,0]