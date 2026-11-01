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
    char pad10[0x1B - 0x10];
    signed char hp;           /* +1B: current hit points (src/EOV6.C); 0C82 requires it above -10 */
    char pad1C[0x1E - 0x1C];
    unsigned char m1E;        /* +1E: a bit set/cleared by the per-slot helper 0770 */
    char pad1F[0x23 - 0x1F];
    unsigned char m23;        /* +23: the level-like byte 0C82 tests and decrements */
    char pad24[0xAD - 0x24];
    unsigned long mAD[10];    /* +AD: 10 32-bit effect times (07CA, 0B2E, 0C2D, 05ED) */
    signed char mD5[10];      /* +D5: 10 effect type bytes, one per mAD slot */
    char padDF[0xE3 - 0xDF];
    int mE3;                  /* +E3: flags tested by 05ED and 07CA */
    char padE5[0xE7 - 0xE5];
    unsigned char mE7;        /* +E7: a counter cleared by 07CA */
    unsigned char mE8[0xF3 - 0xE8]; /* +E8: a byte table cleared at +E8 + slot by 0770 */
};

/* The 18-byte records at ds:DD2C, 10 of them, walked by 0285 and 0D63. */
struct DDRec {
    char active;              /* +00: 1 when the record is live, cleared by 020F */
    char pad01;
    int m02;                  /* +02 */
    int m04;                  /* +04: a character or object index */
    char m06;                 /* +06 */
    signed char m07;          /* +07 */
    int m08;                  /* +08: an index into the 14-byte records at ds:8DD2 */
    char m0A;                 /* +0A */
    char m0B;                 /* +0B */
    char m0C;                 /* +0C: flags, bit 0x20 tested by 0285 */
    char m0D;                 /* +0D */
    int (far *fn)(struct DDRec far *rec); /* +0E: called by 0172 when active is 2 */
};

/* The 14-byte records at ds:8DD2 (src/E0D1C.C names the same table Entry14); 020F clears the low
   two bits of the byte at +3 of one. */
struct Entry14 {
    char pad00[3];
    unsigned char m03;        /* +03: 020F clears its low two bits */
    char pad04[0x0E - 0x04];
};

/* The 28-byte object records at ds:8A82 (src/E08E6.C). */
struct Obj28 {
    char pad00[2];
    int m02;                  /* +02 */
    char pad04[0x17 - 0x04];
    unsigned char m17;        /* +17: the value 0CE7 randomises */
    char pad18[0x1C - 0x18];
};

