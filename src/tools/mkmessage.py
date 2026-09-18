#!/usr/bin/env python3
"""Convert selected UTF-8 hiragana messages to MZ-1500 display codes."""

import argparse
import re
import struct
import subprocess
import tempfile
from pathlib import Path


KANA_MACROS = {
    "あ": "DC_KANA_A", "い": "DC_KANA_I", "う": "DC_KANA_U",
    "え": "DC_KANA_E", "お": "DC_KANA_O",
    "か": "DC_KANA_KA", "き": "DC_KANA_KI", "く": "DC_KANA_KU",
    "け": "DC_KANA_KE", "こ": "DC_KANA_KO",
    "さ": "DC_KANA_SA", "し": "DC_KANA_SHI", "す": "DC_KANA_SU",
    "せ": "DC_KANA_SE", "そ": "DC_KANA_SO",
    "た": "DC_KANA_TA", "ち": "DC_KANA_CHI", "つ": "DC_KANA_TSU",
    "て": "DC_KANA_TE", "と": "DC_KANA_TO",
    "な": "DC_KANA_NA", "に": "DC_KANA_NI", "ぬ": "DC_KANA_NU",
    "ね": "DC_KANA_NE", "の": "DC_KANA_NO",
    "は": "DC_KANA_HA", "ひ": "DC_KANA_HI", "ふ": "DC_KANA_FU",
    "へ": "DC_KANA_HE", "ほ": "DC_KANA_HO",
    "ま": "DC_KANA_MA", "み": "DC_KANA_MI", "む": "DC_KANA_MU",
    "め": "DC_KANA_ME", "も": "DC_KANA_MO",
    "や": "DC_KANA_YA", "ゆ": "DC_KANA_YU", "よ": "DC_KANA_YO",
    "ら": "DC_KANA_RA", "り": "DC_KANA_RI", "る": "DC_KANA_RU",
    "れ": "DC_KANA_RE", "ろ": "DC_KANA_RO",
    "わ": "DC_KANA_WA", "を": "DC_KANA_WO", "ん": "DC_KANA_N",
    "ぁ": "DC_KANA_XA", "ぃ": "DC_KANA_XI", "ぅ": "DC_KANA_XU",
    "ぇ": "DC_KANA_XE", "ぉ": "DC_KANA_XO", "っ": "DC_KANA_XTSU",
    "ゃ": "DC_KANA_XYA", "ゅ": "DC_KANA_XYU", "ょ": "DC_KANA_XYO",
    "ー": "DC_KANA_HYPHEN", "、": "DC_KUTEN", "。": "DC_TOUTEN",
    "「": "DC_L_BRACKET", "」": "DC_R_BRACKET",
}

VOICED = {
    "が": "か", "ぎ": "き", "ぐ": "く", "げ": "け", "ご": "こ",
    "ざ": "さ", "じ": "し", "ず": "す", "ぜ": "せ", "ぞ": "そ",
    "だ": "た", "ぢ": "ち", "づ": "つ", "で": "て", "ど": "と",
    "ば": "は", "び": "ひ", "ぶ": "ふ", "べ": "へ", "ぼ": "ほ",
    "ゔ": "う",
}

SEMI_VOICED = {
    "ぱ": "は", "ぴ": "ひ", "ぷ": "ふ", "ぺ": "へ", "ぽ": "ほ",
}

ASCII_MACROS = {
    " ": "DC_SPC", ".": "DC_PERIOD", ",": "DC_COMMA",
    "!": "DC_EXCLAM", "?": "DC_QUESTION", "-": "DC_MINUS",
    ":": "DC_COLON", ";": "DC_SEMICOLON", "(": "DC_L_BLACKET",
    ")": "DC_R_BLACKET", "[": "DC_L_SQ_BLACKET",
    "]": "DC_R_SQ_BLACKET", "/": "DC_SLASH",
    "\\": "DC_BACK_SLASH", "_": "DC_D_BAR", "|": "DC_PIPE",
    "*": "DC_STAR", "@": "DC_AT",
}
ASCII_MACROS.update({chr(code): "DC_" + chr(code).upper()
                     for code in range(ord("a"), ord("z") + 1)})
