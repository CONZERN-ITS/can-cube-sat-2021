import typing
import contextlib
import struct
import time

MAGIC = b"ZMQLOG"

""" В этом файле лежат классы для записи в логфайл и чтения оттуда записей с ZMQ брокера
	Записи представлены в виде List[bytes], где List - это мультипарт zmq сообщение,
	а каждая отдельная запись bytes это собственно части мультипарт сообщения

	В лог файле это ложится в следующем формате
	uint64_t time- время получения записи (posix timestamp)
	uint32_t parts_count - количество записей в конкретном multipart сообщении
	uint32_t parts_sizes[records_count] - размеры частей сообщения
	uint8_t[...] records[records_count] - набор массивов байт, которые и являются частями собщения

	Записи идут в фале подряд без разделителей и прочего
"""


def _probe_file_version(file_stream):
	""" Проверка версии файла """
	file_header_struct = struct.Struct("<6sL")
	data = file_stream.read(file_header_struct.size)
	magic, version_no = file_header_struct.unpack(data)
	if magic != MAGIC:
		return 0
	else:
		return version_no


def _write_file_header(file_stream, version_no: int):
	file_header_struct = struct.Struct("<6sL")
	data = file_header_struct.pack(MAGIC, version_no)
	file_stream.write(data)


def _prim_header_size_v0():
	""" размер первичного заголовка """
	return 8 + 4  # uint64_t time, uin32_t records_count;


def _prim_header_size_v1():
	""" размер первичного заголовка """
	return 8 + 4 + 4  # uint64_t seconds, uint32_t microseconds, uin32_t records_count;


def _sec_header_size(parts_count):
	""" размер вторичного заголовка исходя из количества частей сообщения """
	return parts_count * 4


def _unpack_message_prim_header_v0(header_bytes: bytes):
	""" Распаковка заголовка сообщения. Заголовок это uint64_t время
		и uint32_t количество частей в нем """

	if len(header_bytes) != _prim_header_size_v0():
		# Это не правильный заголовок
		raise ValueError(
			"Invalid ITS ZMQ LOG header size: %s. %s were expected"
			% (len(bytes), 8 + 4)
		)

	msg_time, parts_count = struct.unpack("<QI", header_bytes)
	return msg_time, parts_count


def _unpack_message_prim_header_v1(header_bytes: bytes):
	""" Распаковка заголовка сообщения. Заголовок это uint64_t время
		и uint32_t количество частей в нем """

	if len(header_bytes) != _prim_header_size_v1():
		# Это не правильный заголовок
		raise ValueError(
			"Invalid ITS ZMQ LOG header size: %s. %s were expected"
			% (len(bytes), 8 + 4)
		)

	time_s, time_us, parts_count = struct.unpack("<QLI", header_bytes)
	msg_time = float(time_s) + time_us / 10**6
	return msg_time, parts_count


def _unpack_message_sec_header(parts_count, parts_sizes_bytes: bytes):
	""" Распаковка сообщения из буфера """

	# Сперва вычитываем размеры частей
	if len(parts_sizes_bytes) != _sec_header_size(parts_count):
		# ну тут даже вторичный заголовок не влезает
		raise ValueError(
			"Invalid ITS ZMQ LOG secondary header size: %s. at least %s where expected"
			% (len(parts_sizes_bytes), _sec_header_size(parts_count))
		)

	parts_sizes = struct.unpack("<%dI" % parts_count, parts_sizes_bytes)
	return list(parts_sizes)


def _unpack_message_parts(parts_sizes, parts_bytes):
	# Пилим общий массив на байты
	
	if len(parts_bytes) != sum(parts_sizes):
		raise ValueError(
			"Invalid ITS ZMQ LOG parts data: %s. %s were expected"
			% (len(parts_bytes), sum(parts_sizes))
		)

	retval = []
	tail = parts_bytes
	for part_size in parts_sizes:
		head = tail[:part_size]
		tail = tail[part_size:]
		retval.append(head)

	assert not tail 
	return retval


