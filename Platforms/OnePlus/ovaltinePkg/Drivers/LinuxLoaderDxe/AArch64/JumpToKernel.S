//
// Jump to a raw arm64 Linux Image with the MMU and caches disabled, as
// required by the arm64 boot protocol.
//
// VOID
// JumpToKernel (
//   IN UINT64  Dtb,     // x0: physical address of the device tree
//   IN UINT64  Entry    // x1: physical address of the Image start
//   )
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include <AsmMacroLib.h>

ASM_FUNC(JumpToKernel)
  // Save arguments across the transition
  mov   x2, x0
  mov   x3, x1

  // Mask all interrupts
  msr   daifset, #0xf
  dsb   sy

  // Disable MMU (M), D-cache (C) and I-cache (I)
  mrs   x4, sctlr_el1
  bic   x4, x4, #0x1      // SCTLR_EL1_M
  bic   x4, x4, #0x4      // SCTLR_EL1_C
  bic   x4, x4, #0x1000   // SCTLR_EL1_I
  msr   sctlr_el1, x4
  isb

  tlbi  vmalle1
  dsb   sy
  isb

  // Diagnostic: paint a green block at the top-left of the framebuffer.
  // If this is visible, the MMU-off transition succeeded and we are
  // executing from physical memory.
  movz  x5, #0xB800, lsl #16   // framebuffer base 0xB8000000
  mov   x6, #200               // rows
1:
  mov   x7, #200               // pixels per row
  mov   w8, #0x0000FF00        // green (XRGB8888)
2:
  str   w8, [x5], #4
  subs  x7, x7, #1
  b.ne  2b
  add   x5, x5, #3520          // (1080 - 200) * 4
  subs  x6, x6, #1
  b.ne  1b

  // x0 = DTB, branch to the Image entry
  mov   x0, x2
  br    x3
