/* Unit at EOB.EXE 08E6:000B (6331 bytes, 36 functions); the object update dispatch and the
   per-type behaviour routines of the game's map objects: UpdateObjects walks the 30 28-byte object
   records at ds:8A82 (struct Obj28, indexed by the 27-byte character records at ds:1A3B, struct
   Entry27) and dispatches each live object by its type to the 000B/00C3/03FE/0AB8/0B34/0B94/0CA6
   behaviours. 26 functions are at L2, four are drafts (0EC3, 106B, 1410, 17FD) and six are stubs
   (0309, 0430, 05EA, 0765, 0B94, 10F3). Whitespace and empty-body stubs keep the siblings in
   address order. */

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

struct CharRec {            /* 243-byte character records at ds:F580; the fields this unit reads */
    char pad00[0x1D];
    signed char m1D;        /* +0x1D */
    char pad1E[0xE3 - 0x1E];
    int mE3;                /* +0xE3 */
    char padE5[0xF3 - 0xE5];
};

extern struct CharRec gChars[]; /*@ ds:F580 */

extern struct Obj28 gObjs[]; /*@ ds:8A82 */
extern signed char gArg3237[]; /*@ ds:3237 */
extern signed char gArg3234[]; /*@ ds:3234 */
extern signed char g323B[]; /*@ ds:323B */
extern unsigned char gB894[]; /*@ ds:B894 */
extern struct Entry27 gTable1A3B[]; /*@ ds:1A3B */
extern signed char g194B[]; /*@ ds:194B */
extern signed char g1790[]; /*@ ds:1790 */
extern int g321E; /*@ ds:321E */
extern int g31FE[]; /*@ ds:31FE */
extern int g320E[]; /*@ ds:320E */
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
extern char g1991[]; /*@ ds:1991 */
extern char gTable32C3[]; /*@ ds:32C3 */
extern char gTable329B[]; /*@ ds:329B */
extern char gTable32BB[]; /*@ ds:32BB */
extern int gFBCC; /*@ ds:FBCC */
extern unsigned char gFBD2; /*@ ds:FBD2 */
extern unsigned long gDeadline; /*@ ds:F512 */   /* a 32-bit deadline at ds:F512:F514 */
extern unsigned long g8492; /*@ ds:8492 */       /* the 32-bit tick counter (src/E0ACD.C) */
extern int gF510; /*@ ds:F510 */
extern char gF51B; /*@ ds:F51B */
extern int gF540; /*@ ds:F540 */
extern int gFB32; /*@ ds:FB32 */

extern void UpdateDDEffects(void); /*@ 1414:0D63 */
extern void f0EF3_097B(char a, int b, int c, int d, int e); /*@ 0EF3:097B */
extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int f17A6_0F61(int x, int y); /*@ 17A6:0F61 */
extern int f0D1C_0BAB(int obj); /*@ 0D1C:0BAB */
extern int GetIndexedByte(char index, char limit, char offset); /*@ 0A72:0006 */
extern int SubtractInts(int minuend, int subtrahend); /*@ 0A72:058C */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void AddPartyExperience(int amount); /*@ 0DE8:0C2B */
extern void f122E_1DA8(int a, int b); /*@ 122E:1DA8 */
extern void fOvl5_160A(void); /*@ 211E:007F */   /* overlay 5 entry, ovl5:160A */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void far *memset(void far *dst, int c, unsigned n); /*@ 0000:6384 */

void Obj0EC3(struct Obj28 far *obj);   /* forward: Obj0E48 reaches it before its definition */
extern int f17A6_0314(int obj, int volume); /*@ 17A6:0314 */
extern int f0D1C_0006(int value); /*@ 0D1C:0006 */
extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern void fOvl4_1F51(int n); /*@ 2116:005C */   /* overlay 4 entry, ovl4:1F51 */
extern void PlaySoundVolume(int sound, int volume); /*@ 177D:0106 */
extern void PlaySoundDefault(int sound); /*@ 177D:00F3 */
void Obj10F3(struct Obj28 far *obj);   /* forward: 05EA reaches it before its definition */

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

