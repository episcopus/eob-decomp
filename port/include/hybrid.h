/* hybrid.h: the per-function test's side of the port (playbook 80, M2; tools/port/hybrid.py).

   The hybrid build (make port-hybrid) is the translated units, the runtime and port/runtime/hybrid.c
   as a shared library, compiled with TC_HYBRID. The oracle maps its conventional memory onto tc_mem,
   so the C runs on the original's own memory; stops a golden session where the original calls a
   translated function; and calls that function's thunk with tc_ss:tc_sp at the original's SS:SP, its
   arguments where the caller pushed them. A routine the port has no C for (an assembly unit, a
   library function the runtime lacks) runs as the original's code in the oracle: unported.c calls
   tc_hybrid_far with the arguments laid out as Turbo C pushes them.

   Nothing here is in the port program (port/main.c never includes it). */
#ifndef TC_HYBRID_H
#define TC_HYBRID_H

#include "tcport.h"
#include "machine.h"

/* One translated function: where the original's callers enter it (load-relative: a resident
   function's address, an overlay function's stub entry; FFFF:FFFF when it has none), its overlay
   and offset there (-1 for a resident one), and how many bytes its value and its arguments take. */
typedef struct {
    u16 seg;
    u16 off;
    s16 overlay;
    u16 code_off;
    const char *name;
    const char *unit;
    u32 (*thunk)(void);
    u8 ret_bytes;
    u8 arg_bytes;
    u8 interrupt;              /* an interrupt handler of port/asm: entered with FLAGS, CS, IP pushed */
} tc_hybrid_entry;

extern const tc_hybrid_entry tc_hybrid_entries[];
extern const unsigned tc_hybrid_entry_count;

/* Runs the original routine at `entry` (a runtime far address) in the oracle on tc_mem, with the
   n bytes of arguments pushed below tc_ss:tc_sp; returns DX:AX. */
u32 tc_hybrid_far(farptr entry, const u8 *args, u16 n);

/* The registers and flags that code left when tc_hybrid_far returned (set by the bench), for a C
   caller of code that answers in registers a C prototype cannot state (E1C00.c's graphics driver). */
extern tc_regs tc_hybrid_far_regs;

#endif
