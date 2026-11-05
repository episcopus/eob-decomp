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
   installed. A port is `machine_in(port, size)` and `machine_out(port, size, value)`. A service reports through the registers and through the flags the INT would return
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
    u32 insns;               /* instructions run */
    u32 extra;               /* what polls charged (3DAh, the AdLib's status): Machine.extra */
    u32 insn_per_sec;        /* 1000000, as the oracle's default */
    int pinned;              /* a session's `clock HH:MM:SS.hh`: the first read returns clock_pin */
    u32 clock_pin;
    int tod_offset_set;
    s32 tod_offset;
} machine_clock;

extern machine_clock machine_time;
static inline u32 machine_now(void) { return machine_time.insns + machine_time.extra; }   /* Machine.now() */
extern int machine_exit_code;      /* -1 while the program runs; INT 21h 4Ch sets it */

/* Builds the vector table, the ROM, the BIOS data area, the PSP and the environment in the arena,
   and resets the services' state, as the oracle's Machine does before it loads the program (the
   load module itself is tc_load_program's). cmdline is the command tail (" 3 4 0 homie 3"); mouse:
   the INT 33h driver is there (0: the oracle's "no driver", which answers a reset with AX=0). */
void machine_boot(const char *exe_name, const u8 *cmdline, u16 cmdlen, int mouse, int adlib);

/* A software interrupt from the game, through the vector table. */
void machine_int(u8 n, tc_regs *r);

/* What the CPU runs for an interrupt whose handler is at seg:off, the registers in r (an IRQ, an INT,
   a handler chaining to the vector it saved with `jmp far` or `pushf; call far`): one of the
   machine's service stubs runs its service; the generic IRET nothing; the ROM's default INT 8 and its
   IRQ default (EOI) run as their code does (machine.c); a routine of port/asm registered at that
   address runs (an interrupt handler: it keeps every register, so r is unchanged). Anything else stops
   (in the hybrid build, the original's code runs on the bench). */
void machine_call_handler(u16 seg, u16 off, tc_regs *r);

/* A hardware interrupt: the handler of vector n, as the CPU enters it (machine_call_handler). */
void machine_irq(u8 n);

/* An interrupt handler of port/asm (its routine marked `interrupt` beside its address) */
typedef void (*machine_isr)(void);

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
void mouse_reset(void);
void mouse_int33(tc_regs *r);

/* the ports (ports.c) */
u16 machine_in(u16 port, int size);
void machine_out(u16 port, int size, u16 value);
void ports_reset(int adlib);

/* A port access by the game's code (an assembly routine, the ROM's code): machine_in and machine_out;
   in the hybrid build the oracle's ports on the bench, whose devices are the session's. */
u16 machine_port_in(u16 port, int size);
void machine_port_out(u16 port, int size, u16 value);
u32 machine_irq0_period(void);
void machine_unimplemented_port(const char *dir, u16 port, int size) __attribute__((noreturn));

/* the hardware state the ports keep (machine.py, ega.py, adlib.py) */
typedef struct {
    u8 pic_mask, port61;
    u8 irq0_in_service, irq1_in_service;   /* the interrupts set them, an EOI clears them */
    s16 kbd_data;                          /* the scancode port 60h returns; -1 none */
    u32 pit_reload;
    u8 pit_latch_lo, pit_mode_access, pit_lo, pit2_lo;
    u16 pit2_reload;
    u32 next_irq0;                         /* the clock of the next timer interrupt */
    u16 dac_read, dac_write;               /* DAC index * 3 + component */
    u8 attr_ff;
    u8 adlib, adlib_index, adlib_flags;
    u8 adlib_regs[256];
    u8 adlib_running[2];
    u32 adlib_started[2];
} machine_hw;
extern machine_hw hw;

/* the INT 33h driver's state (mouse.c) */
typedef struct {
    u16 x, y, buttons, visible;
    u16 range_x0, range_x1, range_y0, range_y1;
    u8 has_handler;
    u16 handler_seg, handler_off, handler_mask;
} machine_mouse;
extern machine_mouse mouse;

/* the flags an INT returns: CF clear (ok) or set with AX the DOS error code (err) */
static inline void machine_ok(tc_regs *r) { r->flags &= (u16)~TC_FLAG_CF; }
static inline void machine_err(tc_regs *r, u16 code) { r->ax = code; r->flags |= TC_FLAG_CF; }

/* A service the oracle has and the machine does not yet: stops with the oracle's words. */
void machine_unimplemented(const char *what, const tc_regs *r) __attribute__((noreturn));

#endif