/* 08E6:00C3: for an object whose type is 0..6 and whose count is positive: run 17A6:0F61(id,
   gSoundVolume) and 17A6:0314(id, gSoundVolume); when the first is under 4 and the second minus
   twice the orientation, minus 3, wrapped into 0..7, is over 2 or the first is under 2, clear the
   type and reset the sound field. Called by 0150 and UpdateObjects. Purpose unknown: what the two
   17A6 calls compute. */
/*@ 08E6:00C3 */ void Obj00C3(struct Obj28 far *obj)
{
    int first, dir;

    if (obj->type == 7 || obj->type == 8 || obj->type == 9 || obj->type == 0xa || obj->count <= 0)
        return;
    first = f17A6_0F61(obj->id, gSoundVolume);
    if (first >= 4)
        return;
    dir = f17A6_0314(obj->id, gSoundVolume) - obj->orient * 2 - 3;
    if (dir < 0)
        dir += 8;
    if (dir <= 2 && first >= 2)
        return;
    obj->type = 0;
    obj->m10 = gSoundVolume;
}

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

/* 08E6:0309: turn an object toward a new id and orientation. When `id` is not -1 and differs from
   the object's own id, look up the nine-byte gB894 record for `id` and pick the byte at +orient^2:
   when g194B of that byte has no bit 2, or `id` is the current gSoundVolume, fail (0). When the
   record's +4 count has bits 0..2 set and `id` is nonzero, take the frame FindObjectFrameBit picks
   (fail if none) and move the object to (id, orient); for a plain turn (`id` is the object's own id
   or -1) only `orient` is applied when it is not -1. Then notify f0D1C_0006 of the old id and,
   unless gF442 is set, play the object's record m17 through PlaySoundVolume. Returns 1 on success.
   Called by 0430, 0B34 and 0B94 (StepObjectOrient passes -1 to leave a field alone). */
/*@ 08E6:0309 */ int TurnObject(struct Obj28 far *obj, int id, int orient)
{
    char far *rec;
    char dir;
    char frame;
    int oldId;

    oldId = obj->id;
    if (obj->id != id && id != -1) {
        rec = (char far *)&gB894[id * 9];
        if (orient == -1)
            orient = obj->orient;
        dir = orient ^ 2;
        if ((g194B[rec[dir]] & 4) == 0)
            return 0;
        if (id == gSoundVolume)
            return 0;
        if ((rec[4] & 7) && id != 0) {
            frame = FindObjectFrameBit(obj, id);
            if (frame == -1)
                return 0;
            obj->frame = frame;
        }
        MoveObjectToId(obj, id, orient);
        orient = -1;
    }
    if (orient != -1)
        obj->orient = orient;
    f0D1C_0006(oldId);
    if (!gF442)
        PlaySoundVolume(gTable1A3B[obj->b0].m17, obj->id);
    return 1;
}

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
/* 08E6:0430: turn an object toward the direction f17A6_0314(id, value) reports. When the object's
   state is over 6 it resets it to 0 and toggles g321E, which selects the direction table g31FE or
   g320E. The target -1 leaves the object alone; bit 3 of m16 XORs 4 into it. When the target is even
   and id + g1790[target >> 1] equals gSoundVolume, the object takes one step through TurnObject with
   -1 (orientation + 1 when the difference from 2*orient is under 5, else - 1). Otherwise the
   difference picks a direction and the loop walks the eight orientations from 7 down to 0, adding
   the table byte, skipping odd directions, and turning through TurnObject(obj, f0D1C_00DA(id,
   dir >> 1), dir >> 1); the first turn that succeeds returns. Called by 0AB8. */
