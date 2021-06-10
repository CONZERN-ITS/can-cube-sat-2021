import numpy as NumPy
from math import sin, cos, radians, acos, asin

class AutoGuidanceMath():
    def __init__ (self, decl,
                        mag_recount_matrix=None,
                        mag_calibration_matrix=None,
                        mag_calibration_vector=None,
                        accel_recount_matrix=None,
                        accel_calibration_matrix=None,
                        accel_calibration_vector=None):
        self.decl = decl
        self.mag_recount_matrix = mag_recount_matrix
        self.mag_calibration_matrix = mag_calibration_matrix
        self.mag_calibration_vector = mag_calibration_vector
        self.accel_recount_matrix = accel_recount_matrix
        self.accel_calibration_matrix = accel_calibration_matrix
        self.accel_calibration_vector = accel_calibration_vector
        self.coord_sys_center = NumPy.zeros((3, 1))
        self.dec_to_top = NumPy.zeros((3, 3))
        self.top_to_gcs = NumPy.zeros((3, 3))

    def dec_to_top_matix(lat, lon):
        lat = radians(lat)
        lon = radians(lon)
        return  NumPy.array([[-sin(lat)*cos(lon),  -sin(lat)*sin(lon), cos(lat)],
                             [ sin(lon),           -cos(lon),          0       ],
                             [ cos(lat)*cos(lon),   cos(lat)*sin(lon), sin(lat)]])

    def top_to_gcscs_matix(mag, accel, decl):
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
        return new_mag

    def recount_accel(self, accel):
        if self.accel_calibration_vector is not None:
            accel = accel - self.accel_calibration_vector
        if self.accel_calibration_matrix is not None:
            accel = NumPy.dot(self.accel_calibration_matrix, accel)
        if self.accel_recount_matrix is not None:
            accel = NumPy.dot(self.accel_recount_matrix, accel)
        return new_mag

    def recount_vector(self, vector):
        vector = NumPy.dot(self.dec_to_top, vector)
        vector = NumPy.dot(self.top_to_gcs, vector)
        return vector

    def setup_coord_system(self, accel, mag, coord_sys_center, lat_lon):
        accel = self.recount_accel(accel)
        mag = self.recount_mag(mag)
        self.coord_sys_center = coord_sys_center

        self.dec_to_top = dec_to_top_matix(lat_lon)
        self.top_to_gcs = top_to_gcscs_matix(mag, accel, self.decl)

    def get_top_to_gcs(self):
        return self.top_to_gcs

    def get_dec_to_top(self):
        return self.dec_to_top

    def count_vector(self, object_coords):
        vector = object_coords - self.coord_sys_center
        vector = self.recount_vector(vector)
        return vector

    def count_target_angles(self, vector):
        vector = vector / NumPy.linalg.norm(vector)
        vector_phi = degrees(asin(vector[2]))
        vector_alpha = degrees(acos(vector[0]/((vector[0]^2 + vector[1]^2)^(1/2))))
        if vector[1] < 0:
            vector_alpha = 360 - vector_alpha

        return (vector_alpha, vector_phi)

    def north(self):
        return self.top_to_gcs[:,0]