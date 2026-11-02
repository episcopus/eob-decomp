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
extern signed char g323B[]; /*@ ds:323B */
extern int gSoundVolume; /*@ ds:FBCA */

extern void UpdateDDEffects(void); /*@ 1414:0D63 */
extern void f0EF3_097B(char a, int b, int c, int d, int e); /*@ 0EF3:097B */

/*@ 08E6:000B stub */ void Obj000B(void) {}
/*@ 08E6:0055 stub */ void Obj0055(void) {}
/*@ 08E6:00C3 stub */ void Obj00C3(struct Obj28 far *obj) {}

/* 08E6:0150: run Obj00C3 over every one of the 30 objects in gObjs */
/*@ 08E6:0150 */ void Obj0150(void)
{
    struct Obj28 far *obj;
    int i;

    for (obj = gObjs, i = 0; i < 30; i++, obj++)
        Obj00C3(obj);
}

/*@ 08E6:017E stub */ void Obj017E(void) {}
/*@ 08E6:0218 stub */ void Obj0218(void) {}
/*@ 08E6:0297 stub */ void Obj0297(void) {}
/*@ 08E6:0309 stub */ void Obj0309(void) {}
/* 08E6:03FE: for an object with flag bit 0 set: when its type is 8, run the 0055 cleanup, when the
   type is not 0xa, clear the type and reset its sound field. Called by UpdateObjects. */
/*@ 08E6:03FE */ void Obj03FE(struct Obj28 far *obj)
{
    if ((obj->m16 & 1) && obj->type != 0xa) {
        if (obj->type == 8) {
            Obj0055();
        } else {
            obj->type = 0;
            obj->m10 = gSoundVolume;
        }
    }
}
/*@ 08E6:0430 stub */ void Obj0430(void) {}
/*@ 08E6:058A stub */ void Obj058A(void) {}
/*@ 08E6:05EA stub */ void Obj05EA(void) {}
/* 08E6:073F: pass an object's id, frame and orientation and a caller value to 0EF3:097B with 0xff.
   Called by 0765 as Obj073F(obj, value). Purpose unknown: what 0EF3:097B does with them is not known. */
/*@ 08E6:073F */ void Obj073F(struct Obj28 far *obj, int value)
{
    f0EF3_097B(0xff, value, obj->id, obj->frame, obj->orient);
}
/*@ 08E6:0765 stub */ void Obj0765(void) {}
/*@ 08E6:0AB8 stub */ void Obj0AB8(struct Obj28 far *o) {}
/*@ 08E6:0B34 stub */ void Obj0B34(struct Obj28 far *o, int arg) {}
/*@ 08E6:0B94 stub */ void Obj0B94(struct Obj28 far *o, int arg) {}
/* 08E6:0CA6: count an object's timer down and clear its type when it reaches zero. Called for the
   object types 7 and 10 by UpdateObjects (08E6:0CC8). */
/*@ 08E6:0CA6 */ void Obj0CA6(struct Obj28 far *obj)
{
    if (obj->timer) {
        if (--obj->timer == 0)
            obj->type = 0;
    }
}

/* 08E6:0CC8: update every object of a kind; dispatch on its type through an 11-entry table */
/*@ 08E6:0CC8 */ void UpdateObjects(int kind)
{
    struct Obj28 far *obj;
    int i;

    for (obj = gObjs, i = 0; i < 30; i++, obj++) {
        if (obj->kind != kind || obj->count <= 0)
            continue;
        Obj00C3(obj);
        if (obj->type > 0)
            Obj03FE(obj);
        switch (obj->type) {
        case 0:
            Obj0AB8(obj);
            break;
        case 1:
        case 2:
        case 3:
            Obj0B34(obj, gArg3237[obj->type]);
            break;
        case 5:
        case 6:
            Obj0B94(obj, gArg3234[obj->type]);
            break;
        case 7:
        case 10:
            Obj0CA6(obj);
            break;
        }
        if (obj->type != 4 && obj->type != 7)
            obj->toggle ^= 1;
    }
    UpdateDDEffects();
}

/*@ 08E6:0DCA stub */ void Obj0DCA(void) {}
/*@ 08E6:0E48 stub */ void Obj0E48(void) {}
/*@ 08E6:0EC3 stub */ void Obj0EC3(void) {}
/*@ 08E6:0FBE stub */ void Obj0FBE(void) {}
/* 08E6:103D: an object's orientation row byte: 1 when its frame is 4, else the byte at
   ds:323B[orient * 4 + frame]. Called by 05EA. */
/*@ 08E6:103D */ int Obj103D(struct Obj28 far *obj, int unused)
{
    if (obj->frame == 4)
        return 1;
    return g323B[obj->orient * 4 + obj->frame];
}
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
/* 08E6:189D: does this object have the given key as its id and either the given field or 4 as its
   frame? Returns 1 when it does, else 0. Called by 0D1C:09CB through E1414 (which passes an object,
   an index and a selection). */
/*@ 08E6:189D */ int Obj189D(struct Obj28 far *obj, int key, int field)
{
    if (obj->id == key && (obj->frame == field || obj->frame == 4))
        return 1;
    return 0;
}