/*@ 08E6:0430 */ void Obj0430(struct Obj28 far *obj, int value)
{
    int target;
    int handle;
    int i;
    int id;
    register int diff;
    register int dir;
    int far *table;

    if (obj->state > 6) {
        obj->state = 0;
        g321E ^= 1;
    }
    table = g31FE;
    if (g321E != 0)
        table = g320E;
    dir = obj->orient * 2;
    id = obj->id;
    target = f17A6_0314(id, value);
    if (target == -1)
        return;
    if (obj->m16 & 8)
        target ^= 4;
    diff = (target - dir) & 7;
    if (!(target & 1) && id + g1790[target >> 1] == gSoundVolume) {
        if (diff != 0 && diff < 5)
            dir = obj->orient + 1;
        else if (diff != 0)
            dir = obj->orient - 1;
        dir &= 3;
        TurnObject(obj, -1, dir);
        return;
    }
    if (diff != 0) {
        if (target & 1) {
            if (diff < 5)
                dir++;
            else
                dir--;
        } else {
            if (diff < 5)
                dir += 2;
            else
                dir -= 2;
        }
        dir &= 7;
    }
    for (i = 7; i > -1; i--) {
        dir = (dir + table[i]) & 7;
        if (dir & 1) {
            handle = 0;
        } else {
            handle = f0D1C_00DA(id, dir >> 1);
        }
        if (handle != 0 && TurnObject(obj, handle, dir >> 1) != 0)
            return;
    }
}
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
/* 08E6:05EA: check the object against `handle` and turn it. When `handle` is -1 it is recomputed as
   f0D1C_00DA(obj->id, obj->orient); when it is not the current gSoundVolume the object does nothing
   and 0 is returned. Otherwise, when Obj103D(obj) is 0 the object takes the frame SetObjectFrameFromId
   picks and 1 is returned. Else m16 bit 2 toggles, and when it is not now set 1 is returned. Then
   f0D1C_0006(obj->id) decides: when nonzero the object plays gTable1A3B[b0].m16 only when b0 is 4,
   runs the 0xfe/0xff m0A sequence with the countdown and the overlay-4 entry, and does not repeat the
   sound when b0 is not 4; when it is zero the sound plays unconditionally. Either way 10F3 runs and,
   when the id was found, m0A clears, the toggle flips, gDF4C is set, gFlags849E bit 1 is set and the
   countdown is 8. Returns 1. Called by 0AB8. */
/*@ 08E6:05EA */ int Obj05EA(struct Obj28 far *obj, int handle)
{
    register int found;

    if (handle == -1)
        handle = f0D1C_00DA(obj->id, obj->orient);
    if (handle != gSoundVolume)
        return 0;
    if (Obj103D(obj) != 0) {
        obj->m16 ^= 4;
        if (obj->m16 & 4) {
            found = f0D1C_0006(obj->id);
            if (found != 0) {
                gFlags849E &= ~2;
                if (obj->b0 == 4)
                    PlaySoundDefault(gTable1A3B[obj->b0].m16);
                obj->m0A = 0xfe;
                gCount849A = 0;
                fOvl4_1F51(1);
                obj->m0A = 0xff;
                if (obj->b0 != 4)
                    PlaySoundDefault(gTable1A3B[obj->b0].m16);
                gCount849A = 8;
                fOvl4_1F51(1);
            } else {
                PlaySoundDefault(gTable1A3B[obj->b0].m16);
            }
            Obj10F3(obj);
            if (found != 0) {
                obj->m0A = 0;
                obj->toggle ^= 1;
                gDF4C = 1;
                gFlags849E |= 2;
                gCount849A = 8;
            }
        }
    } else {
        SetObjectFrameFromId(obj);
    }
    return 1;
}
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

/* 08E6:0E48: run the overlay-4 entry with 1, then clear m16 bit 1 of every object that has it; when
   an object with that bit has no count left, run 0EC3. When any was found set gDF4C and the
   countdown to 1, else clear gDF4C; finally clear gDD10 and gF518. An uninitialized `any` is a
   QUIRK 08E6:0E99 of the original: only objects with m16 bit 1 set define its value. Called by
   0D1C:09CB, 0EF3:0E1B and 0EF3:18D8. */
