#!/usr/bin/env python3
"""Convert selected UTF-8 hiragana messages to MZ-1500 display codes."""

import argparse
import re
import struct
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
    "*": "DC_STAR",
}
ASCII_MACROS.update({chr(code): "DC_" + chr(code).upper()
                     for code in range(ord("a"), ord("z") + 1)})

MESSAGE_FORMAT_STRING = 0xFE
MESSAGE_CSET_1 = 0xCE
MESSAGE_CSET_0 = 0xCF
MZT_HEADER_SIZE = 128
MZT_ATTRIBUTE_MACHINE_CODE = 0x01


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
        if text.startswith("%s", index):
            encoded.append(MESSAGE_FORMAT_STRING)
            index += 2
            continue
        char = text[index]
        if "ァ" <= char <= "ヶ":
            if not katakana:
                encoded.append(MESSAGE_CSET_0)
                katakana = True
            char = chr(ord(char) - 0x60)
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
    for msg_id, encoded in messages:
        entries.append((msg_id, len(data), len(encoded)))
        data.extend(encoded)
        data.append(0)
    if len(entries) > 255:
        raise ValueError("message count exceeds 255")
    data_offset = 8 + len(entries) * 5
    image = bytearray(b"MZMG")
    image.extend((len(entries), 1))
    image.extend(struct.pack("<H", data_offset))
    for msg_id, offset, length in entries:
        image.extend(struct.pack("<HHB", msg_id, offset, length))
    image.extend(data)
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


def write_mzt(path, messages, filename, load_address, exec_address):
    image = make_message_image(messages)
    header = make_mzt_header(filename, len(image), load_address, exec_address)
    path.write_bytes(header + image)
    return len(image)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--display", type=Path, required=True)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--ids", required=True,
                        help="comma-separated message IDs")
    parser.add_argument("--mzt-name", default="MESG",
                        help="filename stored in the MZT header")
    parser.add_argument("--load-address", type=lambda value: int(value, 0),
                        default=0xd000)
    parser.add_argument("--exec-address", type=lambda value: int(value, 0),
                        default=0xd000)
    args = parser.parse_args()
    selected = {int(value) for value in args.ids.split(",")}
    values = read_defines(args.display)
    source = parse_messages(args.input, selected)
    encoded = [(msg_id, encode_message(text, values, line_no))
               for msg_id, text, line_no in source]
    image_size = write_mzt(args.output, encoded, args.mzt_name,
                           args.load_address, args.exec_address)
    for msg_id, data in encoded:
        print(f"message {msg_id}: {len(data)} bytes")
    print(f"MZT: {args.output} data={image_size} bytes "
          f"load=0x{args.load_address:04x} exec=0x{args.exec_address:04x}")


if __name__ == "__main__":
    main()
