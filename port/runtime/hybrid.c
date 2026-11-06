/* hybrid.c: the shared library's entry points for the per-function test (hybrid.h,
   tools/port/hybrid.py). Built only into build/port/hybrid/libeobhybrid (make port-hybrid).

   A test calls tc_hybrid_run with the original's registers at a function's entry. The function's C
   runs on tc_mem (the oracle's memory); a stop of the runtime (tc_fail: a routine with no C and no
   address, a far pointer to code with no C) or the time limit ends it with a message instead of
   ending the process. The time limit is CPU time (ITIMER_VIRTUAL), so a C loop that waits for
   something only an interrupt changes (the original waits on the tick count) is reported, not hung
   on; while the oracle runs a routine for the C (tc_hybrid_far) the limit only marks the call, which
   ends when the routine returns. */
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "tcport.h"
#include "hybrid.h"
#include "ported.h"
#include "machine.h"
#include "driver.h"

void tc2c_init_unported(void);

/* set by the oracle (ctypes): runs an original routine; returns DX:AX */
u32 (*tc_hybrid_oracle)(u32 entry, const u8 *args, u16 n);
/* set by the oracle when the routine it ran did not return (a fault, an unimplemented service) */
volatile int tc_hybrid_oracle_failed;
char tc_hybrid_message[512];
tc_regs tc_hybrid_far_regs;

static sigjmp_buf jb;
static volatile sig_atomic_t running, in_oracle, late;

static void fail_hook(int code, const char *msg)
{
    snprintf(tc_hybrid_message, sizeof tc_hybrid_message, "%s", msg);
    if (in_oracle) {
        /* a stop in C that a Python callback called: a jump would leave that callback holding Python's
           lock, and the test would hang for good; end it loudly instead */
        fprintf(stderr, "hybrid: stop %d inside a Python callback: %s\n", code, msg);
        abort();
    }
    siglongjmp(jb, code);
}

static void on_alarm(int sig)
{
    (void)sig;
    if (!running) {
        return;
    }
    if (in_oracle) {
        late = 1;            /* Python is running: end the test when the routine returns */
        return;
    }
    snprintf(tc_hybrid_message, sizeof tc_hybrid_message, "the C did not return within the time limit");
    siglongjmp(jb, 72);
}

u32 tc_hybrid_far(farptr entry, const u8 *args, u16 n)
{
    u32 r;
    if (tc_hybrid_oracle == NULL) {
        tc_fail(71, "no oracle to run %04X:%04X", TC_SEG(entry), TC_OFF(entry));
    }
    {
        sig_atomic_t was = in_oracle;
        in_oracle = 1;
        r = tc_hybrid_oracle(entry, args, n);
        in_oracle = was;
    }
    if (tc_hybrid_oracle_failed) {
        tc_hybrid_oracle_failed = 0;
        tc_fail(73, "the original's %04X:%04X, run for the C, did not return (%s)", TC_SEG(entry),
                TC_OFF(entry), tc_hybrid_message);
    }
    if (late && !in_oracle) {
        tc_fail(72, "the C did not return within the time limit (last in the original's %04X:%04X)",
                TC_SEG(entry), TC_OFF(entry));
    }
    return r;
}

void tc_hybrid_init(u16 load_seg, u16 ds)
{
    tc_load_seg = load_seg;
    tc_ds = ds;
    tc_fail_hook = fail_hook;
    tc2c_init();
    tc2c_init_unported();
    signal(SIGVTALRM, on_alarm);
}

/* Runs entry `index` of tc_hybrid_entries with the original's SS:SP and DS at its entry. Returns 0
   and DX:AX in *ret, or the stop's code with its message in tc_hybrid_message. */
int tc_hybrid_run(unsigned index, u16 ss, u16 sp, u16 ds, u32 *ret, unsigned limit_ms)
{
    struct itimerval t;
    int code;
    if (index >= tc_hybrid_entry_count) {
        snprintf(tc_hybrid_message, sizeof tc_hybrid_message, "no entry %u", index);
        return 70;
    }
    tc_ss = ss;
    tc_sp = sp;
    tc_ds = ds;
    tc_hybrid_message[0] = 0;
    tc_hybrid_oracle_failed = 0;
    late = 0;
    in_oracle = 0;
    memset(&t, 0, sizeof t);
    code = sigsetjmp(jb, 1);
    if (code == 0) {
        t.it_value.tv_sec = limit_ms / 1000u;
        t.it_value.tv_usec = (limit_ms % 1000u) * 1000;
        running = 1;
        setitimer(ITIMER_VIRTUAL, &t, NULL);
        /* an interrupt due when the routine is entered is taken before its first instruction, as the
           original took it (the port's next check would come after an access) */
        machine_deliver_due();
        *ret = tc_hybrid_entries[index].thunk();
    }
    running = 0;
    memset(&t, 0, sizeof t);
    setitimer(ITIMER_VIRTUAL, &t, NULL);
    in_oracle = 0;
    return code;
}

/* ---- the graphics driver (M4) ------------------------------------------------------------------- */

