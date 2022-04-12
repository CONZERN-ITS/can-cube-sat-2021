import contextlib
import os
import math
import numpy as np
import scipy as sp
from scipy import signal

from itertools import tee

from common import TIME_OFFSET, \
	TIME_CUTOFF_LEFT, \
	TIME_CUTOFF_RIGHT, \
	RESTART_TIME_DELTA

from common import read_file, write_file


def pairwise(iterable):
	"s -> (s0,s1), (s1,s2), (s2, s3), ..."
	a, b = tee(iterable)
	next(b, None)
	return zip(a, b)


def median_smooth_field(lines, field_name, kernel_size, medsmooth_field_name=None):
	if not medsmooth_field_name:
		medsmooth_field_name = "msmooth_" + field_name

		raw_field_values = list([float(line[field_name]) for line in lines])
		smooth_field_values = signal.medfilt(raw_field_values, kernel_size=kernel_size)

	for line, smooth_value in zip(lines, smooth_field_values):
		line[medsmooth_field_name] = smooth_value


def smooth_field(lines, field_name, window_size, smooth_field_name=None):
	if not smooth_field_name:
		smooth_field_name = "smooth_" + field_name

	raw_field_values = list([float(line[field_name]) for line in lines])
	kernel = np.ones(window_size)/window_size
	smooth_field_values = np.convolve(raw_field_values, kernel, mode='same')

	for line, smooth_value in zip(lines, smooth_field_values):
		line[smooth_field_name] = smooth_value


def derivative_field(lines, field_name, dt_field_name=None):
	if not dt_field_name:
		dt_field_name = field_name[field_name + "_derivative"]

	lines[0][dt_field_name] = None # ну нету его

	for prev, cur in pairwise(lines):
		cur_time, cur_value = cur["time_fs_mins"], float(cur[field_name])
		prev_time, prev_value = prev["time_fs_mins"], float(prev[field_name])

		delta_time_seconds = (cur_time - prev_time) * 60
		value_dt = (cur_value - prev_value) / delta_time_seconds

		cur[dt_field_name] = value_dt


def interpolate_field(lines, time, field_name):
	for prev, cur in pairwise(lines):
		if cur["time_fs_mins"] > time:
			break

	prev_time, cur_time = prev["time_fs_mins"], cur["time_fs_mins"]
	prev_value, cur_value = float(prev[field_name]), float(cur[field_name])

	k = (cur_value - prev_value) / (cur_time - prev_time)
	rel_time = time - prev_time
	rv = k * rel_time + prev_value
	#print(f"cv={cur_value}, pv={prev_value}, ct={cur_time}, pt={prev_time}, t={time}, v={rv}")
	return rv


DOSIM_FILE = "PLD_DOSIM_DATA-13-0.csv"
data = read_file(DOSIM_FILE)
data_ascent = []

for line in data:
	tics, delta_time_ms = int(line["count_tick"]), int(line["delta_time"])
	line["tics_per_sec"] = tics / (delta_time_ms/1000)

# Прокатываем радиацию скользящим окном
smooth_field(data, "tics_per_sec", 75)
write_file("radiation+" + DOSIM_FILE, data)

for line in data:
	if line["time_fs_mins"] < 59:
		data_ascent.append(line)

write_file("radiation+ascent+" + DOSIM_FILE, data_ascent)

dosim_data = data
print("dosim done")


BAROMETER_FILE = "PLD_MS5611_DATA-13-1.csv"
data = read_file(BAROMETER_FILE)

# Сглаживаем высоту
# Рубим на две части -  подъем и спуск, так как между ними есть дырка :c
data1 = []
data2 = []
data_cursor = data1
#data_cursor.append(data[0])
for prev, cur in pairwise(data):
	cur_time = cur["time_fs_mins"]
	prev_time = prev["time_fs_mins"]

	if cur_time - prev_time >= RESTART_TIME_DELTA / 60:
		data_cursor = data2

	data_cursor.append(cur)


window_size = 50
smooth_field(data1, 'altitude', window_size)
smooth_field(data2, 'altitude', window_size)
derivative_field(data1, 'smooth_altitude', 'h_vel')
derivative_field(data2, 'smooth_altitude', 'h_vel')
 # Отрезаем стремные точки
data1 = data1[window_size:-window_size]
data2 = data2[window_size:-window_size]
median_smooth_field(data1, 'h_vel', kernel_size=5)
median_smooth_field(data2, 'h_vel', kernel_size=5)

data = data1 + data2
write_file("radiation+" + BAROMETER_FILE, data)

# Приклеим к этим данным соответвующие данные от дозиметра
# Чтобы можно было построить график по высоте
data_ascent = list(data)
for line in data_ascent:
	line_time = line["time_fs_mins"]
	line["tics_per_sec"] = interpolate_field(dosim_data, line_time, "tics_per_sec")
	line["smooth_tics_per_sec"] = interpolate_field(dosim_data, line_time, "smooth_tics_per_sec")
	if line_time > 59:
		break

write_file("radiation+ascent+" + BAROMETER_FILE, data_ascent)
print("barometer done")


ACCELEROMETER_FILE = 'SINS_ISC-11-0.csv'
data = read_file(ACCELEROMETER_FILE)
for line in data:
	x, y, z = line['accel[0]'], line['accel[1]'], line['accel[2]']
	x, y, z = float(x), float(y), float(z)
	acc = np.linalg.norm([x,y,z])
	line['accel'] = acc
	line['accel_g'] = acc/9.8

write_file('radiation+' + ACCELEROMETER_FILE, data)

# Теперь проходим по барометру