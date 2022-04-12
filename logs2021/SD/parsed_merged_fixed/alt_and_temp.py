import contextlib
import os
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



common_file_list = [
	("THERMAL_STATE-10-0.csv",),
	("THERMAL_STATE-10-1.csv",),
	("THERMAL_STATE-10-2.csv",),
	("THERMAL_STATE-10-3.csv",),
	("THERMAL_STATE-10-4.csv",),
	("THERMAL_STATE-10-5.csv",),
	
	("THERMAL_STATE-12-0.csv",),
	("THERMAL_STATE-12-1.csv",),
	("THERMAL_STATE-12-2.csv",),
	("THERMAL_STATE-12-3.csv",),
	
	("OWN_TEMP-11-0.csv",),
	("OWN_TEMP-12-0.csv",),
	("OWN_TEMP-13-0.csv",),

	("PLD_BME280_DATA-13-1.csv",),
	("PLD_BME280_DATA-13-2.csv",),

	#("PLD_MS5611_DATA-13-1.csv",), # этот файл специально со сглаживанием высоты и скоростью
	("PLD_MS5611_DATA-13-2.csv",),
]


for file in common_file_list:
	input_path = file[0]
	base_path, fname = os.path.split(input_path)
	output_path = os.path.join(base_path, "+" + fname)

	data = read_file(input_path)
	print(f"{input_path} done")
	write_file(output_path, data)


print("temperature files done")



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


write_file("+" + BAROMETER_FILE, data)
print("barometer done")

# Теперь проходим по барометру