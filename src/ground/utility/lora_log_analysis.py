import typing
import struct
import dataclasses
import enum


def _twos_comp(val, bits=8):
    """compute the 2's complement of int value val"""
    if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
        val = val - (1 << bits)        # compute negative value
    return val                         # return positive value as is


def _crc16_ccitt(data: bytes) -> int:
    crc = 0x0000
    for byte in data:
        for i in range(0,8):
            if ((crc & 0x8000) >> 8) ^ (byte & 0x80):
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1

            byte = byte << 1

    return crc & 0xFFFF


class LoraBandwidth(enum.IntEnum):
    BAND_INVALID = -1
    BAND_125_kHz = 1
    BAND_250_kHz = 2
    BAND_500_kHz = 3


class LoraSpreadFactor(enum.IntEnum):
    SF_INVALID = -1
    SF_7 = 7
    SF_8 = 8
    SF_9 = 9
    SF_10 = 10
    SF_11 = 11


@dataclasses.dataclass
class LoraChannel:
    frequency: int
    bandwidth: int
    sf: int

    def bandwidth_value(self) -> LoraBandwidth:
        try:
            return LoraBandwidth(self.bandwidth)
        except:
            return LoraBandwidth.BAND_INVALID

    def sf_value(self) -> LoraSpreadFactor:
        try:
            return LoraSpreadFactor(self.sf)
        except:
            return LoraSpreadFactor.SF_INVALID

    @staticmethod
    def from_bytes(data: bytes) -> "LoraChannel":
        freq, band, sf = struct.unpack("<LBB", data)
        return LoraChannel(frequency=freq, bandwidth=band, sf=sf)

    @staticmethod
    def byte_size() -> int:
        return 4 + 1 + 1



@dataclasses.dataclass
class LoraRSSI:
    packet_rssi: int
    max_rssi: int
    current_rssi: int
    snr: int

    def packet_rssi_value(self) -> int:
        if self.snr_value() > 0:
            return self.packet_rssi - 139
        else:
            return self.packet_rssi // 4 - 139

    def max_rssi_value(self) -> int:
        return -139 + self.max_rssi

    def current_rssi_value(self) -> int:
        return -139 + self.current_rssi

    def snr_value(self) -> int:
        return _twos_comp(self.snr)//4

    @staticmethod
    def from_bytes(data: bytes) -> "LoraRSSI":
        p, m, c, s = struct.unpack("<4B", data)
        return LoraRSSI(packet_rssi=p, max_rssi=m, current_rssi=c, snr=s)

    @staticmethod
    def byte_size() -> int:
        return 1 + 1 + 1 + 1


@dataclasses.dataclass
class LoraTapHeader:
    version: int
    hdr_len: int
    channel: LoraChannel
    rssi: LoraRSSI
    syncword: int

    @staticmethod
    def from_bytes(data: bytes) -> "LoraTapHeader":
        part0 = data[:1 + 1 + 2]
        version, hdr_len = struct.unpack("<BxH", part0)

        part_channel = data[len(part0):][:LoraChannel.byte_size()]
        channel = LoraChannel.from_bytes(part_channel)

        part_rssi = data[len(part0) + len(part_channel):][:LoraRSSI.byte_size()]
        rssi = LoraRSSI.from_bytes(part_rssi)

        part1 = data[len(part0) + len(part_channel) + len(part_rssi):][:1]
        syncword = struct.unpack("<B", part1)

        return LoraTapHeader(
            version=version,
            hdr_len=hdr_len,
            channel=channel,
            rssi=rssi,
            syncword=syncword
        )

    @staticmethod
    def byte_size() -> int:
        return 1+1+2 + LoraChannel.byte_size() + LoraRSSI.byte_size() + 1


@dataclasses.dataclass
class LoraPhyHeader:
    length: int
    crc_msn: int
    has_mac_crc: bool
    cr: int
    crc_lsn: int

    @staticmethod
    def from_bytes(data: bytes)-> "LoraPhyHeader":
        length, b0, b1 = struct.unpack("<BBB", data)
        crc_msn = b0 & 0x0F
        has_mac_crc = True if b0 & 0x10 else False
        cr = (b0 & 0xE0) >> 5
        crc_lsn = b1 & 0x0F

        return LoraPhyHeader(
            length=length,
            crc_msn=crc_msn,
            has_mac_crc=has_mac_crc,
            cr=cr,
            crc_lsn=crc_lsn
        )

    @staticmethod
    def byte_size() -> int:
        return 1 + 1 + 1


def read_record(stream):
    rec_header = stream.read(4)
    if not rec_header:
        return None

    rec_len, = struct.unpack("<L", rec_header)
    data = stream.read(rec_len)
    return data



def slice_gr_lora_record(record: bytes) -> typing.Tuple[bytes, bytes, bytes, bytes]:
    """ Разибает запись от gr-lora на tap заголовок, phy заголовок,
        собственно пейлоад и crc """ 

    tap_header_len = LoraTapHeader.byte_size()
    phy_header_len = LoraPhyHeader.byte_size()
    crc_len = 2 # байта на контрольную сумму

    tap_bytes = record[:tap_header_len]
    phy_bytes = record[tap_header_len:][:phy_header_len]

    tap_header = LoraTapHeader.from_bytes(tap_bytes)
    phy_header = LoraPhyHeader.from_bytes(phy_bytes)

    payload = record[tap_header_len + phy_header_len:]
    if phy_header.has_mac_crc:
        crc, = struct.unpack("<H", payload[-crc_len:])
        payload = payload[:-crc_len]
    else:
        crc = None

    print((phy_bytes + payload).hex())
    return tap_header, phy_header, payload, crc


stream = open("lora_log.bin", mode="rb")

while True:
    record = read_record(stream)
    if not record:
        break

    print(f"record: {record.hex()}")
    tap, phy, pld, crc = slice_gr_lora_record(record)
    
    print(
        f"band: {tap.channel.bandwidth_value()}, sf: {tap.channel.sf_value()} "
    )
    print(
        f"p_rssi: {tap.rssi.packet_rssi_value()}, m_rssi: {tap.rssi.max_rssi_value()}, "
        f"c_rssi: {tap.rssi.current_rssi_value()}, snr: {tap.rssi.snr_value()}"
    )
    print(
        f"hdr_len: {phy.length}, crc_msn: {phy.crc_msn}, "
        f"has_mac_crc: {phy.has_mac_crc}, cr: {phy.cr}, crc_lsn: {phy.crc_lsn}"
    )
    print(f"pld={pld}")
    if crc is not None:
        actual_crc =  _crc16_ccitt(pld)
        print(f"crc; in_frame=0x{crc:04X}, calculated=0x{actual_crc:04X}")
