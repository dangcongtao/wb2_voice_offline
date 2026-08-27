#!/usr/bin/env python3
"""
test_update.py - CI13LC/CI13162 UART flasher reconstructed from captures.

Default:
  python test_update.py COM3

Dry run:
  python test_update.py --dry-run

Flow:
  115200: maskrom handshake/control
  460800: send CI13LC updater/agent to SRAM 0x1FF80000
  115200: updater init/get-info/change baud
  921600: send firmware blocks on A1/08 requests
"""

from __future__ import annotations

import argparse
import struct
import sys
import time
import zlib
from dataclasses import dataclass
from pathlib import Path


HEAD = b"\xA5\x0F"
TAIL = 0xFF

MSG_CMD = 0xA0
MSG_REQ = 0xA1
MSG_ACK = 0xA2
MSG_NOTIFY = 0xA3

CMD_HANDSHAKE = 0x83
CMD_GET_INFO = 0x04
CMD_CHECK_READY = 0x05
CMD_BLOCK_INFO = 0x06
CMD_ERASE = 0x07
CMD_WRITE = 0x08
CMD_BLOCK_DONE = 0x09
CMD_VERIFY = 0x0A
CMD_CHANGE_BAUD = 0x0B
CMD_TEST_BAUD = 0x0C
CMD_COMPLETE = 0x0E
CMD_PROGRESS = 0x11
CMD_SET_FW_FMT = 0x12
CMD_VERIFY_AGENT = 0x8A

CHUNK = 0x1000
AGENT_ADDR = 0x1FF80000
PARTITION_TABLE_ADDR = 0x2000
FW_BLOCK1_BASE = 0x00000000

# =====================================================================
# ==  CAU HINH NHANH - SUA PATH O DAY                                 ==
# ==  (duong dan tuong doi tinh tu thu muc chua file .py nay,         ==
# ==   hoac dien duong dan tuyet doi: r"D:\...\fw.bin")               ==
# =====================================================================
HERE = Path(__file__).resolve().parent
ROOT = HERE.parent

#  >>> FIRMWARE can nap (full flash) <<<
FW_PATH    = r"D:\Project\Offline_voice_control\vendor\firmware\Gavit_260827_V26.08.27.bin"

#  >>> AGENT / updater (thuong KHONG can doi) <<<
AGENT_PATH = r"ci13lc_agent_from_tool_padded_0x5000.bin"
# =====================================================================


def _abs(p):
    """Tuyet doi hoa: neu la path tuong doi thi tinh tu thu muc file .py."""
    p = Path(p)
    return p if p.is_absolute() else (HERE / p)


DEFAULT_FW = _abs(FW_PATH)
DEFAULT_AGENT = _abs(AGENT_PATH)
DEFAULT_LOG = HERE / "test_update_last.log"


def crc16_ccitt(data: bytes, crc: int = 0) -> int:
    """CRC16-CCITT, poly 0x1021, init 0x0000."""
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def ts() -> str:
    ms = int(time.time() * 1000) % 1000
    return time.strftime("%H:%M:%S") + ".%03d" % ms


def hx(data: bytes, limit: int = 32) -> str:
    if len(data) <= limit:
        return " ".join("%02X" % x for x in data)
    head = " ".join("%02X" % x for x in data[:limit])
    return "%s ... (%d bytes)" % (head, len(data))


def printable(data: bytes) -> str:
    text = "".join(chr(x) if 32 <= x < 127 else "." for x in data)
    return text.strip(".")


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def cstr(raw: bytes) -> str:
    return raw.split(b"\x00", 1)[0].decode("ascii", errors="replace")


class Logger:
    def __init__(self, path: Path | None):
        self.fp = open(path, "w", encoding="utf-8") if path else None

    def close(self) -> None:
        if self.fp:
            self.fp.close()
            self.fp = None

    def __call__(self, msg: str) -> None:
        line = "[%s] %s" % (ts(), msg)
        print(line, flush=True)
        if self.fp:
            self.fp.write(line + "\n")
            self.fp.flush()


