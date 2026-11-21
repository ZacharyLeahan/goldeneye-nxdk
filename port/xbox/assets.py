"""Convert cartridge records to the Xbox's little-endian, 32-bit game ABI."""

from pathlib import Path
import re
import sys
import struct
import zlib

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools_pc"))
from d88_propdefs import PROPDEF_N64_WORDS, OBJ_TAIL_DESC

MASK = 0xffffff


class Records:
    def __init__(self, src):
        self.src = src
        self.dst = bytearray(src)

    def read(self, pos, width=4):
        if pos < 0 or pos + width > len(self.src):
            raise ValueError(f"Record outside cartridge resource at {pos:#x}")
        return int.from_bytes(self.src[pos:pos + width], "big")

    def swap(self, pos, width=4, count=1):
        if pos < 0 or pos + width * count > len(self.src):
            raise ValueError(f"Field outside cartridge resource at {pos:#x}")
        for i in range(pos, pos + width * count, width):
            self.dst[i:i + width] = self.src[i:i + width][::-1]


def audio_bank(src, pcm_ranges):
    r = Records(src)
    visited = set()

    def visit(kind, pos):
        if not pos or (kind, pos) in visited:
            return
        visited.add((kind, pos))
        if kind == "bank":
            n = r.read(pos, 2)
            r.swap(pos, 2)
            r.swap(pos + 4, 4, n + 2)
            for i in range(n + 1):
                visit("instrument", r.read(pos + 8 + i * 4))
        elif kind == "instrument":
            n = r.read(pos + 14, 2)
            r.swap(pos + 12, 2, 2)
            r.swap(pos + 16, 4, n)
            for i in range(n):
                visit("sound", r.read(pos + 16 + i * 4))
        elif kind == "sound":
            r.swap(pos, 4, 3)
            visit("env", r.read(pos))
            visit("wave", r.read(pos + 8))
        elif kind == "wave":
            wave_type = src[pos + 8]
            r.swap(pos, 4, 2)
            r.swap(pos + 12, 4, 2 if wave_type == 0 else 1)
            if wave_type == 1:
                start, size = r.read(pos), r.read(pos + 4)
                pcm_ranges.add((start, start + size))
            loop = r.read(pos + 12)
            if loop:
                r.swap(loop, 4, 3)
                if wave_type == 0:
                    r.swap(loop + 12, 2, 16)
            if wave_type == 0:
                visit("book", r.read(pos + 16))
        elif kind == "book":
            count = 8 * r.read(pos) * r.read(pos + 4)
            r.swap(pos, 4, 2)
            r.swap(pos + 8, 2, count)
        elif kind == "env":
            r.swap(pos, 4, 3)

    banks = r.read(2, 2)
    r.swap(0, 2, 2)
    r.swap(4, 4, banks)
    for i in range(banks):
        visit("bank", r.read(4 + 4 * i))
    return r.dst


# (width, count) groups in the original cartridge's model record layout.
MODEL_FIELDS = {
    1: [(2, 2), (4, 1), (2, 3)],
    2: [(4, 3), (2, 4), (4, 2)],
    3: [(4, 3), (2, 4), (4, 2)],
    4: [(4, 4), (2, 1)], 8: [(4, 3), (2, 1)],
    9: [(4, 8), (2, 2)], 10: [(4, 7)],
    12: [(4, 8), (2, 1)], 13: [(4, 7)], 15: [(4, 7)],
    18: [(4, 1), (2, 1)], 21: [(4, 3), (2, 2), (4, 1)],
    22: [(4, 3)], 23: [(2, 1)], 24: [(4, 3), (2, 2), (4, 2), (2, 2)],
}


def model(src, switches, textures):
    r = Records(src)
    pointer = lambda pos: r.read(pos) & MASK
    r.swap(0, 4, switches)
    for i in range(textures):
        r.swap(4 * switches + 12 * i)
    root = 4 * switches + 12 * textures
    pending = [root]
    visited, boundaries, lists = set(), set(), set()
    verts = {}
    while pending:
        node = pending.pop()
        if node in visited:
            continue
        visited.add(node)
        op = r.read(node, 2) & 0xff
        data = pointer(node + 4)
        boundaries.update((node, data))
        r.swap(node, 2)
        r.swap(node + 4, 4, 5)
        child = pointer(data + 8 if op == 8 else data if op == 18 else node + 20)
        pending.extend(p for p in (child, pointer(node + 12)) if p)
        pos = data
        for width, count in MODEL_FIELDS[op]:
            r.swap(pos, width, count)
            pos += width * count
        if op in (4, 24):
            lists.update((pointer(data), pointer(data + 4)))
        if op == 4:
            verts[pointer(data + 12)] = r.read(data + 16, 2)
        if op == 22:
            lists.add(pointer(data + 8))
            verts[pointer(data + 4)] = r.read(data) * 4
        if op == 24:
            n = r.read(data + 12, 2)
            verts[pointer(data + 8)] = n
            hit = pointer(data + 16)
            usage = pointer(data + 20)
            if usage:
                r.swap(usage, 2, n)
            if hit:
                for i in range(r.read(data + 14, 2)):
                    p = hit + i * 16
                    r.swap(p, 2, 4)
                    r.swap(p + 8)
                    r.swap(p + 12, 2, 2)
    boundaries.update(lists | verts.keys() | {len(src)})
    for pos, n in verts.items():
        if pos:
            end = pos + n * 16 if n else min(p for p in boundaries if p > pos)
            for p in range(pos, end, 16):
                r.swap(p, 2, 6)
    for pos in lists - {0}:
        while True:
            r.swap(pos, 4, 2)
            if src[pos] == 0xb8:
                break
            pos += 8
    return r.dst