/*@ 08E6:0E48 */ void Obj0E48(void)
{
    struct Obj28 far *obj = gObjs;
    int i, any;

    fOvl4_1F51(1);
    for (i = 0; i < 30; i++, obj++) {
        if (obj->m16 & 2) {
            obj->m16 &= ~2;
            any = 1;
            if (obj->count <= 0)
                Obj0EC3(obj);
        }
    }
    if (any) {
        gDF4C = 1;
        gCount849A = 1;
    } else {
        gDF4C = 0;
    }
    gF518 = gDD10 = 0;
}
/* 08E6:0EC3: an object's death: pick a hit value (its frame, or a 1d4 when the frame is 4), and for
   each of m12 and m14 that is set, roll 1d2 and on a 1 resolve it through 0BD2:089E and hand it to
   0BD2:01D0 with the object's id and hit; give the party the record's m13 experience, move the
   object to id 0, shut the game down when its b0 is 0x15, and clear the type-8 objects. Called by
   0E48, 0FBE and 1410. DRAFT: the body is written and every byte agrees except the register the
   pinned compiler picks for `roll`: ours keeps it in SI (0EC3:000E `mov si, ax`) and the original in
   DI (`mov di, ax`, and the calls then push DI before SI); making `hit` block-local did not change
   it. */
/*@ 08E6:0EC3 draft */ void Obj0EC3(struct Obj28 far *obj)
{
    int roll;

    roll = obj->frame;
    if (roll == 4)
        roll = RollDice(1, 4, 0xff);
    if (obj->m12 != 0 && RollDice(1, 2, 0) == 1) {
        int hit = CopyItemToFreeSlot(obj->m12);
        InsertListItem(&gB894[obj->id * 9 + 5], obj->id, hit, roll);
    }
    if (obj->m14 != 0) {
        int hit = CopyItemToFreeSlot(obj->m14);
        InsertListItem(&gB894[obj->id * 9 + 5], obj->id, hit, roll);
    }
    AddPartyExperience(gTable1A3B[obj->b0].m13);
    MoveObjectToId(obj, 0, -1);
    if (obj->b0 == 0x15) {
        fOvl5_160A();
        ShutdownGame();
    }
    if (obj->type == 8)
        ClearType8Objects();
}
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
/*@ 08E6:103D */ int Obj103D(struct Obj28 far *obj)
{
    if (obj->frame == 4)
        return 1;
    return g323B[obj->orient * 4 + obj->frame];
}
/* 08E6:106B: roll 1d20 for a hit against character `who`; a roll of 20 always hits; otherwise the
   target's mE3 bit 9 costs 2 and gFBD2 bit 5 costs 1. The hit succeeds when the roll reaches
   SubtractInts(the object's record m01, the character's m1D). Called by 10F3 and 0EF3:0A3F.
   DRAFT: blocked on the project-wide mE3 `and ax, K; or ax, 0` codegen (the same as 0D1C:0A43 and
   1547:0009): ours folds `gChars[who].mE3 & 0x200` into `test word ptr [..], 0x200`
   (106B:0033), the original is `mov ax, es:[..]; and ax, 0x200; or ax, 0; je`. */
/*@ 08E6:106B draft */ int Obj106B(struct Obj28 far *obj, int who)
{
    int index;
    int roll;
    int need;

    index = obj->b0;
    roll = RollDice(1, 0x14, 0);
    if (roll != 0x14) {
        if (gChars[who].mE3 & 0x200)
            roll -= 2;
        if (gFBD2 & 0x20)
            roll--;
    }
    need = SubtractInts(gTable1A3B[index].m01, gChars[who].m1D);
    return roll >= need || roll == 0x14;
}
/*@ 08E6:10F3 stub */ void Obj10F3(struct Obj28 far *obj) {}
/* 08E6:131D: build object `index` from scratch: clear it, set its b0, kind, frame, orientation,
   m07, type and timer and its record's m19, roll its count from the record's m02 (or 1d4 when m02
   is 0xff), set m12 and m14, and move it to `id`. Does nothing when id is 0. Called by 1505. */
