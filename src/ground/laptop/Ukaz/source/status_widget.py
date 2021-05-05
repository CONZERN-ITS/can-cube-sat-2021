from PyQt5 import QtWidgets, QtGui, QtCore, QtMultimedia, QtMultimediaWidgets
import time

from source import settings_control
from source import RES_ROOT
import os


class StatusWidget(QtWidgets.QWidget):


    class StatusModel(QtCore.QAbstractTableModel):


        class Command():
            STATUS_UNKNOWN = 0
            STATUS_PROCESSING = 1
            STATUS_SUCCSESS = 2
            STATUS_FAILURE = 3

            def __init__(self, cookie, name, status='Undefined', status_type=STATUS_UNKNOWN):
                self.name = name
                self.cookie = cookie
                self.status = status
                self.set_enabled(True)
                self.set_is_viewed(False)
                self.set_status(status)
                self.set_status_type(status_type)

            def get_name(self):
                return self.name

            def get_cookie(self):
                return self.cookie

            def get_status(self):
                return self.status

            def set_status(self, status='Undefined'):
                self.status = status

            def get_status_type(self):
                return self.status_type

            def set_status_type(self, status_type=STATUS_UNKNOWN):
                self.status_type = status_type

            def set_is_viewed(self, is_viewed=False):
                self.is_viewed = is_viewed

            def get_is_viewed(self):
                return self.is_viewed

            def set_enabled(self, enabled=True):
                self.enabled = enabled
                if enabled:
                    self.start_time = time.time()
                    self.stop_time = 0
                else:
                    self.stop_time = time.time()

            def get_enabled(self):
                return self.enabled

            def get_start_time(self):
                return self.start_time

            def get_stop_time(self):
                return self.stop_time


        def __init__(self, cmd_list=[]):
            super(StatusWidget.StatusModel, self).__init__()
            self.cmd_list = cmd_list

        def set_background_color(self, color):
            self.background_brush = QtGui.QBrush(color)

        def set_background_success_color(self, color):
            self.background_success_brush = QtGui.QBrush(color)

        def set_background_processing_color(self, color):
            self.background_processing_brush = QtGui.QBrush(color)

        def set_background_failure_color(self, color):
            self.background_failure_brush = QtGui.QBrush(color)

        def is_event_enabled(self, row):
            return self.cmd_list[row].get_enabled()

        def is_event_viewed(self, row):
            return self.cmd_list[row].get_is_viewed()

        def rowCount(self, parent=QtCore.QModelIndex()):
            return len(self.cmd_list)

        def columnCount(self, parent=QtCore.QModelIndex()):
            return 5

        def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
            if orientation != QtCore.Qt.Horizontal:
                return QtCore.QVariant()

            if role == QtCore.Qt.DisplayRole:
                if section == 0:
                    return QtCore.QVariant("Cookie")

                elif section == 1:
                    return QtCore.QVariant("Name")

                elif section == 2:
                    return QtCore.QVariant("Status")

                elif section == 3:
                    return QtCore.QVariant("Start time")

                elif section == 4:
                    return QtCore.QVariant("Stop time")
            return QtCore.QVariant()

        def data(self, index, role):
            if role == QtCore.Qt.DisplayRole:
                cmd = self.cmd_list[index.row()]
                if index.column() == 0:
                    return QtCore.QVariant(cmd.get_cookie())

                elif index.column() == 1:
                    return QtCore.QVariant(cmd.get_name())

                elif index.column() == 2:
                    return QtCore.QVariant(cmd.get_status())

                elif index.column() == 3:
                    return QtCore.QVariant(self.get_cmd_time_str(cmd.get_start_time()))

                elif index.column() == 4:
                    return QtCore.QVariant(self.get_cmd_time_str(cmd.get_stop_time()))

            elif role == QtCore.Qt.BackgroundRole:
                cmd = self.cmd_list[index.row()]
                status_type = cmd.get_status_type()
                if status_type == StatusWidget.StatusModel.Command.STATUS_PROCESSING:
                    return self.background_processing_brush
                elif status_type == StatusWidget.StatusModel.Command.STATUS_SUCCSESS:
                    return self.background_success_brush
                elif status_type == StatusWidget.StatusModel.Command.STATUS_FAILURE:
                    return self.background_failure_brush
                else:
                    return self.background_brush

            return QtCore.QVariant()

        def beginReset(self):
            self.beginResetModel()

        def endReset(self):
            self.endResetModel()

        def update_cmd(self, cookie, status, status_type):
            for cmd in self.cmd_list:
                if cmd.get_cookie() == cookie:
                    self.beginReset()
                    cmd.set_status_type(status_type)
                    cmd.set_status(status)
                    if (status_type == StatusWidget.StatusModel.Command.STATUS_FAILURE) or (status_type == StatusWidget.StatusModel.Command.STATUS_SUCCSESS):
                        cmd.set_enabled(False)
                    self.endReset()
                    return
            self.add_command(cookie, status, status_type, name='Undefined')

        def add_command(self, cookie, status='Undefined', status_type=Command.STATUS_UNKNOWN, name=''):
            cmd = StatusWidget.StatusModel.Command(name=name,
                              cookie=cookie,
                              status=status,
                              status_type=status_type)

            self.beginReset()
            self.cmd_list.append(cmd)
            self.endReset()

        def get_cmd_list(self):
            return self.cmd_list

        def get_cmd_time_str(self, cmd_time):
            if cmd_time > 0:
                return time.strftime("%H-%M-%S", time.gmtime(cmd_time))
            else:
                return '..-..-..'

        def clear(self):
            self.beginReset()
            self.cmd_list = []
            self.endReset()


    class SortFilterProxyStatusModel(QtCore.QSortFilterProxyModel):

        def filterAcceptsRow(self, sourceRow, sourceParent):
            return self.sourceModel().is_event_enabled(sourceRow) or self.sourceModel().is_event_enabled(sourceRow)


    def __init__(self):
        super(StatusWidget, self).__init__()
        self.settings = settings_control.init_settings()

        self.setup_ui()
        self.setup_ui_design()

    def setup_ui(self):
        self.layout = QtWidgets.QVBoxLayout(self)

        self.cmds = StatusWidget.StatusModel()
        self.processing_cmds = StatusWidget.SortFilterProxyStatusModel()
        self.processing_cmds.setSourceModel(self.cmds)

        self.cmds_tree = QtWidgets.QTreeView()
        self.layout.addWidget(self.cmds_tree)
        self.cmds_tree.setModel(self.cmds)

        self.processing_cmds_tree = QtWidgets.QTreeView()
        self.layout.addWidget(self.processing_cmds_tree)
        self.processing_cmds_tree.setModel(self.processing_cmds)

        self.viewed_btn = QtWidgets.QPushButton('Viewed')
        self.layout.addWidget(self.viewed_btn)

    def setup_ui_design(self):
        self.settings.beginGroup("CentralWidget/StatusWidget")
        self.background_color = QtGui.QColor(self.settings.value("background_color"))
        palette = QtGui.QPalette()
        palette.setColor(QtGui.QPalette.Base, self.background_color)

        self.cmds.clear()
        self.cmds.set_background_color(self.background_color)
        self.cmds.set_background_success_color(QtGui.QColor(self.settings.value("succsess_color")))
        self.cmds.set_background_processing_color(QtGui.QColor(self.settings.value("processing_color")))
        self.cmds.set_background_failure_color(QtGui.QColor(self.settings.value("failure_color")))

        for tree in [self.processing_cmds_tree, self.cmds_tree]:
            tree.setPalette(palette)
            tree.setStyleSheet("QHeaderView::section { background-color:" + self.settings.value("background_color") + '}') 
            for i in range(5):
                tree.resizeColumnToContents(i)

        self.viewed_btn.clicked.connect(self.viewed_btn_action)
        self.settings.endGroup()

    def add_command(self, name, cookie):
        self.cmds.add_command(cookie=cookie,
                                  name=name)

    def viewed_btn_action(self):
        self.cmds.beginReset()
        for cmd in self.cmds.get_cmd_list():
            if not cmd.is_viewed():
                cmds.set_is_viewed(True)
        self.cmds.endReset()

    def new_msg_reaction(self, data):
        self.cmds.update_cmd(*data)

    def clear_data(self):
        self.cmds.clear()