def pack(rom):
    symbols = {s: int(addr, 16) - 0x10000000 for s, addr in
               re.findall(r'^\.set (\w+), (0x[\da-fA-F]+)', (REPO / 'port/src/romassets_u.s').read_text(), re.M)}
    replacements = []
    for bank in ("sfx", "instruments"):
        start = symbols[f"_{bank}ctlSegmentRomStart"]
        end = symbols[f"_{bank}tblSegmentRomStart"]
        pcm_ranges = set()
        replacements.append((0, start, audio_bank(rom[start:end], pcm_ranges)))
        for first, last in sorted(pcm_ranges):
            wave = rom[end + first:end + last]
            if len(wave) != last - first or len(wave) % 2:
                raise ValueError(f"Invalid {bank} PCM range")
            swapped = bytearray(wave)
            swapped[0::2], swapped[1::2] = wave[1::2], wave[0::2]
            replacements.append((0, end + first, swapped))
    metadata = {}
    for header in sorted((REPO / "assets").rglob("*ileHeader.inc.c")):
        for m in re.finditer(r'^\s*MODELFILEHEADER\((.*)\)\s*;?\s*$', header.read_text(), re.M):
            fields = [f.strip() for f in m[1].split(",")]
            metadata[fields[0]] = (int(fields[4], 0), int(fields[8], 0))
    names = re.findall(r'\{\s*\w+\s*,\s*"([CGP][^"]+Z)"', (REPO / 'assets/obseg/file_resource_table.inc.c').read_text())
    for name in names:
        key = next((k for k in (name[1:-1],name[:-1],name[1:],name) if k in metadata), None)
        if key is None: raise ValueError(f"Missing model header for {name}")
        start = symbols[name]
        if rom[start:start+2] != b'\x11\x72':
            raise ValueError(f"{name} lacks the RZ header")
        raw = zlib.decompress(rom[start+2:], -15)
        try:
            new = model(raw, *metadata[key])
        except (ValueError, KeyError) as exc:
            raise ValueError(f"{name}: {exc}") from exc
        enc = zlib.compressobj(9, wbits=-15)
        replacements.append((1, start, b'\x11\x72' + enc.compress(new) + enc.flush()))
    # Basement and Stack use the Library geometry with distinct prop setups.
    geometry = ("dish", "ref", "oat", "ame", "ark", "sevb", "arch", "cave", "cryp")
    file_table = (REPO / "assets/obseg/file_resource_table.inc.c").read_text()
    bg_symbols = dict(re.findall(r'"bg/bg_(\w+)_all_p\.seg",\s*&(\w+)', file_table))
    stages = [(name, background) for name in dict.fromkeys(bg_symbols.values())
              if symbols["end_" + name] > symbols[name] or name == bg_symbols["ame"]]
    stages += [(name, collision) for name in symbols if name.startswith("Tbg_")
               and symbols["end_" + name] > symbols[name]]
    stages += [(f"Ump_setup{tag}Z", stage_setup) for tag in (*geometry, "imp", "ash", "statue")]
    stages += [(name, stage_setup) for name in symbols if name.startswith("Usetup") and name.endswith("Z")]
    for name, convert in stages:
        start, stop = symbols[name], symbols["end_" + name]
        if name == bg_symbols["ame"] and stop == start:
            # The IMP linker label has zero length. The Library file spans
            # this shared offset up to the next distinct background resource.
            stop = min(addr for symbol, addr in symbols.items()
                       if symbol.startswith("bg_") and addr > start)
        blob = rom[start:stop]
        if name.startswith("bg_"):
            replacements.append((1, start, convert(blob)))
        else:
            enc = zlib.compressobj(9, wbits=-15)
            data = convert(zlib.decompress(blob[2:], -15))
            replacements.append((1, start, b'\x11\x72' + enc.compress(data) + enc.flush()))
    header = bytearray(struct.pack('<4sI', b'GE33', len(replacements)))
    data = bytearray()
    for kind, start, blob in replacements:
        data += bytes(-len(data) % 16)
        header += struct.pack('<4I', kind, start, len(data), len(blob))
        data.extend(blob)
    return header + bytes(-len(header) % 16) + data


