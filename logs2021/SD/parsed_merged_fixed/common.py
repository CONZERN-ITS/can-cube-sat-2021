import csv

TIME_OFFSET = 1633357251 - 30
TIME_CUTOFF_LEFT = 1633357041
TIME_CUTOFF_RIGHT = 1633365222
RESTART_TIME_DELTA = (1633362572 - 1633361256) - 100


class Break:
	pass


def make_time_from_start_mins(csv_record):
	return (int(csv_record["time_s"]) - TIME_OFFSET + int(csv_record["time_us"])/1000_000)/60


def read_file(input_path):
	data = []
	with open(input_path, "r") as stream:
		reader = csv.DictReader(stream)
		for line in reader:
			line_time = make_time_from_start_mins(line)

			if line_time >= (TIME_CUTOFF_RIGHT - TIME_OFFSET) / 60:
				continue

			if line_time <= (TIME_CUTOFF_LEFT - TIME_OFFSET) / 60:
				continue

			line["time_fs_mins"] = line_time
			data.append(line)

	return data


def write_file(output_path, data):
	if not data:
		return

	with open(output_path, "w") as stream:
		writer = csv.DictWriter(stream, fieldnames=data[0].keys())
		writer.writeheader()

		line = data[0]
		prev_line_time = make_time_from_start_mins(line)
		for line in data:
			if isinstance(line, Break):
				stream.write("\n")
				continue

			line_time = make_time_from_start_mins(line)
			if line_time - prev_line_time >= RESTART_TIME_DELTA / 60: # /60 в минуты
				# Это дырка по времени в течение которой аппарат не работал
				# Чтобы гнуплот рисал её как отдельные графики
				# Здесь должна быть пустая строка
				stream.write("\n")
				pass

			prev_line_time = line_time
			writer.writerow(line)
