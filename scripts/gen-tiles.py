#!/usr/bin/python2

import os
import string
import math

def convert_to_bin(binstring):
    binstring = binstring.replace(" ", "")
    return "0b%s" % binstring

def gen_charmap():
    charmap = {}

    for i in range(0, 128, 1):
        charmap[i] = [
            "00",
            "00"
        ]

    half = ["01", "10"]
    full = ["11", "11"]

    num_tick_types = math.factorial(5)
    ticks = {i : [0 if j % i != 0 else 1 for j in range(num_tick_types)] for i in range(1, 6)}
    ticks[0] = [0 for _ in range(num_tick_types)]

    charmap[ord('0')] = half

    charmap[ord('1')] = full

    for i in range(ord('A'), ord('Z') + 1, 1):
        offset = i - ord('A')
        charmap[ord('a') + offset] = charmap[i]

    return charmap, ticks


def gen_source(charmap, ticks):
    source = []

    source.append("#ifndef CHARMAP_H")
    source.append("#define CHARMAP_H")
    source.append("")
    source.append("#define CHAR_WIDTH 2")
    source.append("#define TICK_INTERVAL %d" % len(ticks.values()[0]))
    source.append("")
    source.append("static const char charmap[128][CHAR_WIDTH] = {")
    for i in range(0, 128, 1):
        c = chr(i)
        symbol = "control"
        if c.isspace():
            symbol = "whitespace"
        elif c in string.printable:
            symbol = c

        source.append("")
        source.append("    /* [%d] %s */" % (i, symbol))
        source.append("    { %s," % convert_to_bin(charmap[i][0]))
        source.append("      %s}," % convert_to_bin(charmap[i][1]))

    source.append("")
    source.append("}; /* static const charmap[128][CHAR_WIDTH] */")
    source.append("")
    source.append("static const int ticks[%d][%d] = {" % (len(ticks.keys()), len(ticks.values()[0])))
    for key in sorted(ticks.keys()):
        source.append("{ %s }," % ",".join(str(v) for v in ticks[key]))

    source.append("};")
    source.append("#endif /* CHARMAP_H */")
    source.append("")
    return source


def main():
    source = gen_source(*gen_charmap())

    #print("\n".join(source))

    output_dir_path = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src"))
    output_file_path = os.path.join(output_dir_path, "tiles.h")

    output_file = open(output_file_path, "wb")
    output_file.write("\n".join(source))
    output_file.close()

    print("Header \"%s\" has been generated." % output_file_path)


if __name__ == "__main__":
    main()

