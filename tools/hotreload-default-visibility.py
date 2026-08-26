#!/usr/bin/env python3
"""Copy a directory of prebuilt SDK libraries, clearing ELF symbol visibility.

The shipped .a archives (tier2, tier3, particles, ...) were compiled with
hidden visibility, so their globals (mdlcache, materials, g_pParticleSystemMgr)
never reach the module's dynamic symbol table. That is fine for a normal build
and fatal for the hot reload preset: a shim rebuilt from a translation unit
that references one of them cannot link against the module.

This script writes byte-identical copies of every library into the destination
directory, changing exactly one thing: for every GLOBAL or WEAK symbol whose
visibility is INTERNAL or HIDDEN, st_other is set to DEFAULT. No code bytes,
sizes or offsets change. Shared libraries (.so) are copied unchanged. Files
whose copy is already newer than the source are skipped.

Usage: hotreload-default-visibility.py <src lib dir> <dest dir>
"""
import os
import shutil
import struct
import sys

AR_MAGIC = b"!<arch>\n"
ELF_MAGIC = b"\x7fELF"


def patch_elf_object(buf, base):
    """Promote hidden/internal GLOBAL and WEAK symbols in one ELF object
    inside bytearray buf at offset base. Returns the number patched."""
    if buf[base + 4] != 2:  # ELFCLASS64
        return 0
    e_shoff = struct.unpack_from("<Q", buf, base + 0x28)[0]
    e_shentsize = struct.unpack_from("<H", buf, base + 0x3A)[0]
    e_shnum = struct.unpack_from("<H", buf, base + 0x3C)[0]
    patched = 0
    for i in range(e_shnum):
        sh = base + e_shoff + i * e_shentsize
        sh_type = struct.unpack_from("<I", buf, sh + 4)[0]
        if sh_type != 2:  # SHT_SYMTAB
            continue
        sh_offset = struct.unpack_from("<Q", buf, sh + 0x18)[0]
        sh_size = struct.unpack_from("<Q", buf, sh + 0x20)[0]
        sh_entsize = struct.unpack_from("<Q", buf, sh + 0x38)[0]
        if sh_entsize == 0:
            continue
        for s in range(sh_size // sh_entsize):
            sym = base + sh_offset + s * sh_entsize
            st_info = buf[sym + 4]
            st_other = buf[sym + 5]
            binding = st_info >> 4
            visibility = st_other & 3
            if binding in (1, 2) and visibility in (1, 2):  # GLOBAL/WEAK, INTERNAL/HIDDEN
                buf[sym + 5] = st_other & ~3
                patched += 1
    return patched


def patch_archive(src, dest):
    buf = bytearray(open(src, "rb").read())
    if buf[: len(AR_MAGIC)] != AR_MAGIC:
        raise SystemExit(f"{src}: not an ar archive")
    patched = 0
    off = len(AR_MAGIC)
    while off + 60 <= len(buf):
        size = int(bytes(buf[off + 48 : off + 58]).decode().strip() or "0")
        data = off + 60
        if buf[data : data + 4] == ELF_MAGIC:
            patched += patch_elf_object(buf, data)
        off = data + size + (size & 1)
    open(dest, "wb").write(buf)
    return patched


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    src_dir, dest_dir = sys.argv[1], sys.argv[2]
    os.makedirs(dest_dir, exist_ok=True)
    total = 0
    for name in sorted(os.listdir(src_dir)):
        src = os.path.join(src_dir, name)
        dest = os.path.join(dest_dir, name)
        if not os.path.isfile(src):
            continue
        if os.path.exists(dest) and os.path.getmtime(dest) >= os.path.getmtime(src):
            continue
        if name.endswith(".a"):
            n = patch_archive(src, dest)
            print(f"{name}: {n} symbols promoted to default visibility")
            total += n
        else:
            shutil.copy2(src, dest)
            print(f"{name}: copied")
    print(f"done: {dest_dir} ({total} symbols promoted)")


if __name__ == "__main__":
    main()
