from PyQt5 import QtWidgets, QtGui, QtCore
import os.path
import yaml
import re
from source import RES_ROOT, USER_SETTINGS_PATH

SETTINGS_PATH = USER_SETTINGS_PATH
if 'AMCS_CONFIG' in os.environ:
    if os.path.exists(os.environ['AMCS_CONFIG']):
        SETTINGS_PATH = os.environ['AMCS_CONFIG']


class Settings():


    class Settings_data():
        file_path = None
        def __new__(self, *args, **kwargs):
            if not hasattr(self, '_instance'):
                self._instance = super(Settings.Settings_data, self).__new__(self)
            return self._instance

        def __init__(self, file_path):
            self.file_path = os.path.abspath(file_path)
            try:
                self.load()
            except FileNotFoundError:
                self.data = {}
                self.save()

        def fileName(self):
            return self.file_path

        def get_data(self, path_list,  data=None):
            if data is None:
                cell = self.data
            else:
                cell = data
            for name in path_list[0:-1]:
                cell = cell.get(name, {})
            return cell.get(path_list[-1], None)

        def update_data(self, path_list, value, data=None):
            if data is None:
                cell = self.data
            else:
                cell = data
            if len(path_list) > 1:
                cell.update({path_list[0] : self.update_data(path_list[1:], value, cell.get(path_list[0], {}))})
            else:
                cell.update({path_list[0] : value})
            return cell

        def load(self):
            file = open(self.file_path, 'r', encoding = "utf-8")
            self.data = yaml.load(file, Loader=yaml.SafeLoader)
            if self.data is None:
                self.data = {}
            file.close()

        def save(self):
            file = open(self.file_path, 'w', encoding = "utf-8")
            yaml.dump(self.data, file, default_flow_style = False, allow_unicode = True, encoding = None)
            file.close()


    def __init__(self, path):
        self.data = Settings.Settings_data(path)
        self.prefix_list = []

    def fileName(self):
        return self.data.fileName()

    def setValue(self, path, value):
        path_list = []
        for part in self.prefix_list:
            path_list.extend(part)
        path_list.extend(self.path_to_list(path))
        self.data.update_data(path_list, value)

    def value(self, path):
        path_list = []
        for part in self.prefix_list:
            path_list.extend(part)
        path_list.extend(self.path_to_list(path))
        value = self.data.get_data(path_list)
        return value

    def path_to_list(self, path):
        if path[-1] == '/':
            raise ValueError('Invalid parameter path %s' % path)
        return re.split(r'/', path)

    def beginGroup(self, path):
        self.prefix_list.append(self.path_to_list(path))

    def endGroup(self):
        self.prefix_list.pop()

    def childGroups(self):
        path_list = []
        for part in self.prefix_list:
            path_list.extend(part)
        group_list = []
        data = self.data.get_data(path_list)
        if (data is not None) and (isinstance(data, dict)):
            for cell in data.items():
                if isinstance(cell[1], dict):
                    group_list.append(cell[0])
        return group_list

    def childKeys(self):
        path_list = []
        for part in self.prefix_list:
            path_list.extend(part)
        keys_list = []
        data = self.data.get_data(path_list)
        if (data is not None) and (isinstance(data, dict)):
            for cell in data.items():
                if not isinstance(cell[1], dict):
                    keys_list.append(cell[0])
        return keys_list


def init_settings():
    if not os.path.exists(SETTINGS_PATH):
        set_to_default(SETTINGS_PATH)
    settings = Settings(SETTINGS_PATH)
    return settings

def set_to_default(file_path):
    config = open(file_path, 'w')
    def_config = open(DEFAULT_SETTINGS_PATH, 'r')
    for line in def_config:
        config.write(line)
    config.close()
    def_config.close()