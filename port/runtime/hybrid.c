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
#include <string.h>
#include <sys/time.h>

#include "tcport.h"
#include "hybrid.h"
#include "ported.h"

void tc2c_init_unported(void);

/* set by the oracle (ctypes): runs an original routine; returns DX:AX */
u32 (*tc_hybrid_oracle)(u32 entry, const u8 *args, u16 n);
/* set by the oracle when the routine it ran did not return (a fault, an unimplemented service) */
volatile int tc_hybrid_oracle_failed;
char tc_hybrid_message[512];

static sigjmp_buf jb;
static volatile sig_atomic_t running, in_oracle, late;

static void fail_hook(int code, const char *msg)
{
    snprintf(tc_hybrid_message, sizeof tc_hybrid_message, "%s", msg);
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
    in_oracle = 1;
    r = tc_hybrid_oracle(entry, args, n);
    in_oracle = 0;
    if (tc_hybrid_oracle_failed) {
        tc_hybrid_oracle_failed = 0;
        tc_fail(73, "the original's %04X:%04X, run for the C, did not return (%s)", TC_SEG(entry),
                TC_OFF(entry), tc_hybrid_message);
    }
    if (late) {
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
        *ret = tc_hybrid_entries[index].thunk();
    }
    running = 0;
    memset(&t, 0, sizeof t);
    setitimer(ITIMER_VIRTUAL, &t, NULL);
    in_oracle = 0;
    return code;
}
