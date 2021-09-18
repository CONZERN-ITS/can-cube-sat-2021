from PyQt5 import QtWidgets, QtGui, QtCore
import numpy as NumPy
import time
import logging

from math import nan

from source import settings_control
from source import map_widget
from source import graph_widget
from source import model_widget
from source import data_widget
from source import event_widget
from source.data_control import *
from source import LOG_FOLDER_PATH


_log = logging.getLogger(__name__)


class CentralWidget(QtWidgets.QWidget):
    current_values_changed = QtCore.pyqtSignal()
    def __init__(self):
        super(CentralWidget, self).__init__()
        self.settings = settings_control.init_settings()
        self.widgets_dict = {} 

        self.setup_ui()
        self.setup_ui_design()

    def setup_ui(self):
        self.grid_layout = QtWidgets.QGridLayout(self)

        self.settings.beginGroup("CentralWidget")
        if self.settings.value("GraphWidget/is_on") != False:
            self.widgets_dict.update([("GraphWidget", graph_widget.GraphWidget())])
        if self.settings.value("MapWidget/is_on") != False:
            self.widgets_dict.update([("MapWidget", map_widget.MapWidget())])
        if self.settings.value("ModelWidget/is_on") != False:
            self.widgets_dict.update([("ModelWidget", model_widget.ModelWidget())])
        if self.settings.value("DataWidget/is_on") != False:
            self.widgets_dict.update([("DataWidget", data_widget.DataWidget())])
        if self.settings.value("EventWidget/is_on") != False:
            self.widgets_dict.update([("EventWidget", event_widget.EventWidget())])

        for key in self.widgets_dict.keys():
            self.settings.beginGroup(key)
            position = self.settings.value("position")
            self.grid_layout.addWidget(self.widgets_dict[key], *[int(num) for num in position])
            self.settings.endGroup()
        self.settings.endGroup()

        for i in range(self.grid_layout.columnCount()):
            self.grid_layout.setColumnMinimumWidth(i, 50)
            self.grid_layout.setColumnStretch(i, 1)
        for i in range(self.grid_layout.rowCount()):
            self.grid_layout.setRowMinimumHeight(i, 50)
            self.grid_layout.setRowStretch(i, 1)

        self.connect_widgets()

    def new_data_reaction (self, data):
        for widget in self.widgets_dict.items():
            widget[1].new_data_reaction(data)

    def connect_widgets (self):
        for widget in self.widgets_dict.items():
            self.current_values_changed.connect(widget[1].update_current_values)

    def set_time_shift (self, shift=None):
        if shift is None:
            self.settings.setValue("CurrentValues/time_shift", self.settings.value("DefaultValues/time_shift"))
        else:
            self.settings.setValue("CurrentValues/time_shift", float(shift))

    def clear_data(self):
        for widget in self.widgets_dict.items():
            widget[1].clear_data()

    def setup_ui_design(self):
        for widget in self.widgets_dict.items():
            widget[1].setup_ui_design()

        self.set_time_shift()