/*@ 08E6:131D */ void Obj131D(int index, int kind, int id, int frame, int orient, int b0, int m07, int type, int timer, int m12, int m14)
{
    struct Obj28 far *obj;

    obj = &gObjs[index];
    memset(obj, 0, 0x1c);
    if (id == 0)
        return;
    kind <<= 1;
    if (index & 1)
        kind++;
    obj->b0 = b0;
    obj->m19 = gTable1A3B[b0].m18;
    obj->kind = kind;
    obj->frame = frame;
    obj->m07 = m07;
    obj->type = type;
    obj->timer = timer;
    obj->orient = orient;
    if (gTable1A3B[b0].m02 != -1)
        obj->hitPoints = obj->count = RollDice(gTable1A3B[b0].m02, 8, 0);
    else
        obj->hitPoints = obj->count = RollDice(1, 4, 0);
    obj->m12 = m12;
    obj->m14 = m14;
    MoveObjectToId(obj, id, orient);
}
/* 08E6:1410: when the effect mode gF51B and its deadline say an effect is due, run the 122E entry
   with (0, 0x20): when no object of the 30 has a count left, run 0EC3 on the first object (from
   index 1) whose 17A6:0F61(id, gSoundVolume) is over 3; then advance the deadline by gF510 and
   clear gF540. Called by 1414:04E7 and overlay 6. DRAFT: the six 32-bit deadline comparisons are
   written and the mode-1 and mode-2 tests now agree, but the second half of the mode-3 test
   `gDeadline >= g8492` is laid out the other way round: ours (1410:005A `jb` skip, fall through to
   fire) where the original has `jae` fire, `jmp` skip. */
/*@ 08E6:1410 draft */ void Obj1410(void)
{
    int i, any;

    if (gF51B == 1) {
        if (gDeadline >= g8492)
            ;
        else
            goto fire;
    }
    if (gF51B == 2) {
        if (gF540 >= gFB32)
            goto fire;
    }
    if (gF51B == 3) {
        if (gF540 >= gFB32)
            goto fire;
        if (gDeadline >= g8492)
            goto fire;
        return;
    }
    return;
fire:
    f122E_1DA8(0, 0x20);
    any = 0;
    for (i = 1; i < 30; i++) {
        if (gObjs[i].count <= 0)
            any += 1;
    }
    if (any == 0) {
        for (i = 1; i < 30; i++) {
            if (f17A6_0F61(gObjs[i].id, gSoundVolume) > 3) {
                Obj0EC3(&gObjs[i]);
                break;
            }
        }
    }
    gDeadline += gF510;
    gF540 = 0;
}
/* 08E6:1505: when the object id `id` has no count in gB894, fill the first free object slot (count
   <= 0) by calling 131D with the eight new-object fields. Called by 122E:0B89. Purpose unknown:
   what the fields select. */
/*@ 08E6:1505 */ void Obj1505(int kind, int id, int frame, int orient, int b0, int m07, int type, int timer, int m12, int m14)
{
    int i;

    if (gB894[id * 9 + 4] & 7)
        return;
    for (i = 0; i < 30; i++) {
        if (gObjs[i].count <= 0) {
            Obj131D(i, kind, id, frame, orient, b0, m07, type, timer, m12, m14);
            break;
        }
    }
}
/* 08E6:155F: fill a six-entry list with -1, then set list[frame] = i for every object of the 30
   whose id equals `key`. The caller passes an eight-word stack array as a far pointer. Called by
   15B4 and 16D6. */
/*@ 08E6:155F */ void BuildObjectIdList(int far *list, int key)
{
    struct Obj28 far *obj;
    int i;

    for (i = 0; i < 6; i++)
        list[i] = -1;
    for (obj = gObjs, i = 0; i < 30; i++, obj++) {
        if (obj->id == key)
            list[obj->frame] = i;
    }
}
/* 08E6:15B4: list the objects with id `b`, then return list[4] when set, else the first of the four
   indices in gTable329B + gFBCC * 8 + (a & 1) * 4 whose list entry is set, else -1. Called by
   0DE8:0359 and 0EF3:0E1B. */
/*@ 08E6:15B4 */ int Obj15B4(char who, int value)
{
    int list[6];
    char far *row;
    int i;

    BuildObjectIdList(list, value);
    if (list[4] != -1)
        return list[4];
    row = gTable329B + gFBCC * 8 + (who & 1) * 4;
    for (i = 0; i < 4; i++, row++) {
        if (list[*row] != -1)
            return list[*row];
    }
    return -1;
}
/* 08E6:1634: roll RollDice(count, sides, bonus) against the object `index`: when `flag` is set and
   a 1d20 reaches GetIndexedByte(0, gTable1A3B[b0].m02, 4), halve the roll when flag is 1, else
   zero it; then, unless 0D1C:0BAB rejects the object, damage it by the roll. Called by 16D6. */
