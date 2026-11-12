/** @file
  Minimal Linux loader for Project Silicium UEFI on the OnePlus Ace Pro
  (ovaltine / SM8475).

  The "boot_b" partition contains, in order:

    0x000000    Mu UEFI image (Android boot image v4, what ABL boots)
    0x800000    KERNEL_DESCRIPTOR (magic, kernel size/offset, cmdline)
    0x801000    the user's mainline ARM64 Image (valid PE/COFF via EFI stub)

  This driver finds the partition, reads the descriptor and the kernel,
  installs the device tree embedded in the Image as the EFI FDT
  configuration table, and starts the kernel with the stored cmdline.

  On any failure it paints a status band and a two-digit stage code on the
  framebuffer and halts, so the reason is visible even before ConOut is
  connected.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PartitionInfo.h>

#include <Guid/Fdt.h>

#define PE_DOS_MAGIC            0x5A4D  // "MZ"
#define FDT_MAGIC               0xD00DFEED
#define FDT_MIN_TOTAL_SIZE      0x1000
#define BOOT_CMDLINE_MAX        512

//
// Layout of the combined boot_b image (see make_boot_with_kernel.py).
//
#define KERNEL_DESC_OFFSET      0x800000ULL
#define KERNEL_DATA_OFFSET      0x801000ULL
#define KERNEL_DESC_MAGIC       0x4B584E4C  // "LNXK"

//
// SimpleFb framebuffer, used for boot-time diagnostics that survive an
// unavailable ConOut.
//
#define FB_BASE                 0xB8000000u
#define FB_WIDTH                1080u
#define FB_HEIGHT               2412u
#define FB_BAND_ROWS            12u
#define COLOR_RED               0x00FF0000u
#define COLOR_GREEN             0x0000FF00u
#define COLOR_BLUE              0x000000FFu
#define COLOR_YELLOW            0x00FFFF00u
#define COLOR_ORANGE            0x00FF8000u
#define COLOR_MAGENTA           0x00FF00FFu
#define COLOR_CYAN              0x0000FFFFu
#define COLOR_WHITE             0x00FFFFFFu
#define COLOR_LIME              0x0080FF00u
#define COLOR_PINK              0x00FF0080u
#define COLOR_VIOLET            0x008000FFu
#define COLOR_TEAL              0x00008080u
#define COLOR_AZURE             0x000080FFu
#define COLOR_BROWN             0x00804000u

#pragma pack(1)
typedef struct {
  UINT32    Magic;
  UINT32    KernelSize;
  UINT64    KernelOffset;
  UINT32    DtbSize;
  UINT32    Reserved;
  UINT64    DtbOffset;
  CHAR8     CmdLine[BOOT_CMDLINE_MAX];
} KERNEL_DESCRIPTOR;
#pragma pack()

STATIC CONST CHAR16  mBootPartitionName[] = L"boot_b";

/**
  Locate the EFI_BLOCK_IO_PROTOCOL of the GPT partition named "boot_b".

  @param[out] BlockIo  Block I/O protocol of the boot partition.

  @retval EFI_SUCCESS    Partition found.
  @retval EFI_NOT_FOUND  No matching partition.
**/
STATIC
EFI_STATUS
FindBootPartition (
  OUT EFI_BLOCK_IO_PROTOCOL  **BlockIo
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   *Handles;
  UINTN                        HandleCount;
  UINTN                        Index;
  EFI_PARTITION_INFO_PROTOCOL  *PartitionInfo;
  UINTN                        NameLen;

  *BlockIo = NULL;
  NameLen  = StrLen (mBootPartitionName);

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPartitionInfoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiPartitionInfoProtocolGuid,
                    (VOID **)&PartitionInfo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (PartitionInfo->Type != PARTITION_TYPE_GPT) {
      continue;
    }

    if ((StrnCmp (PartitionInfo->Info.Gpt.PartitionName, mBootPartitionName, NameLen) != 0) ||
        (PartitionInfo->Info.Gpt.PartitionName[NameLen] != L'\0'))
    {
      continue;
    }

    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **)BlockIo
                    );
    if (!EFI_ERROR (Status)) {
      gBS->FreePool (Handles);
      return EFI_SUCCESS;
    }
  }

  gBS->FreePool (Handles);
  return EFI_NOT_FOUND;
}

