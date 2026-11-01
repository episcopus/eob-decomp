/* Unit at EOB.EXE 1414:0000 (3511 bytes, 19 functions): the timed-effect scheduler. It owns the
   16-entry table of timed effects at ds:5BA8 (a far handler, a period, the next expiry time, a
   word argument and an active byte) and the tick routine that fires each due effect. The effects
   themselves are here: the selected-character blink (0000), a pass over the 10 moving/effect
   records at ds:DD2C (0285), object-list updates (05DF), the per-character status messages
   ("%s no longer has giant strength.", "%s feels the effects of poison!", "%s is no longer
   paralyzed!", 07CA), the paralysis/poison byte handling (03B4), the 18-entry far table at
   ds:EF6A (04B0), and the creature upkeep (0C82, 0CE7). The game timer is the 32-bit counter
   g8492 that the 1Ch handler bumps (src/EOV6.C); most meanings are not established, so names say
   what a function does rather than what the game calls it. */

/* The 16 timed-effect entries at ds:5BA8, 16 bytes each (ds:5CA8 holds the count, 16). */
struct Effect {
    void (far *fn)(int);      /* +00: the far handler, called with `arg` as its argument */
    unsigned long period;     /* +04: re-added to the expiry after each run */
    unsigned long expiry;     /* +08: the time the effect next runs (g8492) */
    int arg;                  /* +0C: the handler's argument */
    char sub;                 /* +0E: a byte the span-setters store */
    char active;              /* +0F: nonzero: the effect is armed */
};

/* A character record: the 243-byte (0xF3) records at ds:F580 (src/E0A72.C, src/E0D1C.C). Only
   the fields this unit touches are named. */
struct CharRec {
    char pad00;
    char m01;                 /* +01: a status byte cleared by the character cleanup */
    char pad02[0x0D - 0x02];
    signed char m0D;          /* +0D */
    char pad0E;
    signed char m0F;          /* +0F */
    char pad10[0x1E - 0x10];
    unsigned char m1E;        /* +1E: a bit set/cleared by the per-slot helper 0770 */
    char pad1F[0xAD - 0x1F];
    unsigned long mAD[10];    /* +AD: 10 32-bit effect times (07CA, 0B2E, 0C2D, 05ED) */
    signed char mD5[10];      /* +D5: 10 effect type bytes, one per mAD slot */
    char padDF[0xE3 - 0xDF];
    int mE3;                  /* +E3: flags tested by 05ED and 07CA */
    char padE5[0xE7 - 0xE5];
    unsigned char mE7;        /* +E7: a counter cleared by 07CA */
    unsigned char mE8[];      /* +E8: a byte table cleared at +E8 + slot by 0770 */
};

/* The 18-byte records at ds:DD2C, 10 of them, walked by 0285 and 0D63. */
struct DDRec {
    char active;              /* +00: nonzero: the record is live */
    int m02;                  /* +02 */
    int m04;                  /* +04 */
    char m06;                 /* +06 */
    signed char m07;          /* +07 */
    char pad08[0x0C - 0x08];
    char m0C;                 /* +0C: flags, bit 0x20 tested by 0285 */
    char m0D;                 /* +0D */
};

extern struct Effect gEffects[]; /*@ ds:5BA8 */
extern int gEffectCount; /*@ ds:5CA8 */       /* the number of entries, 16 */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct DDRec gDD2C[]; /*@ ds:DD2C */   /* 10 records, 18 bytes each */
extern unsigned char far *gEF6A[]; /*@ ds:EF6A */ /* 18 far pointers, walked by 04B0 */
extern signed char gA9C6; /*@ ds:A9C6 */      /* an offset added into gEF6A's targets */
extern int gFB78; /*@ ds:FB78 */              /* a flag toggled by 04B0 */
extern signed char g5CAA; /*@ ds:5CAA */      /* the blink phase toggled by 0000 */
extern signed char g5CAB[]; /*@ ds:5CAB */    /* a byte table indexed by direction in 0285 */
extern char gFB34; /*@ ds:FB34 */             /* the character the blink highlights (src/EOV2.C) */
extern char gDF4C; /*@ ds:DF4C */             /* set when the object table has a hit (src/E0D1C.C) */
extern unsigned long g8492; /*@ ds:8492 */    /* the 32-bit tick counter (src/EOV6.C) */

