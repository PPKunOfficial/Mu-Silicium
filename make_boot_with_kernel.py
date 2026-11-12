#!/usr/bin/env python3
"""Build the combined boot_b image: Mu UEFI + DTB + mainline kernel.

Layout (all offsets in the boot_b partition):

    0x000000   Mu-ovaltine.img  (Android boot image v4, booted by ABL)
    0x800000   KERNEL_DESCRIPTOR (4096 bytes)
    0x801000   device tree (simple-framebuffer variant for console output)
    0x900000   Linux Image (EFI-stub PE/COFF)

The LinuxLoaderDxe driver in the UEFI image reads the descriptor, installs
the device tree as the EFI FDT configuration table, and starts the kernel
with the stored cmdline.

Usage: make_boot_with_kernel.py [uefi.img] [kernel Image] [dtb] [output]
"""
import os
import struct
import sys
from pathlib import Path

UEFI_IMG   = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("Mu-ovaltine.img")
KERNEL_IMG = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(
    "../references/backup/boot_b_kernel.img")
DTB        = Path(sys.argv[3]) if len(sys.argv) > 3 else Path(
    "Resources/DTBs/ovaltine-simplefb.dtb")
OUTPUT     = Path(sys.argv[4]) if len(sys.argv) > 4 else Path("Mu-ovaltine-kernel.img")

DESC_OFFSET = 0x800000
DTB_OFFSET  = 0x801000
DATA_OFFSET = 0x900000
DESC_MAGIC  = 0x4B584E4C  # "LNXK"
CMDLINE     = os.environ.get(
    "CMDLINE",
    "earlycon=efifb keep_bootcon console=tty0 console=ttyGS0,115200 ignore_loglevel "
    "clk_ignore_unused pd_ignore_unused nokaslr panic=0",
).encode()


def main():
    uefi = UEFI_IMG.read_bytes()
    kernel = KERNEL_IMG.read_bytes()
    dtb = DTB.read_bytes()

    if kernel[:2] != b"MZ":
        sys.exit(f"{KERNEL_IMG} does not start with MZ")
    if dtb[:4] != b"\xd0\x0d\xfe\xed":
        sys.exit(f"{DTB} is not a flattened device tree")

    # Sanity: the UEFI image must be a valid Android boot image with its
    # kernel field entirely below the descriptor offset.
    if uefi[:8] != b"ANDROID!":
        sys.exit(f"{UEFI_IMG} is not an Android boot image")
    kernel_size = struct.unpack_from("<I", uefi, 8)[0]
    if 0x1000 + kernel_size > DESC_OFFSET:
        sys.exit("UEFI image overlaps the descriptor area")

    if DTB_OFFSET + len(dtb) > DATA_OFFSET:
        sys.exit("DTB overlaps the kernel area")

    # descriptor: magic, kernel size/offset, dtb size/offset, cmdline
    desc = struct.pack(
        "<IIQIIQ", DESC_MAGIC, len(kernel), DATA_OFFSET, len(dtb), 0, DTB_OFFSET
    )
    desc += CMDLINE + b"\0"
    desc = desc.ljust(0x1000, b"\0")

    image = bytearray()
    image += uefi
    image += b"\0" * (DESC_OFFSET - len(image))
    image += desc
    image += dtb
    image += b"\0" * (DATA_OFFSET - len(image))
    image += kernel
    image += b"\0" * ((-len(image)) % 0x1000)

    OUTPUT.write_bytes(image)
    print(f"UEFI     : {len(uefi)} bytes")
    print(f"dtb      : {len(dtb)} bytes at 0x{DTB_OFFSET:x}")
    print(f"kernel   : {len(kernel)} bytes at 0x{DATA_OFFSET:x}")
    print(f"cmdline  : {CMDLINE.decode()}")
    print(f"output   : {OUTPUT} ({len(image)} bytes, {len(image)/1e6:.1f} MB)")


if __name__ == "__main__":
    main()