ASCII_MACROS.update({chr(code): "DC_" + chr(code)
                     for code in range(ord("A"), ord("Z") + 1)})

MESSAGE_FORMAT_STRING = 0xFE
MESSAGE_FORMAT_DECIMAL = 0xFD
MESSAGE_FORMAT_UNSIGNED = 0xFC
MESSAGE_FORMAT_LONG = 0xFB
MESSAGE_CSET_1 = 0xCE
MESSAGE_CSET_0 = 0xCF
MZT_HEADER_SIZE = 128
MZT_ATTRIBUTE_MACHINE_CODE = 0x01
MONSTER_RECORD_SIZE = 38

LEVEL_POINTS = (
    10, 20, 40, 80, 160, 320, 640, 1300, 2600, 5200,
    10000, 20000, 40000, 80000, 160000, 320000, 1000000,
    3333333, 6666666, 10000000, 99900000,
)

WAND_VALUES = (25, 50, 45, 8, 55, 2, 25, 20, 20, 0)
RING_VALUES = (250, 100, 255, 295, 200, 250, 250, 25, 300, 290, 270)

MONSTER_FLAGS = {
    "HASTED": 0o1, "SLOWED": 0o2, "INVISIBLE": 0o4,
    "ASLEEP": 0o10, "WAKENS": 0o20, "WANDERS": 0o40,
    "FLIES": 0o100, "FLITS": 0o200, "CAN_FLIT": 0o400,
    "CONFUSED": 0o1000, "RUSTS": 0o2000, "HOLDS": 0o4000,
    "FREEZES": 0o10000, "STEALS_GOLD": 0o20000,
    "STEALS_ITEM": 0o40000, "STINGS": 0o100000,
    "DRAINS_LIFE": 0o200000, "DROPS_LEVEL": 0o400000,
    "SEEKS_GOLD": 0o1000000, "FREEZING_ROGUE": 0o2000000,
    "RUST_VANISHED": 0o4000000, "CONFUSES": 0o10000000,
    "IMITATES": 0o20000000, "FLAMES": 0o40000000,
    "STATIONARY": 0o100000000, "NAPPING": 0o200000000,
    "ALREADY_MOVED": 0o400000000,
}


def read_defines(path):
    values = {}
    define = re.compile(r"^\s*#define\s+(DC_[A-Z0-9_]+)\s+(0x[0-9a-fA-F]+|[0-9]+)")
    for line in path.read_text(encoding="utf-8").splitlines():
        match = define.match(line)
        if match:
            values[match.group(1)] = int(match.group(2), 0)
    return values


def parse_messages(path, selected):
    result = []
    pattern = re.compile(r'^\s*(\d+)\s+"([^"]*)"')
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = pattern.match(line)
        if not match:
            continue
        msg_id = int(match.group(1))
        if msg_id in selected:
            result.append((msg_id, match.group(2), line_no))
    found = {msg_id for msg_id, _, _ in result}
    missing = selected - found
    if missing:
        raise ValueError("message ID not found: " + ", ".join(map(str, sorted(missing))))
    if len(found) != len(result):
        raise ValueError("duplicate selected message ID")
    return sorted(result)


def macro_value(values, name, char, line_no):
    if name not in values:
        raise ValueError(f"line {line_no}: {name} is not defined in mz_display.h ({char})")
    return values[name]


