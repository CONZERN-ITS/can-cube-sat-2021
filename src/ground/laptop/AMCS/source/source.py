from PyQt5 import QtWidgets, QtGui, QtCore
import os
from source import RES_ROOT
APP_ICON_PATH = os.path.join(RES_ROOT, "images/StrelA_MS.png")

os.environ['MAVLINK_DIALECT'] = "its"
os.environ['MAVLINK20'] = "its"
from pymavlink.dialects.v20 import its as mavlink2
from pymavlink import mavutil

from source import data_widget
from source import pos_control_widget
from source import log_widget
from source import settings_control
from source.data_control import *
from source.antenna_interface import *

import time


class CentralWidget(QtWidgets.QWidget):
    def __init__(self, antenna):
        super(CentralWidget, self).__init__()

        self.antenna = antenna

        self.setup_ui()
        self.setup_ui_design()

    def setup_ui(self):
        self.h_layout = QtWidgets.QHBoxLayout(self)

        self.position_widget = data_widget.PositionWidget()
        self.position_control_widget = pos_control_widget.PositionControlWidget()
        self.command_log = log_widget.CommandLogPanel()

        self.h_layout.addWidget(self.position_widget)
        self.h_layout.addWidget(self.position_control_widget)
        self.h_layout.addWidget(self.command_log)
        self.h_layout.setStretch(0, 2)
        self.h_layout.setStretch(1, 1)
        self.h_layout.setStretch(2, 1)

        self.antenna.antenna_pos_changed.connect(self.position_widget.change_antenna_pos)
        self.antenna.target_pos_changed.connect(self.position_widget.change_target_pos)
        self.antenna.lat_lon_alt_changed.connect(self.position_widget.change_lat_lon_alt)
        self.antenna.ecef_changed.connect(self.position_widget.change_ecef)
        self.antenna.top_to_ascs_matrix_changed.connect(self.position_widget.change_top_to_ascs_matrix)
        self.antenna.dec_to_top_matrix_changed.connect(self.position_widget.change_dec_to_top_matrix)
        self.antenna.control_mode_changed.connect(self.position_widget.change_control_mode)
        self.antenna.aiming_period_changed.connect(self.position_widget.change_aiming_period)
        self.antenna.motors_enable_changed.connect(self.position_widget.change_motors_enable)
        self.antenna.motors_auto_disable_mode_changed.connect(self.position_widget.change_motors_auto_disable_mode)
        self.antenna.motors_timeout_changed.connect(self.position_widget.change_motors_timeout)
        self.antenna.rssi_changed.connect(self.position_widget.change_rssi)

        self.position_control_widget.pos_control_panel.top_btn_clicked.connect(self.antenna.put_up)
        self.position_control_widget.pos_control_panel.bottom_btn_clicked.connect(self.antenna.put_down)
        self.position_control_widget.pos_control_panel.right_btn_clicked.connect(self.antenna.turn_right)
        self.position_control_widget.pos_control_panel.left_btn_clicked.connect(self.antenna.turn_left)
        self.position_control_widget.pos_control_panel.central_btn_clicked.connect(self.antenna.park)
        self.position_control_widget.control_btn.clicked.connect(self.control_btn_action)
        self.position_control_widget.mode_true_btn.toggled.connect(self.antenna.set_mode)
        self.position_control_widget.set_aiming_period_btn.clicked.connect(self.set_aiming_period_btn_action)
        self.position_control_widget.set_motors_timeout_btn.clicked.connect(self.set_motors_timeout_btn_action)

        self.antenna.command_sent.connect(self.command_log.add_data)

    def new_state_reaction(self):
        pass

    def setup_ui_design(self):
        pass

    def control_btn_action(self):
        try:
            self.antenna.manual_control(*[float(line.text()) for line in self.position_control_widget.pos_control_line_edit])
        except ValueError:
            pass

    def set_aiming_period_btn_action(self):
        try:
            self.antenna.set_aiming_period(float(self.position_control_widget.aiming_period_line_edit.text()))
        except ValueError:
            print(self.position_control_widget.aiming_period_line_edit.text())

    def set_motors_timeout_btn_action(self):
        try:
            self.antenna.set_motors_timeout(float(self.position_control_widget.motors_timeout_line_edit.text()))
        except ValueError:
            pass