def background(src):
    r = Records(src)
    rooms, portals, env = (r.read(p) & MASK for p in (4, 8, 12))
    r.swap(0, 4, 5)
    end = env or portals
    r.swap(rooms, 4, (end - rooms) // 4)
    points = set()
    if env:
        for p in range(env, portals, 8):
            r.swap(p + 4)
            if src[p] == 100 and r.read(p + 4):
                points.add(r.read(p + 4) & MASK)
            if not src[p]:
                break
    p = portals
    while (offset := r.read(p) & MASK):
        points.add(offset)
        r.swap(p)
        p += 8
    for p in points:
        r.swap(p + 4, 4, 3 * src[p])
    return r.dst


def collision(src):
    r = Records(src)
    pos = 4
    while r.read(pos):
        r.swap(pos)
        pos += 4
    r.swap(0)
    pos += 4
    while r.read(pos):
        n = r.read(pos + 6, 2) >> 12
        if n > 10:
            raise ValueError("Invalid collision tile")
        r.swap(pos, 2)
        r.swap(pos + 4, 2, 2 + 4 * n)
        pos += max(32, 8 + n * 8)
    return r.dst


def stage_setup(src):
    r = Records(src)
    tables = [r.read(4 * i) for i in range(10)]
    r.swap(0, 4, 10)
    paths = set()
    for index, stride, termination in ((0, 16, 0), (1, 12, 0), (4, 8, 0),
            (5, 8, 0), (6, 44, 36), (7, 68, 36), (8, 4, 0), (9, 4, 0)):
        p = tables[index]
        while p:
            value = r.read(p + termination)
            r.swap(p, 4, stride // 4)
            if index == 4:  # patrol path id and flags are bytes, length is u16
                r.dst[p + 4:p + 6] = src[p + 4:p + 6]
                r.swap(p + 6, 2)
            if (index == 0 and value >= 0x80000000) or (index != 0 and value == 0):
                break
            if index in (0, 1):
                paths.add(r.read(p + 4))
            if index in (1, 4):
                paths.add(value)
            p += stride
    for p in paths - {0}:
        while True:
            r.swap(p)
            if r.read(p) >= 0x80000000:
                break
            p += 4
    p = tables[2]
    while p:
        tag = r.read(p)
        n = (3, 4, 4, 8, 2, 2, 10, 3, 2, 1)[tag]
        r.swap(p, 4, n)
        if tag == 6:  # camera language index is a pair of u16s
            r.swap(p + 28, 2, 2)
        if tag == 9:
            break
        p += n * 4
    p = tables[3]
    while p:
        tag = src[p + 3]
        words = PROPDEF_N64_WORDS[tag]
        r.swap(p, 4, words)
        r.dst[p + 2:p + 4] = src[p + 2:p + 4]
        r.swap(p, 2)
        if tag == 48:
            break
        if tag == 9:  # GuardRecord
            r.swap(p + 4, 2, 10)
        elif tag == 18:  # SetGuardAttribute
            r.swap(p + 8, 2)
            r.dst[p + 10:p + 12] = src[p + 10:p + 12]
        elif tag == 22:  # TagRecord
            r.swap(p + 4, 2, 2)
        if tag not in (1, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 17, 20, 21, 36, 39, 40, 42, 43, 45, 47):
            p += 4 * words
            continue
        r.swap(p + 4, 2, 2)
        r.dst[p + 120:p + 128] = src[p + 120:p + 128]
        if tag == 8:
            r.dst[p + 128:p + 130] = src[p + 128:p + 130]
            r.swap(p + 130, 2)
        if tag in OBJ_TAIL_DESC:
            for word in OBJ_TAIL_DESC[tag][1]:
                r.swap(p + word * 4, 2, 2)
        if tag == 11:  # MultiMonitor.ImageNums
            r.dst[p + 592:p + 596] = src[p + 592:p + 596]
        if tag == 1:
            r.swap(p + 38 * 4, 2, 2)
            r.dst[p + 47*4:p + 47*4+2] = src[p + 47*4:p + 47*4+2]
            r.swap(p + 47*4+2, 2)
            r.swap(p + 49*4, 2)
            r.dst[p + 49*4+2:p + 50*4] = src[p + 49*4+2:p + 50*4]
        p += 4 * words
    return r.dst
