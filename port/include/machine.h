/* machine.h: the PC the game expects (playbook 80, M3, "What the machine replaces").

   The machine is the port's DOS and BIOS: the interrupt vector table and the ROM in the arena, the
   services behind them, and their state (open files, the memory arena, the clock). It follows the
   oracle's models (tools/oracle/machine.py, dos.py, vfs.py, bios.py, input.py), which made the
   goldens: the same vectors and ROM bytes, the same answers, the same errors. tools/port/hybrid.py
   proves it so: through a golden, every service call the original makes is also made to the machine,
   on the same memory and registers, and the two answers are compared (the shadow test).

   A game routine issues a software interrupt as `machine_int(n, &regs)`: the vector in the arena
   decides, as the CPU's INT does: one of the machine's service stubs (F000:1000 + 4n) runs that
   service; the generic IRET at F000:0000 does nothing; any other address is a handler the game
   installed. A service reports through the registers and through the flags the INT would return
   (CF for an error), as the oracle's services edit the flags the INT pushed. */
#ifndef TC_MACHINE_H
#define TC_MACHINE_H

#include "tcport.h"

typedef struct {
    u16 ax, bx, cx, dx, si, di, bp, ds, es, flags;
} tc_regs;

#define TC_FLAG_CF 0x0001u
#define TC_FLAG_ZF 0x0040u

/* the layout of tools/oracle/machine.py */
#define MACHINE_ENV_SEG 0x0F00u
#define MACHINE_PSP_SEG 0x0FF0u
#define MACHINE_LOAD_SEG 0x1000u
#define MACHINE_MEM_TOP 0xA000u
#define MACHINE_ROM_SEG 0xF000u
#define MACHINE_STUB_OFF0 0x1000u      /* F000:1000 + 4n: the stub of service n (IRET, 3 NOPs) */
#define MACHINE_ROM_INT8 0x2000u       /* F000:2000: the default timer handler (real code) */
#define MACHINE_ROM_EOI 0x2040u        /* F000:2040: IRQ 1 to 7's default, EOI and IRET */

/* The clock the services read (the date, the time of day). The oracle's is its instruction count
   plus what port polls charge; the port's is the clock of the C units (playbook 80, "The clock").
   The host sets `now` before a service that reads it. */
typedef struct {
    u32 now;                 /* instructions, as Machine.now() */
    u32 insn_per_sec;        /* 1000000, as the oracle's default */
    int pinned;              /* a session's `clock HH:MM:SS.hh`: the first read returns clock_pin */
    u32 clock_pin;
    int tod_offset_set;
    s32 tod_offset;
} machine_clock;

extern machine_clock machine_time;
extern int machine_exit_code;      /* -1 while the program runs; INT 21h 4Ch sets it */

/* Builds the vector table, the ROM, the BIOS data area, the PSP and the environment in the arena,
   and resets the services' state, as the oracle's Machine does before it loads the program (the
   load module itself is tc_load_program's). cmdline is the command tail (" 3 4 0 homie 3"); mouse:
   the INT 33h driver is there (0: the oracle's "no driver", which answers a reset with AX=0). */
void machine_boot(const char *exe_name, const u8 *cmdline, u16 cmdlen, int mouse);

/* A software interrupt from the game, through the vector table. */
void machine_int(u8 n, tc_regs *r);

/* What the stub of service n does; 0, or -1 when the machine has no service n. */
int machine_service(u8 n, tc_regs *r);

/* time of day in hundredths of a second since midnight (Machine.time_of_day) */
u32 machine_time_of_day(void);

/* the services, by file (machine.c dispatches) */
void dos_reset(const u8 *cmdline, u16 cmdlen, const char *exe_name);
void dos_int20(tc_regs *r);
void dos_int21(tc_regs *r);
void dos_int29(tc_regs *r);
void dos_int2f(tc_regs *r);
void bios_reset(void);
void bios_int10(tc_regs *r);
void video_reset(void);

/* the video state the oracle keeps in its EGA model and the host shows: MCGA's (video.c) */
typedef struct {
    u8 mode;
    u8 page;
    u8 dac[768];
} machine_video;
extern machine_video video;
void bios_int11(tc_regs *r);
void bios_int12(tc_regs *r);
void bios_int13(tc_regs *r);
void bios_int1a(tc_regs *r);
void mouse_none_int33(tc_regs *r);

/* the flags an INT returns: CF clear (ok) or set with AX the DOS error code (err) */
static inline void machine_ok(tc_regs *r) { r->flags &= (u16)~TC_FLAG_CF; }
static inline void machine_err(tc_regs *r, u16 code) { r->ax = code; r->flags |= TC_FLAG_CF; }

/* A service the oracle has and the machine does not yet: stops with the oracle's words. */
void machine_unimplemented(const char *what, const tc_regs *r) __attribute__((noreturn));

#endif