/*@ 08E6:1634 */ void Obj1634(char index, char count, char sides, char bonus, char flag)
{
    int roll;
    int cls;
    int save;

    cls = gTable1A3B[gObjs[index].b0].m02;
    roll = RollDice(count, sides, bonus);
    if (flag != 0) {
        save = GetIndexedByte(0, cls, 4);
        if (RollDice(1, 0x14, 0) >= save) {
            if (flag == 1)
                roll >>= 1;
            else
                roll = 0;
        }
    }
    if (f0D1C_0BAB(index) == 0)
        DamageObject(&gObjs[index], roll);
}
/* 08E6:16D6: list the objects with id `key`, mark gF518, damage list[4] and then the two indices
   in gTable32BB + gFBCC * 2 through 1634, and run the 0E48 cleanup. Called by 0EF3:0F99. */
/*@ 08E6:16D6 */ void Obj16D6(int key, char count, char sides, char bonus, char flag)
{
    int list[6];
    char far *row;
    int i;

    BuildObjectIdList(list, key);
    gF518 = 1;
    if (list[4] != -1)
        Obj1634(list[4], count, sides, bonus, flag);
    row = gTable32BB + gFBCC * 2;
    for (i = 0; i < 2; i++, row++) {
        if (list[*row] != -1)
            Obj1634(list[*row], count, sides, bonus, flag);
    }
    Obj0E48();
}
/* 08E6:1775: find the object whose id is `key`: when its frame equals `b` return its index;
   otherwise scan the four bytes of gTable32C3 + a * 16 + b * 4 and remember the first (lowest) slot
   whose byte equals the object's frame and beats the current best slot. Returns -1 when none. Called
   by 17FD. Purpose unknown: what the two index arguments select. */
/*@ 08E6:1775 */ int Obj1775(int key, int field, int flags)
{
    char far *row;
    int best, bestSlot;
    struct Obj28 far *obj;
    int i, slot;

    best = -1;
    bestSlot = 5;
    for (i = 0, obj = gObjs; i < 30; i++, obj++) {
        row = gTable32C3 + flags * 16 + field * 4;
        if (obj->id == key) {
            if (obj->frame == field)
                return i;
            for (slot = 0; slot < 4; slot++) {
                if (obj->frame == row[slot] && slot < bestSlot) {
                    bestSlot = slot;
                    best = i;
                }
            }
        }
    }
    return best;
}
/* 08E6:17FD: fill `list` with gObjs indices and end it with -1: when scanAll is set, every object
   whose id is key; otherwise when scanFiltered is set, the single index 1775(key, field, flags)
   finds; otherwise the first object 189D accepts for (key, field). Called by 0D1C:0A43. DRAFT: the
   body is written and the first two branches agree; the shared `list++` tail after the scanFiltered
   and 189D branches is placed 12 bytes nearer in the original (17FD:0059 `jmp` 0x28 to the tail, ours
   0x34), a -O basic-block layout difference. */
/*@ 08E6:17FD draft */ void Obj17FD(int far *list, int key, int field, char flags, char scanAll, char scanFiltered)
{
    struct Obj28 far *obj;
    int i;

    if (scanAll) {
        for (i = 0, obj = gObjs; i < 30; i++, obj++) {
            if (obj->id == key) {
                *list = i;
                list++;
            }
        }
    } else {
        if (scanFiltered) {
            *list = Obj1775(key, field, flags);
        } else {
            for (i = 0, obj = gObjs; i < 30; i++, obj++) {
                if (Obj189D(obj, key, field)) {
                    *list = i;
                    break;
                }
            }
        }
        list++;
    }
    *list = -1;
}
/* 08E6:189D: does this object have the given key as its id and either the given field or 4 as its
   frame? Returns 1 when it does, else 0. Called by 0D1C:09CB through E1414 (which passes an object,
   an index and a selection). */
/*@ 08E6:189D */ int Obj189D(struct Obj28 far *obj, int key, int field)
{
    if (obj->id == key && (obj->frame == field || obj->frame == 4))
        return 1;
    return 0;
}
