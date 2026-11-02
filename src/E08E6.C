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

struct Entry27 {            /* 27-byte records at ds:1A3B, indexed by Obj28.b0 (src/E0D1C.C) */
    char pad00;
    char m01;               /* +1 (ds:1A3C) */
    char m02;               /* +2 (ds:1A3D) */
    char m03;               /* +3 (ds:1A3E) */
    char pad04;
    char m05;               /* +5 (ds:1A40) */
    char m06;               /* +6 (ds:1A41) */
    char pad07[0x0E - 0x07];
    char m0E;               /* +0xE (ds:1A49) */
    char pad0F[0x13 - 0x0F];
    int m13;                /* +0x13 (ds:1A4E) */
    char m15;               /* +0x15 (ds:1A50) */
    char m16;               /* +0x16 (ds:1A51) */
    char m17;               /* +0x17 (ds:1A52) */
    char m18;               /* +0x18 (ds:1A53) */
    char pad19[0x1B - 0x19];
};

extern struct Obj28 gObjs[]; /*@ ds:8A82 */
extern signed char gArg3237[]; /*@ ds:3237 */
extern signed char gArg3234[]; /*@ ds:3234 */
extern signed char g323B[]; /*@ ds:323B */
extern unsigned char gB894[]; /*@ ds:B894 */
extern struct Entry27 gTable1A3B[]; /*@ ds:1A3B */
extern signed char g194B[]; /*@ ds:194B */
extern char g31F2[]; /*@ ds:31F2 */
extern char g31F6[]; /*@ ds:31F6 */
extern int gSoundVolume; /*@ ds:FBCA */
extern char gF442; /*@ ds:F442 */
extern unsigned long gFC04; /*@ ds:FC04 */
extern unsigned long gCount849A; /*@ ds:849A */
extern int gFlags849E; /*@ ds:849E */
extern char gDF4C; /*@ ds:DF4C */
extern int gF518; /*@ ds:F518 */
extern int gDD10; /*@ ds:DD10 */
extern int g8102; /*@ ds:8102 */
extern char g1CD3[]; /*@ ds:1CD3 */
extern char g1CE3[]; /*@ ds:1CE3 */
extern void (far *gFn80D2)(char far *map); /*@ ds:80D2 */

extern void UpdateDDEffects(void); /*@ 1414:0D63 */
extern void f0EF3_097B(char a, int b, int c, int d, int e); /*@ 0EF3:097B */
extern int f17A6_0314(int obj, int volume); /*@ 17A6:0314 */
extern int f0D1C_0006(int value); /*@ 0D1C:0006 */
extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern void fOvl4_1F51(int n); /*@ 2116:005C */   /* overlay 4 entry, ovl4:1F51 */

/* 08E6:000B: reset an object whose type is not 7, 8 or 0xa: clear the type, pick an orientation
   from 17A6:0314(id, gSoundVolume) and set flag bit 3. Called by 0EF3:1702. */
/*@ 08E6:000B */ void ClearObjectType(struct Obj28 far *obj)
{
    if (obj->type == 7 || obj->type == 8 || obj->type == 0xa)
        return;
    obj->type = 0;
    obj->orient = (f17A6_0314(obj->id, gSoundVolume) ^ 4) >> 1;
    obj->m16 |= 8;
}

/* 08E6:0055: clear the type and reset the sound field of every type-8 object, remembering the last
   one; when that object's b0 is 7 set bit 18 of ds:FC04, when it is 0xc set bit 27. Called by 03FE
   and 0EC3. Purpose unknown: what ds:FC04 flags. */
/*@ 08E6:0055 */ void ClearType8Objects(void)
{
    struct Obj28 far *obj, *last;
    int i;

    for (obj = gObjs, i = 0; i < 30; i++, obj++) {
        if (obj->type == 8) {
            obj->type = 0;
            obj->m10 = gSoundVolume;
            last = obj;
        }
    }
    if (last->b0 == 7)
        gFC04 |= 0x40000L;
    if (last->b0 == 0xc)
        gFC04 |= 0x8000000L;
}

/*@ 08E6:00C3 stub */ void Obj00C3(struct Obj28 far *obj) {}