def encode_message(text, values, line_no):
    encoded = []
    index = 0
    katakana = False
    while index < len(text):
        if text.startswith("%ld", index):
            encoded.append(MESSAGE_FORMAT_LONG)
            index += 3
            continue
        if text.startswith("%s", index):
            encoded.append(MESSAGE_FORMAT_STRING)
            index += 2
            continue
        if text.startswith("%d", index):
            encoded.append(MESSAGE_FORMAT_DECIMAL)
            index += 2
            continue
        if text.startswith("%u", index):
            encoded.append(MESSAGE_FORMAT_UNSIGNED)
            index += 2
            continue
        char = text[index]
        if "ァ" <= char <= "ヶ":
            if not katakana:
                encoded.append(MESSAGE_CSET_0)
                katakana = True
            char = chr(ord(char) - 0x60)
        elif "A" <= char <= "Z":
            if not katakana:
                encoded.append(MESSAGE_CSET_0)
                katakana = True
        elif "a" <= char <= "z":
            if katakana:
                encoded.append(MESSAGE_CSET_1)
                katakana = False
        elif (char in KANA_MACROS or char in VOICED or
              char in SEMI_VOICED) and char != "ー":
            if katakana:
                encoded.append(MESSAGE_CSET_1)
                katakana = False
        if char in KANA_MACROS:
            encoded.append(macro_value(values, KANA_MACROS[char], char, line_no))
        elif char in VOICED:
            encoded.append(macro_value(values, KANA_MACROS[VOICED[char]], char, line_no))
            encoded.append(macro_value(values, "DC_DAKUTEN", char, line_no))
        elif char in SEMI_VOICED:
            encoded.append(macro_value(values, KANA_MACROS[SEMI_VOICED[char]], char, line_no))
            encoded.append(macro_value(values, "DC_HANDAKUTEN", char, line_no))
        elif char in ASCII_MACROS:
            encoded.append(macro_value(values, ASCII_MACROS[char], char, line_no))
        elif char == "%":
            encoded.append(macro_value(values, "DC_PERCENT", char, line_no))
        else:
            raise ValueError(f"line {line_no}: unsupported character {char!r}")
        index += 1
    if katakana:
        encoded.append(MESSAGE_CSET_1)
    if len(encoded) > 79:
        raise ValueError(f"line {line_no}: encoded message is {len(encoded)} bytes (maximum 79)")
    return encoded


def make_message_image(messages):
    data = []
    entries = []
    stored = {}
    data_offset = (len(messages) + 1) * 4
    for msg_id, encoded in messages:
        key = bytes(encoded)
        if key not in stored:
            stored[key] = data_offset + len(data)
            data.extend(encoded)
            data.append(0)
        entries.append((msg_id, stored[key]))
    image = bytearray()
    for msg_id, offset in entries:
        image.extend(struct.pack("<HH", msg_id, offset))
    image.extend(struct.pack("<HH", 0xffff, 0))
    image.extend(data)
    return image, {msg_id: offset for msg_id, offset in entries}


def parse_damage(value, line_no):
    result = []
    for dice in value.split("/"):
        match = re.fullmatch(r"(\d+)d(\d+)", dice)
        if not match:
            raise ValueError(f"monster line {line_no}: invalid damage {value!r}")
        result.extend((int(match.group(1)), int(match.group(2))))
    if len(result) == 2:
        result.extend((0, 0))
    if len(result) != 4 or any(number > 255 for number in result):
        raise ValueError(f"monster line {line_no}: invalid damage {value!r}")
    return result


def parse_monsters(path, display_values):
    records = []
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = line.partition("#")[0].strip()
        if not line:
            continue
        fields = line.split()
        if len(fields) != 10:
            raise ValueError(f"monster line {line_no}: expected 10 fields")
        letter, flag_text, damage = fields[:3]
        if len(letter) != 1 or not "A" <= letter <= "Z":
            raise ValueError(f"monster line {line_no}: invalid letter {letter!r}")
        try:
            flags = sum(MONSTER_FLAGS[name] for name in flag_text.split("|"))
        except KeyError as error:
            raise ValueError(f"monster line {line_no}: unknown flag {error.args[0]}") from None
        hp, kill_exp, first_level, last_level, hit_chance, drop_percent, name_id = (
            int(value, 0) for value in fields[3:]
        )
        damage_n1, damage_s1, damage_n2, damage_s2 = parse_damage(damage, line_no)
        byte_values = (hp, first_level, last_level, hit_chance, drop_percent,
                       damage_n1, damage_s1, damage_n2, damage_s2)
        if any(value < 0 or value > 255 for value in byte_values):
            raise ValueError(f"monster line {line_no}: byte value out of range")
        if not 0 <= kill_exp <= 65535 or not 0 <= name_id <= 65535:
            raise ValueError(f"monster line {line_no}: word value out of range")
        m_char = macro_value(display_values, "DC_A", letter, line_no) + \
            ord(letter) - ord("A")
        records.append(struct.pack(
            "<IhbhBBhhHhhbbHBHHBBBBBh",
            flags,
            hp, m_char, kill_exp,     # quantity/hp, ichar/m_char, kill_exp
            first_level, last_level, # is_protected/first, is_cursed/last
            hit_chance, 0,           # class/m_hit_chance, identified
            drop_percent,            # which_kind/drop_percent
            0, 0,                    # row, col
            0, 0,                    # d_enchant, hit_enchant
            0, 0, 0, 0,             # what_is, picked_up, in_use_flags, next
            0,                       # trail_char
            damage_n1, damage_s1, damage_n2, damage_s2,
            name_id))
    if len(records) != 26:
        raise ValueError(f"monster table has {len(records)} records (expected 26)")
    return bytearray(b"".join(records))


