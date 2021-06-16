import numpy as NumPy

DATA_CONTROL_TYPE = 'MAVLink'
CONTROL_INTERFACE_TYPE = 'MAVITS'
DRIVE_TYPE = 'BOW'

if DATA_CONTROL_TYPE == 'MAVLink':
	CONNECTION_STR_IN = 'udpin:0.0.0.0:41313'
	CONNECTION_STR_OUT = 'udpin:0.0.0.0:13404'
	LOG_PATH = './'
elif DATA_CONTROL_TYPE == 'ZMQ':
	BUS_BPCS = "tcp://127.0.0.1:7778"
	BUS_BSCP = "tcp://127.0.0.1:7777"
	TOPICS = []


# I2C
PORT_I2C = 1
I2C_TIMEOUT = 1

# Sensors
SENSORS_ACCUMULATION_NUM = 50
SENSORS_TIMEOUT = 20

# Mag sensor
LIS3MDL_ADRESS = 0x1e
MAG_RECOUNT_MATRIX = NumPy.array([[-1, 0, 0],
                                  [ 0, 0, 1],
                                  [ 0, 1, 0]])

MAG_CALIBRATION_MATRIX = NumPy.array([[ 0.948874, -0.010167, 0.266488],
                                      [-0.010167,  1.230079, 0.015828],
                                      [ 0.266488,  0.015828, 1.178367]])

MAG_CALIBRATION_VECTOR = NumPy.array([[-0.784921], [-0.038389], [-0.837087]])

# Accel sensor
LSM6DS3_ADRESS = 0x6B

ACCEL_RECOUNT_MATRIX = NumPy.array([[1,  0,  0],
                                    [0, -1,  0],
                                    [0,  0, -1]])

ACCEL_CALIBRATION_MATRIX = None

ACCEL_CALIBRATION_VECTOR = None

# GPS
GPS_ACCUMULATION_NUM = 5
GPS_TIMEOUT = 20

# Vertical stepper motor
VSM_PUL_PIN = 18
VSM_DIR_PIN = 16
VSM_ENABLE_PIN = 32
VSM_GEARBOX_NUM = 102
VSM_DEG_PER_STEP = 1.8 / 8
VSM_POS_DIR_STATE = False
VSM_STOP_STATE = 0
VSM_POS_LIMIT_PINS_MAP = {7:90}
VSM_NEG_LIMIT_PINS_MAP = {11:-10.5}

# Horizontal stepper motor
HSM_PUL_PIN = 13
HSM_DIR_PIN = 15
HSM_ENABLE_PIN = 31
HSM_GEARBOX_NUM = 102
HSM_DEG_PER_STEP = 1.8 / 8
HSM_POS_DIR_STATE = False
HSM_STOP_STATE = 0
HSM_POS_LIMIT_PINS_MAP = {}
HSM_NEG_LIMIT_PINS_MAP = {}

# Antenna settings
DEFAULT_ANTENNA_AIMING_PERIOD = 1

# Drive settings
MOTORS_TIMEOUT = 20

# Auto guidance settings
MIN_ROTATION_ANGLE = 0.2