/* driver.h: code the port calls with its arguments as the original's stack holds them (playbook 80,
   M4, "The graphics driver").

   The graphics driver's routines are reached through far pointers the game keeps in its slot table
   (ds:802A, whitepaper 2.5), and the units of src/ declare those slots with differing C types (int or
   unsigned, five arguments or none). So the port does not call a driver routine with C arguments: the
   caller lays out the bytes of its arguments as Turbo C pushes them (tc_code_far, as for any far
   pointer with no C function behind it), and the routine reads them where the original reads them,
   [bp+6] onward. Its value comes back as DX:AX; a routine that answers in other registers or in the
   flags (DriverEntry's BX, LinkDriver's carry) sets them in *r.

   A routine registered here is found by tc_code_far before anything else, in the port and in the
   hybrid build alike. */
#ifndef TC_DRIVER_H
#define TC_DRIVER_H

#include "tcport.h"
#include "machine.h"

/* arg: the argument bytes, at least TC_ARG_MAX of them (the caller's, then zeros); r: the registers
   and flags the routine answers in besides DX:AX, cleared by the caller */
typedef u32 (*tc_argfn)(const u8 *arg, tc_regs *r);

#define TC_ARG_MAX 32u                  /* the most argument bytes a driver routine reads */

void tc_code_register_args(farptr addr, tc_argfn fn, const char *name);
tc_argfn tc_code_find_args(farptr addr);

/* Calls the routine registered at addr with n bytes of arguments (more than TC_ARG_MAX stops); 0 if
   none is registered there. */
int tc_code_call_args(farptr addr, const u8 *args, u16 n, tc_regs *r, u32 *ret);

/* One routine of a driver: its offset in the driver's segment, its name (as the TASM source names
   it), its C, the argument bytes it reads (the per-function test masks them), the bytes of DX:AX it
   returns (0, 1, 2 or 4) and the other registers it answers in (compared by the test). */
#define TC_ANSWERS_BX 1u
#define TC_ANSWERS_CF 2u
typedef struct {
    u16 off;
    const char *name;
    tc_argfn fn;
    u8 arg_bytes;
    u8 ret_bytes;
    u8 regs;
} tc_driver_routine;

/* MCGA.OVL in C (port/drivers): its routines, and their registration at the segment the driver was
   loaded at (the machine's DOS 4B03h does it; the per-function test does it when the original links
   the driver) */
const tc_driver_routine *mcga_routine(unsigned i);     /* routine i, 0 past the last */
void mcga_register(u16 seg);

#endif
