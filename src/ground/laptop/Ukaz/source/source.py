from PyQt5 import QtWidgets, QtGui, QtCore
import numpy as NumPy
import time

from math import nan

from source import settings_control
from source import command_widget
from source import status_widget
from source.data_control import *
from source.command_interface import *
from source import LOG_FOLDER_PATH

class CentralWidget(QtWidgets.QWidget):
    def __init__(self):
        super(CentralWidget, self).__init__()
        self.settings = settings_control.init_settings()

        self.setup_ui()
        self.setup_ui_design()

    def setup_ui(self):
        self.layout = QtWidgets.QHBoxLayout(self)

        self.command_widget = command_widget.CommandWidget()
        self.layout.addWidget(self.command_widget)

    def new_data_reaction (self, data):
        pass

    def clear_data(self):
        self.command_widget.clear()

    def setup_ui_design(self):
        self.command_widget.setup_ui_design()


class MainWindow(QtWidgets.QMainWindow):
    class DataManager(QtCore.QObject):
        new_data = QtCore.pyqtSignal(object)
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
                #except Exception as e:
                #    print(e)
                else:
                    self.new_data.emit(data)
                self.mutex.lock()
                close = self.close_flag
                self.mutex.unlock()

        def write_data(self, msg):
            self.mutex.lock()
            if not self.close_flag:
                self.data_obj.write_data(msg)
            self.mutex.unlock()

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

        #self.setWindowIcon(QtGui.QIcon(APP_ICON_PATH))

        self.setup_ui()
        self.setup_ui_design()

        self.move_to_center()

    def setup_ui(self):
        self.menu_bar = self.menuBar()
        self.menu_file = self.menu_bar.addMenu("&File")
        self.action_exit = self.menu_file.addAction("&Exit")
        self.action_exit.setShortcut('Ctrl+Q')
        self.action_exit.triggered.connect(QtWidgets.qApp.quit)

        self.msg_box = QtWidgets.QMessageBox()
        self.msg_box.setText('Command is generated')
        self.msg_box.setStandardButtons(QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No)
        self.msg_box.setDefaultButton(QtWidgets.QMessageBox.No)
        self.msg_box.setInformativeText("Do you want to send command?")
        self.msg_box.setIcon(QtWidgets.QMessageBox.Question)

        self.central_widget = CentralWidget()
        self.setCentralWidget(self.central_widget)

        self.interface = self.get_data_interface()

        self.data_obj = self.get_data_object()
        self.central_widget.command_widget.send_msg.connect(self.send_msg)

        self.data_manager = MainWindow.DataManager(self.data_obj)
        self.interface.send_msg.connect(self.data_manager.write_data)
        self.data_thread = QtCore.QThread(self)
        self.data_manager.moveToThread(self.data_thread)
        self.data_thread.started.connect(self.data_manager.start)
        self.data_manager.new_data.connect(self.interface.msg_reaction)
        self.data_thread.start()

    def setup_ui_design(self):
        self.resize(600, 800)
        self.setWindowTitle("Ukaz")

        self.menu_file.setTitle("&File")
        self.action_exit.setText("&Exit")
        self.action_exit.setStatusTip("Exit")

    def send_msg(self, data):
        print('msg')
        msg = self.interface.generate_message(*data)
        if msg is not None:
            self.msg_box.setDetailedText(str(msg))
            if self.msg_box.exec() == QtWidgets.QMessageBox.Yes:
                print(msg)
                self.interface.send_command(msg)

    def get_data_object(self):
        self.settings.beginGroup('MainWindow/DataSourse')
        sourse = self.settings.value('type')
        if sourse == 'MAVLink':
            data = MAVDataSource(connection_str_in=self.settings.value('MAVLink/connection_in'),
                                 connection_str_out=self.settings.value('MAVLink/connection_out'),
                                 log_path=LOG_FOLDER_PATH)
        elif sourse == 'ZMQ':
            data = ZMQDataSource(bus_pub=self.settings.value('ZMQ/bus_bpcs'),
                                 bus_sub=self.settings.value('ZMQ/bus_bscp'),
                                 log_path=LOG_FOLDER_PATH)
        self.settings.endGroup()
        return data

    def get_data_interface(self):
        self.settings.beginGroup('MainWindow/DataInterface')
        sourse = self.settings.value('type')
        if sourse == 'MAVITS':
            interface = MAVITSInterface()
        elif sourse == 'ZMQITS':
            interface = ZMQITSInterface()
        else:
        	interface = AbstractCommanInterface()
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
