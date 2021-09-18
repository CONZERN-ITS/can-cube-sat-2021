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
                self.set_status(status)
                self.set_status_type(status_type)
                self.set_stage_id()

            def get_name(self):
                return self.name

            def get_cookie(self):
                return self.cookie

            def get_status(self):
                return self.status

            def set_status(self, status='Undefined'):
                self.status = status

            def get_stage_id(self):
                return self.stage_id

            def set_stage_id(self, stage_id=0):
                self.stage_id = stage_id

            def get_status_type(self):
                return self.status_type

            def set_status_type(self, status_type=STATUS_UNKNOWN):
                self.status_type = status_type

            def set_enabled(self, enabled=True):
                self.enabled = enabled
                if enabled:
                    self.start_time = time.time()
                    self.stop_time = 0
                else:
                    if self.stop_time == 0:
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

        def is_command_enabled(self, row):
            return self.cmd_list[row].get_enabled()

        def rowCount(self, parent=QtCore.QModelIndex()):
            return len(self.cmd_list)

        def columnCount(self, parent=QtCore.QModelIndex()):
            return 4

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
                    return QtCore.QVariant("Start time\nStop time")

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
                    return QtCore.QVariant(self.get_cmd_time_str(cmd.get_start_time()) + '\n' +
                                           self.get_cmd_time_str(cmd.get_stop_time()))

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

        def update_cmd(self, cookie, status='Undefined', status_type=Command.STATUS_UNKNOWN, stage_id=0, name='Undefined'):
            for cmd in self.cmd_list:
                if cmd.get_cookie() == cookie:
                    if cmd.get_stage_id() <= stage_id:
                        self.beginReset()
                        cmd.set_status_type(status_type)
                        cmd.set_status(status)
                        cmd.set_stage_id(stage_id)
                        if (status_type == StatusWidget.StatusModel.Command.STATUS_FAILURE) or (status_type == StatusWidget.StatusModel.Command.STATUS_SUCCSESS):
                            cmd.set_enabled(False)
                        self.endReset()
                    return

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
            return self.sourceModel().is_command_enabled(sourceRow)


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
        self.processing_cmds_tree.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.processing_cmds_tree.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.processing_cmds_tree.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.processing_cmds_tree.customContextMenuRequested.connect(self.menu_request)

        self.context_menu = QtWidgets.QMenu()
        self.delete_cmd_act = self.context_menu.addAction('&Delete command from viewer')
        self.delete_cmd_act.triggered.connect(self.delete_command)

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

        self.settings.endGroup()

    def menu_request(self, pos):
        self.context_menu.popup(self.processing_cmds_tree.mapToGlobal(pos))

    def add_command(self, name, cookie):
        self.cmds.update_cmd(cookie=cookie,
                             name=name)

    def delete_command(self):
        cmd = self.cmds.get_cmd_list()[self.processing_cmds.mapToSource(self.processing_cmds_tree.selectionModel().currentIndex()).row()]
        self.cmds.update_cmd(cmd.get_cookie(), 'deleted by operator', StatusWidget.StatusModel.Command.STATUS_FAILURE)

    def new_msg_reaction(self, msg):
        self.cmds.update_cmd(cookie=msg.get_cookie(),
                             status=msg.get_status(),
                             status_type=msg.get_status_type(),
                             stage_id=msg.get_stage_id(),
                             name=msg.get_name())

    def clear_data(self):
        self.cmds.clear()

