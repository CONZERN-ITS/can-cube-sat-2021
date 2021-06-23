import datetime

import source.lib.i2cdev as i2cdev

import source.lib.DM422 as DM422
from source.lib.strela_ms_rpi import Lis3mdl, Lsm6ds3, WMM2020
from source.lib.gps_data import GPS_data
from source import drive_control
from source import data_control
from source import control_interface
from source import auto_guidance_math

from config import *

if __name__ == '__main__':
    # Initialization block
    # I2C data line initialization
    i2c = i2cdev.I2C(PORT_I2C)
    i2c.set_timeout(I2C_TIMEOUT)

    # Mag and Accel sensors initialization
    lis3mdl = Lis3mdl(i2c, LIS3MDL_ADRESS)
    lis3mdl.setup()
    lsm6ds3 = Lsm6ds3(i2c, LSM6DS3_ADRESS)
    lsm6ds3.setup()

    # GPS initialization
    gps = GPS_data()
    gps.setup()

    # Mag model initialization
    wwm = WMM2020()

    # Drive motors initialization
    vertical_motor = DM422.DM422_control_client(pul_pin=VSM_PUL_PIN,
                                          dir_pin=VSM_DIR_PIN,
                                          enable_pin=VSM_ENABLE_PIN,
                                          gearbox_num=VSM_GEARBOX_NUM,
                                          deg_per_step=VSM_DEG_PER_STEP,
                                          pos_dir_state=VSM_POS_DIR_STATE,
                                          stop_state=VSM_STOP_STATE)

    horizontal_motor = DM422.DM422_control_client(pul_pin=HSM_PUL_PIN,
                                            dir_pin=HSM_DIR_PIN,
                                            enable_pin=HSM_ENABLE_PIN,
                                            gearbox_num=HSM_GEARBOX_NUM,
                                            deg_per_step=HSM_DEG_PER_STEP,
                                            pos_dir_state=HSM_POS_DIR_STATE,
                                            stop_state=HSM_STOP_STATE)

    # Drive control interface initialization
    drive = drive_control.BowElectromechanicalDrive(vertical_motor=vertical_motor,
                                                    horizontal_motor=horizontal_motor)
    drive.setup_vertical_limits(positive_limits=VSM_POS_LIMIT_PINS_MAP,
                                negative_limits=VSM_NEG_LIMIT_PINS_MAP)
    drive.setup_horizontal_limits(positive_limits=HSM_POS_LIMIT_PINS_MAP,
                                  negative_limits=HSM_NEG_LIMIT_PINS_MAP)

    # Auto guidance math initialization
    guidance_math = auto_guidance_math.AutoGuidanceMath(accel_sensor=lsm6ds3,
                                                        mag_sensor=lis3mdl,
                                                        gps_sensor=gps,
                                                        sensors_accumulation_num=SENSORS_ACCUMULATION_NUM,
                                                        gps_accumulation_num=GPS_ACCUMULATION_NUM,
                                                        sensors_timeout=SENSORS_TIMEOUT,
                                                        gps_timeout=GPS_TIMEOUT,
                                                        mag_recount_matrix=MAG_RECOUNT_MATRIX,
                                                        mag_calibration_matrix=MAG_CALIBRATION_MATRIX,
                                                        mag_calibration_vector=MAG_CALIBRATION_VECTOR,
                                                        accel_recount_matrix=ACCEL_RECOUNT_MATRIX,
                                                        accel_calibration_matrix=ACCEL_CALIBRATION_MATRIX,
                                                        accel_calibration_vector=ACCEL_CALIBRATION_VECTOR)

    # Data source initialization
    if DATA_CONTROL_TYPE == 'MAVLink':
        data_object = data_control.MAVDataSource(connection_str_in=CONNECTION_STR_IN,
                                                 connection_str_out=CONNECTION_STR_OUT,
                                                 log_path=LOG_PATH)
    elif DATA_CONTROL_TYPE == 'ZMQ':
        data_object = data_control.ZMQDataSource(bus_bpcs=BUS_BPCS,
                                                 bus_bscp=BUS_BSCP,
                                                 topics=TOPICS)
    data_object.start()

    # Control interface (protocol) initialization
    if CONTROL_INTERFACE_TYPE == 'MAVITS':
        ctrl_interface = control_interface.MAVITSControlInterface(drive_object=drive,
                                                                  auto_guidance_math=guidance_math,
                                                                  aiming_period=DEFAULT_ANTENNA_AIMING_PERIOD,
                                                                  min_rotation_angle=MIN_ROTATION_ANGLE)
    elif CONTROL_INTERFACE_TYPE == 'ZMQITS':
        ctrl_interface = control_interface.ZMQITSControlInterface(drive_object=drive,
                                                                  auto_guidance_math=guidance_math,
                                                                  aiming_period=DEFAULT_ANTENNA_AIMING_PERIOD,
                                                                  min_rotation_angle=MIN_ROTATION_ANGLE)

    # Preparation block
    try:
        guidance_math.setup_coord_system()
        date = datetime.date.today()
        wwm.setup_location(*(list(guidance_math.get_lat_lon()) +
                             list(guidance_math.get_alt()) +
                             [date.day, date.month, date.year]))
        guidance_math.set_decl(wwm.get_declination())
        guidance_math.setup_coord_system_math(guidance_math.get_accel(),
                                              guidance_math.get_mag(),
                                              guidance_math.get_x_y_z(),
                                              guidance_math.get_lat_lon())
    except Exception as e:
        print(e)
        pass

    while True:
        try:
            msgs = data_object.read_data()
        except RuntimeError as e:
            continue
        except Exception as e:
            print(e)
            continue

        ctrl_interface.msg_reaction([msgs])

    i2c.close()