class MainWindow(QtWidgets.QMainWindow):
    class DataManager(QtCore.QObject):
        new_data = QtCore.pyqtSignal(list)
        autoclose = QtCore.pyqtSignal(str)
        def __init__(self, data_obj):
            super(MainWindow.DataManager, self).__init__()
            self.data_obj = data_obj
            self.mutex = QtCore.QMutex()
            self._set_close_flag(True)

        def _set_close_flag(self, mode):
            self.mutex.lock()
            self.close_flag = mode
            self.mutex.unlock()

        def change_data_obj(self, data_obj):
            self.data_obj = data_obj

        def start(self):
            self._set_close_flag(False)
            close = False
            try:
                self.data_obj.start()
            except Exception as e:
                self.autoclose.emit(str(e))
                return
            while not close:
                try:
                    data = self.data_obj.read_data()
                except RuntimeError:
                    pass
                except EOFError as e:
                    self.autoclose.emit(str(e))
                    break
                except Exception as e:
                    print(e)
                else:
                    self.new_data.emit(data)
                self.mutex.lock()
                close = self.close_flag
                self.mutex.unlock()

        def write_data(self, msg):
            self.data_obj.write_data(msg)

        def quit(self):
            self._set_close_flag(True)
            time.sleep(0.01)
            try:
                self.data_obj.stop()
            except Exception as e:
                pass

    def __init__(self):
        super(MainWindow, self).__init__()

        self.settings = settings_control.init_settings()

        self.setWindowIcon(QtGui.QIcon(APP_ICON_PATH))

        self.setup_ui()
        self.setup_ui_design()

        self.move_to_center()

    def setup_ui(self):
        self.antenna = self.get_data_interface()

        self.toolbar = self.addToolBar('Commands')
        self.auto_control_on_btn = self.toolbar.addAction('Turn on\nautomatic\ncontrol')
        self.auto_control_on_btn.triggered.connect(self.antenna.automatic_control_on)

        self.auto_control_off_btn = self.toolbar.addAction('Turn off\nautomatic\ncontrol')
        self.auto_control_off_btn.triggered.connect(self.antenna.automatic_control_off)

        self.elevation_zero_btn = self.toolbar.addAction('Find elevation\nzero')
        self.elevation_zero_btn.triggered.connect(self.antenna.setup_elevation_zero)

        self.target_to_north_btn = self.toolbar.addAction('Turn target\nto north')
        self.target_to_north_btn.triggered.connect(self.antenna.target_to_north)

        self.motors_enable_pin_high = self.toolbar.addAction('Enable motors')
        self.motors_enable_pin_high.triggered.connect(self.antenna.pull_motors_enable_pin_high)

        self.motors_enable_pin_low = self.toolbar.addAction('Disable motors')
        self.motors_enable_pin_low.triggered.connect(self.antenna.pull_motors_enable_pin_low)

        self.auto_control_on_btn = self.toolbar.addAction('Turn on motors\nauto disable mode')
        self.auto_control_on_btn.triggered.connect(self.antenna.motors_auto_disable_on)

        self.auto_control_off_btn = self.toolbar.addAction('Turn off motors\nauto disable mode')
        self.auto_control_off_btn.triggered.connect(self.antenna.motors_auto_disable_off)

        self.setup_coord_system_btn = self.toolbar.addAction('Setup coord\nsystem')
        self.setup_coord_system_btn.triggered.connect(self.antenna.setup_coord_system)

        self.state_request_btn = self.toolbar.addAction('State\nrequest')
        self.state_request_btn.triggered.connect(self.antenna.state_request)

        self.menu_bar = self.menuBar()
        self.menu_file = self.menu_bar.addMenu("&File")
        self.action_exit = self.menu_file.addAction("&Exit")
        self.action_exit.setShortcut('Ctrl+Q')
        self.action_exit.triggered.connect(QtWidgets.qApp.quit)

        self.central_widget = CentralWidget(self.antenna)
        self.setCentralWidget(self.central_widget)

        self.data_obj = self.get_data_object()
        self.antenna.send_msg.connect(self.data_manager.write_data())

        self.data_manager = MainWindow.DataManager(self.data_obj)
        self.data_thread = QtCore.QThread(self)
        self.data_manager.moveToThread(self.data_thread)
        self.data_thread.started.connect(self.data_manager.start)
        self.data_manager.new_msg.connect(self.antenna.msg_reaction)
        self.data_thread.start()

    def setup_ui_design(self):
        self.resize(600, 800)
        self.setWindowTitle("Antenna manual control system")

        self.menu_file.setTitle("&File")
        self.action_exit.setText("&Exit")
        self.action_exit.setStatusTip("Exit")

    def get_data_object(self):
        self.settings.beginGroup('MainWindow/DataSourse')
        sourse = self.settings.value('type')
        if sourse == 'MAVLink':
            data = MAVDataSource(connection_str_in=self.settings.value('MAVLink/connection_in'),
                                 connection_str_out=self.settings.value('MAVLink/connection_out'),
                                 log_path=LOG_FOLDER_PATH)
        elif sourse == 'ZMQ':
            data = ZMQDataSource(bus_pub=self.settings.value('ZMQ/bus_pub'),
                                 bus_sub=self.settings.value('ZMQ/bus_sub'),
                                 topics=self.settings.value('ZMQ/topics'),
                                 log_path=LOG_FOLDER_PATH)
        self.settings.endGroup()
        return data

    def get_data_interface(self):
        self.settings.beginGroup('MainWindow/DataInterface')
        sourse = self.settings.value('type')
        if sourse == 'MAVITS':
            interface = MAVITSInterface()
        elif sourse == 'ZMQITS':
            interface = ZMQitsInterface()
        self.settings.endGroup()
        return interface

    def move_to_center(self):
        frame = self.frameGeometry()
        frame.moveCenter(QtWidgets.QDesktopWidget().availableGeometry().center())
        self.move(frame.topLeft())

    def closeEvent(self, evnt):
        self.data_manager.quit()
        self.data_thread.quit()
        super(MainWindow, self).closeEvent(evnt)



