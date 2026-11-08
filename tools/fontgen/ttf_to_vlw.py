#!/usr/bin/env python3
"""Converts a TTF to a VLW font file (Processing/LovyanGFX runtime font
format) - unlike ttf_to_gfxfont.py's 1-bit Adafruit-GFX output, this keeps
each glyph's real 8-bit anti-aliased coverage mask, so LovyanGFX's VLWfont
renderer can alpha-blend glyph edges against an explicit background color
(not a destination readback - see AntiAlias.h for why that distinction
matters on this write-only-SPI panel).

VLW binary format (reverse-engineered from LovyanGFX's VLWfont::loadFont /
VLWfont::drawChar in lgfx_fonts.cpp - all integers big-endian):

  Header (24 bytes = 6x int32):
    glyphCount, version(unused,11), pointSize(unused), unused,
    ascent, descent

  Glyph table (glyphCount x 28 bytes = 7x int32), immediately after header:
    unicode, height, width, xAdvance, topExtent(dY), leftExtent(dX), reserved(0)

  Bitmap data, immediately after the glyph table, one block per glyph in
  table order, width*height bytes each, row-major, 0=background/255=foreground.

Usage: ttf_to_vlw.py <ttf_path> <pixel_size> <output.vlw>
"""
import struct
import sys
from PIL import ImageFont

FIRST_CHAR = 0x20
LAST_CHAR = 0x7E


def main():
    ttf_path, pixel_size, out_path = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    font = ImageFont.truetype(ttf_path, pixel_size)
    ascent, descent = font.getmetrics()

    glyph_headers = []  # (unicode, height, width, xAdvance, dY, dX)
    bitmaps = []  # bytes, one per glyph, same order

    for code in range(FIRST_CHAR, LAST_CHAR + 1):
        ch = chr(code)
        # anchor="ls": origin sits ON the baseline at the left edge, so bbox
        # top/left are directly the signed offsets VLW's topExtent/leftExtent
        # expect (top is negative for ink above the baseline).
        bbox = font.getbbox(ch, anchor="ls")
        x_advance = round(font.getlength(ch))

        if bbox is None or bbox[2] <= bbox[0] or bbox[3] <= bbox[1]:
            glyph_headers.append((code, 0, 0, x_advance, 0, 0))
            bitmaps.append(b"")
            continue

        left, top, right, bottom = bbox
        width = right - left
        height = bottom - top
        if width > 255 or height > 255:
            # VLWfont::loadFont truncates width to uint8_t - this project's
            # font sizes (<=36px) never get remotely close.
            raise ValueError(f"glyph '{ch}' at {pixel_size}px is {width}x{height} - too large for VLW's 8-bit width field")

        mask = font.getmask(ch, mode="L", anchor="ls")  # 8-bit AA coverage
        mw, mh = mask.size

        row_major = bytearray(width * height)
        for y in range(height):
            for x in range(width):
                row_major[y * width + x] = mask.getpixel((x, y)) if x < mw and y < mh else 0

        dY = -top   # distance from baseline up to the top of the bitmap
        dX = left   # distance from cursor to the left edge of the bitmap
        if not (-128 <= dX <= 127):
            raise ValueError(f"glyph '{ch}' leftExtent {dX} out of VLW's int8 range")

        glyph_headers.append((code, height, width, x_advance, dY, dX))
        bitmaps.append(bytes(row_major))

    with open(out_path, "wb") as f:
        f.write(struct.pack(">iiiiii", len(glyph_headers), 11, pixel_size, 0, ascent, descent))
        for (unicode, height, width, x_advance, dY, dX) in glyph_headers:
            f.write(struct.pack(">iiiiiii", unicode, height, width, x_advance, dY, dX, 0))
        for bm in bitmaps:
            f.write(bm)

    total_bitmap_bytes = sum(len(b) for b in bitmaps)
    print(f"Wrote {out_path}: {len(glyph_headers)} glyphs, {total_bitmap_bytes} bitmap bytes, "
          f"ascent={ascent} descent={descent}, file size={24 + 28*len(glyph_headers) + total_bitmap_bytes}")


if __name__ == "__main__":
    main()