/* 08E6:0150: run Obj00C3 over every one of the 30 objects in gObjs */
/*@ 08E6:0150 */ void Obj0150(void)
{
    struct Obj28 far *obj;
    int i;

    for (obj = gObjs, i = 0; i < 30; i++, obj++)
        Obj00C3(obj);
}

/* 08E6:017E: OR together one bit (1 << frame) for each of the first (gB894[id * 9 + 4] & 7)
   objects with this id; -2 when that count is 4, -1 when one of their ds:1A3B records has m15 set,
   and an object whose frame is 4 first takes frame = g31F2[orient]. */
/*@ 08E6:017E */ int BuildObjectFrameMask(int id)
{
    int count;
    struct Obj28 far *obj;
    int i, bits;

    count = gB894[id * 9 + 4] & 7;
    if (count == 4)
        return -2;
    bits = 0;
    for (obj = gObjs, i = 0; i < 30; i++, obj++) {
        if (obj->id == id) {
            if (gTable1A3B[obj->b0].m15)
                return -1;
            if (obj->frame == 4)
                obj->frame = g31F2[obj->orient];
            bits |= 1 << obj->frame;
            if (--count == 0)
                break;
        }
    }
    return bits;
}

/* 08E6:0218: the first frame bit in g31F6 + (orient & 1) * 4 that BuildObjectFrameMask(id) does
   not have; -1 when the object's record's m15 is set, the mask is negative, or no bit qualifies.
   Called by 0309 and 058A. */
/*@ 08E6:0218 */ int FindObjectFrameBit(struct Obj28 far *obj, int id)
{
    int bit;
    char far *row;
    int bits, i;

    if (gTable1A3B[obj->b0].m15)
        return -1;
    bits = BuildObjectFrameMask(id);
    if (bits < 0)
        return -1;
    row = g31F6 + (obj->orient & 1) * 4;
    for (i = 0; i < 4; i++, row++) {
        bit = (char)(obj->orient ^ *row);
        if (!(bits & (1 << bit)))
            return bit;
    }
    return -1;
}

/* 08E6:0297: move an object to a new id and orientation: count the move in gB894 (dropping the old
   id's count when bits 0..2 are set) and notify f0D1C_0006 of the id. Either argument may be -1 to
   leave that field alone. Called by 0309 and 131D. */
/*@ 08E6:0297 */ void MoveObjectToId(struct Obj28 far *obj, int id, int orient)
{
    if (id != -1) {
        f0D1C_0006(obj->id);
        if (gB894[obj->id * 9 + 4] & 7)
            gB894[obj->id * 9 + 4]--;
        obj->id = id;
        gB894[id * 9 + 4]++;
    }
    if (orient != -1) {
        obj->orient = orient;
        id = obj->id;
    }
    f0D1C_0006(id);
}

/*@ 08E6:0309 stub */ int TurnObject(struct Obj28 far *obj, int arg2, int arg3) { return 0; }

/* 08E6:03FE: for an object with flag bit 0 set: when its type is 8, run the 0055 cleanup, when the
   type is not 0xa, clear the type and reset its sound field. Called by UpdateObjects. */
/*@ 08E6:03FE */ void Obj03FE(struct Obj28 far *obj)
{
    if ((obj->m16 & 1) && obj->type != 0xa) {
        if (obj->type == 8) {
            ClearType8Objects();
        } else {
            obj->type = 0;
            obj->m10 = gSoundVolume;
        }
    }
}
/*@ 08E6:0430 stub */ void Obj0430(struct Obj28 far *obj, int value) {}
/* 08E6:058A: give an object the frame the first free id frame bit picks; when its gB894 count is 1
   its frame is 4 instead. Called by 05EA. */
/*@ 08E6:058A */ void SetObjectFrameFromId(struct Obj28 far *obj)
{
    int id;

    id = obj->id;
    if ((gB894[id * 9 + 4] & 7) == 1) {
        obj->frame = 4;
    } else {
        id = FindObjectFrameBit(obj, id);
        if (id < 0)
            return;
        obj->frame = id;
    }
    f0D1C_0006(obj->id);
}
/*@ 08E6:05EA stub */ int Obj05EA(struct Obj28 far *obj, int v) { return 0; }
/* 08E6:073F: pass an object's id, frame and orientation and a caller value to 0EF3:097B with 0xff.
   Called by 0765 as Obj073F(obj, value). Purpose unknown: what 0EF3:097B does with them is not known. */
