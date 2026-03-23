#!/usr/bin/env python3

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent
GEN_DIR = ROOT / "gen"
HEADER = GEN_DIR / "roms.h"
SOURCE = GEN_DIR / "roms.c"


def sanitize(name: str) -> str:
    return re.sub(r"\W|^(?=\d)", "_", name)


def chunk_bytes(data: bytes, width: int = 12):
    for i in range(0, len(data), width):
        yield data[i:i + width]


def write_header(roms):
    with HEADER.open("w", encoding="utf-8") as fp:
        fp.write("#ifndef MGBA_AM_ROMS_H\n")
        fp.write("#define MGBA_AM_ROMS_H\n\n")
        fp.write("struct embedded_rom {\n")
        fp.write("  const char *name;\n")
        fp.write("  const unsigned char *data;\n")
        fp.write("  unsigned int size;\n")
        fp.write("};\n\n")
        fp.write("extern const struct embedded_rom roms[];\n")
        fp.write("extern const int nroms;\n\n")
        fp.write("#endif\n")


def write_source(roms):
    with SOURCE.open("w", encoding="utf-8") as fp:
        fp.write('#include "roms.h"\n\n')
        for rom in roms:
            fp.write(f"static const unsigned char rom_{rom['symbol']}[] = {{\n")
            for chunk in chunk_bytes(rom["data"]):
                body = ", ".join(f"0x{b:02x}" for b in chunk)
                fp.write(f"  {body},\n")
            fp.write("};\n\n")

        fp.write("const struct embedded_rom roms[] = {\n")
        for rom in roms:
            fp.write(
                f'  {{ .name = "{rom["name"]}", .data = rom_{rom["symbol"]}, .size = sizeof(rom_{rom["symbol"]}) }},\n'
            )
        fp.write("};\n\n")
        fp.write(f"const int nroms = {len(roms)};\n")


def main():
    GEN_DIR.mkdir(exist_ok=True)

    roms = []
    for path in sorted(ROOT.glob("*.gba")):
        roms.append(
            {
                "name": path.stem,
                "symbol": sanitize(path.stem),
                "data": path.read_bytes(),
            }
        )

    write_header(roms)
    write_source(roms)


if __name__ == "__main__":
    main()
