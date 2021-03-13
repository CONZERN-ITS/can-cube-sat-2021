from PyQt5 import QtWidgets, QtGui, QtCore, QtMultimedia, QtMultimediaWidgets
import time

from source import settings_control
from source import RES_ROOT
import os

DEFAULT_ALARM = QtMultimedia.QMediaContent(QtCore.QUrl.fromLocalFile(os.path.join(RES_ROOT, 'images/alarm.mp3')))

class EventWidget(QtWidgets.QWidget):


    class AbstractEvent(QtCore.QObject):
        STATE_ALL_IS_FINE = 0
        STATE_ALARM = 1
        STATE_IRRELEVANT_DATE = 2
        STATE_CHECK = 3
        STATE_ATTEMPT_CHECK = 4
        STATE_TIMEOUT_CHECK = 5

        TYPE_WARNING = 'Warning'
        TYPE_ERROR = 'Error'

        event_begin = QtCore.pyqtSignal()
        event_end = QtCore.pyqtSignal()
        relevance_changed = QtCore.pyqtSignal(bool)

        def __init__(self, name, packet_name, index, trigger_value, time_limit, event_type=None, priority=1):
            super(EventWidget.AbstractEvent, self).__init__()
            self.trigger_value = trigger_value
            self.packet_name = packet_name
            self.index = index

            self.name = name
            self.event_type = event_type

            self.state = self.STATE_ALL_IS_FINE
            self.timer = QtCore.QTimer()
            self.timer.setSingleShot(True)
            self.timer.setInterval(int(time_limit * 1000))
            self.timer.timeout.connect(self.timeout_action)

            self.set_event_index()
            self.set_priority(priority)

        def get_packet_name(self):
            return self.packet_name

        def get_name(self):
            return self.name

        def get_event_type(self):
            return self.event_type

        def set_event_index(self, index=None):
            self.event_index = index

        def get_event_index(self):
            return self.event_index

        def set_playlist_index(self, index=None):
            self.playlist_index = index

        def get_playlist_index(self):
            return self.playlist_index

        def set_priority(self, priority=1):
            self.priority = priority

        def get_priority(self):
            return self.priority

        def check(self, data):
            if self.state == self.STATE_ALL_IS_FINE:
                if self.check_condition(data):
                    self.begin_event()
            elif (self.state == self.STATE_ALARM) or (self.state == self.STATE_IRRELEVANT_DATE):
                if not self.check_condition(data):
                    self.state = self.STATE_CHECK
            else:
                if self.check_condition(data):
                    self.state == self.STATE_ALARM
                else:
                    if self.state == self.STATE_ATTEMPT_CHECK:
                        self.end_event()
                    else:
                        self.state = self.STATE_TIMEOUT_CHECK
                    return

            self.change_relevance(True)
            self.timer.start()

        def check_condition(self, data):
            return any(bool(data))

        def begin_event(self):
            self.state = self.STATE_ALARM
            self.event_begin.emit()

        def end_event(self):
            self.state = self.STATE_ALL_IS_FINE
            self.event_end.emit()

        def change_relevance(self, relevance):
            if relevance:
                if self.state == self.STATE_IRRELEVANT_DATE:
                    self.relevance_changed.emit(relevance)
                    self.state = self.STATE_ALARM
            else:
                if self.state != self.STATE_IRRELEVANT_DATE:
                    self.relevance_changed.emit(relevance)
                    self.state = self.STATE_IRRELEVANT_DATE

        def timeout_action(self):
            if self.state == self.STATE_CHECK:
                self.state = self.STATE_ATTEMPT_CHECK
            elif self.state == self.STATE_TIMEOUT_CHECK:
                self.event_end.emit()
            self.change_relevance(False)

        def clear(self):
            self.timer.stop()
            self.state = self.STATE_ALL_IS_FINE
            self.set_event_index()


    class OutOfRangeEvent(AbstractEvent):
        def check_condition(self, data):
            return ((data[self.index] < self.trigger_value[0]) or (data[self.index] > self.trigger_value[1]))


    class UnderRangeEvent(AbstractEvent):
        def check_condition(self, data):
            return (data[self.index] < self.trigger_value)


    class OverRangeEvent(AbstractEvent):
        def check_condition(self, data):
            return (data[self.index] > self.trigger_value)


    class EqualToEvent(AbstractEvent):
        def check_condition(self, data):
            return (data[self.index] == self.trigger_value)


    class Event():
        def __init__(self, name, event_type=None, relevance=True):
            self.name = name
            self.event_type = event_type
            self.relevance = relevance
            self.set_enabled(True)
            self.set_is_viewed(False)

        def get_name(self):
            return self.name

        def get_type(self):
            return self.event_type

        def get_relevance(self):
            return self.relevance

        def set_relevance(self, relevance=True):
            self.relevance = relevance

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


    class EventModel(QtCore.QAbstractTableModel):
        def __init__(self, event_list=[]):
            super(EventWidget.EventModel, self).__init__()
            self.event_list = event_list

        def set_background_color(self, color):
            self.background_brush = QtGui.QBrush(color)

        def set_background_attention_color(self, color):
            self.background_attention_brush = QtGui.QBrush(color)

        def is_event_enabled(self, row):
            return self.event_list[row].get_enabled()

        def rowCount(self, parent=QtCore.QModelIndex()):
            return len(self.event_list)

        def columnCount(self, parent=QtCore.QModelIndex()):
            return 4

        def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
            if orientation != QtCore.Qt.Horizontal:
                return QtCore.QVariant()

            if role == QtCore.Qt.DisplayRole:
                if section == 0:
                    return QtCore.QVariant("Type")

                elif section == 1:
                    return QtCore.QVariant("Name")

                elif section == 2:
                    return QtCore.QVariant("Start time")

                elif section == 3:
                    return QtCore.QVariant("Stop time")
            return QtCore.QVariant()

        def data(self, index, role):
            if role == QtCore.Qt.DisplayRole:
                event = self.event_list[index.row()]
                if index.column() == 0:
                    return self.get_event_type_item(event.get_type())

                elif index.column() == 1:
                    return QtCore.QVariant(event.get_name() + 
                                           self.get_event_relevance_str(event.get_relevance()))

                elif index.column() == 2:
                    return QtCore.QVariant(self.get_event_time_str(event.get_start_time()) + '\n' + 
                                           self.get_event_time_str(event.get_stop_time()))

            elif role == QtCore.Qt.BackgroundRole:
                event = self.event_list[index.row()]
                if event.get_is_viewed():
                    return self.background_brush
                else:
                    return self.background_attention_brush

            return QtCore.QVariant()

        def beginReset(self):
            self.beginResetModel()

        def endReset(self):
            self.endResetModel()

        def add_event(self, event):
            self.event_list.append(event)
            return len(self.event_list) - 1

        def get_event_list(self):
            return self.event_list

        def get_event_time_str(self, event_time):
            if event_time > 0:
                return time.strftime("%H-%M-%S", time.gmtime(event_time))
            else:
                return '..-..-..'

        def get_event_relevance_str(self, relevance=True):
            if relevance:
                return ''
            else:
                return '\n(Not relevant)'

        def get_event_type_item(self, event_type):
            if event_type == EventWidget.AbstractEvent.TYPE_WARNING:
                return QtCore.QVariant('W')
            elif event_type == EventWidget.AbstractEvent.TYPE_ERROR:
                return QtCore.QVariant('E')
            return QtCore.QVariant('?')

        def clear(self):
            self.beginReset()
            self.event_list = []
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

        print(self.playlist.mediaCount())

        for tree in [self.active_events_tree, self.events_tree]:
            tree.setPalette(palette)
            tree.setStyleSheet("QHeaderView::section { background-color:" + self.settings.value("background_color") + '}') 
            for i in range(5):
                tree.resizeColumnToContents(i)

        self.event_list = []
        self.settings.beginGroup("Event")
        for group in self.settings.childGroups():
            self.settings.beginGroup(group)
            self.event_list.append(self.init_event(self.settings.value("identifier"),
                                                   self.settings.value("name"),
                                                   self.settings.value("packet_name"),
                                                   self.settings.value("index"),
                                                   self.settings.value("trigger_value"),
                                                   self.settings.value("time_limit"),
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
            pack = data.get(event.get_packet_name(), None)
            if pack is not None:
                event.check(pack[-1])

    def clear_data(self):
        self.events.clear()
        for event in self.event_list:
            event.clear()

        self.player.stop()

