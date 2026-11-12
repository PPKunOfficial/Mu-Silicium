# ovaltinePkg — OnePlus 10T / Ace Pro (SM8475)

Platform package for the OnePlus 10T (ovaltine, PGP110 / CPH2413/CPH2415,
SM8475 "cape"). It boots a Linux kernel through the EFI stub from the
`boot_b` partition, using the device tree supplied by `LinuxLoaderDxe`
instead of the stub DTB.

## Dependencies

* **Vendor DXE binaries**: `Binaries/ovaltine`. They live in the
  [`PPKunOfficial/Device-Binaries`](https://github.com/PPKunOfficial/Device-Binaries)
  fork, which the `Binaries` submodule points at.
* **Mu_Basecore nested submodules** must be initialized, otherwise
  `MdePkg.dec` fails with `Library/MipiSysTLib/mipisyst ... not found`:

  ```sh
  git -C Mu_Basecore submodule update --init
  ```

* **NASM NuGet dependency**: `Mu_Basecore/BaseTools/Bin/nasm_ext_dep.yaml`
  requires Mono on Linux. AArch64 builds do not use NASM, so switch its
  scope to disable the dependency:

  ```diff
  -  "scope": "edk2-build",
  +  "scope": "edk2-build-unused",
  ```
* On some hosts the EDK2 Python venv breaks when the shell exports
  `LD_LIBRARY_PATH` (e.g. when the build runs inside an AppImage):
  run the build with `env -u LD_LIBRARY_PATH`.

## Build

```sh
python3 -m venv .venv
.venv/bin/pip install -r pip-requirements.txt
PATH="$PWD/.venv/bin:$PATH" env -u LD_LIBRARY_PATH .venv/bin/python \
    build_uefi.py -d ovaltine
# -> Mu-ovaltine.img (Android boot image, ~3.0 MB)
```

The combined `boot_b` image (Mu + kernel + DTB) is built with
`make_boot_with_kernel.py`:

```sh
python3 -I make_boot_with_kernel.py Mu-ovaltine.img \
    ../linux-7.3/arch/arm64/boot/Image \
    ../linux-7.3/arch/arm64/boot/dts/qcom/sm8475-oneplus-ovaltine.dtb \
    Mu-ovaltine-kernel.img
fastboot flash boot_b Mu-ovaltine-kernel.img
fastboot --set-active=b
fastboot reboot
```

## OnePlus ABL requirement

OnePlus' ABL validates the ARM64 Image header embedded by the BootShim and
**requires `text_offset == 0` and `image_size == 0`**. Upstream
`BootShim/BootShim.S` writes `FD_BASE`/`FD_SIZE` there; this tree carries
the zero values (see the `REQUIRES_KERNEL_HEADER` block). With the upstream
values ABL silently bounces the image back to fastboot.

## Layout

* `Include/APRIORI.inc`, `Include/DXE.inc`, `Include/RAW.inc` — driver
  lists (vendor QcomPkg DXEs + Simplerfb + `LinuxLoaderDxe`).
* `Drivers/LinuxLoaderDxe` — reads the `LNXK` descriptor at `boot_b+0x800000`,
  installs the DTB as the EFI FDT configuration table and starts the
  EFI-stub kernel.
* `Library/MemoryMapLib`, `Library/ConfigurationMapLib` — device memory map
  (incl. the high DDR heap Mu needs for the kernel image).
* `FdtBlob/` — fallback DTB embedded in the firmware (the live DTB is the
  one passed in the descriptor).
