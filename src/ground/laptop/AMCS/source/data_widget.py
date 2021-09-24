from PyQt5 import QtWidgets, QtGui, QtCore

import math
import time


class PositionPanel(QtWidgets.QWidget):
    def __init__(self):
        super(PositionPanel, self).__init__()
        self.setMinimumSize(150, 150)
        palette = QtGui.QPalette()
        palette.setColor(QtGui.QPalette.Background, QtGui.QColor(225, 225, 225, 255))
        self.setPalette(palette)
        self.setAutoFillBackground(True) 

        self.target_color = QtGui.QColor(50, 50, 200, 255)
        self.antenna_color = QtGui.QColor(200, 50, 50, 255)
        self.background_color = QtGui.QColor(10, 10, 10, 255)

        self.setup_ui()
        self.setup_ui_design()

    def setup_ui(self):
        pass

    def setup_ui_design(self):
        self.set_antenna_pos()
        self.set_target_pos()

    def set_antenna_pos(self, azimuth=None, elevation=None):
        self.antenna_elevation = elevation
        self.antenna_azimuth = azimuth 
        self.update()

    def set_target_pos(self, azimuth=None, elevation=None):
        self.target_elevation = elevation
        self.target_azimuth = azimuth 
        self.update()

    def paintEvent(self, event):
        super(PositionPanel, self).paintEvent(event)
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing)
        painter.translate(event.rect().center())
        painter.rotate(270)
        diam = min(event.rect().height(), event.rect().width())

        self.draw_background(painter, event)
        self.draw_antenna(painter, event)
        self.draw_target(painter, event)

    def draw_antenna(self, painter, event):
        if (self.antenna_elevation is not None) and (self.antenna_azimuth is not None):
            self.setup_pen(painter, brush=self.antenna_color, width=5)
            painter.setBrush(QtCore.Qt.NoBrush)
            ws_rad = self.count_workspace(event)
            rad = math.cos(math.radians(self.antenna_elevation)) * ws_rad
            painter.save()
            painter.rotate(self.antenna_azimuth)
            self.draw_circle(painter, QtCore.QPoint(rad, 0), ws_rad * 0.2)
            painter.setBrush(self.antenna_color)
            self.draw_circle(painter, QtCore.QPoint(rad, 0), ws_rad * 0.05)
            painter.restore()

    def draw_target(self, painter, event):
        if (self.target_elevation is not None) and (self.target_azimuth is not None):
            self.setup_pen(painter, brush=self.target_color, width=5, cap_style=QtCore.Qt.RoundCap)
            painter.setBrush(QtCore.Qt.NoBrush)
            ws_rad = self.count_workspace(event)
            rad = math.cos(math.radians(self.target_elevation)) * ws_rad
            painter.save()
            painter.rotate(self.target_azimuth)
            self.draw_circle(painter, QtCore.QPoint(rad, 0), ws_rad * 0.2)
            self.draw_cross(painter, QtCore.QPoint(rad, 0), ws_rad * 0.2)
            painter.restore()

    def count_workspace(self, event):
        diam = min(event.rect().height(), event.rect().width())
        return diam / 2 - 10

    def setup_pen(self, painter, brush=None, width=None, style=None, cap_style=None, join=None):
        pen = painter.pen()
        if brush is not None:
            pen.setBrush(brush)
        if width is not None:
            pen.setWidth(width)
        if style is not None:
            pen.setStyle(style)
        if cap_style is not None:
            pen.setCapStyle(cap_style)
        if join is not None:
            pen.setJoinStyle(join)
        painter.setPen(pen)

    def setup_brush(self, painter, color=None, texture=None, style=None):
        brush = painter.brush()
        if color is not None:
            brush.setColor(color)
        if texture is not None:
            brush.setTexture(texture)
        if style is not None:
            brush.setStyle(style)
        painter.setBrush(brush)

    def draw_background(self, painter, event):
        self.setup_pen(painter, brush=self.background_color, width=7, cap_style=QtCore.Qt.RoundCap)
        self.setup_brush(painter, style=QtCore.Qt.NoBrush)
        rad = self.count_workspace(event)
        center = event.rect().center()
        self.draw_circle(painter, QtCore.QPoint(0, 0), rad * 2)
        color = QtGui.QColor(self.background_color)
        color.setAlpha(color.alpha() * 0.60)
        self.setup_pen(painter, brush=color, width=5)
        self.draw_circle(painter, QtCore.QPoint(0, 0), rad * 0.1)
        painter.save()
        for i in range(60):
            if i % 5 == 0:
                self.setup_pen(painter, width=5)
                painter.drawLine(rad, 0, rad * 0.80, 0)
            else:
                self.setup_pen(painter, width=3)
                painter.drawLine(rad, 0, rad * 0.90, 0)
            painter.rotate(6.0)
        painter.restore()

    def draw_cross(self, painter, center, diam):
        rad = diam / 2
        painter.save()
        painter.translate(center)
        painter.rotate(90)
        painter.drawLine(-rad, 0, rad, 0)
        painter.drawLine(0, -rad, 0, rad)
        painter.restore()

    def draw_circle(self, painter, center, diam):
        rect = QtCore.QRect(center, QtCore.QSize(diam, diam))
        rect.moveCenter(center)
        painter.drawEllipse(rect)


