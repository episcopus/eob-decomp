/* Overlay 3 of EOB.EXE (6961 bytes, 23 functions). Purpose unknown at this point: the unit owns the
   0x34-byte character/object records at ds:E06A, the item buffer table at ds:EFD4 and the dungeon
   map screen at ds:0528, and talks to the party/map screens. Placeholder names. */

/*@flags -ml -O -Z */

extern void farfree(void far *block); /*@ 0000:71EE */
extern void far *gEFD4[]; /*@ ds:EFD4 */
extern int gF538; /*@ ds:F538 */

/* ovl3:0000: frees every far pointer in the 200-entry table at ds:EFD4 (farfree), clears each slot
   and resets the counter ds:F538. Called by ovl3:0A7C and overlay 5. */
/*@ ovl3:0000 */ void FreePtrTable(void)
{
    int i;

    for (i = 0; i < 0xc8; i++) {
        if (gEFD4[i]) {
            farfree(gEFD4[i]);
            gEFD4[i] = 0;
        }
    }
    gF538 = 0;
}

/*@ ovl3:004F stub */ void Ovl3_004F(void) {}
/*@ ovl3:01AA stub */ void Ovl3_01AA(void) {}
/*@ ovl3:01C8 stub */ void Ovl3_01C8(void) {}
/*@ ovl3:031A stub */ void Ovl3_031A(void) {}
/*@ ovl3:03A2 stub */ void Ovl3_03A2(void) {}
/*@ ovl3:05D2 stub */ void Ovl3_05D2(void) {}
/*@ ovl3:0624 stub */ void Ovl3_0624(void) {}
/*@ ovl3:069E stub */ void Ovl3_069E(void) {}
/*@ ovl3:0718 stub */ void Ovl3_0718(void) {}
/*@ ovl3:07A5 stub */ void Ovl3_07A5(void) {}
/*@ ovl3:0951 stub */ void Ovl3_0951(void) {}
/*@ ovl3:09AA stub */ void Ovl3_09AA(void) {}
/*@ ovl3:0A7C stub */ void Ovl3_0A7C(void) {}
/*@ ovl3:0FD1 stub */ void Ovl3_0FD1(void) {}
/*@ ovl3:109D stub */ void Ovl3_109D(void) {}
/*@ ovl3:1147 stub */ void Ovl3_1147(void) {}
/*@ ovl3:11AF stub */ void Ovl3_11AF(void) {}
/*@ ovl3:123D stub */ void Ovl3_123D(void) {}
/*@ ovl3:12F1 stub */ void Ovl3_12F1(void) {}
/*@ ovl3:1368 stub */ void Ovl3_1368(void) {}
/*@ ovl3:15B9 stub */ void Ovl3_15B9(void) {}
/*@ ovl3:19F7 stub */ void Ovl3_19F7(void) {}