def make_external_image(messages, appearance_messages, monsters,
                        load_address, monster_address, level_points_address, id_wands_address,
                        id_rings_address, wand_materials_address,
                        gems_address):
    image, message_offsets = make_message_image(messages)
    appearance_offsets = {}
    stored_appearances = {}
    for msg_id, encoded in appearance_messages:
        key = bytes(encoded)
        if key not in stored_appearances:
            stored_appearances[key] = load_address + len(image)
            image.extend(encoded)
            image.append(0)
        appearance_offsets[msg_id] = stored_appearances[key]

    def message_pointer(msg_id, required=True):
        if msg_id in message_offsets:
            return load_address + message_offsets[msg_id]
        if required:
            raise ValueError(f"message {msg_id} is required by external tables")
        return 0

    def appearance_pointer(msg_id):
        return appearance_offsets[msg_id]

    def pad_to(address, description):
        offset = address - load_address
        if offset < len(image):
            raise ValueError(f"external data overlaps {description}")
        image.extend(bytes(offset - len(image)))

    monster_offset = monster_address - load_address
    if monster_offset < len(image):
        raise ValueError("message table overlaps monster table")
    image.extend(bytes(monster_offset - len(image)))
    image.extend(monsters)
    level_points_offset = level_points_address - load_address
    if level_points_offset < len(image):
        raise ValueError("monster table overlaps level points table")
    image.extend(bytes(level_points_offset - len(image)))
    image.extend(struct.pack("<" + "l" * len(LEVEL_POINTS), *LEVEL_POINTS))

    pad_to(id_wands_address, "wand identification table")
    for i, value in enumerate(WAND_VALUES):
        image.extend(struct.pack("<hHHH", value, 0,
                                 message_pointer(389 + i, False), 0))

    pad_to(id_rings_address, "ring identification table")
    for i, value in enumerate(RING_VALUES):
        image.extend(struct.pack("<hHHH", value, 0,
                                 message_pointer(399 + i, False), 0))

    pad_to(wand_materials_address, "wand material pointers")
    for msg_id in range(410, 440):
        image.extend(struct.pack("<H", appearance_pointer(msg_id)))

    pad_to(gems_address, "ring gem pointers")
    for msg_id in range(440, 454):
        image.extend(struct.pack("<H", appearance_pointer(msg_id)))

    return image


def make_mzt_header(filename, data_size, load_address, exec_address):
    encoded_name = filename.encode("ascii")
    if len(encoded_name) > 16:
        raise ValueError("MZT filename exceeds 16 characters")
    if data_size > 0xffff:
        raise ValueError("message data exceeds the MZT size limit")
    if not 0 <= load_address <= 0xffff or not 0 <= exec_address <= 0xffff:
        raise ValueError("MZT load/exec address is outside the 16-bit range")

    header = bytearray(MZT_HEADER_SIZE)
    header[0] = MZT_ATTRIBUTE_MACHINE_CODE
    header[1:18] = b" " * 17
    header[1:1 + len(encoded_name)] = encoded_name
    header[1 + len(encoded_name)] = 0x0d
    struct.pack_into("<HHH", header, 18, data_size,
                     load_address, exec_address)
    return header


