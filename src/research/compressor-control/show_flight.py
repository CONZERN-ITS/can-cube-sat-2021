# -*- coding: utf-8 -*-
"""
This example demonstrates many of the 2D plotting capabilities
in pyqtgraph. All of the plots may be panned/scaled by dragging with 
the left/right mouse buttons. Right click on any plot to show a context menu.
"""

from csv import DictReader

from pyqtgraph.Qt import QtGui, QtCore
import numpy as np
import pyqtgraph as pg


app = QtGui.QApplication([])


win = pg.GraphicsLayoutWidget(show=True, title="Basic plotting examples")
win.resize(1000,600)
win.setWindowTitle('pyqtgraph example: Plotting')

with open("Debug/data.csv") as stream:
    reader = DictReader(stream)
    data = list(reader)

t = np.array([float(x['minutes']) for x in data])
inner_pressure = np.array([float(x['inner_pressure']) for x in data])
outer_pressure = np.array([float(x['outer_pressure']) for x in data])
# Enable antialiasing for prettier plots
pg.setConfigOptions(antialias=True)
p1 = win.addPlot(x=t, y=inner_pressure, pen='g')
p2 = p1.plot(x=t, y=outer_pressure, pen='r')


if __name__ == '__main__':
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        QtGui.QApplication.instance().exec_()