class LogfileWriter(contextlib.AbstractContextManager):

	def __init__(self, filepath=None, stream=None, version=1):
		super(LogfileWriter, self).__init__()
		self.stream = None
		self.owns_stream = False
		self.version = version
		if self.version == 0:
			self._packer = self._pack_v0
		elif self.version == 1:
			self._packer = self._pack_v1
		else:
			raise ValueError("Unsupported version %d" % version)

		if filepath:
			self.open(filepath)
		elif stream:
			self.open_stream(stream)

	def _prepare_stream(self):
		if self.version > 0:
			_write_file_header(self.stream, self.version)

	def _pack_v0(self, msg_time, parts):
		""" Запаковка сообщения в буфер для записи в файл версии 0
			@param msg_time время получения сообщения
			@param parts части сообщения
		"""
		time_s = int(msg_time)

		# Пакуем время и количество записей
		data = struct.pack("<QI", time_s, len(parts))
		# Дальше пакуем список длин
		data += struct.pack("<%dI" % len(parts), *[len(x) for x in parts])
		# а дальше просто приклеиваем записи
		for part in parts:
			data += part

		return data

	def _pack_v1(self, msg_time, parts):
		""" Запаковка сообщения в буфер для записи в файл версии 1
			@param msg_time время получения сообщения
			@param parts части сообщения
		"""
		if isinstance(msg_time, int):
			time_s = msg_time
			time_us = 0
		elif isinstance(msg_time, float):
			time_s = int(msg_time)
			time_us = (msg_time - time_s) * 10**6
			time_us = int(time_us)

		# Пакуем время и количество записей
		data = struct.pack("<QLI", time_s, time_us, len(parts))
		# Дальше пакуем список длин
		data += struct.pack("<%dI" % len(parts), *[len(x) for x in parts])
		# а дальше просто приклеиваем записи
		for part in parts:
			data += part

		return data

	def open(self, filepath):
		self.close()
		self.stream = open(filepath, mode="wb")
		self.owns_stream = False
		self._prepare_stream()

	def open_stream(self, stream):
		self.close()
		self.stream = stream
		self.owns_stream = True
		self._prepare_stream()

	def close(self):
		if self.stream and self.owns_stream:
			self.stream.close()

		self.stream = None
		self.owns_stream = False

	def write(self, zmq_multipart_message: typing.List[bytes]):

		msg_bytes = self._packer(time.time(), zmq_multipart_message)
		self.stream.write(msg_bytes)

	def write_all(self, msg_list: typing.List[typing.List[bytes]]):
		for msg in msg_list:
			self.write(msg)

	def flush(self):
		self.stream.flush()

	def __exit__(self, exc_type, exc_value, traceback):
		self.close()


class LogfileReader(contextlib.AbstractContextManager):

	def __init__(self, filepath=None, stream=None):
		super(LogfileReader, self).__init__()
		self.stream = None
		self.owns_stream = False
		self.version = None
		self._prim_header_unpacker = None
		self._prim_header_size = None

		if filepath:
			self.open(filepath)
		elif stream:
			self.open_stream(stream)

	def _fetch_version(self):
		version = _probe_file_version(self.stream)
		if version == 0:
			self.stream.seek(0)
			self._prim_header_unpacker = _unpack_message_prim_header_v0
			self._prim_header_size = _prim_header_size_v0()
		elif version == 1:
			self._prim_header_unpacker = _unpack_message_prim_header_v1
			self._prim_header_size = _prim_header_size_v1()
		else:
			raise ValueError("Unsupported file version %d" % version)

		self.version = version

	def open(self, filepath):
		self.close()
		self.stream = open(filepath, mode="rb")
		self.owns_stream = True
		self._fetch_version()

	def open_stream(self, stream):
		self.close()
		self.stream = stream
		self.owns_stream = False
		self._fetch_version()

	def close(self):
		if self.stream and self.owns_stream:
			self.stream.close()
			
		self.stream = None

	def __exit__(self, exc_type, exc_value, traceback):
		self.close()

	def read(self):
		# Читаем первичный заголовок
		prim_header_size = self._prim_header_size
		prim_header_bytes = self.stream.read(prim_header_size)
		# Возможно файл кончился?
		if not prim_header_bytes:
			return None  # Так же покажем eof

		if len(prim_header_bytes) != prim_header_size:
			raise ValueError("Unexpected EOF in primary header")

		msg_time, parts_count = self._prim_header_unpacker(prim_header_bytes)
		# Читаем вторичный заголовок
		sec_header_size = _sec_header_size(parts_count)
		parts_sizes_bytes = self.stream.read(sec_header_size)
		if len(parts_sizes_bytes) != sec_header_size:
			raise ValueError("Unexpected EOF in secondary header")

		parts_sizes = _unpack_message_sec_header(parts_count, parts_sizes_bytes)

		# Читаем части
		to_read = sum(parts_sizes)
		parts_bytes = self.stream.read(to_read)
		if len(parts_bytes) != to_read:
			raise ValueError("Unexpected EOF in msg body")

		parts = _unpack_message_parts(parts_sizes, parts_bytes)

		return msg_time, parts

	def read_all(self):
		retval = []
		while True:
			msg = self.read()
			if not msg:
				break

			retval.append(msg)

		return retval


def test():
	filepath = "/tmp/testfile.itslog"

	msgs = [
		[b"hurdurdur", b"ramramram", b"vetvetvet"],
		[b"", b"", b""],
		[b"123", b"", b""],
		[b"", b"321", b"cadebaba"],
	]

	with LogfileWriter(filepath, version=1) as writer:
		writer.write_all(msgs)

	with LogfileReader(filepath) as reader:
		rmsgs = reader.read_all()
		for time, rmsg in rmsgs:
			print(time, rmsg)


# if __name__ == "__main__":
# 	test()
