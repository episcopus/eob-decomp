/* Unit at EOB.EXE 0D1C:0006 (3265 bytes, 26 functions): the party-status and character-effect
   module. It walks the six 243-byte character records at ds:F580 (struct CharRec, src/E0A72.C),
   the 28-byte object records at ds:8A82 (struct Obj28, src/E08E6.C) and a 15-word table at
   ds:A9C8, and it draws the party rows through the rectangle and region front ends. Several
   helpers are pulled in from the character unit (0A72), the object unit (08E6) and the resident
   drawing code. Most meanings are not established, so most function names say what a function
   does rather than what the game calls it. */

/*@flags -ml -O -Z */

struct CharRec {            /* 243-byte (0xF3) character records at ds:F580; see src/E0A72.C */
    char pad00;
    signed char m01;        /* +01 */
    char pad02[0x17 - 0x02];
    signed char m17;        /* +17 */
    char pad18[0x1B - 0x18];
    signed char m1B;        /* +1B */
    char pad1C[0x1F - 0x1C];
    signed char m1F;        /* +1F */
    signed char m20;        /* +20 */
    char pad21[3];
    signed char m24;        /* +24 */
    char pad25[0x77 - 0x25];
    int m77[27];            /* +77: 27 word slots; slot 16 (+97) is also read on its own */
    char padAD[0xF3 - 0xAD];
};

extern int gA9C8[]; /*@ ds:A9C8 */       /* a 15-word table searched by 0D1C:0006 */
extern char gDF4C; /*@ ds:DF4C */        /* set to 1 when 0D1C:0006 finds the value */
extern signed char gA99A; /*@ ds:A99A */ /* the selected character index (src/E0ACD.C) */
extern char gF51A; /*@ ds:F51A */        /* the selected row (src/E0ACD.C) */
extern struct CharRec gChars[]; /*@ ds:F580 */

extern void f1547_0991(int dir); /*@ 1547:0991 */

/* 0D1C:0006: Purpose unknown: the value `value` is looked for in the 15 words at ds:A9C8; when
   it is found, ds:DF4C is set to 1 and the function returns 1, otherwise it returns 0. Called
   from the object unit (08E6) and from 1414. */
/*@ 0D1C:0006 */ int f0D1C_0006(int value)
{
    int far *slot;
    int i;

    slot = gA9C8;
    for (i = 0; i < 15; i++, slot++) {
        if (*slot == value) {
            gDF4C = 1;
            return 1;
        }
    }
    return 0;
}

/* 0D1C:003F: selects the party member at index `value` (the signed char at ds:A99A). When
   `value` differs from the current one, the old index and the new index are passed to f1547_0991
   (which redraws that member's row), and nothing is done when they are equal. gA99A indexes the
   character records read by 0D1C:0670 and is toggled 0..5 by Turn091C (src/E0ACD.C). Called by
   0ACD, 0DE8, 0EF3, 1414 and overlays 1, 5 and 6. */
/*@ 0D1C:003F */ void SelectCharacter(int value)
{
    int old;

    if (gA99A != value) {
        old = gA99A;
        gA99A = value;
        f1547_0991(old);
        f1547_0991(gA99A);
    }
}

/* 0D1C:006D: Purpose unknown: increments its two word arguments (which the caller passes by
   value, so the increment has no effect). Called by 0ACD as (gF51A, 9). */
/*@ 0D1C:006D */ void f0D1C_006D(int first, int second)
{
    first++;
    second++;
}

/* 0D1C:0078: Purpose unknown: increments its word argument (passed by value, so the increment
   has no effect). Called by 0ACD as (gF51A). */
/*@ 0D1C:0078 */ void f0D1C_0078(int value)
{
    value++;
}
/*@ 0D1C:0080 stub */ void f0D1C_0080(void) {}
/*@ 0D1C:00DA stub */ void f0D1C_00DA(void) {}
/*@ 0D1C:00F1 stub */ void f0D1C_00F1(void) {}
/*@ 0D1C:0163 stub */ void f0D1C_0163(void) {}
/*@ 0D1C:01CA stub */ void f0D1C_01CA(void) {}
/*@ 0D1C:02DC stub */ void f0D1C_02DC(void) {}
/*@ 0D1C:0304 stub */ void f0D1C_0304(void) {}
/*@ 0D1C:035A stub */ void f0D1C_035A(void) {}
/*@ 0D1C:0454 stub */ void f0D1C_0454(void) {}
/*@ 0D1C:0583 stub */ void f0D1C_0583(void) {}
/*@ 0D1C:059B stub */ void f0D1C_059B(void) {}
/*@ 0D1C:0670 stub */ void f0D1C_0670(void) {}
/*@ 0D1C:070F stub */ void f0D1C_070F(void) {}
/*@ 0D1C:07A7 stub */ void f0D1C_07A7(void) {}
/*@ 0D1C:07DB stub */ void f0D1C_07DB(void) {}
/*@ 0D1C:08B1 stub */ void f0D1C_08B1(void) {}
/*@ 0D1C:0903 stub */ void f0D1C_0903(void) {}
/*@ 0D1C:09CB stub */ void f0D1C_09CB(void) {}
/*@ 0D1C:0A43 stub */ void f0D1C_0A43(void) {}
/*@ 0D1C:0BAB stub */ void f0D1C_0BAB(void) {}
/*@ 0D1C:0BEB stub */ void f0D1C_0BEB(void) {}
/*@ 0D1C:0C21 stub */ void f0D1C_0C21(void) {}
