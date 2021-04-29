#!/usr/bin/env python3

import struct
from contextlib import contextmanager
from i2cdev import I2C as I2C
from enum import IntEnum


I2C_BUS_NO = 1
SLAVE_ADDR = 0xAA


class Command(IntEnum):
	NONE = 0x00
	GET_SIZE = 0x01
	GET_PACKET = 0x02
	SET_PACKET = 0x01


@contextmanager
def i2c_open(bus_no):
	bus = I2C(bus_no)
	yield bus
	bus.close()


def send_command(bus: I2C, addr: int, cmd: Command):
	data = struct.pack("<B", cmd.value)
	bus.write(data, addr=addr)


def send_message(bus: I2C, addr: int, data: bytes):
	size_data = struct.pack("<H". len(data))
	data = size_data + data
	bus.write(data, addr=addr)


def get_message(bus: I2C, addr: int):
	send_command(bus, addr, Command.GET_SIZE)

	size_struct = struct.Struct("<H")
	size_data = bus.read(bus, size_struct.size(), addr=addr)
	size = size_struct.unpack(size_data)

	data = bus.read(bus, size)
	return data


def main():
	with i2c_open(I2C_BUS_NO) as bus:
		while True:
			msg_bytes = get_message(bus, SLAVE_ADDR)
			print("got message: %s", msg_bytes)


if __name__ == "__main__":
	exit(main())