@dataclass(frozen=True)
class Frame:
    msg_type: int
    cmd: int
    seq: int
    data: bytes
    raw: bytes


@dataclass(frozen=True)
class PartitionEntry:
    name: str
    version: int
    offset: int
    size: int
    crc: int
    status: int


@dataclass(frozen=True)
class FirmwareLayout:
    manufacturer_id: int
    product_id0: int
    product_id1: int
    hardware_name: str
    software_name: str
    chip_name: str
    firmware_format: int
    nv_offset: int
    nv_size: int
    table_checksum: int
    entries: tuple[PartitionEntry, ...]


@dataclass(frozen=True)
class FlashBlock:
    name: str
    base: int
    data: bytes


def pack_frame(msg_type: int, cmd: int, data: bytes = b"", seq: int = 0) -> bytes:
    body = bytes([msg_type, cmd, seq]) + data
    crc = crc16_ccitt(body)
    return HEAD + len(data).to_bytes(2, "little") + body + crc.to_bytes(2, "little") + bytes([TAIL])


def frame_crc(raw: bytes) -> int:
    return raw[-3] | (raw[-2] << 8)


def pop_frame(rx: bytearray, max_len: int = 8192) -> Frame | None:
    while len(rx) >= 10:
        idx = rx.find(HEAD)
        if idx < 0:
            keep = b"\xA5" if rx[-1:] == b"\xA5" else b""
            rx.clear()
            rx += keep
            return None
        if idx:
            del rx[:idx]
        if len(rx) < 4:
            return None

        data_len = rx[2] | (rx[3] << 8)
        if data_len > max_len:
            del rx[0]
            continue
        frame_len = 10 + data_len
        if len(rx) < frame_len:
            return None
        if rx[frame_len - 1] != TAIL:
            del rx[0]
            continue

        raw = bytes(rx[:frame_len])
        msg_type = raw[4]
        cmd = raw[5]
        seq = raw[6]
        data = raw[7 : 7 + data_len]
        got = frame_crc(raw)
        calc = crc16_ccitt(bytes([msg_type, cmd, seq]) + data)
        if got != calc:
            del rx[0]
            continue

        del rx[:frame_len]
        return Frame(msg_type, cmd, seq, data, raw)
    return None


def u32(value: int) -> bytes:
    return value.to_bytes(4, "little")


def u16(value: int) -> bytes:
    return value.to_bytes(2, "little")


def block_info_data(base: int, size: int, block_crc: int) -> bytes:
    return u32(base) + u32(size) + u16(block_crc)


def erase_data(base: int, size: int) -> bytes:
    return u32(base) + u32(size) + u32(CHUNK)