extern struct Effect gEffects[]; /*@ ds:5BA8 */
extern int gEffectCount; /*@ ds:5CA8 */       /* the number of entries, 16 */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct DDRec gDD2C[]; /*@ ds:DD2C */   /* 10 records, 18 bytes each */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */ /* 14-byte records (src/E0D1C.C) */
extern struct Obj28 gObjs[]; /*@ ds:8A82 */   /* 30 records, 28 bytes each (src/E08E6.C) */
extern unsigned char gB894[]; /*@ ds:B894 */    /* nine-byte records (src/E0D1C.C) */
extern int gSoundVolume; /*@ ds:FBCA */         /* the global sound volume (src/E177D.C) */
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
extern int f0D1C_0163(int who, int fieldB); /*@ 0D1C:0163 */
extern int f0D1C_0670(int member, int what); /*@ 0D1C:0670 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void UpdateObjects(int kind); /*@ 08E6:0CC8 */
extern void f08E6_1410(void); /*@ 08E6:1410 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern void DrawText(char *text); /*@ 1DE7:060A */
extern void f1547_04A0(int member); /*@ 1547:04A0 */
extern void f122E_0896(int member, int a); /*@ 122E:0896 */
extern void f122E_1DA8(int member, int a); /*@ 122E:1DA8 */
extern int f08E6_189D(struct Obj28 far *obj, int a, int b); /*@ 08E6:189D */
extern int f0DE8_01FF(struct DDRec far *rec); /*@ 0DE8:01FF */
extern int f0DE8_02D4(struct DDRec far *rec, int idx); /*@ 0DE8:02D4 */
extern void f0EF3_00CD(int seg, int off); /*@ 0EF3:00CD */
extern void f0EF3_0D78(int a, int b); /*@ 0EF3:0D78 */
extern void f0BD2_01D0(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int f0BD2_02B9(void far *head, int key, int item); /*@ 0BD2:02B9 */

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
/* 1414:0172: the per-record test 0285 and 0D63 use. It returns 0 when the record already has a
   nonzero m02. A record in state 2 runs its cell callback rec->fn(rec) and returns that res. Otherwise it looks up gB894[a * 9 + 4]: when that byte has bits 0..2 set it scans the 30 objects
   at ds:8A82 for one that f08E6_189D accepts and that f0DE8_02D4 then also accepts, returning 1
   on the first; when the bits are clear it runs f0DE8_01FF(rec) only when `a` is the current
   sound volume and returns that. Called by 0285 and 0D63. */
/*@ 1414:0172 */ int TestDDEffect(struct DDRec far *rec, int a, int b)
{
    struct Obj28 far *obj;
    int i;

    if (rec->m02 != 0)
        return 0;
    if (rec->active == 2) {
        return rec->fn(rec);
    }
    if (gB894[a * 9 + 4] & 7) {
        i = 0;
        obj = gObjs;
        for (; i < 30; i++, obj++) {
            if (f08E6_189D(obj, a, b)) {
                if (f0DE8_02D4(rec, i))
                    return 1;
            }
        }
        return 0;
    } else {
        if (a == gSoundVolume)
            return f0DE8_01FF(rec);
        return 0;
    }
}

/* 1414:020F: clears a live ds:DD2C record. When it is in state 1 it first clears the low two
   bits of gTable8DD2[rec->m08].m03, redraws rec->m04 through 122E:1DA8 with mode 4 and plays
   sound 0x12, then clears the record's active flag and its m0D, m07, m04, m0B, m0A and m08
   fields. Called by 0285 and 0D63. */
/*@ 1414:020F */ void ClearDDEffect(struct DDRec far *rec)
{
    if (rec->active == 1) {
        gTable8DD2[rec->m08].m03 &= 3;
        f122E_1DA8(rec->m04, 4);
        PlaySoundDefault(0x12);
    }
    rec->active = 0;
    rec->m0D = 0;
    rec->m07 = 0;
    rec->m04 = 0;
    rec->m0B = 0;
    rec->m0A = 0;
    rec->m08 = 0;
}
/*@ 1414:0285 stub */ void f1414_0285(void) {}
/*@ 1414:03B4 stub */ void f1414_03B4(void) {}
/* 1414:04B0: effect 13 of the table. Toggles gFB78, then walks the 18 far pointers at ds:EF6A,
   adding gA9C6 to each target and setting gDF4C when a byte there is 0x34 ('4'). QUIRK
   1414:04B4: the argument is incremented by value, so the increment has no effect. */
/*@ 1414:04B0 */ void CheckFarTable(int unused)
{
    int i;

    unused++;
    gFB78 ^= 1;
    for (i = 0; i < 0x12; i++) {
        if (gEF6A[i][gA9C6] == 0x34) {
            gDF4C = 1;
            break;
        }
    }
}

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
/* 1414:0770: toggles or clears one of a character's 10 effect slots. In modes 0 and 2 it flips
   bit `slot` of gChars[who].m1E, then modes 0, 1 and 2 clear gChars[who].mE8[slot] and redraw
   the member's row through f1547_0991; any other mode does nothing. Called by 07CA. */
/*@ 1414:0770 */ void SetCharEffectSlot(int who, int mode, int slot)
{
    switch (mode) {
    case 0:
    case 2:
        gChars[who].m1E ^= 1 << slot;
        /* falls through */
    case 1:
        gChars[who].mE8[slot] = 0;
        f1547_0991(who);
    }
}
/*@ 1414:07CA stub */ void f1414_07CA(void) {}

/* 1414:0B2E: re-arms effect `who` + 6 for every party member `who` that passes f0D1C_0670(who, 1).
   It takes the smallest nonzero of the member's 10 effect times gChars[who].mAD and, when there
   is one, sets the entry's period to that time minus gTimer and arms it; when all ten are zero it
   sets the period to -1 and disarms it. Called by 0ACD, 0D1C:035A, 0C2D and overlays 1 and 3. */
/*@ 1414:0B2E */ void RearmCharEffects(void)
{
    struct CharRec far *rec;
    struct Effect far *e;
    int idx;
    unsigned long min;
    int who;
    int i;

    who = 0;
    rec = gChars;
    for (; who < 6; who++, rec++) {
        if (!f0D1C_0670(who, 1))
            continue;
        min = -1;
        for (i = 0; i < 10; i++) {
            if (rec->mAD[i] < min && rec->mAD[i] != 0)
                min = rec->mAD[i];
        }
        idx = who + 6;
        e = &gEffects[idx];
        if (min != -1) {
            e->period = min - g8492;
            ArmEffect(idx, 1);
        } else {
            e->period = min;
            ArmEffect(idx, 0);
        }
    }
}
/* 1414:0C2D: clears effect slot `value` of character `who`: for each of the member's 10 effect
   slots whose gChars[who].mD5 equals `value`, clears the type byte and the 32-bit time
   gChars[who].mAD, then re-arms the per-member effects through 0B2E. Called by 0EF3 and
   overlay 5. */
/*@ 1414:0C2D */ void ClearCharEffectSlot(int who, int value)
{
    struct CharRec far *rec;
    int i;

    rec = &gChars[who];
    for (i = 0; i < 10; i++) {
        if (rec->mD5[i] == value) {
            rec->mD5[i] = 0;
            rec->mAD[i] = 0;
        }
    }
    RearmCharEffects();
}
/* 1414:0C82: effect 14 of the table. For every party member whose f0D1C_0163(who, 2) test
   passes and who is alive (level-like m23 nonzero, m01 bit 0 set, hit points above -10), it
   decrements m23 and runs 1547:04A0 for that member. */
/*@ 1414:0C82 */ void DecayMemberState(void)
{
    int who;

    for (who = 0; who < 6; who++) {
        if (f0D1C_0163(who, 2) < 0)
            continue;
        if (gChars[who].m23 == 0)
            continue;
        if ((gChars[who].m01 & 1) == 0)
            continue;
        if (gChars[who].hp <= -10)
            continue;
        gChars[who].m23--;
        f1547_04A0(who);
    }
}
/* 1414:0CE7: effect 15 of the table. For every one of the 30 objects at ds:8A82 that wins a
   1d2 roll, it stores a fresh (1d2 << 4) + 1d2 value in obj->m17 and reports the object's m02
   through f0D1C_0006. */
/*@ 1414:0CE7 */ void RandomiseObjectState(void)
{
    struct Obj28 far *obj;
    int i;

    i = 0;
    obj = gObjs;
    for (; i < 30; i++, obj++) {
        if (RollDice(1, 2, 0) == 1) {
            obj->m17 = RollDice(1, 2, 0) << 4;
            obj->m17 += RollDice(1, 2, 0);
            f0D1C_0006(obj->m02);
        }
    }
}
/* 1414:0D63: walks the 10 records at ds:DD2C and, for every live one whose 0172 test passes,
   runs 020F on it. Called by UpdateObjects (08E6:0CC8) and 122E:0001. */
/*@ 1414:0D63 */ void UpdateDDEffects(void)
{
    struct DDRec far *rec;
    int result;
    int i;

    rec = gDD2C;
    for (i = 0; i < 10; i++, rec++) {
        if (rec->active != 0) {
            result = TestDDEffect(rec, rec->m04, rec->m06);
            if (result)
                ClearDDEffect(rec);
        }
    }
}
