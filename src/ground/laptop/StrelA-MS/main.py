#!/usr/bin/python3
import logging

from PyQt5 import QtWidgets, QtCore
from sys import argv, exit, stdout

from source.source import MainWindow


if __name__ == "__main__":
    logging.basicConfig(stream=stdout, level=logging.INFO, format="%(asctime)-15s [%(name)s] <%(levelname)s>: %(message)s")
    slg = logging.getLogger("source").setLevel(logging.DEBUG)

    QtCore.QCoreApplication.setOrganizationName("ITS")
    QtCore.QCoreApplication.setApplicationName("StrelA_MS")

    application = QtWidgets.QApplication(argv)
    window = MainWindow()
    window.show()
    exit(application.exec_())
    application.exit()