/**
  Read a byte range from a block device at a block-aligned offset.

  The destination buffer must be large enough to hold the range rounded up
  to the device block size.
**/
STATIC
EFI_STATUS
ReadBlockRange (
  IN EFI_BLOCK_IO_PROTOCOL  *Blk,
  IN UINT64                 Offset,
  IN OUT VOID               *Buffer,
  IN UINTN                  Size
  )
{
  UINT32  BlockSize;

  BlockSize = Blk->Media->BlockSize;
  if ((BlockSize == 0) || ((Offset % BlockSize) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  return Blk->ReadBlocks (
                Blk,
                Blk->Media->MediaId,
                Offset / BlockSize,
                ALIGN_VALUE (Size, BlockSize),
                Buffer
                );
}

/**
  Scan a kernel image for the first plausible flattened device tree.

  The kernel was built with CONFIG_EMBED_MAINLINE_DTB, so the working DTB
  is embedded in the Image.  The first FDT header whose total size is sane
  and which fits inside the image is the dtb we want.
**/
STATIC
VOID *
FindFdt (
  IN VOID   *Image,
  IN UINTN  Size
  )
{
  UINT8   *Ptr;
  UINTN   Offset;
  UINT32  TotalSize;

  Ptr = (UINT8 *)Image;
  for (Offset = 0; Offset + 8 <= Size; Offset += 4) {
    if ((Ptr[Offset] == 0xD0) && (Ptr[Offset + 1] == 0x0D) &&
        (Ptr[Offset + 2] == 0xFE) && (Ptr[Offset + 3] == 0xED))
    {
      CopyMem (&TotalSize, Ptr + Offset + 4, sizeof (TotalSize));
      TotalSize = SwapBytes32 (TotalSize);
      if ((TotalSize >= FDT_MIN_TOTAL_SIZE) && (TotalSize <= Size - Offset)) {
        return Ptr + Offset;
      }
    }
  }

  return NULL;
}

/**
  Convert a NUL-terminated ASCII string to an allocated CHAR16 string.
**/
STATIC
CHAR16 *
AsciiToUcs2 (
  IN CONST CHAR8  *Ascii,
  IN UINTN        MaxLen
  )
{
  UINTN   Len;
  UINTN   Index;
  CHAR16  *Out;

  Len = 0;
  while ((Len < MaxLen) && (Ascii[Len] != '\0')) {
    Len++;
  }

  Out = AllocateZeroPool ((Len + 1) * sizeof (CHAR16));
  if (Out == NULL) {
    return NULL;
  }

  for (Index = 0; Index < Len; Index++) {
    Out[Index] = (CHAR16)(UINT8)Ascii[Index];
  }

  return Out;
}

//
// Seven-segment stage display (kept terminal, no dependency on ConOut).
//
STATIC
CONST UINT8  mSegPattern[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

STATIC
VOID
FillRect (
  IN UINTN   X0,
  IN UINTN   Y0,
  IN UINTN   W,
  IN UINTN   H,
  IN UINT32  Color
  )
{
  volatile UINT32  *FrameBuffer;
  UINTN            X;
  UINTN            Y;

  FrameBuffer = (volatile UINT32 *)(UINTN)FB_BASE;
  for (Y = Y0; Y < Y0 + H; Y++) {
    for (X = X0; X < X0 + W; X++) {
      FrameBuffer[Y * FB_WIDTH + X] = Color;
    }
  }
}

STATIC
VOID
DrawDigit (
  IN UINTN   X,
  IN UINTN   Y,
  IN UINTN   Digit,
  IN UINT32  Color
  )
{
  UINT8  S;
  UINTN  T;
  UINTN  W;
  UINTN  H;

  S = mSegPattern[Digit % 10];
  T = 14;
  W = 56;
  H = 110;

  if (S & 0x01) {
    FillRect (X, Y, W, T, Color);
  }
  if (S & 0x02) {
    FillRect (X + W - T, Y, T, H / 2 + T / 2, Color);
  }
  if (S & 0x04) {
    FillRect (X + W - T, Y + H / 2 - T / 2, T, H / 2 + T / 2, Color);
  }
  if (S & 0x08) {
    FillRect (X, Y + H - T, W, T, Color);
  }
  if (S & 0x10) {
    FillRect (X, Y + H / 2 - T / 2, T, H / 2 + T / 2, Color);
  }
  if (S & 0x20) {
    FillRect (X, Y, T, H / 2 + T / 2, Color);
  }
  if (S & 0x40) {
    FillRect (X, Y + H / 2 - T / 2, W, T, Color);
  }
}

STATIC
VOID
ShowNumber (
  IN UINTN  Stage
  )
{
  UINTN  Y;

  Y = (UINTN)FB_HEIGHT - 160;
  FillRect (0, Y, 240, 160, 0xFF000000);
  DrawDigit (20, Y + 20, (Stage / 10) % 10, 0xFFFFFFFF);
  DrawDigit (120, Y + 20, Stage % 10, 0xFFFFFFFF);
}

STATIC
VOID
ShowNumber3 (
  IN UINTN  Value
  )
{
  UINTN  Y;

  Y = (UINTN)FB_HEIGHT - 160;
  FillRect (0, Y, 360, 160, 0xFF000000);
  DrawDigit (20, Y + 20, (Value / 100) % 10, 0xFFFFFFFF);
  DrawDigit (120, Y + 20, (Value / 10) % 10, 0xFFFFFFFF);
  DrawDigit (220, Y + 20, Value % 10, 0xFFFFFFFF);
}

STATIC
VOID
PaintBottomBand (
  IN UINT32  Color
  )
{
  volatile UINT32  *Fb;
  UINTN            Index;
  UINTN            Start;

  Fb    = (volatile UINT32 *)(UINTN)FB_BASE;
  Start = (UINTN)(FB_HEIGHT - FB_BAND_ROWS) * FB_WIDTH;
  for (Index = 0; Index < (UINTN)FB_WIDTH * FB_BAND_ROWS; Index++) {
    Fb[Start + Index] = Color;
  }
}

/**
  Draw the stage code, paint the status band, then park the CPU forever so
  the screen survives for inspection instead of being cleared by BDS.
**/
STATIC
VOID
HaltWithStage (
  IN UINT32  Color,
  IN UINTN   Stage
  )
{
  PaintBottomBand (Color);
  ShowNumber (Stage);
  for ( ; ; ) {
    gBS->Stall (1000 * 1000);
  }
}

STATIC
VOID
HaltWithStage3 (
  IN UINT32  Color,
  IN UINTN   Stage
  )
{
  PaintBottomBand (Color);
  ShowNumber3 (Stage);
  for ( ; ; ) {
    gBS->Stall (1000 * 1000);
  }
}

/**
  Driver entry point: load and start the Linux kernel from boot_b.
**/
EFI_STATUS
EFIAPI
LinuxLoaderEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_BLOCK_IO_PROTOCOL      *BlockIo;
  KERNEL_DESCRIPTOR          *Desc;
  UINTN                      DescBufferSize;
  EFI_PHYSICAL_ADDRESS       DescBuffer;
  UINT32                     KernelSize;
  UINT64                     KernelOffset;
  UINT32                     DtbSize;
  UINT64                     DtbOffset;
  BOOLEAN                    DtbInstalled;
  UINTN                      KernelBufferSize;
  EFI_PHYSICAL_ADDRESS       KernelBuffer;
  VOID                       *Fdt;
  UINT32                     FdtSize;
  VOID                       *FdtCopy;
  VOID                       *DtbForKernel;
  CHAR8                      CmdLineAscii[BOOT_CMDLINE_MAX + 1];
  CHAR16                     *CmdLine;
  EFI_HANDLE                 KernelHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  //
  // Diagnostic build: yellow band = driver entered.
  //
  PaintBottomBand (COLOR_YELLOW);

  Status = FindBootPartition (&BlockIo);
  if (EFI_ERROR (Status)) {
    HaltWithStage (COLOR_RED, 10);
  }

  //
  // Read the kernel descriptor.
  //
  DescBufferSize = ALIGN_VALUE (0x1000, BlockIo->Media->BlockSize);
  DescBuffer     = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (DescBufferSize));
  if (DescBuffer == 0) {
    HaltWithStage (COLOR_ORANGE, 20);
  }

  Status = ReadBlockRange (BlockIo, KERNEL_DESC_OFFSET, (VOID *)(UINTN)DescBuffer, DescBufferSize);
  if (EFI_ERROR (Status)) {
    HaltWithStage (COLOR_ORANGE, 21);
  }

  Desc = (KERNEL_DESCRIPTOR *)(UINTN)DescBuffer;
  if (Desc->Magic != KERNEL_DESC_MAGIC) {
    HaltWithStage (COLOR_MAGENTA, 22);
  }

  if ((Desc->KernelSize < 0x10000) || (Desc->KernelSize > 0x8000000)) {
    HaltWithStage (COLOR_LIME, 24);
  }

  KernelSize   = Desc->KernelSize;
  KernelOffset = Desc->KernelOffset;
  DtbSize      = Desc->DtbSize;
  DtbOffset    = Desc->DtbOffset;
  if (KernelOffset == 0) {
    KernelOffset = KERNEL_DATA_OFFSET;
  }

  CopyMem (CmdLineAscii, Desc->CmdLine, BOOT_CMDLINE_MAX);
  CmdLineAscii[BOOT_CMDLINE_MAX] = '\0';
  CmdLine                        = AsciiToUcs2 (CmdLineAscii, BOOT_CMDLINE_MAX);

  gBS->FreePages (DescBuffer, EFI_SIZE_TO_PAGES (DescBufferSize));

  DtbForKernel = NULL;

  //
  // Read the kernel into a 2 MiB aligned buffer (arm64 Image alignment).
  // Claim the big contiguous block first, before the small DTB allocation,
  // so the page allocator cannot split it.  AllocateAnyPages lets the
  // allocation land in the platform's high DDR heap.
  //
  KernelBufferSize = ALIGN_VALUE (KernelSize, BlockIo->Media->BlockSize);
  {
    UINTN                RawSize;
    EFI_PHYSICAL_ADDRESS RawBuffer;

    RawSize   = KernelBufferSize + 0x200000;
    RawBuffer = 0;
    Status    = gBS->AllocatePages (
                      AllocateAnyPages,
                      EfiLoaderCode,
                      EFI_SIZE_TO_PAGES (RawSize),
                      &RawBuffer
                      );
    if (EFI_ERROR (Status) || (RawBuffer == 0)) {
      HaltWithStage (COLOR_PINK, 25);
    }

    KernelBuffer = (RawBuffer + 0x1FFFFF) & ~(EFI_PHYSICAL_ADDRESS)0x1FFFFF;
  }

  //
  // Install the device tree stored in the boot image.  It carries the
  // simple-framebuffer node and the bootargs, so the kernel console is
  // visible on screen.
  //
  DtbInstalled = FALSE;
  if ((DtbSize >= 0x1000) && (DtbSize <= 0x200000) && (DtbOffset != 0)) {
    UINTN                DtbBufferSize;
    EFI_PHYSICAL_ADDRESS DtbBuffer;

    DtbBufferSize = ALIGN_VALUE (DtbSize, BlockIo->Media->BlockSize);
    DtbBuffer     = 0;
    Status        = gBS->AllocatePages (
                          AllocateAnyPages,
                          EfiLoaderData,
                          EFI_SIZE_TO_PAGES (DtbBufferSize),
                          &DtbBuffer
                          );
    if (!EFI_ERROR (Status) && (DtbBuffer != 0)) {
      Status = ReadBlockRange (BlockIo, DtbOffset, (VOID *)(UINTN)DtbBuffer, DtbBufferSize);
      if (!EFI_ERROR (Status)) {
        DtbForKernel = (VOID *)(UINTN)DtbBuffer;
        DtbInstalled = TRUE;
        Status = gBS->InstallConfigurationTable (&gFdtTableGuid, (VOID *)(UINTN)DtbBuffer);
        Print (L"LinuxLoader: DTB %u bytes loaded\n", DtbSize);
      }

      if (!DtbInstalled) {
        gBS->FreePages (DtbBuffer, EFI_SIZE_TO_PAGES (DtbBufferSize));
      }
    }
  }

  Status = ReadBlockRange (BlockIo, KernelOffset, (VOID *)(UINTN)KernelBuffer, KernelBufferSize);
  if (EFI_ERROR (Status)) {
    HaltWithStage (COLOR_VIOLET, 26);
  }

  if (*(UINT16 *)(UINTN)KernelBuffer != PE_DOS_MAGIC) {
    HaltWithStage (COLOR_TEAL, 27);
  }

  //
  // Fallback: use the DTB embedded in the kernel (CONFIG_EMBED_MAINLINE_DTB)
  // if the boot image did not provide one.
  //
  if (!DtbInstalled) {
    Fdt = FindFdt ((VOID *)(UINTN)KernelBuffer, KernelSize);
    if (Fdt != NULL) {
      CopyMem (&FdtSize, (UINT8 *)Fdt + 4, sizeof (FdtSize));
      FdtSize = SwapBytes32 (FdtSize);
      FdtCopy = AllocatePool (FdtSize);
      if (FdtCopy != NULL) {
        CopyMem (FdtCopy, Fdt, FdtSize);
        DtbForKernel = FdtCopy;
        Status = gBS->InstallConfigurationTable (&gFdtTableGuid, FdtCopy);
        Print (L"LinuxLoader: embedded FDT %u bytes loaded\n", FdtSize);
      }
    }
  }

  if (DtbForKernel == NULL) {
    HaltWithStage (COLOR_WHITE, 23);
  }

  //
  // Load the kernel through the EFI stub and start it.
  //
  Status = gBS->LoadImage (
                  FALSE,
                  ImageHandle,
                  NULL,
                  (VOID *)(UINTN)KernelBuffer,
                  KernelSize,
                  &KernelHandle
                  );
  if (EFI_ERROR (Status)) {
    HaltWithStage3 (COLOR_AZURE, 400 + (Status & 0xFF));
  }

  Status = gBS->HandleProtocol (
                  KernelHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (!EFI_ERROR (Status) && (CmdLine != NULL)) {
    LoadedImage->LoadOptions     = CmdLine;
    LoadedImage->LoadOptionsSize = (UINT32)((StrLen (CmdLine) + 1) * sizeof (CHAR16));
  }

  PaintBottomBand (COLOR_BLUE);
  ShowNumber (30);
  gBS->SetWatchdogTimer (0, 0, 0, NULL);
  Print (L"LinuxLoader: starting kernel via EFI stub...\n");

  Status = gBS->StartImage (KernelHandle, NULL, NULL);
  Print (L"LinuxLoader: StartImage returned (%r)\n", Status);
  HaltWithStage (COLOR_BROWN, 29);

  return EFI_SUCCESS;
}
