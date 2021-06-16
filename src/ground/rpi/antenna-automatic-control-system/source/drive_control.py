

class AbstractElectromechanicalDrive():
    def __init__(self, vertical_motor=None, horizontal_motor=None):
        # Subsystems
        self.vertical_motor = vertical_motor
        self.horizontal_motor = horizontal_motor

        # Variables default value
        self.vertical_position = 0
        self.horizontal_position = 0

        self.vertical_limits = {}
        self.horizontal_limits = {}
        self.last_vertical_limit = None
        self.last_horizontal_limit = None

        self.setup_drive_timeout()
        self.set_drive_auto_disable_mode()

    def setup_vertical_motor(self, vertical_motor):
        self.vertical_motor = vertical_motor

    def setup_horizontal_motor(self, horizontal_motor):
        self.horizontal_motor = horizontal_motor

    def setup_vertical_limits(self, positive_limits, negative_limits):
        pass

    def setup_horizontal_limits(self, positive_limits, negative_limits):
        pass

    def setup_drive_timeout(self, timeout=1):
        self.drive_timeout = timeout

    def set_drive_auto_disable_mode(self, mode=False):
        self.drive_auto_disable_mode = mode

    def set_vertical_motor_enabled(self, mode):
        pass

    def set_horizontal_motor_enabled(self, mode):
        pass

    def vertical_rotation(self, angle):
        return None

    def horizontal_rotation(self, angle):
        return None

    def get_vertical_limits(self):
        return self.vertical_limits

    def get_horizontal_limits(self):
        return self.horizontal_limits

    def get_last_vertical_limit(self):
        return self.last_vertical_limit

    def get_last_horizontal_limit(self):
        return self.last_horizontal_limit

    def get_vertical_position(self):
        return self.vertical_position

    def get_horizontal_position(self):
        return self.horizontal_position

    def get_vertical_enable_state(self):
        return None

    def get_horizontal_enable_state(self):
        return None

    def get_drive_timeout(self):
        return self.drive_timeout

    def get_drive_auto_disable_mode(self):
        return self.drive_auto_disable_mode


class BowElectromechanicalDrive(AbstractElectromechanicalDrive):
    def __init__(self, *args, **kwargs):
        super(BowElectromechanicalDrive, self).__init__(*args, **kwargs)

    def setup_vertical_limits(self, positive_limits, negative_limits):
        if self.vertical_motor is not None:
            self.vertical_motor.setup_stop_triggers(list(positive_limits.keys()), list(negative_limits.keys()))
            self.vertical_limits = positive_limits
            self.vertical_limits.update(negative_limits)

    def setup_horizontal_limits(self, positive_limits, negative_limits):
        if self.horizontal_motor is not None:
            self.horizontal_motor.setup_stop_triggers(list(positive_limits.keys()), list(negative_limits.keys()))
            self.horizontal_limits = positive_limits
            self.horizontal_limits.update(negative_limits)

    def set_vertical_motor_enabled(self, mode):
        self.vertical_motor.set_enable(mode)

    def set_horizontal_motor_enabled(self, mode):
        self.horizontal_motor.set_enable(mode)

    def vertical_rotation(self, angle):
        if self.vertical_motor is not None:
            self.last_vertical_limit = self.vertical_motor.rotate_using_angle(ang)
            phi = self.vertical_motor.steps_to_angle(self.vertical_motor.get_last_steps_num(),
                                                     self.vertical_motor.get_last_steps_direction())
            if self.last_vertical_limit is not None:
                self.vertical_position = self.vertical_limits.get(self.last_vertical_limit)
            else:
                self.vertical_position += phi

            return phi
        return None

    def horizontal_rotation(self, angle):
        if self.horizontal_motor is not None:
            self.last_horizontal_limit = self.horizontal_motor.rotate_using_angle(ang)
            phi = self.horizontal_motor.steps_to_angle(self.horizontal_motor.get_last_steps_num(),
                                                       self.horizontal_motor.get_last_steps_direction())
            if self.last_horizontal_limit is not None:
                self.horizontal_position = self.horizontal_limits.get(self.last_horizontal_limit)
            else:
                self.horizontal_position += phi
            return phi
        return None

    def get_vertical_enable_state(self):
        if self.vertical_motor is not None:
            return self.vertical_motor.get_enable_state()
        return None

    def get_horizontal_enable_state(self):
        if self.horizontal_motor is not None:
            return self.horizontal_motor.get_enable_state()
        return None