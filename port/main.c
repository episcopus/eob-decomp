/* main.c: the portable build's skeleton program (playbook 80, "The skeleton").

   It proves the approach on the units of port/units.txt, translated from src/ by tools/port/tc2c.py
   and compiled natively: EOB.EXE's load module goes into the arena at 1000:0000 with its
   relocations, so DGROUP and its far pointer tables are the original's bytes, and the translated
   functions run on them.

     eob-port[-headless] --self-test EOB.EXE   run the checks below; exit 0 when they pass
     eob-port --window                       open the SDL3 window and show a test frame (Escape closes it)

   The checks are for the skeleton units, src/E0CF7.C and src/E1658.C: each BuildList links its
   table's records into a list at gList (ds:FB48) through ListAdd; the list must be the table's
   records in table order, ending in a null pointer, and the node accessors of E1658 must read the
   +0E flags and +2E state words of each record. Playing the game is the plan's later steps. */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

#include "tcport.h"
#include "platform.h"
#include "ported.h"   /* build/port/gen: the translated functions */

/* EOB.EXE: DGROUP is paragraph 212Eh of the load module (whitepaper 2.2). */
#define EOB_DGROUP_PARA 0x212Eu
/* where the oracle and DOS put the load module (tools/oracle/machine.py LOAD_SEG) */
#define LOAD_SEG 0x1000u

/* the globals and tables they use, with their addresses in src/E0CF7.C */
#define G_FLAG 0xF3B9u
#define G_LIST 0xFB48u

static const struct {
    const char *name;
    void (*build)(void);
    u8 flag;
    u16 table;
    u16 count;
} lists[] = {
    {"BuildList1, gFlag 1 (gTable1a)", BuildList1, 1, 0x5102, 52},
    {"BuildList1, gFlag 0 (gTable1b)", BuildList1, 0, 0x506A, 38},
    {"BuildList2, gFlag 1 (gTable2a)", BuildList2, 1, 0x52DA, 59},
    {"BuildList2, gFlag 0 (gTable2b)", BuildList2, 0, 0x51D2, 47},
    {"BuildList3, gFlag 1 (gTable3a)", BuildList3, 1, 0x528E, 19},
    {"BuildList3, gFlag 0 (gTable3b)", BuildList3, 0, 0x53C6, 32},
    {"BuildList4 (gTable4)", BuildList4, 0, 0x5446, 7},
    {"BuildList5 (gTable5)", BuildList5, 0, 0x5462, 7},
};

static int failures;

static void check(int ok, const char *what, const char *detail)
{
    if (!ok) {
        printf("FAIL %s: %s\n", what, detail);
        failures++;
    }
}

