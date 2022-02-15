import numpy as np
import csv
import math

from common import TIME_OFFSET, \
	TIME_CUTOFF_LEFT, \
	TIME_CUTOFF_RIGHT, \
	RESTART_TIME_DELTA

from common import read_file, write_file
from common import Break

def sph_to_cartesian(ro, theta, phi):
	x = ro * math.cos(phi) * math.sin(theta)
	y = ro * math.sin(phi) * math.sin(theta)
	z = ro * math.cos(theta)

	return (x, y, z,)


SINS_FILE = "SINS_ISC-11-0.csv"
data = read_file(SINS_FILE)

vertical = np.array([0, 0, 1])

new_data = []
for line in data:
	error = float(line["light_dir_error"])
	if error < 0:
		if new_data and not isinstance(new_data[-1], Break):
			new_data.append(Break())
		continue # Эта строчка никуда не годится

	acc = np.array([float(line["accel[%d]" % x]) for x in (0, 1, 2,)])
	acc = acc / np.linalg.norm(acc)

	light = np.array([float(line["light_dir_measured[%d]" % x]) for x in (0, 1, 2,)])
	ro, theta, phi = light[0], light[1], light[2]
	line["ro"], line["theta"], line["phi"] = ro, theta * 180/math.pi, phi * 180/math.pi

	light_cartesian = sph_to_cartesian(light[0], light[1], light[2])
	light_cartesian = light_cartesian / np.linalg.norm(light_cartesian)
	dot = np.dot(acc, light_cartesian)
	line["angle_acc_sun"] = math.acos(dot) * 180/math.pi
	
	dot = np.dot(acc, vertical)
	line["angle_acc_verical"] = math.acos(dot) * 180/math.pi
	
	new_data.append(line)

while isinstance(data[0], Break):
	del data[0]

write_file("+" + SINS_FILE, new_data)
