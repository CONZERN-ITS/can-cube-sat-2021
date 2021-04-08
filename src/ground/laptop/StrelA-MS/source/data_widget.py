from PyQt5 import QtWidgets, QtGui, QtCore

from source import settings_control


class DataWidget(QtWidgets.QTreeWidget):

    class AbstractTreeItem(QtWidgets.QTreeWidgetItem):
        background_color = QtGui.QColor('#000000')
        timeout_color = QtGui.QColor('#FF00FF')
        timer = None

        def set_background_color(self, background_color):
            self.background_color = background_color

        def setup_fields(self, group_name=None):
            if group_name is not None:
                self.setText(0, group_name)
                self.setText(1, '')

        def setup_background(self, color):
            for i in range(self.columnCount()):
                self.setBackground(i, color)

        def set_value(self, value):
            self.setup_background(self.background_color)
            self.setText(1, str(value))
            if self.timer is not None:
                self.timer.start()

        def get_value(self):
            return self.text(1)

        def setup_timeout(self, color, time_limit):
            self.timeout_color = color
            self.timer = QtCore.QTimer()
            self.timer.setSingleShot(True)
            self.timer.setInterval(int(time_limit * 1000))
            self.timer.timeout.connect(self.timeout_action)

        def timeout_action(self):
            self.setup_background(self.timeout_color)
            for i in range(self.childCount()):
                DataWidget.AbstractTreeItem.setup_background(self.child(i), self.timeout_color)


    class TimeTreeItem(AbstractTreeItem):
        sourse_id_list = []

        def get_sourse_id_list(self):
            return self.sourse_id_list

        def set_sourse_id_list(self, sourse_id_list=[]):
            self.sourse_id_list = sourse_id_list


    class PacketCountTreeItem(AbstractTreeItem):
        packet_id = [None, None]

        def get_packet_id(self):
            return self.packet_id

        def set_packet_id(self, packet_id):
            self.packet_id = packet_id


    class DataTreeItem(AbstractTreeItem):
        packet_id = [None, None]
        data_range = None
        colors = [QtGui.QColor('#0000FF'), QtGui.QColor('#FF0000')]

        def get_packet_id(self):
            return self.packet_id

        def set_packet_id(self, packet_id):
            self.packet_id = packet_id

        def setup_fields(self, names, group_name=None):
            super(DataWidget.DataTreeItem, self).setup_fields(group_name)
            for name in names:
                item = QtWidgets.QTreeWidgetItem(self)
                item.setText(0, name)
            self.set_data_range([])


        def set_data(self, data_list):
            self.setup_background(self.background_color)
            sorted_keys = sorted(data_list.keys())

            if len(sorted_keys) <= self.childCount():
                for i in range(len(sorted_keys)):
                    self.child(i).setText(1, str(data_list[sorted_keys[i]]))
                    DataWidget.AbstractTreeItem.setup_background(self.child(i), self.background_color)
            else:
                for i in range(self.childCount()):
                    self.child(i).setText(1, str(data_list[sorted_keys[i]]))
                    DataWidget.AbstractTreeItem.setup_background(self.child(i), self.background_color)   
                for i in range(self.childCount(), len(sorted_keys)):
                    item = QtWidgets.QTreeWidgetItem(self)
                    item.setText(0, str(sorted_keys[i]))
                    item.setText(1, str(data_list[sorted_keys[i]]))
                    DataWidget.AbstractTreeItem.setup_background(item, self.background_color)          

            if self.data_range is not None:
                for i in range(min(len(self.data_range), len(sorted_keys))):
                    if self.data_range[i] != 'None':
                        if (self.data_range[i][0] != 'None') and (data_list[sorted_keys[i]] < self.data_range[i][0]):
                            DataWidget.TreeItem.setup_background(self.child(i), self.colors[0])
                        elif (self.data_range[i][1] != 'None') and (data_list[sorted_keys[i]] > self.data_range[i][1]):
                            DataWidget.TreeItem.setup_background(self.child(i), self.colors[1])
            if self.timer is not None:
                self.timer.start()

        def set_data_range(self, data_range):
            self.data_range = data_range

        def set_colors(self, colors):
            self.colors = colors


    def __init__(self):
        super(DataWidget, self).__init__()
        self.settings = settings_control.init_settings()

        self.setup_ui()
        self.setup_ui_design()
        self.update_current_values()

    def setup_ui(self):
        self.setColumnCount(2)

    def setup_ui_design(self):
        self.clear()
        self.settings.beginGroup("CentralWidget/DataWidget")
        self.background_color = QtGui.QColor(self.settings.value("background_color"))
        self.headerItem().setText(0, '')
        self.headerItem().setText(1, '')
        palette = QtGui.QPalette()
        palette.setColor(QtGui.QPalette.Base, self.background_color)
        self.setPalette(palette)
        self.setStyleSheet("QHeaderView::section { background-color:" + self.settings.value("background_color") + '}')
        self.colors = tuple([QtGui.QColor(self.settings.value("colors")[i]) for i in range(3)])

        self.table_dict = {}
        self.time_tuple = []

        self.time_table = 0
        self.packet_table = 0

        if self.settings.value("Time_table/is_on") != False:
            self.time_table = 1
            self.settings.beginGroup("Time_table")
            top_item = QtWidgets.QTreeWidgetItem()
            top_item.setText(0, 'Systems time')
            time_limit = self.settings.value("def_time_limit")
            if time_limit is None:
                time_limit = 1
            for group in self.settings.childGroups():
                if self.settings.value(group + "/is_on") != False:
                    self.settings.beginGroup(group)
                    item = DataWidget.TimeTreeItem(top_item)
                    item.set_background_color(self.background_color)
                    item.setup_fields(self.settings.value("system_name"))
                    item.set_sourse_id_list(self.settings.value("sourse_id"))
                    if self.settings.value("time_limit") is not None:
                        item.setup_timeout(self.colors[2], self.settings.value("time_limit"))
                    else: 
                        item.setup_timeout(self.colors[2], time_limit)
                    self.settings.endGroup()
            self.addTopLevelItem(top_item)
            self.settings.endGroup()

        if self.settings.value("Packet_table/is_on") != False:
            self.packet_table = 1
            self.settings.beginGroup("Packet_table")
            top_item = QtWidgets.QTreeWidgetItem()
            top_item.setText(0, 'Packets count')
            for group in self.settings.childGroups():
                if self.settings.value(group + "/is_on") != False:
                    self.settings.beginGroup(group)
                    item = DataWidget.PacketCountTreeItem(top_item)
                    item.set_background_color(self.background_color)
                    item.setup_fields(self.settings.value("name"))
                    item.set_packet_id((self.settings.value("sourse_id"), self.settings.value("message_id")))
                    item.set_value(0)
                    self.settings.endGroup()
            self.addTopLevelItem(top_item)
            self.settings.endGroup()

        if self.settings.value("Data_table/is_on") != False:
            self.settings.beginGroup("Data_table")
            top_item = QtWidgets.QTreeWidgetItem()
            top_item.setText(0, 'Data')
            time_limit = self.settings.value("def_time_limit")
            if time_limit is None:
                time_limit = 1
            for group in self.settings.childGroups():
                if self.settings.value(group + "/is_on") != False:
                    self.settings.beginGroup(group)
                    item = DataWidget.DataTreeItem(top_item)
                    item.set_background_color(self.background_color)
                    field_name = self.settings.value("field_name")
                    if field_name is None:
                        field_name = []
                    item.setup_fields(field_name, self.settings.value("name"))
                    item.set_packet_id((self.settings.value("sourse_id"), self.settings.value("message_id")))
                    if self.settings.value("time_limit") is not None:
                        item.setup_timeout(self.colors[2], self.settings.value("time_limit"))
                    else: 
                        item.setup_timeout(self.colors[2], time_limit)
                    if self.settings.value("range") != None:
                    	item.set_data_range(self.settings.value("range"))
                    item.set_colors(self.colors[:2])
                    self.settings.endGroup()
                self.addTopLevelItem(top_item)
            self.settings.endGroup()
        self.settings.endGroup()

        for i in range(self.topLevelItemCount()):
            self.expandItem(self.topLevelItem(i))
            for j in range(self.topLevelItem(i).childCount()):
                self.expandItem(self.topLevelItem(i).child(j))

        self.resizeColumnToContents(0)

    def update_current_values (self):
        pass

    def new_data_reaction(self, data):
        if self.time_table:
            for i in range(self.topLevelItem(0).childCount()):
                item = self.topLevelItem(0).child(i)
                last_msg = None
                for msg in data[::-1]:
                    for sourse_id in item.get_sourse_id_list():
                        if msg.get_source_id() == sourse_id:
                            last_msg = msg
                            break
                if last_msg is not None:
                    item.set_value(last_msg.get_time())

        if self.packet_table:
            for i in range(self.topLevelItem(self.time_table).childCount()):
                item = self.topLevelItem(self.time_table).child(i)
                packet_id = item.get_packet_id()
                packet_count = 0
                for msg in data:
                    if (msg.get_source_id() == packet_id[0]) or (packet_id[0] == None):
                        if (msg.get_message_id() == packet_id[1]) or (packet_id[1] == None):
                            packet_count += 1
                item.set_value(int(item.get_value()) + packet_count)

        for j in range((self.time_table + self.packet_table), self.topLevelItemCount()):
            for i in range(self.topLevelItem(j).childCount()):
                item = self.topLevelItem(j).child(i)
                packet_id = item.get_packet_id()
                last_msg = None
                for msg in data[::-1]:
                    if (msg.get_source_id() == packet_id[0]) and (msg.get_message_id() == packet_id[1]):
                            last_msg = msg
                            break
                if last_msg is not None:
                    item.set_data(last_msg.get_data_dict())

    def clear_data(self):
        self.setup_ui_design()
