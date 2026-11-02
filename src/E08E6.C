/* Unit at EOB.EXE 08E6:000B (6331 bytes, 36 functions); the object update dispatch and the
   per-type behaviour routines of the game's map objects. One function decompiled so far, its
   siblings are stubs. Placeholder names. */

struct Obj28 {              /* 28-byte records, 30 of them at ds:8A82 */
    char b0;                /* +0: index into the ds:1A3B table (the character record) */
    signed char kind;       /* +1 */
    int id;                 /* +2 */
    char frame;             /* +4 */
    char orient;            /* +5 */
    char toggle;            /* +6 */
    char m07;               /* +7 */
    signed char type;       /* +8 */
    signed char state;      /* +9 */
    char m0A;               /* +A */
    char timer;             /* +B */
    int hitPoints;          /* +C */
    int count;              /* +E */
    int m10;                /* +10 */
    int m12;                /* +12 */
    int m14;                /* +14 */
    char m16;               /* +16: flag byte */
    char m17;               /* +17 */
    char m18;               /* +18 */
    char m19;               /* +19 */
    char m1A;               /* +1A */
    char m1B;               /* +1B */
};

extern struct Obj28 gObjs[]; /*@ ds:8A82 */
extern signed char gArg3237[]; /*@ ds:3237 */
extern signed char gArg3234[]; /*@ ds:3234 */

extern void UpdateDDEffects(void); /*@ 1414:0D63 */

/*@ 08E6:000B stub */ void Obj000B(void) {}
/*@ 08E6:0055 stub */ void Obj0055(void) {}
/*@ 08E6:00C3 stub */ void Obj00C3(struct Obj28 far *o) {}
/*@ 08E6:0150 stub */ void Obj0150(void) {}
/*@ 08E6:017E stub */ void Obj017E(void) {}
/*@ 08E6:0218 stub */ void Obj0218(void) {}
/*@ 08E6:0297 stub */ void Obj0297(void) {}
/*@ 08E6:0309 stub */ void Obj0309(void) {}
/*@ 08E6:03FE stub */ void Obj03FE(struct Obj28 far *o) {}
/*@ 08E6:0430 stub */ void Obj0430(void) {}
/*@ 08E6:058A stub */ void Obj058A(void) {}
/*@ 08E6:05EA stub */ void Obj05EA(void) {}
/*@ 08E6:073F stub */ void Obj073F(void) {}
/*@ 08E6:0765 stub */ void Obj0765(void) {}
/*@ 08E6:0AB8 stub */ void Obj0AB8(struct Obj28 far *o) {}
/*@ 08E6:0B34 stub */ void Obj0B34(struct Obj28 far *o, int arg) {}
/*@ 08E6:0B94 stub */ void Obj0B94(struct Obj28 far *o, int arg) {}
/* 08E6:0CA6: count an object's timer down and clear its type when it reaches zero. Called for the
   object types 7 and 10 by UpdateObjects (08E6:0CC8). */
/*@ 08E6:0CA6 */ void Obj0CA6(struct Obj28 far *o)
{
    if (o->timer) {
        if (--o->timer == 0)
            o->type = 0;
    }
}

/* 08E6:0CC8: update every object of a kind; dispatch on its type through an 11-entry table */
/*@ 08E6:0CC8 */ void UpdateObjects(int kind)
{
    struct Obj28 far *o;
    int i;

    for (o = gObjs, i = 0; i < 30; i++, o++) {
        if (o->kind != kind || o->count <= 0)
            continue;
        Obj00C3(o);
        if (o->type > 0)
            Obj03FE(o);
        switch (o->type) {
        case 0:
            Obj0AB8(o);
            break;
        case 1:
        case 2:
        case 3:
            Obj0B34(o, gArg3237[o->type]);
            break;
        case 5:
        case 6:
            Obj0B94(o, gArg3234[o->type]);
            break;
        case 7:
        case 10:
            Obj0CA6(o);
            break;
        }
        if (o->type != 4 && o->type != 7)
            o->toggle ^= 1;
    }
    UpdateDDEffects();
}

/*@ 08E6:0DCA stub */ void Obj0DCA(void) {}
/*@ 08E6:0E48 stub */ void Obj0E48(void) {}
/*@ 08E6:0EC3 stub */ void Obj0EC3(void) {}
/*@ 08E6:0FBE stub */ void Obj0FBE(void) {}
/*@ 08E6:103D stub */ void Obj103D(void) {}
/*@ 08E6:106B stub */ void Obj106B(void) {}
/*@ 08E6:10F3 stub */ void Obj10F3(void) {}
/*@ 08E6:131D stub */ void Obj131D(void) {}
/*@ 08E6:1410 stub */ void Obj1410(void) {}
/*@ 08E6:1505 stub */ void Obj1505(void) {}
/*@ 08E6:155F stub */ void Obj155F(void) {}
/*@ 08E6:15B4 stub */ void Obj15B4(void) {}
/*@ 08E6:1634 stub */ void Obj1634(void) {}
/*@ 08E6:16D6 stub */ void Obj16D6(void) {}
/*@ 08E6:1775 stub */ void Obj1775(void) {}
/*@ 08E6:17FD stub */ void Obj17FD(void) {}
/*@ 08E6:189D stub */ void Obj189D(void) {}
