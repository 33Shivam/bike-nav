#!/usr/bin/env python3
"""Converts a TTF to an Adafruit-GFX-compatible C header (the exact format
LovyanGFX's GFXFF font system expects - same struct layout as the bundled
FreeSans*.h fonts: GFXglyph{bitmapOffset,width,height,xAdvance,xOffset,yOffset}
and GFXfont{bitmap,glyph,first,last,yAdvance}).

Usage: ttf_to_gfxfont.py <ttf_path> <pixel_size> <c_identifier> <output.h>
"""
import sys
from PIL import ImageFont

FIRST_CHAR = 0x20
LAST_CHAR = 0x7E


def main():
    ttf_path, pixel_size, ident, out_path = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]
    font = ImageFont.truetype(ttf_path, pixel_size)

    bitmap_bytes = bytearray()
    glyphs = []  # (bitmapOffset, width, height, xAdvance, xOffset, yOffset, char)

    ascent, descent = font.getmetrics()
    y_advance = ascent + descent

    for code in range(FIRST_CHAR, LAST_CHAR + 1):
        ch = chr(code)
        # anchor="ls" => bbox is relative to a cursor sitting ON the baseline,
        # which is exactly what GFXglyph's xOffset/yOffset expect (distance
        # from cursor to the glyph's upper-left corner).
        bbox = font.getbbox(ch, anchor="ls")
        x_advance = round(font.getlength(ch))

        if bbox is None or bbox[2] <= bbox[0] or bbox[3] <= bbox[1]:
            # Blank glyph (e.g. space) - zero-size bitmap, advance only.
            glyphs.append((len(bitmap_bytes), 0, 0, x_advance, 0, 0, ch))
            continue

        left, top, right, bottom = bbox
        width = right - left
        height = bottom - top

        mask = font.getmask(ch, mode="1", anchor="ls")
        # mask.size should match (width, height) exactly given the same bbox.
        mw, mh = mask.size

        bit_buffer = 0
        bit_count = 0
        offset_before = len(bitmap_bytes)
        for y in range(height):
            for x in range(width):
                px = mask.getpixel((x, y)) if x < mw and y < mh else 0
                bit_buffer = (bit_buffer << 1) | (1 if px else 0)
                bit_count += 1
                if bit_count == 8:
                    bitmap_bytes.append(bit_buffer)
                    bit_buffer = 0
                    bit_count = 0
        if bit_count > 0:
            bit_buffer <<= (8 - bit_count)
            bitmap_bytes.append(bit_buffer)

        glyphs.append((offset_before, width, height, x_advance, left, top, ch))

    with open(out_path, "w") as f:
        f.write(f"// Generated from {ttf_path} at {pixel_size}px - Adafruit GFXfont format.\n")
        f.write("#pragma once\n#include <cstdint>\n\n")
        f.write(f"const uint8_t {ident}Bitmaps[] = {{\n")
        for i in range(0, len(bitmap_bytes), 16):
            chunk = bitmap_bytes[i:i + 16]
            f.write("  " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("};\n\n")

        f.write(f"const GFXglyph {ident}Glyphs[] = {{\n")
        for (off, w, h, adv, xo, yo, ch) in glyphs:
            safe = ch if ch not in ('\\', "'") else '?'
            f.write(f"  {{ {off}, {w}, {h}, {adv}, {xo}, {yo} }}, // 0x{ord(ch):02X} '{safe}'\n")
        f.write("};\n\n")

        f.write(f"const GFXfont {ident} = {{\n")
        f.write(f"  (uint8_t*){ident}Bitmaps, (GFXglyph*){ident}Glyphs,\n")
        f.write(f"  0x{FIRST_CHAR:02X}, 0x{LAST_CHAR:02X}, {y_advance}\n")
        f.write("};\n")
        f.write(f"// {len(bitmap_bytes)} bitmap bytes, {len(glyphs)} glyphs\n")

    print(f"Wrote {out_path}: {len(bitmap_bytes)} bitmap bytes, {len(glyphs)} glyphs, yAdvance={y_advance}")


if __name__ == "__main__":
    main()