def write_compressed_mzt(path, image, filename, load_address, exec_address, zx0,
                         max_compressed_size):
    with tempfile.TemporaryDirectory(dir=path.parent) as temp_dir:
        raw_path = Path(temp_dir) / "rogue_data.bin"
        compressed_path = Path(temp_dir) / "rogue_data.zx0"
        raw_path.write_bytes(image)
        subprocess.run((str(zx0.resolve()), "-f", str(raw_path),
                        str(compressed_path)), check=True)
        compressed = compressed_path.read_bytes()
    if len(compressed) > max_compressed_size:
        raise ValueError(
            f"compressed data is {len(compressed)} bytes "
            f"(maximum {max_compressed_size})")
    header = make_mzt_header(filename, len(compressed), load_address, exec_address)
    path.write_bytes(header + compressed)
    return len(image), len(compressed)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--display", type=Path, required=True)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--monster", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--zx0", type=Path, required=True)
    parser.add_argument("--ids", required=True,
                        help="comma-separated message IDs")
    parser.add_argument("--exclude-ids", default="",
                        help="comma-separated IDs omitted from this build")
    parser.add_argument("--mzt-name", default="MESG",
                        help="filename stored in the MZT header")
    parser.add_argument("--load-address", type=lambda value: int(value, 0),
                        default=0xc000)
    parser.add_argument("--exec-address", type=lambda value: int(value, 0),
                        default=0xc000)
    parser.add_argument("--max-compressed-size",
                        type=lambda value: int(value, 0), default=0x1000)
    parser.add_argument("--data-address", type=lambda value: int(value, 0),
                        default=0xd000)
    parser.add_argument("--monster-address", type=lambda value: int(value, 0),
                        default=0xe800)
    parser.add_argument("--level-points-address",
                        type=lambda value: int(value, 0), default=0xeda0)
    parser.add_argument("--id-wands-address",
                        type=lambda value: int(value, 0), default=0xee00)
    parser.add_argument("--id-rings-address",
                        type=lambda value: int(value, 0), default=0xee50)
    parser.add_argument("--wand-materials-address",
                        type=lambda value: int(value, 0), default=0xeea8)
    parser.add_argument("--gems-address",
                        type=lambda value: int(value, 0), default=0xeee4)
    parser.add_argument("--data-limit-address",
                        type=lambda value: int(value, 0), default=0xef00)
    args = parser.parse_args()
    selected = {int(value) for value in args.ids.split(",")}
    if args.exclude_ids:
        selected -= {int(value) for value in args.exclude_ids.split(",")}
    values = read_defines(args.display)
    source = parse_messages(args.input, selected)
    all_encoded = [(msg_id, encode_message(text, values, line_no))
                   for msg_id, text, line_no in source]
    appearance_messages = [(msg_id, data) for msg_id, data in all_encoded
                           if 410 <= msg_id <= 453]
    encoded = [(msg_id, data) for msg_id, data in all_encoded
               if not 410 <= msg_id <= 453]
    monsters = parse_monsters(args.monster, values)
    image = make_external_image(encoded, appearance_messages, monsters,
                                args.data_address, args.monster_address,
                                args.level_points_address,
                                args.id_wands_address, args.id_rings_address,
                                args.wand_materials_address,
                                args.gems_address)
    image_end = args.data_address + len(image)
    if image_end > args.data_limit_address:
        raise ValueError(
            f"external data ends at 0x{image_end:04x} "
            f"(limit 0x{args.data_limit_address:04x})")
    image_size, compressed_size = write_compressed_mzt(
        args.output, image, args.mzt_name, args.load_address,
        args.exec_address, args.zx0, args.max_compressed_size)
    for msg_id, data in all_encoded:
        print(f"message {msg_id}: {len(data)} bytes")
    print(f"MZT: {args.output} compressed={compressed_size} bytes "
          f"expanded={image_size} bytes load=0x{args.load_address:04x} "
          f"exec=0x{args.exec_address:04x}")


if __name__ == "__main__":
    main()