class MainWindow(QtWidgets.QMainWindow):
    class DataManager(QtCore.QObject):
        new_data = QtCore.pyqtSignal(list)
        autoclose = QtCore.pyqtSignal(str)
        finished = QtCore.pyqtSignal()
        def __init__(self, data_obj, update_time=0.2):
            super(MainWindow.DataManager, self).__init__()
            self.data_obj = data_obj
            self.mutex = QtCore.QMutex()
            self._set_close_flag(True)
            self.last_time = 0
            self.update_time = update_time

        def _set_close_flag(self, mode):
            self.mutex.lock()
            self.close_flag = mode
            self.mutex.unlock()

        def get_last_time(self):
            self.mutex.lock()
            last_time = self.last_time
            self.mutex.unlock()
            return last_time

        def change_data_obj(self, data_obj):
            self.data_obj = data_obj

        def start(self):
            self._set_close_flag(False)
            close = False
            last_time = 0
            try:
                self.data_obj.start()
            except Exception as e:
                self.autoclose.emit(str(e))
                raise e
                return
            start_time = time.time()
            data_buf = []
            while not close:
                try:
                    data = self.data_obj.read_data()
                except RuntimeError:
                    pass
                except EOFError as e:
                    self.autoclose.emit(str(e))
                    break
                except Exception as e:
                    _log.exception("Error in data thread: %s", e)
                else:
                    data_buf.extend(data)
                    if (time.time() - start_time) > self.update_time:
                        if len(data_buf) > 0:
                            last_time = data_buf[-1].get_time()
                        self.new_data.emit(data_buf)
                        start_time = time.time()
                        data_buf = []
                self.mutex.lock()
                close = self.close_flag
                self.last_time = last_time
                self.mutex.unlock()

        def quit(self):
            self._set_close_flag(True)
            time.sleep(0.01)
            try:
                self.data_obj.stop()
            except Exception as e:
                pass
            last_time = 0
            self.finished.emit()

    def __init__(self):
        super(MainWindow, self).__init__()

        self.settings = settings_control.init_settings()

        self.setWindowIcon(QtGui.QIcon(settings_control.APP_ICON_PATH))

        self.setup_ui()
        self.setup_ui_design()

        self.move_to_center()

    def setup_ui(self):
        self.toolbar = self.addToolBar('Common')
        self.toolbar.setMovable(False)
        self.toolbar.setIconSize(QtCore.QSize(50, 50))
        self.toolbar.setContextMenuPolicy(QtCore.Qt.PreventContextMenu)
        self.connection_btn = self.toolbar.addAction('Connect')
        self.connection_btn.triggered.connect(self.connection_action)
        self.time_btn = self.toolbar.addAction('Reset time')
        self.clear_btn = self.toolbar.addAction('Clear data')

        self.settings_window = settings_control.SettingsWindow()
        self.settings_window.change_settings.connect(self.setup_ui_design)

        self.menu_bar = self.menuBar()
        self.menu_file = self.menu_bar.addMenu("&File")
        self.action_settings = self.menu_file.addAction("&Settings")
        self.action_settings.setShortcut('Ctrl+S')
        self.action_settings.triggered.connect(self.settings_window.show)
        self.action_exit = self.menu_file.addAction("&Exit")
        self.action_exit.setShortcut('Ctrl+Q')
        self.action_exit.triggered.connect(QtWidgets.qApp.quit)

        self.central_widget = CentralWidget()
        self.setCentralWidget(self.central_widget)
        self.clear_btn.triggered.connect(self.central_widget.clear_data)

        self.data_obj = self.get_data_object()
        self.data_manager = MainWindow.DataManager(self.data_obj,
                                                   update_time=float(self.settings.value('MainWindow/update_time')))
        self.data_thread = QtCore.QThread(self)
        self.data_manager.moveToThread(self.data_thread)
        self.data_thread.started.connect(self.data_manager.start)
        self.data_manager.new_data.connect(self.central_widget.new_data_reaction)
        self.data_manager.autoclose.connect(self.connection_action)
        self.data_manager.finished.connect(self.data_thread.quit)

        self.time_btn.triggered.connect(self.reset_time)

    def setup_ui_design(self):
        if not self.data_thread.isRunning():
            self.resize(*[int(num) for num in self.settings.value('MainWindow/size')])
            self.setWindowTitle("StrelA MS")

            self.menu_file.setTitle("&File")
            self.action_settings.setText("&Settings")
            self.action_settings.setStatusTip("Settings")
            self.action_exit.setText("&Exit")
            self.action_exit.setStatusTip("Exit")

            self.central_widget.setup_ui_design()
            self.settings_window.setup_ui_design()

    def move_to_center(self):
        frame = self.frameGeometry()
        frame.moveCenter(QtWidgets.QDesktopWidget().availableGeometry().center())
        self.move(frame.topLeft())

    def reset_time(self):
        self.central_widget.set_time_shift(self.data_manager.get_last_time())
        self.central_widget.current_values_changed.emit()
        self.central_widget.clear_data()

    def get_data_object(self):
        self.settings.beginGroup('MainWindow/DataSourse')
        sourse = self.settings.value('type')
        if sourse == 'Log':
            log = self.settings.value('Log/type')
            if log == "MAV":
                data = MAVLogDataSource(log_path=self.settings.value('Log/MAV/path'),
                                        real_time=int(self.settings.value('Log/MAV/real_time')),
                                        time_delay=float(self.settings.value('Log/MAV/time_delay')),
                                        notimestamps=bool(self.settings.value('Log/MAV/notimestamps')))
        elif sourse == 'MAVLink':
            data = MAVDataSource(connection_str=self.settings.value('MAVLink/connection'),
                                 log_path=LOG_FOLDER_PATH,
                                 notimestamps=bool(self.settings.value('MAVLink/notimestamps')))
        elif sourse == 'ZMQ':
            data = ZMQDataSource(bus_bpcs=self.settings.value('ZMQ/bus_bpcs'),
                                 topics=self.settings.value('ZMQ/topics'),
                                 log_path=LOG_FOLDER_PATH,
                                 notimestamps=bool(self.settings.value('ZMQ/notimestamps')))
        self.settings.endGroup()
        return data

    def closeEvent(self, evnt):
        self.settings_window.close()
        self.disconnect()
        QtCore.QMetaObject.invokeMethod(self.data_thread, "deleteLater")
        super(MainWindow, self).closeEvent(evnt)


    def connection_action(self, stat_bar_msg=None):
        if not self.data_thread.isRunning():
            self.connect()
        else:
            self.disconnect()
        if (stat_bar_msg is not None) and (stat_bar_msg is not None):
            pass#print(stat_bar_msg)

    def connect(self):
        self.central_widget.clear_data()
        self.settings_window.settings_enabled(False)
        self.central_widget.set_time_shift()
        self.central_widget.current_values_changed.emit()
        self.data_thread.start()
        self.connection_btn.setText("&Disconnect")

    def disconnect(self):
        self.data_manager.quit()
        start_time = time.time()
        while self.data_thread.isRunning():
            if (time.time() - start_time) > 2:
                break
            time.sleep(0.01)
        self.connection_btn.setText("&Connect")
        self.settings_window.settings_enabled(True)