def parse_fw2_layout(fw: bytes) -> FirmwareLayout:
    struct_size = 278
    if len(fw) < PARTITION_TABLE_ADDR + struct_size:
        raise ValueError("firmware is too small to contain a FW2 partition table at 0x%X" % PARTITION_TABLE_ADDR)

    base = PARTITION_TABLE_ADDR
    off = base

    def read_u32(pos: int) -> int:
        return struct.unpack_from("<I", fw, pos)[0]

    def read_u16(pos: int) -> int:
        return struct.unpack_from("<H", fw, pos)[0]

    manufacturer_id = read_u32(off)
    off += 4
    product_id0 = read_u32(off)
    off += 4
    product_id1 = read_u32(off)
    off += 4
    hardware_name = cstr(fw[off : off + 64])
    off += 64
    off += 4  # hardware version
    software_name = cstr(fw[off : off + 64])
    off += 64
    off += 4  # software version
    off += 4  # bootloader version
    chip_name = cstr(fw[off : off + 9])
    off += 9
    firmware_format = fw[off]
    off += 1
    off += 4  # reserved

    entries: list[PartitionEntry] = []
    for name in ("user_code1", "user_code2", "asr", "dnn", "voice", "user_file"):
        entries.append(
            PartitionEntry(
                name=name,
                version=read_u32(off),
                offset=read_u32(off + 4),
                size=read_u32(off + 8),
                crc=read_u32(off + 12),
                status=fw[off + 16],
            )
        )
        off += 17

    nv_offset = read_u32(off)
    nv_size = read_u32(off + 4)
    off += 8
    table_checksum = read_u16(off)

    layout = FirmwareLayout(
        manufacturer_id=manufacturer_id,
        product_id0=product_id0,
        product_id1=product_id1,
        hardware_name=hardware_name,
        software_name=software_name,
        chip_name=chip_name,
        firmware_format=firmware_format,
        nv_offset=nv_offset,
        nv_size=nv_size,
        table_checksum=table_checksum,
        entries=tuple(entries),
    )

    if layout.firmware_format != 2:
        raise ValueError("only FW_V2 / Format 2 is supported; firmware format is %d" % layout.firmware_format)
    if not layout.chip_name.startswith("CI13LC"):
        raise ValueError("unexpected chip name in firmware partition table: %r" % layout.chip_name)
    if layout.nv_offset == 0xFFFFFFFF or layout.nv_size == 0xFFFFFFFF:
        raise ValueError("FW2 partition table does not contain a valid NV region")
    if layout.nv_offset % CHUNK or layout.nv_size % CHUNK:
        raise ValueError("NV region is not 4K aligned: offset=0x%X size=0x%X" % (layout.nv_offset, layout.nv_size))

    return layout


def build_firmware_blocks(fw: bytes, erase_nv: bool) -> tuple[FirmwareLayout, list[FlashBlock]]:
    layout = parse_fw2_layout(fw)
    block1_size = align_up(len(fw), CHUNK)
    if block1_size > layout.nv_offset:
        raise ValueError(
            "firmware image overlaps NV region after 4K padding: block1_end=0x%X nv_start=0x%X"
            % (block1_size, layout.nv_offset)
        )

    block1 = fw + b"\xFF" * (block1_size - len(fw))
    blocks = [FlashBlock("firmware", FW_BLOCK1_BASE, block1)]
    if erase_nv:
        blocks.append(FlashBlock("nv", layout.nv_offset, b"\xFF" * layout.nv_size))
    return layout, blocks


class UartLink:
    def __init__(self, ser, log: Logger, verbose: bool = False):
        self.ser = ser
        self.log = log
        self.verbose = verbose
        self.rx = bytearray()

    @property
    def baudrate(self) -> int:
        return int(self.ser.baudrate)

    def set_baud(self, baud: int) -> None:
        self.log("UART baud -> %d" % baud)
        self.ser.flush()
        time.sleep(0.05)
        self.ser.baudrate = baud
        self.rx.clear()
        try:
            self.ser.reset_input_buffer()
        except Exception:
            pass
        time.sleep(0.12)

    def send_raw(self, raw: bytes) -> None:
        self.ser.write(raw)

    def send_frame(
        self,
        msg_type: int,
        cmd: int,
        data: bytes = b"",
        label: str | None = None,
        quiet: bool = False,
    ) -> None:
        raw = pack_frame(msg_type, cmd, data)
        self.ser.write(raw)
        if not quiet:
            desc = label or "TX type=%02X cmd=%02X len=%d" % (msg_type, cmd, len(data))
            self.log("%s data=%s" % (desc, hx(data, 24)))

    def read_frame(self, timeout: float) -> Frame | None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            fr = pop_frame(self.rx)
            if fr:
                return fr
            data = self.ser.read(8192)
            if data:
                self.rx += data
                continue
            time.sleep(0.002)
        return None

    def wait_for(self, msg_type: int, cmd: int, timeout: float, name: str) -> Frame:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            fr = self.read_frame(max(0.001, deadline - time.monotonic()))
            if not fr:
                break
            if fr.msg_type == msg_type and fr.cmd == cmd:
                self.log("RX %s OK type=%02X cmd=%02X len=%d data=%s" % (name, fr.msg_type, fr.cmd, len(fr.data), hx(fr.data, 24)))
                return fr
            self.log_rx(fr, expected="%s type=%02X cmd=%02X" % (name, msg_type, cmd))
        raise TimeoutError("timeout waiting for %s type=%02X cmd=%02X" % (name, msg_type, cmd))

    def wait_ack(self, cmd: int, timeout: float = 5.0) -> Frame:
        return self.wait_for(MSG_ACK, cmd, timeout, "ACK")

    def wait_req(self, cmd: int, timeout: float = 5.0) -> Frame:
        return self.wait_for(MSG_REQ, cmd, timeout, "REQ")

    def log_rx(self, fr: Frame, expected: str | None = None) -> None:
        if fr.msg_type == MSG_NOTIFY and fr.cmd == CMD_PROGRESS:
            text = printable(fr.data)
            self.log("RX progress len=%d text=%r" % (len(fr.data), text))
            return
        suffix = " while waiting %s" % expected if expected else ""
        if self.verbose or fr.msg_type != MSG_NOTIFY:
            self.log(
                "RX unexpected type=%02X cmd=%02X len=%d data=%s%s"
                % (fr.msg_type, fr.cmd, len(fr.data), hx(fr.data, 24), suffix)
            )


