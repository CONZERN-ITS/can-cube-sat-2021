from PyQt5 import QtWidgets, QtGui, QtCore, QtMultimedia, QtMultimediaWidgets
import time

from source import settings_control
from source import RES_ROOT
import os


class StatusWidget(QtWidgets.QWidget):
          

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


    class StatusModel(QtCore.QAbstractTableModel):
        def __init__(self, command_list=[]):
            super(StatusWidget.StatusModel, self).__init__()
            self.command_list = command_list

        def set_background_color(self, color):
            self.background_brush = QtGui.QBrush(color)

        def set_background_success_color(self, color):
            self.background_success_brush = QtGui.QBrush(color)

        def set_background_processing_color(self, color):
            self.background_processing_brush = QtGui.QBrush(color)

        def set_background_failure_color(self, color):
            self.background_failure_brush = QtGui.QBrush(color)

        def is_event_enabled(self, row):
            return self.command_list[row].get_enabled()

        def rowCount(self, parent=QtCore.QModelIndex()):
            return len(self.command_list)

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
                command = self.command_list[index.row()]
                if index.column() == 0:
                    return QtCore.QVariant(command.get_cookie())

                elif index.column() == 1:
                    return QtCore.QVariant(command.get_name())

                elif index.column() == 2:
                    return QtCore.QVariant(command.get_status())

                elif index.column() == 3:
                    return QtCore.QVariant(self.get_command_time_str(command.get_start_time()))

                elif index.column() == 4:
                    return QtCore.QVariant(self.get_command_time_str(command.get_stop_time()))

            elif role == QtCore.Qt.BackgroundRole:
                command = self.command_list[index.row()]
                status_type = command.status_type()
                if status_type == STATUS_PROCESSING:
                    return self.background_processing_brush
                elif status_type == STATUS_SUCCSESS:
                    return self.background_success_brush
                elif status_type == STATUS_FAILURE:
                    return self.background_failure_brush
                else:
                    return self.background_brush

            return QtCore.QVariant()

        def beginReset(self):
            self.beginResetModel()

        def endReset(self):
            self.endResetModel()

        def update_command(self, cookie, status, status_type):
            self.event_list.append(event)
            return len(self.event_list) - 1

        def add_command(self, cookie, status, status_type, name=''):
            command = Command(name=name,
                              cookie=cookie,
                              status=status,
                              status_type=status_type)

            self.beginReset()
            self.command_list.append(command)
            self.endReset()

        def get_command_list(self):
            return self.command_list

        def get_command_time_str(self, command_time):
            if command_time > 0:
                return time.strftime("%H-%M-%S", time.gmtime(command_time))
            else:
                return '..-..-..'

        def clear(self):
            self.beginReset()
            self.command_list = []
            self.endReset()


    class SortFilterProxyEventModel(QtCore.QSortFilterProxyModel):

        def filterAcceptsRow(self, sourceRow, sourceParent):
            return self.sourceModel().is_event_enabled(sourceRow)


    def __init__(self):
        super(EventWidget, self).__init__()
        self.settings = settings_control.init_settings()

        self.setup_ui()
        self.setup_ui_design()
        self.update_current_values()

    def setup_ui(self):
        self.layout = QtWidgets.QVBoxLayout(self)

        self.events = EventWidget.EventModel()
        self.active_events = EventWidget.SortFilterProxyEventModel()
        self.active_events.setSourceModel(self.events)

        self.events_tree = QtWidgets.QTreeView()
        self.layout.addWidget(self.events_tree)
        self.events_tree.setModel(self.events)

        self.active_events_tree = QtWidgets.QTreeView()
        self.layout.addWidget(self.active_events_tree)
        self.active_events_tree.setModel(self.active_events)

        self.viewed_btn = QtWidgets.QPushButton('Viewed')
        self.layout.addWidget(self.viewed_btn)

        self.player = QtMultimedia.QMediaPlayer()
        self.player.setVolume(50)

    def init_event(self, identifier, *args, **kwargs):
        if identifier == 'OutOfRange':
            return EventWidget.OutOfRangeEvent(*args, **kwargs)
        elif identifier == 'UnderRange':
            return EventWidget.UnderRangeEvent(*args, **kwargs)
        elif identifier == 'OverRange':
            return EventWidget.OverRangeEvent(*args, **kwargs)
        elif identifier == 'EqualTo':
            return EventWidget.EqualToEvent(*args, **kwargs)
        else:
            return EventWidget.AbstractEvent(*args, **kwargs)

    def setup_ui_design(self):
        self.settings.beginGroup("CentralWidget/EventWidget")
        self.background_color = QtGui.QColor(self.settings.value("background_color"))
        palette = QtGui.QPalette()
        palette.setColor(QtGui.QPalette.Base, self.background_color)
        self.attention_color = QtGui.QColor(self.settings.value("attention_color"))

        self.events.clear()
        self.events.set_background_color(self.background_color)
        self.events.set_background_attention_color(self.attention_color)

        self.playlist = QtMultimedia.QMediaPlaylist()
        self.playlist.setPlaybackMode(QtMultimedia.QMediaPlaylist.CurrentItemInLoop)
        self.player.setPlaylist(self.playlist)
        self.playlist.insertMedia(0, DEFAULT_ALARM)
        self.player.stop()

        for tree in [self.active_events_tree, self.events_tree]:
            tree.setPalette(palette)
            tree.setStyleSheet("QHeaderView::section { background-color:" + self.settings.value("background_color") + '}') 
            for i in range(5):
                tree.resizeColumnToContents(i)

        self.event_list = []
        self.settings.beginGroup("Event")
        def_time_limit = self.settings.value("def_time_limit")
        if def_time_limit is None:
            def_time_limit = 1
        for group in self.settings.childGroups():
            self.settings.beginGroup(group)
            time_limit = self.settings.value("time_limit")
            if time_limit is None:
                time_limit = def_time_limit
            self.event_list.append(self.init_event(self.settings.value("identifier"),
                                                   self.settings.value("name"),
                                                   [self.settings.value("sourse_id"), self.settings.value("message_id")],
                                                   self.settings.value("field_id"),
                                                   self.settings.value("trigger_value"),
                                                   time_limit,
                                                   event_type=self.settings.value("event_type"),
                                                   priority=self.settings.value("priority")))
            if self.settings.value('audio') is not None:
                audio = QtMultimedia.QMediaContent(QtCore.QUrl.fromLocalFile(self.settings.value('audio')))
            else:
                audio = DEFAULT_ALARM
            self.event_list[-1].set_playlist_index(self.playlist.mediaCount())
            self.playlist.insertMedia(self.playlist.mediaCount(), audio)
            self.playlist_priority = 0
            self.event_list[-1].event_begin.connect(self.setup_event_item)
            self.event_list[-1].event_end.connect(self.archive_item)
            self.event_list[-1].relevance_changed.connect(self.change_item_relevance)
            self.settings.endGroup()
        self.viewed_btn.clicked.connect(self.viewed_btn_action)
        self.settings.endGroup()
        self.settings.endGroup()

    def viewed_btn_action(self):
        for event in self.event_list:
            self.events.beginReset()
            if event.get_event_index() is not None:
                self.events.get_event_list()[event.get_event_index()].set_is_viewed(True)
            self.events.endReset()

        self.playlist_priority = 0
        self.player.stop()

    def archive_item(self):
        sender = self.sender()
        if isinstance(sender, EventWidget.AbstractEvent):
            if sender.get_event_index() is not None:
                self.events.beginReset()
                self.events.get_event_list()[sender.get_event_index()].set_enabled(False)
                sender.set_event_index()
                self.events.endReset()

    def setup_event_item(self):
        sender = self.sender()
        if isinstance(sender, EventWidget.AbstractEvent):
            self.events.beginReset()
            event = EventWidget.Event(sender.get_name(), event_type=sender.get_event_type())
            sender.set_event_index(self.events.add_event(event))
            self.events.endReset()
            if self.playlist_priority < sender.get_priority():
                self.playlist.setCurrentIndex(sender.get_playlist_index())
            self.player.play()

    def change_item_relevance(self, relevance):
        sender = self.sender()
        if isinstance(sender, EventWidget.AbstractEvent):
            if sender.get_event_index() is not None:
                self.events.beginReset()
                self.events.get_event_list()[sender.get_event_index()].set_relevance(relevance)
                self.events.endReset()

    def update_current_values (self):
        pass

    def new_data_reaction(self, data):
        for event in self.event_list:
            packet_id = event.get_packet_id()
            last_msg = None
            for msg in data[::-1]:
                if (msg.get_source_id() == packet_id[0]) and (msg.get_message_id() == packet_id[1]):
                    last_msg = msg
                    break
            if last_msg is not None:
                event.check(msg)

    def clear_data(self):
        self.events.clear()
        for event in self.event_list:
            event.clear()

        self.player.stop()