static void check_list(unsigned li)
{
    char msg[160];
    u16 i;
    farptr node;
    farptr table = TC_MKFP(tc_ds, lists[li].table);

    tc_st_u8(TC_MKFP(tc_ds, G_FLAG), lists[li].flag);
    lists[li].build();
    node = tc_ld_u32(TC_MKFP(tc_ds, G_LIST));
    for (i = 0; i < lists[li].count; i++) {
        farptr want = tc_ld_u32(tc_fp_add(table, (u16)(i * 4u)));
        if (i == 0) {
            snprintf(msg, sizeof msg, "the records are in DGROUP (segment %04X, DGROUP %04X)", TC_SEG(want), tc_ds);
            check(TC_SEG(want) == tc_ds, lists[li].name, msg);
        }
        snprintf(msg, sizeof msg, "node %u is %04X:%04X, the table has %04X:%04X", i, TC_SEG(node), TC_OFF(node),
                 TC_SEG(want), TC_OFF(want));
        check(node == want, lists[li].name, msg);
        if (node != want) {
            return;
        }
        /* E1658's accessors read the record's +0E flags and +2E state words: each is given a
           pattern for the check and its own value back after */
        {
            farptr fl = tc_fp_add(node, 0x0E), st = tc_fp_add(node, 0x2E);
            u16 keep_fl = tc_ld_u16(fl), keep_st = tc_ld_u16(st);
            u16 pat;
            for (pat = 0; pat < 16; pat++) {
                tc_st_u16(fl, (u16)(0x5A50u | pat));
                tc_st_u16(st, (u16)(0xA5A0u | pat));
                check(IsNodeEnabled(node) == !(pat & 0x0008), lists[li].name, "IsNodeEnabled");
                check(IsNodeOn(node) == (pat & 0x0001), lists[li].name, "IsNodeOn");
                check(GetNodeState1(node) == (pat & 0x0002), lists[li].name, "GetNodeState1");
            }
            tc_st_u16(fl, keep_fl);
            tc_st_u16(st, keep_st);
        }
        node = tc_ld_u32(node);   /* next, at +00 */
    }
    check(node == 0, lists[li].name, "the last node's link is not null");
    /* FindNode returns the first node with the key of the last record */
    {
        farptr head = tc_ld_u32(TC_MKFP(tc_ds, G_LIST));
        farptr last = tc_ld_u32(tc_fp_add(table, (u16)((lists[li].count - 1u) * 4u)));
        u16 key = tc_ld_u16(tc_fp_add(last, 4));
        farptr want = head;
        while (tc_ld_u16(tc_fp_add(want, 4)) != key) {
            want = tc_ld_u32(want);
        }
        check(FindNode(head, key) == want, lists[li].name, "FindNode");
    }
}

#ifdef SIGALRM
/* a list the code under test made circular would loop for ever: fail instead */
static void watchdog(int sig)
{
    (void)sig;
    printf("FAIL watchdog: the self-test did not end within 10 s (a circular list?)\n");
    exit(1);
}
#endif

static int self_test(const char *exe)
{
    unsigned i;
#ifdef SIGALRM
    signal(SIGALRM, watchdog);
    alarm(10);
#endif
    if (tc_load_program(exe, LOAD_SEG, EOB_DGROUP_PARA) != 0) {
        return 2;
    }
    tc2c_init();
    check(tc_code_lookup(TC_MKFP(tc_load_seg + 0x1658u, 0x001Eu), "self-test") == (tc_fn)ListAdd,
          "code table", "1658:001E is not ListAdd");
    for (i = 0; i < sizeof lists / sizeof lists[0]; i++) {
        check_list(i);
    }
    printf("eob-port self-test: %u units (", tc2c_unit_count);
    for (i = 0; i < tc2c_unit_count; i++) {
        printf("%s%s", i ? " " : "", tc2c_units[i]);
    }
    printf("), %u lists: %s\n", (unsigned)(sizeof lists / sizeof lists[0]), failures ? "FAILED" : "ok");
    return failures ? 1 : 0;
}

static int window_demo(void)
{
    static u8 pixels[320 * 200];
    static u8 dac[256][3];
    unsigned x, y, i;
    u32 start;
    if (platform_init("Eye of the Beholder (port skeleton)", 3) != 0) {
        return 2;
    }
    for (i = 0; i < 256; i++) {
        dac[i][0] = (u8)(i >> 2);
        dac[i][1] = (u8)((i * 3) & 63);
        dac[i][2] = (u8)(63 - (i >> 2));
    }
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
            pixels[y * 320 + x] = (u8)((x * 256u) / 320u);
        }
    }
    start = platform_ms();
    for (;;) {
        struct platform_event e = platform_poll_event();
        if (e.kind == PLATFORM_EVENT_QUIT || (e.kind == PLATFORM_EVENT_SCANCODE && e.scancode == 0x01)) {
            break;
        }
        if (platform_ms() - start > 10000u) {
            break;
        }
        platform_present(pixels, (const u8 (*)[3])dac);
        platform_sleep_ms(16);
    }
    platform_quit();
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--self-test") == 0) {
        return self_test(argv[2]);
    }
    if (argc == 2 && strcmp(argv[1], "--window") == 0) {
        return window_demo();
    }
    fprintf(stderr, "usage: eob-port --self-test EOB.EXE | --window\n");
    return 2;
}