class Ci13lcFlasher:
    def __init__(self, link: UartLink, agent: bytes, fw_blocks: list[FlashBlock], log: Logger):
        self.link = link
        self.agent = agent
        self.fw_blocks = fw_blocks
        self.log = log

    def run(self, handshake_timeout: float) -> None:
        self.stage_115200_maskrom(handshake_timeout)
        self.stage_460800_agent()
        self.stage_115200_updater_init()
        self.stage_921600_firmware()

    def stage_115200_maskrom(self, handshake_timeout: float) -> None:
        self.log("Stage 1: maskrom handshake/control @115200")
        self.link.set_baud(115200)
        self.spam_handshake(handshake_timeout)

        # Captured tool sends A0/85 seven times and receives six A2/85 replies.
        self.log("Send A0/85 readiness burst")
        for _ in range(7):
            self.link.send_frame(MSG_CMD, 0x85, quiet=True)
            time.sleep(0.015)
        self.collect_acks(0x85, min_count=1, window=1.0)

        self.link.send_frame(MSG_CMD, 0x47, b"\x00", label="TX A0/47")
        self.link.wait_ack(0x47, timeout=3.0)

        self.link.send_frame(MSG_CMD, 0x84, label="TX A0/84")
        self.link.wait_ack(0x84, timeout=3.0)

        self.change_baud(460800)

    def stage_460800_agent(self) -> None:
        self.log("Stage 2: agent download @460800")

        self.link.send_frame(MSG_CMD, CMD_TEST_BAUD, u32(0x10), label="TX TEST_BAUD")
        self.link.wait_ack(CMD_TEST_BAUD, timeout=3.0)

        agent_crc = crc16_ccitt(self.agent)
        self.link.send_frame(
            MSG_CMD,
            CMD_BLOCK_INFO,
            block_info_data(AGENT_ADDR, len(self.agent), agent_crc),
            label="TX AGENT BLOCK_INFO addr=0x%08X size=0x%X crc=0x%04X" % (AGENT_ADDR, len(self.agent), agent_crc),
        )
        self.link.wait_ack(CMD_BLOCK_INFO, timeout=3.0)

        self.serve_write_requests("agent", self.agent, expected_chunks=len(self.agent) // CHUNK, timeout=8.0)

        self.link.send_frame(MSG_CMD, CMD_VERIFY_AGENT, label="TX A0/8A agent verify/jump")
        ack = self.link.wait_ack(CMD_VERIFY_AGENT, timeout=5.0)
        if ack.data not in (b"", b"\x01"):
            raise RuntimeError("agent verify returned unexpected data: %s" % hx(ack.data))

    def stage_115200_updater_init(self) -> None:
        self.log("Stage 3: updater init/control @115200")
        self.link.set_baud(115200)

        self.link.send_frame(MSG_CMD, CMD_CHECK_READY, label="TX CHECK_READY")
        self.link.wait_ack(CMD_CHECK_READY, timeout=5.0)

        self.link.send_frame(MSG_CMD, CMD_SET_FW_FMT, b"\x02", label="TX SET_FW_FMT v2")
        self.link.wait_ack(CMD_SET_FW_FMT, timeout=5.0)

        self.link.send_frame(MSG_CMD, CMD_GET_INFO, label="TX GET_INFO")
        info = self.link.wait_ack(CMD_GET_INFO, timeout=35.0)
        text = printable(info.data)
        if text:
            self.log("GET_INFO printable=%r" % text[:180])

        self.change_baud(921600)

    def stage_921600_firmware(self) -> None:
        self.log("Stage 4: firmware write @921600")

        self.link.send_frame(MSG_CMD, CMD_TEST_BAUD, u32(0x10), label="TX TEST_BAUD")
        self.link.wait_ack(CMD_TEST_BAUD, timeout=3.0)

        for index, block in enumerate(self.fw_blocks, 1):
            self.write_firmware_block(index, block)

        self.link.send_frame(MSG_CMD, CMD_COMPLETE, label="TX COMPLETE")
        self.link.wait_ack(CMD_COMPLETE, timeout=10.0)
        self.log("Flash flow complete")

    def spam_handshake(self, timeout: float) -> None:
        raw = pack_frame(MSG_REQ, CMD_HANDSHAKE)
        deadline = time.monotonic() + timeout
        sent = 0
        last_report = time.monotonic()
        while time.monotonic() < deadline:
            self.link.send_raw(raw)
            sent += 1
            fr = self.link.read_frame(0.020)
            if fr and fr.msg_type == MSG_ACK and fr.cmd == CMD_HANDSHAKE:
                self.log("RX handshake ACK after %d sends" % sent)
                return
            if fr:
                self.link.log_rx(fr, expected="handshake ACK")
            now = time.monotonic()
            if now - last_report >= 1.0:
                self.log("handshake spam sent=%d; reset chip now if no ACK yet" % sent)
                last_report = now
        raise TimeoutError("no A2/83 handshake ACK within %.1fs" % timeout)

    def collect_acks(self, cmd: int, min_count: int, window: float) -> int:
        deadline = time.monotonic() + window
        count = 0
        while time.monotonic() < deadline:
            fr = self.link.read_frame(max(0.001, deadline - time.monotonic()))
            if not fr:
                break
            if fr.msg_type == MSG_ACK and fr.cmd == cmd:
                count += 1
                self.log("RX ACK cmd=%02X #%d len=%d data=%s" % (cmd, count, len(fr.data), hx(fr.data, 24)))
                continue
            self.link.log_rx(fr, expected="ACK cmd=%02X burst" % cmd)
        if count < min_count:
            raise TimeoutError("only got %d ACK cmd=%02X; expected at least %d" % (count, cmd, min_count))
        return count

    def change_baud(self, baud: int) -> None:
        self.link.send_frame(MSG_CMD, CMD_CHANGE_BAUD, u32(baud), label="TX CHANGE_BAUD %d" % baud)
        self.link.wait_ack(CMD_CHANGE_BAUD, timeout=3.0)
        self.link.set_baud(baud)

    def serve_write_requests(self, name: str, data: bytes, expected_chunks: int, timeout: float) -> None:
        sent = 0
        while True:
            fr = self.link.read_frame(timeout)
            if not fr:
                raise TimeoutError("%s: timeout waiting for A1/08 or A1/09" % name)

            if fr.msg_type == MSG_REQ and fr.cmd == CMD_WRITE:
                if len(fr.data) != 8:
                    raise RuntimeError("%s: malformed A1/08 data len=%d" % (name, len(fr.data)))
                offset = int.from_bytes(fr.data[:4], "little")
                size = int.from_bytes(fr.data[4:8], "little")
                chunk = data[offset : offset + size]
                if len(chunk) < size:
                    chunk += b"\xFF" * (size - len(chunk))
                self.link.send_frame(MSG_ACK, CMD_WRITE, u32(offset) + chunk, quiet=True)
                sent += 1
                if sent == 1 or sent == expected_chunks or sent % 32 == 0:
                    self.log(
                        "%s chunk %d/%d offset=0x%X size=0x%X"
                        % (name, sent, expected_chunks, offset, size)
                    )
                continue

            if fr.msg_type == MSG_REQ and fr.cmd == CMD_BLOCK_DONE:
                if sent != expected_chunks:
                    self.log("%s got A1/09 after %d chunks; expected %d" % (name, sent, expected_chunks))
                else:
                    self.log("%s got A1/09 after all %d chunks" % (name, sent))
                return

            self.link.log_rx(fr, expected="%s A1/08 or A1/09" % name)

    def write_firmware_block(self, index: int, block: FlashBlock) -> None:
        base = block.base
        data = block.data
        data_crc = crc16_ccitt(data)
        chunks = len(data) // CHUNK
        self.log(
            "FW block %d %s base=0x%08X size=0x%X chunks=%d verify_crc=0x%04X"
            % (index, block.name, base, len(data), chunks, data_crc)
        )

        # Firmware capture used crc=0 in A0/06 and the real CRC only in A0/0A.
        self.link.send_frame(
            MSG_CMD,
            CMD_BLOCK_INFO,
            block_info_data(base, len(data), 0),
            label="TX FW BLOCK_INFO #%d crc_field=0" % index,
        )
        self.link.wait_ack(CMD_BLOCK_INFO, timeout=5.0)

        self.link.send_frame(MSG_CMD, CMD_ERASE, erase_data(base, len(data)), label="TX ERASE #%d" % index)
        self.link.wait_ack(CMD_ERASE, timeout=20.0)

        self.serve_write_requests("fw%d-%s" % (index, block.name), data, expected_chunks=chunks, timeout=20.0)

        self.link.send_frame(
            MSG_CMD,
            CMD_VERIFY,
            block_info_data(base, len(data), data_crc),
            label="TX VERIFY #%d crc=0x%04X" % (index, data_crc),
        )
        ack = self.link.wait_ack(CMD_VERIFY, timeout=20.0)
        if ack.data not in (b"", b"\x01"):
            raise RuntimeError("FW block %d verify failed/unexpected: %s" % (index, hx(ack.data)))


def load_inputs(agent_path: Path, fw_path: Path, erase_nv: bool) -> tuple[bytes, bytes, FirmwareLayout, list[FlashBlock]]:
    agent = agent_path.read_bytes()
    fw = fw_path.read_bytes()
    layout, fw_blocks = build_firmware_blocks(fw, erase_nv=erase_nv)
    return agent, fw, layout, fw_blocks


def dry_run(agent: bytes, fw: bytes, layout: FirmwareLayout, fw_blocks: list[FlashBlock], log: Logger) -> None:
    log("DRY RUN")
    log("agent size=%d crc=0x%04X sha-like check not computed here" % (len(agent), crc16_ccitt(agent)))
    log(
        "firmware size=%d aligned_size=%d pad=%d crc16_raw=0x%04X crc32_ui=0x%08X"
        % (len(fw), len(fw_blocks[0].data), len(fw_blocks[0].data) - len(fw), crc16_ccitt(fw), zlib.crc32(fw) & 0xFFFFFFFF)
    )
    log(
        "layout chip=%s format=%d hw=%s sw=%s nv=0x%X+0x%X checksum=0x%04X"
        % (
            layout.chip_name,
            layout.firmware_format,
            layout.hardware_name,
            layout.software_name,
            layout.nv_offset,
            layout.nv_size,
            layout.table_checksum,
        )
    )
    for entry in layout.entries:
        if entry.offset == 0xFFFFFFFF or entry.size == 0xFFFFFFFF:
            log("partition %-10s absent" % entry.name)
        else:
            log(
                "partition %-10s off=0x%X size=0x%X end=0x%X crc=0x%04X status=0x%02X"
                % (entry.name, entry.offset, entry.size, entry.offset + entry.size, entry.crc & 0xFFFF, entry.status)
            )
    for index, block in enumerate(fw_blocks, 1):
        log(
            "block%d %s base=0x%08X size=0x%X chunks=%d crc=0x%04X all_ff=%s"
            % (
                index,
                block.name,
                block.base,
                len(block.data),
                len(block.data) // CHUNK,
                crc16_ccitt(block.data),
                all(x == 0xFF for x in block.data),
            )
        )
    log("sample handshake=%s" % hx(pack_frame(MSG_REQ, CMD_HANDSHAKE)))
    log("sample baud 460800=%s" % hx(pack_frame(MSG_CMD, CMD_CHANGE_BAUD, u32(460800))))
    log("sample baud 921600=%s" % hx(pack_frame(MSG_CMD, CMD_CHANGE_BAUD, u32(921600))))

    if len(agent) == 0x5000 and crc16_ccitt(agent) == 0x6BBC:
        log("agent matches captured 0x5000/0x6BBC")
    if len(fw) == 1599081 and crc16_ccitt(fw_blocks[0].data) == 0x558B:
        nv_ok = len(fw_blocks) > 1 and crc16_ccitt(fw_blocks[1].data) == 0xF1EF
        log("Reboot3 captured firmware block matches: block1=0x558B nv_match=%s" % nv_ok)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Flash CI13LC firmware over UART using reconstructed PACK_UPDATE_TOOL flow.")
    parser.add_argument("port", nargs="?", default="COM3", help="serial port, default COM3")
    parser.add_argument("firmware", nargs="?", type=Path, default=DEFAULT_FW, help="firmware .bin path")
    parser.add_argument("--agent", type=Path, default=DEFAULT_AGENT, help="padded ci13lc updater/agent path")
    parser.add_argument("--handshake-timeout", type=float, default=30.0, help="seconds to spam A1/83 before giving up")
    nv_group = parser.add_mutually_exclusive_group()
    nv_group.add_argument("--erase-nv", dest="erase_nv", action="store_true", default=False,
                          help="erase/fill NV region with 0xFF (giong TICH o Erase NV tren PACK_UPDATE_TOOL)")
    nv_group.add_argument("--keep-nv", dest="erase_nv", action="store_false", default=False,
                          help="giu nguyen vung NV - MAC DINH (bo tich Erase NV)")
    parser.add_argument("--dry-run", action="store_true", help="validate files/CRC and print sample frames only")
    parser.add_argument("--verbose", action="store_true", help="log notify/extra frames more verbosely")
    parser.add_argument("--no-log-file", action="store_true", help="do not write test_update_last.log")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    log = Logger(None if args.no_log_file else DEFAULT_LOG)
    try:
        agent, fw, layout, fw_blocks = load_inputs(args.agent, args.firmware, erase_nv=args.erase_nv)
        log("agent=%s" % args.agent)
        log("firmware=%s" % args.firmware)
        log("nv_mode=%s" % ("erase" if args.erase_nv else "keep"))

        if args.dry_run:
            dry_run(agent, fw, layout, fw_blocks, log)
            return 0

        try:
            import serial
        except ImportError:
            log("ERROR: missing pyserial. Install with: pip install pyserial")
            return 2

        log("Opening %s @115200. Reset chip while handshake is spamming." % args.port)
        ser = serial.Serial(args.port, 115200, timeout=0, write_timeout=5)
        try:
            try:
                ser.setDTR(False)
                ser.setRTS(False)
            except Exception:
                pass
            link = UartLink(ser, log, verbose=args.verbose)
            flasher = Ci13lcFlasher(link, agent, fw_blocks, log)
            flasher.run(args.handshake_timeout)
            return 0
        finally:
            ser.close()
            log("serial port closed")
    except Exception as exc:
        log("ERROR: %s" % exc)
        return 1
    finally:
        log.close()


if __name__ == "__main__":
    sys.exit(main())