/* The driver's routines, for the test to list: routine i, or 0 past the last (driver.h). */
const tc_driver_routine *tc_hybrid_driver_routine(unsigned i)
{
    return mcga_routine(i);
}

/* Registers the driver's C at the segment the original loaded it at. */
void tc_hybrid_driver_register(u16 seg)
{
    mcga_register(seg);
}

/* Runs driver routine i (registered at seg) with the original's SS:SP and DS at its entry, its
   arguments read where the caller pushed them (SS:SP+4). Returns 0 with DX:AX in *ret and the other
   registers it answers in in *regs, or the stop's code with its message in tc_hybrid_message. */
int tc_hybrid_run_driver(unsigned index, u16 seg, u16 ss, u16 sp, u16 ds, u32 *ret, tc_regs *regs,
                         unsigned limit_ms)
{
    struct itimerval t;
    int code;
    u8 args[TC_ARG_MAX];
    unsigned k;
    const tc_driver_routine *d = mcga_routine(index);
    if (d == 0) {
        snprintf(tc_hybrid_message, sizeof tc_hybrid_message, "no driver routine %u", index);
        return 70;
    }
    tc_ss = ss;
    tc_sp = sp;
    tc_ds = ds;
    for (k = 0; k < sizeof args; k++) {
        args[k] = tc_ld_u8(TC_MKFP(ss, (u16)(sp + 4u + k)));
    }
    memset(regs, 0, sizeof *regs);
    tc_hybrid_message[0] = 0;
    tc_hybrid_oracle_failed = 0;
    late = 0;
    in_oracle = 0;
    memset(&t, 0, sizeof t);
    code = sigsetjmp(jb, 1);
    if (code == 0) {
        t.it_value.tv_sec = limit_ms / 1000u;
        t.it_value.tv_usec = (limit_ms % 1000u) * 1000;
        running = 1;
        setitimer(ITIMER_VIRTUAL, &t, NULL);
        machine_deliver_due();
        if (tc_code_find_args(TC_MKFP(seg, d->off)) != d->fn) {
            tc_fail(70, "the driver's %s is not registered at %04X:%04X", d->name, seg, d->off);
        }
        *ret = d->fn(args, regs);
    }
    running = 0;
    memset(&t, 0, sizeof t);
    setitimer(ITIMER_VIRTUAL, &t, NULL);
    in_oracle = 0;
    return code;
}

/* ---- the machine's shadow test (tools/port/hybrid.py --shadow) ---------------------------------- */

#include "machine.h"
#include "platform.h"

/* set by the oracle (ctypes): the files as the session's VFS had them when the shadow began */
int32_t (*tc_hybrid_file_size)(const char *name);
int (*tc_hybrid_file_read)(const char *name, uint8_t *buf, uint32_t size);

int32_t platform_file_size(const char *name)
{
    return tc_hybrid_file_size ? tc_hybrid_file_size(name) : -1;
}

int platform_file_read(const char *name, uint8_t *buf, uint32_t size)
{
    return tc_hybrid_file_read ? tc_hybrid_file_read(name, buf, size) : -1;
}

void platform_set_game_dir(const char *dir) { (void)dir; }

/* Boots the machine on tc_mem (cleared first) and loads the program, as the oracle's Machine does
   before it runs: 0, or the stop's code with its message in tc_hybrid_message. */
int tc_hybrid_boot(const char *exe_path, const char *exe_name, const u8 *cmdline, u16 cmdlen, int mouse,
                   int adlib)
{
    int code;
    tc_hybrid_message[0] = 0;
    code = sigsetjmp(jb, 1);
    if (code == 0) {
        memset(tc_mem, 0, TC_MEM_SIZE);
        machine_boot(exe_name, cmdline, cmdlen, mouse, adlib);
        if (tc_load_program(exe_path, MACHINE_LOAD_SEG, 0x212Eu) != 0) {
            tc_fail(70, "cannot load %s", exe_path);
        }
    }
    return code;
}

/* Resets the machine's state for a session (no memory written but by machine_boot's own writes,
   which the shadow overwrites with the oracle's memory before every call). */
int tc_hybrid_machine_reset(const char *exe_name, const u8 *cmdline, u16 cmdlen, int mouse, int adlib)
{
    int code;
    tc_hybrid_message[0] = 0;
    code = sigsetjmp(jb, 1);
    if (code == 0) {
        machine_boot(exe_name, cmdline, cmdlen, mouse, adlib);
    }
    return code;
}

/* What the stub of service n does, on tc_mem: 0, or the stop's code (74: the machine lacks it, 76:
   the program ended) with its message in tc_hybrid_message. */
int tc_hybrid_service(u8 n, tc_regs *r)
{
    int code;
    tc_hybrid_message[0] = 0;
    code = sigsetjmp(jb, 1);
    if (code == 0) {
        if (machine_service(n, r) != 0) {
            tc_fail(74, "the machine has no service %02Xh", n);
        }
    }
    return code;
}

