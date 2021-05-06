import os
import argparse
import numpy as np
from ambiance import Atmosphere

from templates import HEADER_FILE_TEMPLATE, SOURCE_FILE_TEMPLATE, RECORD_BYTE_SIZE,\
	PRESSURE_VALUE_FORMAT, ALTITUDE_VALUE_FORMMAT


def sizeof_fmt(num, suffix='B'):
	""" Форматирует размер в байтах в человекочитаемый формат
		https://stackoverflow.com/a/1094933/1869025 """
	for unit in ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi']:
		if abs(num) < 1024.0:
			return "%3.1f%s%s" % (num, unit, suffix)
		num /= 1024.0

	return "%.1f%s%s" % (num, 'Yi', suffix)


def prepare_data(alt_start, alt_stop, alt_step):
	""" Готовит данные таблицы """
	alt_range = np.arange(alt_start, alt_stop, alt_step)

	model = Atmosphere(alt_range)  # type: Atmosphere
	pressures = np.array(model.pressure)
	altitudes = np.array(model.h)

	# Клеим в общий список
	# Сразу считаем в интах
	combo = np.array(
		list(zip(pressures, altitudes)), dtype=[('pressure', float), ('altitude', float)]
	)

	# Сортируем по возрастанию давления
	combo.sort(axis=0, order='pressure')

	# Проверяем полученный список на монотонность
	diffs = np.diff(combo['pressure'], axis=0)
	diffs = diffs < 0  # Перегоняем цифры дельты соседних элементов в булево значение больше оно или меньше нуля
	monotonic = np.all(diffs)  # Если все True - значит массив монотонный
	if not monotonic:
		print("WARNING: Resulting table is not monotonic for pressure")

	return combo


def format_as_lines_of_n(array, n, value_format, indent='    '):
	""" Печатает массив блоками-строками по n элементов
		А еще красиво выравнивает их. Ну, за компанию """

	# Копируем массив, чтобы не трогать исходный
	the_slice = array[:]
	# Считаем сколько символов займет самый большой элемент
	max_len = max([len(value_format.format(x)) for x in the_slice])

	# Работаем, пока не истощим все элементы массива
	lines = []
	while len(the_slice) > 0:
		# Берем первые n элементов
		portion = the_slice[:n]
		# Формируем из них строку
		line = ", ".join([value_format.format(x).rjust(max_len, ' ') for x in portion])
		lines.append(line)
		# Отрезаем обработанное
		the_slice = the_slice[n:]

	# Склеиваем строки, добавляя к ним общий отступ
	return ",\n".join(indent + line for line in lines)


def run(alt_start, alt_stop, alt_step, output_dir):
	print(f"preparing data for alts [{alt_start}:{alt_stop}:{alt_step}]")

	data = prepare_data(alt_start, alt_stop, alt_step)
	print(f"data ready. records_count: {len(data)}. memory footprint {sizeof_fmt(RECORD_BYTE_SIZE*len(data))}")

	header_contents = HEADER_FILE_TEMPLATE.format(records_count=len(data))
	source_contents = SOURCE_FILE_TEMPLATE.format(
		altitude_records=format_as_lines_of_n(data['altitude'], 20, ALTITUDE_VALUE_FORMMAT),
		pressure_records=format_as_lines_of_n(data['pressure'], 20, PRESSURE_VALUE_FORMAT),
	)

	header_fpath = os.path.join(output_dir, "icao_table_data.h")
	source_fpath = os.path.join(output_dir, "icao_table_data.c")
	print(f"using files {header_fpath} and {source_fpath}")

	with open(header_fpath, mode='w', encoding='utf-8') as stream:
		stream.write(header_contents)

	with open(source_fpath, mode='w', encoding='utf-8') as stream:
		stream.write(source_contents)

	print("done")


def main(argv):
	parser = argparse.ArgumentParser("ICAO table generator")
	parser.add_argument("--alt-start", nargs='?', type=float, required=True, dest='alt_start')
	parser.add_argument("--alt-stop", nargs='?', type=float, required=True, dest='alt_stop')
	parser.add_argument("--alt-step", nargs='?', type=float, required=True, dest='alt_step')
	parser.add_argument("-o", "--output-dir", nargs='?', type=str, default=os.getcwd(), dest='output_dir')

	args = parser.parse_args(argv)
	return run(args.alt_start, args.alt_stop, args.alt_step, args.output_dir)


if __name__ == "__main__":
	import sys
	exit(main(sys.argv[1:]))
