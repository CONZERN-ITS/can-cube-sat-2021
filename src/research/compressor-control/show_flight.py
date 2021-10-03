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


with open("data.csv") as stream:
    reader = DictReader(stream)
    data = list(reader)

t = np.array([float(x['minutes']) for x in data])
inner_pressure = np.array([float(x['inner_pressure']) for x in data])
outer_pressure = np.array([float(x['outer_pressure']) for x in data])
altitude = np.array([float(x['altitude']) for x in data])
state = np.array([float(x['state']) for x in data])
pump_on = np.array([float(x['pump_on']) for x in data])
valve_open = np.array([float(x['valve_open']) for x in data])

pg.setConfigOptions(antialias=True)
app = QtGui.QApplication([])
win = pg.PlotWidget()
win.show()


p1 = win.plotItem
p2 = pg.ViewBox()
p1.showAxis('right')
p1.scene().addItem(p2)
p1.getAxis('right').linkToView(p2)
p2.setXLink(p1)

## Handle view resizing 
def updateViews():
    global p1, p2, p3
    p2.setGeometry(p1.vb.sceneBoundingRect())
    p2.linkedViewChanged(p1.vb, p2.XAxis)

updateViews()
p1.vb.sigResized.connect(updateViews)
win.addLegend()

p1.setYRange(-40_000, 150_000)
p1.plot(x=t, y=inner_pressure, pen='g', name='pressure_in')
p1.plot(x=t, y=outer_pressure, pen='r', name='pressure_out')
p2.addItem(pg.PlotCurveItem(x=t, y=altitude, pen='b', name="altitude"))
p2.addItem(pg.PlotCurveItem(x=t, y=state*1000-3000, pen='w', name="state"))
p2.addItem(pg.PlotCurveItem(x=t, y=pump_on*2000-6000, pen='r', name="state"))
p2.addItem(pg.PlotCurveItem(x=t, y=valve_open*1000-6000, pen='g', name="state"))


if __name__ == '__main__':
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        QtGui.QApplication.instance().exec_()