class PositionWidget(QtWidgets.QWidget):
    def __init__(self):
        super(PositionWidget, self).__init__()

        self.setup_ui()
        self.setup_ui_design()

    def setup_frame(self, layout, stretch=0):
        frame = QtWidgets.QFrame()
        frame.setFrameStyle(QtWidgets.QFrame.Panel|QtWidgets.QFrame.Raised)
        layout.addWidget(frame, stretch=stretch)
        return frame

    def setup_h_line(self, layout):
        line = QtWidgets.QFrame()
        line.setFrameStyle(QtWidgets.QFrame.HLine|QtWidgets.QFrame.Raised)
        line.setLineWidth(2)
        layout.addWidget(line)
        return line

    def setup_label(self, layout, text, alignment=QtCore.Qt.AlignLeft):
        label = QtWidgets.QLabel(text)
        label.setAlignment(alignment)
        layout.addWidget(label)
        return label

    def setup_data_field(self, source_layout, fields, name=None):
        frame = self.setup_frame(source_layout)
        frame_layout = QtWidgets.QVBoxLayout(frame)
        lbl_list = []
        for i in range(len(fields)):
            layout = QtWidgets.QHBoxLayout()
            frame_layout.addLayout(layout)
            self.setup_label(layout, fields[i])
            lbl_list.append(self.setup_label(layout, 'None'))
        return lbl_list

    def setup_3x3_matrix_data_field(self, source_layout, name):
        frame = self.setup_frame(source_layout)
        frame_layout = QtWidgets.QVBoxLayout(frame)
        self.setup_label(frame_layout, name, QtCore.Qt.AlignHCenter)
        lbl_list = []
        for i in range(3):
            layout = QtWidgets.QHBoxLayout()
            frame_layout.addLayout(layout)
            sub_lbl_list = []
            for j in range(3):
                sub_lbl_list.append(self.setup_label(layout,'None'))
            lbl_list.append(sub_lbl_list)
        return lbl_list

    def setup_pos_field(self, source_layout, name):
        frame = self.setup_frame(source_layout)
        frame_layout = QtWidgets.QVBoxLayout(frame)
        self.setup_label(frame_layout, name, QtCore.Qt.AlignHCenter)
        self.setup_h_line(frame_layout)
        text_list = ['Azimuth:', 'Elevation:', 'Time:']
        lbl_list = []
        for i in range(len(text_list)):
            layout = QtWidgets.QHBoxLayout()
            if i == 2:
                self.setup_h_line(frame_layout)
            frame_layout.addLayout(layout)
            self.setup_label(layout, text_list[i])
            lbl_list.append(self.setup_label(layout, 'None'))
        return lbl_list


    def setup_ui(self):
        self.layout = QtWidgets.QHBoxLayout(self)

        visualization_layout = QtWidgets.QVBoxLayout()
        self.layout.addLayout(visualization_layout)
        self.layout.setStretch(0, 3)

        self.pos_panel = PositionPanel()
        visualization_layout.addWidget(self.pos_panel)
        visualization_layout.setStretch(0, 3)

        frame = self.setup_frame(visualization_layout)
        frame_layout = QtWidgets.QVBoxLayout(frame)
        self.mode_lbl = self.setup_label(frame_layout, 'None', QtCore.Qt.AlignHCenter)
        font = QtGui.QFont()
        font.setPointSize(15)
        self.mode_lbl.setFont(font)

        pos_layout = QtWidgets.QHBoxLayout()
        visualization_layout.addLayout(pos_layout)

        self.target_param_lbl = self.setup_pos_field(pos_layout, 'Target')
        self.antenna_param_lbl = self.setup_pos_field(pos_layout, 'Antenna')

        data_layout = QtWidgets.QHBoxLayout()
        data_layout_1_col = QtWidgets.QVBoxLayout()
        data_layout_2_col = QtWidgets.QVBoxLayout()
        self.layout.addLayout(data_layout)
        data_layout.addLayout(data_layout_1_col)
        data_layout.addLayout(data_layout_2_col)

        self.dec_to_top_lbl = self.setup_3x3_matrix_data_field(data_layout_1_col, 'Decart WGS84 to topocentric\ncoordinate system transition matrix')
        self.top_to_ascs_lbl = self.setup_3x3_matrix_data_field(data_layout_1_col, 'Topocentric to antenna system\ncoordinate system transition matrix')

        self.lat_lon_alt_lbl = self.setup_data_field(data_layout_1_col, ['Latitude:', 'Longitude:', 'Altitude:'])
        self.ecef_lbl = self.setup_data_field(data_layout_1_col, ['EcefX:', 'EcefY:', 'EcefZ:'])
        self.aiming_period_lbl = self.setup_data_field(data_layout_1_col, ['Aiming period:'])
        self.enable_lbl = self.setup_data_field(data_layout_1_col, ['Vertical motor:', 'Horizontal motor:'])
        self.motors_auto_disable_lbl = self.setup_data_field(data_layout_1_col, ['Motors auto disable:', 'Motors timeout:'])
        self.rssi_lbl = self.setup_data_field(data_layout_2_col, ['RSSI INSTANT:', 'RSSI pkt:', 'SNR pkt:', 'RSSI signal:'])
        self.radio_stats_lbl = self.setup_data_field(data_layout_2_col, ["pkt received:", 
                                                                         "crc errors:", 
                                                                         "hdr errors:", 
                                                                         "error rc64k calib:", 
                                                                         "error rc13m calib:", 
                                                                         "error pll calib:",
                                                                         "error adc calib:",
                                                                         "error img calib:",
                                                                         "error xosc calib:",
                                                                         "error pll lock:",
                                                                         "error pa ramp:",
                                                                         "srv rx done:",
                                                                         "srv rx frames:",
                                                                         "srv tx frames:",
                                                                         "current pa power:",
                                                                         "requested pa power:"])
        self.gps_filter_lbl = self.setup_data_field(data_layout_2_col, ['GPS filter:'])

        self.setup_frame(data_layout_1_col, 3)#.addStretch()
        self.setup_frame(data_layout_2_col, 3)#.addStretch()

        self.state_request_btn = QtWidgets.QPushButton('State\nrequest')
        visualization_layout.addWidget(self.state_request_btn)

    def setup_ui_design(self):
        pass

    def change_data_field(self, data_field, data, format_str):
        for i in range(len(data_field)):
            if data[i] is not None:
                data_field[i].setText(format_str.format(data[i]))

    def change_rssi(self, data):
        self.change_data_field(self.rssi_lbl, data, '{:.0f}')

    def change_radio_stats(self, data):
        self.change_data_field(self.radio_stats_lbl, data, '{}')

    def change_antenna_pos(self, data):
        for i in range(3):
            self.antenna_param_lbl[i].setText('{:.2f}'.format(data[i]))
        self.pos_panel.set_antenna_pos(data[0], -data[1])

    def change_target_pos(self, data):
        for i in range(3):
            self.target_param_lbl[i].setText('{:.2f}'.format(data[i]))
        self.pos_panel.set_target_pos(data[0], -data[1])

    def change_gps_filter(self, data):
        self.change_data_field(self.gps_filter_lbl, [data], '{}')

    def change_lat_lon_alt(self, data):
        self.change_data_field(self.lat_lon_alt_lbl, data, '{:.3f}')

    def change_ecef(self, data):
        self.change_data_field(self.ecef_lbl, data, '{:.3f}')

    def change_top_to_ascs_matrix(self, data):
        for i in range(3):
            for j in range (3):
                self.top_to_ascs_lbl[i][j].setText('{:.3f}'.format(data[i * 3 + j]))

    def change_dec_to_top_matrix(self, data):
        for i in range(3):
            for j in range (3):
                self.dec_to_top_lbl[i][j].setText('{:.3f}'.format(data[i * 3 + j]))

    def change_control_mode(self, mode):
        font = self.mode_lbl.font()
        if mode:
            font.setWeight(QtGui.QFont.Black)
            self.mode_lbl.setFont(font)
            self.mode_lbl.setText('Automatic control')
        else:
            font.setWeight(QtGui.QFont.Medium)
            self.mode_lbl.setFont(font)
            self.mode_lbl.setText('Manual control')

    def change_aiming_period(self, period):
        self.change_data_field(self.aiming_period_lbl, [period], '{:.3f}')

    def change_motors_enable(self, data):
        font = QtGui.QFont()
        for i in range(2):
            if data[i]:
                font.setWeight(QtGui.QFont.Black)
                self.enable_lbl[i].setFont(font)
                self.enable_lbl[i].setText('ON')
            else:
                font.setWeight(QtGui.QFont.Medium)
                self.enable_lbl[i].setFont(font)
                self.enable_lbl[i].setText('OFF')

    def change_motors_auto_disable_mode(self, mode):
        font = QtGui.QFont()
        if mode:
            font.setWeight(QtGui.QFont.Black)
            self.motors_auto_disable_lbl[0].setText('ON')
        else:
            font.setWeight(QtGui.QFont.Medium)
            self.motors_auto_disable_lbl[0].setText('OFF')

        self.motors_auto_disable_lbl[0].setFont(font)

    def change_motors_timeout(self, timeout):
        self.motors_auto_disable_lbl[1].setText('{:.3f}'.format(timeout))