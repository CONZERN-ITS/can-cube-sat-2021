from PyQt5 import QtWidgets, QtGui, QtCore

from source import settings_control

import time
import math


class CommandWidget(QtWidgets.QFrame):
    send_msg = QtCore.pyqtSignal(list)


    class CommandItem(QtWidgets.QFrame):
        send_msg = send_msg = QtCore.pyqtSignal()


        class AbstractCommandField():
            def __init__(self, name):
                self.name = name
                self.layout = QtWidgets.QHBoxLayout()
                self.layout.addWidget(QtWidgets.QLabel(str(name)), 1)

            def get_name(self):
                return self.name

            def get_data(self):
                return None

            def clear(self):
                pass

            def update_data(self):
                pass

            def get_qt_object(self):
                return self.layout


        class CheckBoxCommandField(AbstractCommandField):
            def __init__(self, name):
                super(CommandWidget.CommandItem.CheckBoxCommandField, self).__init__(name)
                self.qt_object = QtWidgets.QCheckBox()
                self.qt_object.setTristate(false)
                self.layout.addWidget(self.qt_object, 1)

            def get_data(self):
                return bool(self.qt_object.checkState())

            def clear(self):
                self.qt_object.setCheckState(0)


        class StrCommandField(AbstractCommandField):
            def __init__(self, name):
                super(CommandWidget.CommandItem.StrCommandField, self).__init__(name)
                self.qt_object = QtWidgets.QLineEdit()
                self.layout.addWidget(self.qt_object, 1)

            def get_data(self):
                return self.qt_object.text()

            def clear(self):
                self.qt_object.setText('')


        class IntCommandField(StrCommandField):
            def __init__(self, name):
                super(CommandWidget.CommandItem.IntCommandField, self).__init__(name)
                self.qt_object.setValidator(QtGui.QIntValidator())
                self.qt_object.setText('0')

            def get_data(self):
                return int(self.qt_object.text())


        class FloatCommandField(StrCommandField):
            def __init__(self, name):
                super(CommandWidget.CommandItem.FloatCommandField, self).__init__(name)
                self.qt_object.setValidator(QtGui.QDoubleValidator())
                self.qt_object.setText('0')

            def get_data(self):
                return float(self.qt_object.text())


        class AbstractTimeCommandField(AbstractCommandField):
            def __init__(self, name):
                super(CommandWidget.CommandItem.AbstractTimeCommandField, self).__init__(name)
                self.qt_object = QtWidgets.QFrame()
                self.qt_object.setFrameStyle(QtWidgets.QFrame.Panel|QtWidgets.QFrame.Plain)
                self.qt_object.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Maximum)
                self.container = QtWidgets.QLabel('None')
                layout = QtWidgets.QVBoxLayout()
                layout.addWidget(self.container)
                self.qt_object.setLayout(layout)
                self.layout.addWidget(self.qt_object, 1)

            def count_time(self):
                return None

            def get_data(self):
                return self.count_time()

            def update_data(self):
                self.container.setText(str(self.count_time()))

            def clear(self):
                self.container.setText('None')


        class TimeSCommandField(AbstractTimeCommandField):
            def count_time(self):
                return int(math.modf(time.time())[1])


        class TimeUSCommandField(AbstractTimeCommandField):
            def count_time(self):
                return int(math.modf(time.time())[0] * 1000000)


        def __init__(self, name, msg_name):
            super(CommandWidget.CommandItem, self).__init__()
            self.msg_name = msg_name
            self.item_layout = QtWidgets.QVBoxLayout(self)
            self.label = QtWidgets.QLabel(str(name))
            self.label.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)
            self.item_layout.addWidget(self.label)
            self.fields_layout = QtWidgets.QVBoxLayout()
            self.item_layout.addLayout(self.fields_layout)
            self.send_btn = QtWidgets.QPushButton('Send')
            self.send_btn.clicked.connect(self.send_msg.emit)
            self.item_layout.addWidget(self.send_btn)

            self.fields = []
            self.setFrameStyle(QtWidgets.QFrame.Panel|QtWidgets.QFrame.Raised)

        def get_msg_name(self):
            return self.msg_name

        def add_field(self, name, field_type):
            if field_type == 'str':
                self.fields.append(CommandWidget.CommandItem.StrCommandField(name))
            elif field_type == 'bool':
                self.fields.append(CommandWidget.CommandItem.CheckBoxCommandField(name))
            elif field_type == 'int':
                self.fields.append(CommandWidget.CommandItem.IntCommandField(name))
            elif field_type == 'float':
                self.fields.append(CommandWidget.CommandItem.FloatCommandField(name))
            elif field_type == 'time_s':
                self.fields.append(CommandWidget.CommandItem.TimeSCommandField(name))
            elif field_type == 'time_us':
                self.fields.append(CommandWidget.CommandItem.TimeUSCommandField(name))
            else:
                self.fields.append(CommandWidget.CommandItem.AbstractCommandField(name))

            self.fields_layout.addLayout(self.fields[-1].get_qt_object(), 1)

        def update_data(self):
            for field in self.fields:
                field.update_data()

        def clear(self):
            for field in self.fields:
                field.clear()

        def get_data(self):
            data_list = []
            for field in self.fields:
                data_list.append([field.get_name(), field.get_data()])

            return dict(data_list)


    def __init__(self):
        super(CommandWidget, self).__init__()
        self.settings = settings_control.init_settings()

        self.setup_ui()
        self.setup_ui_design()

    def setup_ui(self):
        self.timer = QtCore.QTimer()
        self.timer.setInterval(200)
        self.timer.timeout.connect(self.update_data)
        self.timer.setSingleShot(False)
        self.timer.start()
        self.layout = QtWidgets.QGridLayout(self)
        self.commands = []

        self.cookie = (time.time() // 1000) % 1000 

    def setup_ui_design(self):
        self.settings.beginGroup("CentralWidget/CommandWidget")

        for command in self.commands:
            command.send_msg.disconnect(self.send_command)
            command.deleteLater()
        self.commands = []

        for group in self.settings.childGroups():
            self.settings.beginGroup(group)
            if self.settings.value("is_on") != False:
                self.commands.append(CommandWidget.CommandItem(self.settings.value("name"), self.settings.value("msg_name")))
                self.layout.addWidget(self.commands[-1], *[int(num) for num in self.settings.value("position")])
                for group in self.settings.childGroups():
                    self.settings.beginGroup(group)
                    name = self.settings.value("name")
                    if name is None:
                        name = group
                    self.commands[-1].add_field(name, self.settings.value("type"))
                    self.settings.endGroup()
                self.commands[-1].send_msg.connect(self.send_command)
            self.settings.endGroup()
        self.settings.endGroup()

    def update_data(self):
        for command in self.commands:
            command.update_data()

    def send_command(self):
        sender = self.sender()
        if isinstance(sender, CommandWidget.CommandItem):
            sender.get_data()
            self.send_msg.emit([sender.get_msg_name(), sender.get_data(), self.cookie])
            self.cookie += 1

    def clear_data(self):
        for command in self.commands:
            command.clear()