extern void f1547_0991(int member); /*@ 1547:0991 */
extern void PlaySoundDefault(int sound); /*@ 177D:00F3 */
extern int f0D1C_0006(int value); /*@ 0D1C:0006 */
extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern void f0D1C_0163(void); /*@ 0D1C:0163 */
extern int f0D1C_0670(int member, int what); /*@ 0D1C:0670 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void UpdateObjects(int kind); /*@ 08E6:0CC8 */
extern void f08E6_1410(void); /*@ 08E6:1410 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern void DrawText(char *text); /*@ 1DE7:060A */
extern void f1547_04A0(int member); /*@ 1547:04A0 */
extern void f122E_0896(int member, int a); /*@ 122E:0896 */
extern void f122E_1DA8(int member, int a); /*@ 122E:1DA8 */
extern void f0DE8_01FF(void far *rec, int a); /*@ 0DE8:01FF */
extern void f0DE8_02D4(void far *rec, int a, int b); /*@ 0DE8:02D4 */
extern void f0EF3_00CD(int seg, int off); /*@ 0EF3:00CD */
extern void f0EF3_0D78(int a, int b); /*@ 0EF3:0D78 */
extern void f0BD2_01D0(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int f0BD2_02B9(void far *head, int key, int item); /*@ 0BD2:02B9 */

void f1414_0B2E(void);

/* 1414:0000: the selected-character blink effect, entry 0 of the table (16-tick period 9). It
   flips the phase g5CAA; when the phase turns on it saves gFB34 (the highlighted member),
   replaces it with 0xFF, redraws that member's row through f1547_0991 and restores gFB34, and
   when the phase turns off it just redraws gFB34's row. QUIRK 1414:0037: the argument comes in
   on the stack and is incremented by value, so the increment has no effect. */
/*@ 1414:0000 */ void BlinkSelected(int slot)
{
    int old;

    if (g5CAA ^= 1) {
        old = gFB34;
        gFB34 = 0xFF;
        f1547_0991(old);
        gFB34 = old;
    } else {
        f1547_0991(gFB34);
    }
    slot++;
}
/*@ 1414:003E stub */ void f1414_003E(void) {}
/*@ 1414:0172 stub */ void f1414_0172(void) {}
/*@ 1414:020F stub */ void f1414_020F(void) {}
/*@ 1414:0285 stub */ void f1414_0285(void) {}
/*@ 1414:03B4 stub */ void f1414_03B4(void) {}
/*@ 1414:04B0 stub */ void f1414_04B0(void) {}

/* 1414:04E7: the effect tick: for every entry of gEffects whose expiry (gTimer, the 32-bit tick
   counter) has passed, call its handler with its argument; when the handler leaves the entry
   active, re-arm it by adding the entry's period to the expiry. Finally runs 08E6:1410. An entry
   the handler disarms (active 0) is left alone. Called by 0ACD when a key or menu passes. */
/*@ 1414:04E7 */ void RunEffects(void)
{
    struct Effect far *e;
    int i;

    i = 0;
    e = gEffects;
    for (; i < gEffectCount; i++, e++) {
        if (e->active != 0 && e->expiry < g8492) {
            e->fn(e->arg);
            if (e->active != 0)
                e->expiry = g8492 + e->period;
        }
    }
    f08E6_1410();
}

/* 1414:055B: arms effect `idx` at its table period (like 05A4) and also stores `arg` in the
   entry's argument word (+0C) and `sub` in its byte (+0E). Called by 122E and 0ACD. */
/*@ 1414:055B */ void ArmEffectWith(int idx, int arg, int sub, int active)
{
    struct Effect far *e;

    e = &gEffects[idx];
    e->active = active;
    e->expiry = g8492 + e->period;
    e->arg = arg;
    e->sub = sub;
}
/* 1414:05A4: (re-)arms effect `idx` at its table period: stores `active` and sets the expiry to
   gTimer plus the entry's period. Called by many units (0ACD, 0D1C, 1414). */
/*@ 1414:05A4 */ void ArmEffect(int idx, int active)
{
    struct Effect far *e;

    e = &gEffects[idx];
    e->active = active;
    e->expiry = g8492 + e->period;
}
/* 1414:05DF: effects 2 to 5 of the table call this to run the object-list update of one kind
   through UpdateObjects (08E6:0CC8). */
/*@ 1414:05DF */ void UpdateEffectObjects(int kind)
{
    UpdateObjects(kind);
}
/*@ 1414:05ED stub */ void f1414_05ED(void) {}
/*@ 1414:0770 stub */ void f1414_0770(void) {}
/*@ 1414:07CA stub */ void f1414_07CA(void) {}
/*@ 1414:0B2E stub */ void f1414_0B2E(void) {}
/*@ 1414:0C2D stub */ void f1414_0C2D(void) {}
/*@ 1414:0C82 stub */ void f1414_0C82(void) {}
/*@ 1414:0CE7 stub */ void f1414_0CE7(void) {}
/*@ 1414:0D63 stub */ void f1414_0D63(void) {}
