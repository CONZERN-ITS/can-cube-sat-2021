import pyqtgraph as PyQtGraph
import numpy as NumPy

from source import settings_control

import math


class GraphWidget(PyQtGraph.GraphicsLayoutWidget):


    class Plot():


        class CurveGroup():


            class Curve():
                def __init__(self, curve, field_id):
                    self.curve = curve
                    self.arr = None
                    self.field_id = field_id
                    self.max_data_length = None

                def get_field_id(self):
                    return self.field_id

                def set_max_data_len(self, length=None):
                    self.max_data_length = length

                def setup_pen(self, color):
                    self.curve.setPen(color=color)

                def add_data(self, data):
                    if self.arr is None:
                        self.arr = data
                    else:
                        self.arr = NumPy.vstack((self.arr, data))

                    if (self.max_data_length is not None) and (len(self.arr) > self.max_data_length):
                        self.arr = self.arr[len(self.arr) - self.max_data_length:-1]

                def show_data(self):
                    if self.arr is not None:
                        self.curve.setData(self.arr)

                def clear(self):
                    self.arr = None
                    self.curve.setData(NumPy.array([[0, 0]]))


            def __init__(self, plot, packet_id, colors=None):
                self.plot = plot
                self.curves = []
                self.arr = None
                self.packet_id = packet_id
                if colors is None:
                    colors = []
                self.colors = colors
                self.set_autocomplete()
                self.max_data_len = None

            def get_autocomplete(self):
                return self.autocomplete

            def set_autocomplete(self, autocomplete=True):
                self.autocomplete = autocomplete

            def get_packet_id(self):
                return self.packet_id

            def add_curve(self, curve, field_id):
                self.curves.append(GraphWidget.Plot.CurveGroup.Curve(curve, field_id))
                if len(self.curves) <= len(self.colors):
                    self.curves[-1].setup_pen(self.colors[len(self.curves) - 1])
                else:
                    self.curves[-1].setup_pen('w')
                self.curves[-1].set_max_data_len(self.max_data_len)

            def show_data(self):
                for curve in self.curves:
                    curve.show_data()

            def get_curves(self):
                return self.curves

            def set_max_data_len(self, length):
                self.max_data_len = length
                for curve in self.curves:
                    curve.set_max_data_len(length)

            def add_data(self, data, x_shift=0):
                time = data.get_time() - x_shift
                for item in data.get_data_dict().items():
                    flag = True
                    for curve in self.curves:
                        if curve.get_field_id() == item[0]:
                            curve.add_data(NumPy.array([[time, item[1]]]))
                            flag = False
                            break
                    if self.autocomplete and flag:
                        self.add_curve(self.plot.plot(NumPy.array([[0, 0]])), item[0])
                        self.curves[-1].add_data(NumPy.array([[item[1], time]]))


            def clear(self):
                for curve in self.curves:
                    curve.clear()

        def __init__(self, plot):
            self.plot = plot
            self.curve_groups = []
            self.max_data_length = None
            self.legend = None

        def set_max_data_len(self, length):
            if length <= 0:
                self.max_data_length = None
            else:
                self.max_data_length = length

        def setup_axis(self, name_x, name_y):
            axis_x = PyQtGraph.AxisItem(orientation='bottom')
            axis_x.setLabel(name_x)
            axis_y = PyQtGraph.AxisItem(orientation='left')
            axis_y.setLabel(name_y)
            self.plot.setAxisItems({'bottom': axis_x, 'left': axis_y})

        def add_curve_group(self, packet_id, field_id=None, field_name=None, colors=None):
            self.curve_groups.append(GraphWidget.Plot.CurveGroup(self.plot, packet_id, colors))
            if field_id is not None:
                if field_name is not None:
                    names_count = len(field_name)
                else:
                    names_count = 0
                for i in range(len(field_id)):
                    curve = self.plot.plot(NumPy.array([[0, 0]]))
                    self.curve_groups[-1].add_curve(curve, field_id[i])
                    if self.legend is not None:
                        if names_count > i:
                            self.legend.addItem(curve, field_name[i]) 
                        else:
                            self.legend.addItem(curve, field_id[i])
                self.curve_groups[-1].set_autocomplete(False)

        def setup_plot_legend(self):
            self.legend = self.plot.addLegend()

        def get_curve_groups(self):
            return self.curve_groups

        def clear(self):
            for group in self.curve_groups:
                group.clear()


    def __init__(self):
        super(GraphWidget, self).__init__()
        self.settings = settings_control.init_settings()

        self.setup_ui()
        self.setup_ui_design()
        self.update_current_values()

    def setup_ui(self):
        self.plots = []

    def get_param(self, beg_num, count, param_list):
        if len(param_list) >= beg_num:
            param = param_list[beg_num:]
            if len(param) > count:
                param = param[:count]
        else:
            param = []
        return param

    def setup_ui_design(self):
        for plot in self.plots:
            try:
                self.removeItem(plot.plot)
            except KeyError:
                pass
        self.plots = []

        self.settings.beginGroup("CentralWidget/GraphWidget")
        x_label_str = self.settings.value("time_units")
        if x_label_str is None:
            x_label_str = 's'
        x_label_str = 'Time '+ x_label_str
        self.settings.beginGroup("Graph")
        for group in self.settings.childGroups():
            self.settings.beginGroup(group)
            if self.settings.value("is_on") != False:
                self.plots.append(GraphWidget.Plot(self.addPlot(*[int(num) for num in self.settings.value("position")])))
                units = self.settings.value("units")
                if units is None:
                    units = ''
                self.plots[-1].setup_axis(x_label_str, self.settings.value("name") + ' ' + units)
                max_data_length = self.settings.value("max_data_length")
                if max_data_length is not None:
                    self.plots[-1].set_max_data_len(max_data_length)
                if self.settings.value("legend_is_on") != False:
                    self.plots[-1].setup_plot_legend()
                self.settings.beginGroup("Packets")
                for group in self.settings.childGroups():
                    self.settings.beginGroup(group)
                    if self.settings.value("is_on") != False:
                        self.plots[-1].add_curve_group(packet_id=[self.settings.value("sourse_id"), self.settings.value("message_id")],
                                                       field_id=self.settings.value("field_id"),
                                                       field_name=self.settings.value("field_name"),
                                                       colors=self.settings.value("color"),)

                    self.settings.endGroup()
                self.settings.endGroup()
            self.settings.endGroup()
        self.plots = tuple(self.plots)
        self.settings.endGroup()
        self.settings.endGroup()

    def update_current_values (self):
        self.time_shift = self.settings.value("CurrentValues/time_shift")

    def new_data_reaction(self, data):
        for plot in self.plots:
            for curve_group in plot.get_curve_groups():
                packet_id = curve_group.get_packet_id()
                for msg in data:
                    if (msg.get_source_id() == packet_id[0]) and (msg.get_message_id() == packet_id[1]):
                        curve_group.add_data(msg, x_shift=self.time_shift)
                curve_group.show_data() 

    def clear_data(self):
        for plot in self.plots:
            plot.clear()