/* the hardware outputs, recorded for the shadow to compare with the oracle's logs */
u32 tc_hybrid_adlib_count;
u8 tc_hybrid_adlib_last[2];
u32 tc_hybrid_speaker_count;
u16 tc_hybrid_speaker_divisor;
u8 tc_hybrid_speaker_bits;

void platform_adlib_write(uint8_t reg, uint8_t value)
{
    tc_hybrid_adlib_count++;
    tc_hybrid_adlib_last[0] = reg;
    tc_hybrid_adlib_last[1] = value;
}

void platform_speaker(uint16_t divisor, uint8_t port61_low2)
{
    tc_hybrid_speaker_count++;
    tc_hybrid_speaker_divisor = divisor;
    tc_hybrid_speaker_bits = port61_low2;
}

/* one port access of the machine: 0, or the stop's code (74: a port the machine lacks) */
int tc_hybrid_in(u16 port, int size, u16 *value)
{
    int code;
    tc_hybrid_message[0] = 0;
    code = sigsetjmp(jb, 1);
    if (code == 0) {
        *value = machine_in(port, size);
    }
    return code;
}

int tc_hybrid_out(u16 port, int size, u16 value)
{
    int code;
    tc_hybrid_message[0] = 0;
    code = sigsetjmp(jb, 1);
    if (code == 0) {
        machine_out(port, size, value);
    }
    return code;
}

/* ---- the oracle's services, handlers and ports, for the routines of port/asm --------------------- */

/* set by the oracle (ctypes) while a test runs: runs the code at seg:off as an interrupt on the bench
   (pushes FLAGS and a return to the sentinel, the registers from and back to r); 0, or -1 when it did
   not return. And a port access on the bench's devices. */
int (*tc_hybrid_int_fn)(u16 seg, u16 off, tc_regs *r);
int (*tc_hybrid_port_fn)(int write, u16 port, int size, u16 *value);

int tc_hybrid_oracle_int(u16 seg, u16 off, tc_regs *r)
{
    int bad;
    if (tc_hybrid_int_fn == NULL) {
        return 0;
    }
    {
        sig_atomic_t was = in_oracle;
        in_oracle = 1;
        bad = tc_hybrid_int_fn(seg, off, r);
        in_oracle = was;
    }
    if (bad) {
        tc_fail(73, "the original's interrupt code at %04X:%04X, run for the C, did not return (%s)", seg, off,
                tc_hybrid_message);
    }
    if (late && !in_oracle) {
        tc_fail(72, "the C did not return within the time limit (last in the original's %04X:%04X)", seg, off);
    }
    return 1;
}

int tc_hybrid_oracle_port(int write, u16 port, int size, u16 *value)
{
    int bad;
    if (tc_hybrid_port_fn == NULL) {
        return 0;
    }
    {
        sig_atomic_t was = in_oracle;
        in_oracle = 1;
        bad = tc_hybrid_port_fn(write, port, size, value);
        in_oracle = was;
    }
    if (bad) {
        tc_fail(73, "the oracle's port %s %04Xh failed (%s)", write ? "out" : "in", port, tc_hybrid_message);
    }
    if (late && !in_oracle) {
        tc_fail(72, "the C did not return within the time limit (last at the oracle's port %04Xh)", port);
    }
    return 1;
}

/* ---- the interrupts on the bench (irq.c) ------------------------------------------------------- */

/* set by the oracle (ctypes): the bench's devices take an interrupt (in-service flags, port 60h, the
   mouse trampoline's data), and its instruction count is set; 0 when done */
int (*tc_hybrid_raise_fn)(u8 kind, u8 scancode, u16 cond, u16 buttons, u16 x, u16 y);
int (*tc_hybrid_insns_fn)(u32 insns);

int tc_hybrid_raise(const machine_event *e)
{
    if (tc_hybrid_raise_fn == NULL) {
        return 0;
    }
    {
        sig_atomic_t was = in_oracle;
        int bad;
        in_oracle = 1;
        bad = tc_hybrid_raise_fn(e->kind, e->scancode, e->cond, e->buttons, e->x, e->y);
        in_oracle = was;
        if (bad) {
            tc_fail(73, "the bench could not take an interrupt (%s)", tc_hybrid_message);
        }
    }
    if (late && !in_oracle) {
        tc_fail(72, "the C did not return within the time limit (last at an interrupt)");
    }
    return 1;
}

int tc_hybrid_set_insns(u32 insns)
{
    if (tc_hybrid_insns_fn == NULL) {
        return 0;
    }
    {
        sig_atomic_t was = in_oracle;
        in_oracle = 1;
        tc_hybrid_insns_fn(insns);
        in_oracle = was;
    }
    if (late && !in_oracle) {
        tc_fail(72, "the C did not return within the time limit");
    }
    return 1;
}

/* After a test's C returns: the events still due are delivered, as the port would at its next check;
   0, or the stop's code */
int tc_hybrid_drain(void)
{
    int code;
    tc_hybrid_message[0] = 0;
    code = sigsetjmp(jb, 1);
    if (code == 0) {
        machine_deliver_due();
    }
    return code;
}