/*@ 08E6:073F */ void Obj073F(struct Obj28 far *obj, int value)
{
    f0EF3_097B(0xff, value, obj->id, obj->frame, obj->orient);
}
/*@ 08E6:0765 stub */ int Obj0765(struct Obj28 far *obj) { return 0; }
/* 08E6:0AB8: run the type-0 object update: notify 0765 and 05EA, clear m0A, turn it with 0430 and
   run 05EA with -1 when its record's m0E has bit 3 set. Called by UpdateObjects. */
/*@ 08E6:0AB8 */ void Obj0AB8(struct Obj28 far *obj)
{
    int handle;

    handle = f0D1C_00DA(obj->id, obj->orient);
    if (Obj0765(obj) != 0)
        return;
    if (Obj05EA(obj, handle) != 0)
        return;
    obj->m0A = 0;
    Obj0430(obj, obj->m10);
    if (gTable1A3B[obj->b0].m0E & 8)
        Obj05EA(obj, -1);
}
/* 08E6:0B34: turn the object through 0309; when that fails, move its orientation by `delta` (mod 4)
   and turn again. Called by UpdateObjects and 0B94. */
/*@ 08E6:0B34 */ void StepObjectOrient(struct Obj28 far *obj, int delta)
{
    int cur;

    cur = f0D1C_00DA(obj->id, obj->orient);
    if (TurnObject(obj, cur, -1) == 0) {
        obj->orient += delta;
        obj->orient &= 3;
        TurnObject(obj, -1, obj->orient);
    }
}
/*@ 08E6:0B94 stub */ void Obj0B94(struct Obj28 far *obj, int arg) {}
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
            StepObjectOrient(obj, gArg3237[obj->type]);
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

/* 08E6:0DCA: notify f0D1C_0006 of the object's id, and when gDF4C is set clear counter flag bit 1,
   set m16 bit 1, run the colour callback on ds:1CD3 or ds:1CE3, reset the countdown, run the
   overlay-4 entry, clear m16 bit 1 again, set the countdown to 1, set flag bit 1 and gDF4C. Called
   by 0FBE. */
/*@ 08E6:0DCA */ void RestoreObjectFlags(struct Obj28 far *obj)
{
    f0D1C_0006(obj->id);
    if (gDF4C) {
        gFlags849E &= ~2;
        obj->m16 |= 2;
        if (!g8102)
            gFn80D2(g1CE3);
        else
            gFn80D2(g1CD3);
        gCount849A = 0;
        fOvl4_1F51(1);
        obj->m16 &= ~2;
        gCount849A = 1;
        gFlags849E |= 2;
        gDF4C = 1;
    }
}

/*@ 08E6:0E48 stub */ void Obj0E48(void) {}
/*@ 08E6:0EC3 stub */ void Obj0EC3(struct Obj28 far *obj) {}
/* 08E6:0FBE: damage an object by `damage`: subtract it from the count, clear m16 bit 3, run 0DCA
   (or mark m16 bit 1 when f0D1C_0006 finds its id and gF518 is set), set m16 bit 0, and when the
   count has run out run 0EC3. Called by 0D1C:09CB and 0DE8. */
/*@ 08E6:0FBE */ void DamageObject(struct Obj28 far *obj, int damage)
{
    int id;

    gDD10 = 1;
    obj->count -= damage;
    obj->m16 &= ~8;
    if (gF518 == 0) {
        RestoreObjectFlags(obj);
    } else {
        id = f0D1C_0006(obj->id);
        if (id != 0)
            obj->m16 |= 2;
    }
    obj->m16 |= 1;
    if (obj->count <= 0) {
        if (gF518 == 0 || id == 0)
            Obj0EC3(obj);
    }
}
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
/*@ 08E6:131D stub */ void Obj131D(int index) {}
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
