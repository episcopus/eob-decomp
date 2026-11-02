/* Unit at EOB.EXE 08E6:000B (6331 bytes, 36 functions); the object update dispatch and the
   per-type behaviour routines of the game's map objects: UpdateObjects walks the 30 28-byte object
   records at ds:8A82 (struct Obj28, indexed by the 27-byte character records at ds:1A3B, struct
   Entry27) and dispatches each live object by its type to the 000B/00C3/03FE/0AB8/0B34/0B94/0CA6
   behaviours. 35 functions are at L2 and one is a draft (00C3). The unit is compiled with -B:
   that is what keeps the identical jump tails of 0765, 0B94, 17FD and 1410 that a -O build merges.
   Whitespace and empty-body stubs keep the siblings in address order. */

/*@flags -ml -O -Z -B */

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
    char m03;               /* +3 (ds:1A3E): number of attack routines */
    char m04[3][3];         /* +4 (ds:1A3F): {count, sides, bonus} per routine */
    char pad0D;
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
    char pad00;
    unsigned char m01;      /* +01: status bits (src/E0DE8.C) */
    char name[0x0B];        /* +02: the character's name, NUL padded (src/E0DE8.C) */
    char pad0D[0x1B - 0x0D];
    signed char hp;         /* +1B: current hit points (src/E0DE8.C) */
    char pad1C;
    signed char m1D;        /* +1D: the armour-class byte (src/E0DE8.C) */
    char pad1E[0xE3 - 0x1E];
    unsigned long mE3;      /* +E3: effect flags, a 32-bit mask (src/E0DE8.C) */
    char padE7[0xF3 - 0xE7];
};

struct Entry14 {            /* 14-byte item records at ds:8DD2 (src/E0D1C.C) */
    unsigned char flags;    /* +0 (ds:8DD2) */
    signed char icon;       /* +1 (ds:8DD3) */
    signed char m02;        /* +2 (ds:8DD4): the item type */
    char pad03;
    int m04;                /* +4 (ds:8DD6): the key; -1 when free */
    char pad06[0x0B - 0x06];
    signed char m0B;        /* +0xB (ds:8DDD) */
    char pad0C[0x0E - 0x0C];
};

extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */

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
extern int gPartyCell; /*@ ds:FBCA */
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
extern signed char gTable324B[]; /*@ ds:324B */   /* (page, orientation) to a 12-slot group */
extern signed char gTable325B[]; /*@ ds:325B */   /* 4 groups of 12 slot to party-member bytes */
extern signed char gTable328B[]; /*@ ds:328B */   /* (orientation, frame) to the group's half */
extern int gFBCC; /*@ ds:FBCC */
extern unsigned char gFBD2; /*@ ds:FBD2 */
extern unsigned long gDeadline; /*@ ds:F512 */   /* a 32-bit deadline at ds:F512:F514 */
extern unsigned long g8492; /*@ ds:8492 */       /* the 32-bit tick counter (src/E0ACD.C) */
extern int gF510; /*@ ds:F510 */
extern char gF51B; /*@ ds:F51B */
extern int gF540; /*@ ds:F540 */
extern int gFB32; /*@ ds:FB32 */

extern void UpdateDDEffects(void); /*@ 1414:0D63 */
extern void StartCellEffect(char a, int b, int c, int d, int e); /*@ 0EF3:097B */
extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int Distance(int x, int y); /*@ 17A6:0F61 */
extern int f0D1C_0BAB(int obj); /*@ 0D1C:0BAB */
extern int GetIndexedByte(char index, char limit, char offset); /*@ 0A72:0006 */
extern int SubtractInts(int minuend, int subtrahend); /*@ 0A72:058C */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void AddPartyExperience(int amount); /*@ 0DE8:0C2B */
extern void RunMemberScript(int a, int b); /*@ 122E:1DA8 */
extern void fOvl5_160A(void); /*@ 211E:007F */   /* overlay 5 entry, ovl5:160A */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void far *memset(void far *dst, int c, unsigned n); /*@ 0000:6384 */

void Obj0EC3(struct Obj28 far *obj);   /* forward: Obj0E48 reaches it before its definition */
extern int DirectionFromPoints(int obj, int volume); /*@ 17A6:0314 */
extern int f0D1C_0006(int value); /*@ 0D1C:0006 */
extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern void DrawViewFrame(int n); /*@ 2116:005C */   /* overlay 4 entry, ovl4:1F51 */
extern void PlaySoundAtCell(int sound, int volume); /*@ 177D:0106 */
extern void PlaySoundDefault(int sound); /*@ 177D:00F3 */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern void DamageCharacter(int who, int amount); /*@ 0DE8:09B9 */
extern void ApplyStatusEffect(int who, int mask, char far *text, int attack, int count, int value, int flag); /*@ 0DE8:101A */
extern void RedrawMember(int dir); /*@ 1547:0991 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern int AddDDEffect(int who, int item, int volume, int kind, int page, int type); /*@ 0BD2:07DE */
extern char gFileNameBuf[]; /*@ ds:FE6E */
extern signed char g3220[]; /*@ ds:3220 */
extern signed char g3223[]; /*@ ds:3223 */
extern signed char g3226[]; /*@ ds:3226 */
extern signed char g322F[]; /*@ ds:322F */
extern int TickBandageCountdown(struct CharRec far *who); /*@ 0EF3:18A2 */
extern void ReportLostItem(int who); /*@ 0BD2:000E */
void Obj10F3(struct Obj28 far *obj);   /* forward: 05EA reaches it before its definition */

/* 08E6:000B: reset an object whose type is not 7, 8 or 0xa: clear the type, pick an orientation
   from 17A6:0314(id, gPartyCell) and set flag bit 3. Called by 0EF3:1702. */
/*@ 08E6:000B */ void ClearObjectType(struct Obj28 far *obj)
{
    if (obj->type == 7 || obj->type == 8 || obj->type == 0xa)
        return;
    obj->type = 0;
    obj->orient = (DirectionFromPoints(obj->id, gPartyCell) ^ 4) >> 1;
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
            obj->m10 = gPartyCell;
            last = obj;
        }
    }
    if (last->b0 == 7)
        gFC04 |= 0x40000L;
    if (last->b0 == 0xc)
        gFC04 |= 0x8000000L;
}

/* 08E6:00C3: for an object whose type is not 7, 8, 9 or 0xa and whose count is positive: run
   17A6:0F61(id, gPartyCell) and 17A6:0314(id, gPartyCell); when the first is under 4 and the
   second minus twice the orientation, minus 3, wrapped into 0..7, is over 2 or the first is under 2,
   clear the type and reset the sound field. Called by 0150 and UpdateObjects. Purpose unknown: what
   the two 17A6 calls compute. DRAFT. blocked: a TCC -S codegen question, one 3-byte instruction.
   Under -B this body matches byte for byte except that the original reloads `les bx,[bp+6]` at
   08E6:00F2 before the Distance argument obj->id (08E6:00EE `push [ds:FBCA]`; 08E6:00F2
   `les bx,[bp+6]`; 08E6:00F5 `push es:[bx+2]`), while ours reuses the bx the five-test `&&` chain
   already holds, so ours is 3 bytes short (ours je 0x86 against the original je 0x89). The reload
   is what TCC emits when it has not proved the far pointer still lives in ES:BX; it shows up in the
   original exactly where a body is a jump target (the else branch at 03FE:041F, the statement after
   a `||` chain, and the `if (cond) ; else { body }` layout, probed 2026-11-01), but here the body
   is fall-through after the `jle`, and -O (jump optimisation) is what suppresses the redundant load
   (WHITEPAPER 2.1.1). Every source form tried either keeps the cached bx as ours does or lays the
   first test out as `jne @@; jmp` instead of `je`: the `&&` chain, one-line, parenthesised and
   negated conditions, nested ifs (with and without braces), a `switch`, `goto`, an empty-body `if`,
   do/while(0), for(;;), while, a nested block, a declaration initialiser, `register` and separate
   locals, a bare `obj;`, an assignment inside the condition, casts, the `if (cond) ; else { body }`
   layout, and the `||` early-return form (which reloads but lays the first test as `jne @@; jmp`
   under /jNOSMART; it matches only without -B, where TCC's own assembler shortens the jump, and then
   0765, 0B94 and 17FD do not match). Flags probed on the whole unit 2026-11-01: -ml -O -Z -B 35 of
   36, -ml -O -Z 32, -ml -O -Z -G -B 10, -ml -Z -B 2; -B is the only set that reaches 35 and the one
   missing function is this. C++ -P, -1 and -k make no difference. Evidence in re/symbols.toml
   e08e6_00c3 (notes). */
/*@ 08E6:00C3 draft */ void Obj00C3(struct Obj28 far *obj)
{
    int first, dir;

    if (obj->type != 7 && obj->type != 8 && obj->type != 9 && obj->type != 0xa
            && obj->count > 0) {
        first = Distance(obj->id, gPartyCell);
        if (first >= 4)
            return;
        dir = DirectionFromPoints(obj->id, gPartyCell) - obj->orient * 2 - 3;
        if (dir < 0)
            dir += 8;
        if (dir <= 2 && first >= 2)
            return;
        obj->type = 0;
        obj->m10 = gPartyCell;
    }
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
   when g194B of that byte has no bit 2, or `id` is the current gPartyCell, fail (0). When the
   record's +4 count has bits 0..2 set and `id` is nonzero, take the frame FindObjectFrameBit picks
   (fail if none) and move the object to (id, orient); for a plain turn (`id` is the object's own id
   or -1) only `orient` is applied when it is not -1. Then notify f0D1C_0006 of the old id and,
   unless gF442 is set, play the object's record m17 through PlaySoundAtCell. Returns 1 on success.
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
        if (id == gPartyCell)
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
        PlaySoundAtCell(gTable1A3B[obj->b0].m17, obj->id);
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
            obj->m10 = gPartyCell;
        }
    }
}
/* 08E6:0430: turn an object toward the direction DirectionFromPoints(id, value) reports. When the object's
   state is over 6 it resets it to 0 and toggles g321E, which selects the direction table g31FE or
   g320E. The target -1 leaves the object alone; bit 3 of m16 XORs 4 into it. When the target is even
   and id + g1790[target >> 1] equals gPartyCell, the object takes one step through TurnObject with
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
    target = DirectionFromPoints(id, value);
    if (target == -1)
        return;
    if (obj->m16 & 8)
        target ^= 4;
    diff = (target - dir) & 7;
    if (!(target & 1) && id + g1790[target >> 1] == gPartyCell) {
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
   f0D1C_00DA(obj->id, obj->orient); when it is not the current gPartyCell the object does nothing
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
    if (handle != gPartyCell)
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
                DrawViewFrame(1);
                obj->m0A = 0xff;
                if (obj->b0 != 4)
                    PlaySoundDefault(gTable1A3B[obj->b0].m16);
                gCount849A = 8;
                DrawViewFrame(1);
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
    StartCellEffect(0xff, value, obj->id, obj->frame, obj->orient);
}
/* 08E6:0765: the object's action for UpdateObjects. It returns 0 unless the object's 1A3B record
   has m0E bit 6, the object has m19 left and m18 is zero (otherwise m18 is decremented and 0 is
   returned). It then checks Distance(id, gPartyCell) is at most 3 and DirectionFromPoints(id,
   gPartyCell) equals orient * 2, and walks f0D1C_00DA(id, orient), following it until it reaches
   gPartyCell; each step must have g194B[gB894[value * 9 + (orient ^ 2)]] bits 0..1 set and no
   gB894[value * 9 + 4] count, else 0 is returned. The action then switches on b0: 4 turns to 9 and
   plays 0x1f; 14 plays g322F[m19] and turns through Obj073F(obj, g3226[m19]); 15 frees an item 0x3c
   and queues AddDDEffect(-1, item, id, frame, orient, gTable8DD2[item].m02), freeing it when that
   fails; 16 calls 0EF3:097B(0xff, 0, id, frame, orient) and plays 0x55; 17 plays 0x53, announces the
   mind blast and paralyzes every affected member; 21 rolls 1d4 and, under 3, turns through Obj073F
   and plays g3223[roll] after g3220[roll], else announces the wounds spell and hits every affected
   member. Every action then runs the overlay-4 entry, clears gDF4C, decrements m19 unless it is 0xff
   and sets m18 to 5, returning 1. Called by 0AB8. The unit's -B flag is what keeps case 21's
   PlaySoundAtCell(0x6c, id) at 08E6:0907 inline instead of tail-merging it into the shared block at
   08E6:0A5E. */
/*@ 08E6:0765 */ int Obj0765(struct Obj28 far *obj)
{
    struct Entry27 far *rec;
    int value;
    int orient;
    int arg;
    int item;
    int roll;
    int i;

    rec = &gTable1A3B[obj->b0];
    if ((rec->m0E & 0x40) == 0 || obj->m19 == 0)
        return 0;
    if (obj->m18 != 0) {
        obj->m18--;
        return 0;
    }
    if (Distance(obj->id, gPartyCell) > 3)
        return 0;
    if (DirectionFromPoints(obj->id, gPartyCell) != obj->orient * 2)
        return 0;
    orient = obj->orient;
    value = f0D1C_00DA(obj->id, orient);
    while (value != gPartyCell) {
        if ((g194B[(signed char)gB894[value * 9 + (orient ^ 2)]] & 3) == 0
                || (gB894[value * 9 + 4] & 7))
            return 0;
        value = f0D1C_00DA(value, orient);
    }
    switch (obj->b0) {
    case 21:
        roll = RollDice(1, 4, 0xff);
        if (roll < 3) {
            arg = g3220[roll];
            Obj073F(obj, arg);
            PlaySoundAtCell(g3223[roll], obj->id);
        } else {
            for (i = 0; i < 6; i++) {
                if (f0D1C_0670(i, 3)) {
                    sprintf(gFileNameBuf, "%s is hit by a cause serious wounds spell!\r",
                            (char far *)gChars[i].name);
                    DrawText(gFileNameBuf);
                    DamageCharacter(i, RollDice(2, 8, 1));
                    break;
                }
            }
            PlaySoundAtCell(0x6c, obj->id);
        }
        break;
    case 4:
        Obj073F(obj, 9);
        PlaySoundAtCell(0x1f, obj->id);
        break;
    case 14:
        arg = g3226[obj->m19];
        PlaySoundAtCell(g322F[obj->m19], obj->id);
        Obj073F(obj, arg);
        break;
    case 15:
        item = CopyItemToFreeSlot(0x3c);
        if (item != 0) {
            if (AddDDEffect(-1, item, obj->id, obj->frame, obj->orient,
                            gTable8DD2[item].m02) == 0)
                gTable8DD2[item].m04 = -1;
        }
        break;
    case 17:
        PlaySoundAtCell(0x53, obj->id);
        DrawText("The party is hit with a psychic mind blast!\r");
        for (i = 0; i < 6; i++) {
            if (f0D1C_0670(i, 3)) {
                ApplyStatusEffect(i, 4, "paralyzed", 1, 5, 9, 1);
                RedrawMember(i);
            }
        }
        break;
    case 16:
        StartCellEffect(0xff, 0, obj->id, obj->frame, obj->orient);
        PlaySoundAtCell(0x55, obj->id);
        break;
    }
    DrawViewFrame(1);
    gDF4C = 0;
    if (obj->m19 != -1)
        obj->m19--;
    obj->m18 = 5;
    return 1;
}
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
/* 08E6:0B94: turn an object by `arg` quarters. A state below zero falls straight to the second half;
   a state of zero first steps the object through StepObjectOrient(obj, -arg). It then looks up the
   direction dir = (orient + arg) & 3, cur = f0D1C_00DA(id, dir) and the byte g1991[dir]: when the
   g194B record at gB894[cur * 9 + g1991[dir]] has bit 2 clear it leaves a state of zero as -1 and
   returns, and a nonzero state with that bit clear falls through to the second half. Otherwise it
   turns through TurnObject(obj, -1, dir), sets the state to -1 and returns. The second half looks up
   handle = f0D1C_00DA(id, orient): when TurnObject(obj, handle, -1) fails it turns the other way,
   TurnObject(obj, -1, (orient - arg) & 3), and clears the state, else it sets the state to 1.
   Called by UpdateObjects. Purpose unknown: what g1991 and the state values select. The unit's -B
   flag is what keeps the two separate `obj->state = -1` stores at 0B94:0083 and 0B94:00A4 that a -O
   build cross-jumps. */
/*@ 08E6:0B94 */ void Obj0B94(struct Obj28 far *obj, int arg)
{
    int handle;
    int cur;
    int add;
    register int dir;
    register int bits;

    if (obj->state >= 0) {
        if (obj->state == 0)
            StepObjectOrient(obj, -arg);
        dir = (obj->orient + arg) & 3;
        cur = f0D1C_00DA(obj->id, dir);
        add = g1991[dir];
        bits = g194B[(signed char)gB894[cur * 9 + add]] & 4;
        if (obj->state == 0) {
            if (bits == 0)
                obj->state = -1;
            return;
        }
        if (bits == 0)
            goto second;
        TurnObject(obj, -1, dir);
        obj->state = -1;
        return;
    }
second:
    handle = f0D1C_00DA(obj->id, obj->orient);
    if (TurnObject(obj, handle, -1) == 0) {
        dir = (obj->orient - arg) & 3;
        TurnObject(obj, -1, dir);
        obj->state = 0;
    } else {
        obj->state = 1;
    }
}
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
        DrawViewFrame(1);
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

    DrawViewFrame(1);
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
   0E48, 0FBE and 1410. `hit` is declared register so it takes SI and `roll` DI, as the original. */
/*@ 08E6:0EC3 */ void Obj0EC3(struct Obj28 far *obj)
{
    int roll;
    register int hit;

    roll = obj->frame;
    if (roll == 4)
        roll = RollDice(1, 4, 0xff);
    if (obj->m12 != 0 && RollDice(1, 2, 0) == 1) {
        hit = CopyItemToFreeSlot(obj->m12);
        InsertListItem(&gB894[obj->id * 9 + 5], obj->id, hit, roll);
    }
    if (obj->m14 != 0) {
        hit = CopyItemToFreeSlot(obj->m14);
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
   SubtractInts(the object's record m01, the character's m1D). Called by 10F3 and 0EF3:0A3F. */
/*@ 08E6:106B */ int Obj106B(struct Obj28 far *obj, int who)
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
/* 08E6:10F3: resolve a type-0 object's attack on the party. It takes the current page and the
   object's orientation to select one of the four 12-slot groups of gTable325B, then picks the
   group's first or second half with gTable328B[orientation * 4 + frame] (frame 4 rolls 1d2 - 1 to
   choose). It scans the six slots of that half for the first party member whose gChars m01 bit 0
   is set, whose m1D is above -10, and who passes a 1d20 < 5 save when its mE3 has bit 2, and
   remembers that member. It then rolls the object record's m03 attack routines through Obj106B,
   adding RollDice(m04[i][0], m04[i][1], m04[i][2]) damage per hit, until 0EF3:18A2 on the member
   negates the total. When the total is positive it reports a loss (m0E bit 7), damages the member,
   applies "poisoned" (m0E bit 4, clearing mE3 bit 7) and "paralyzed" (m0E bit 5). Called by 05EA. */
/*@ 08E6:10F3 */ void Obj10F3(struct Obj28 far *obj)
{
    int index;
    int total;
    int group;
    int roll;
    int end;
    int attacks;
    register int i;
    register int who;
    struct CharRec far *member;

    index = obj->b0;
    group = gTable324B[gFBCC * 4 + obj->orient];
    who = group * 12;
    if (obj->frame != 4)
        roll = gTable328B[obj->orient * 4 + obj->frame];
    else
        roll = RollDice(1, 2, 0xff);
    if (roll != 0)
        who += 6;
    end = who + 6;
    for (i = who; i < end; i++) {
        member = &gChars[gTable325B[i]];
        if ((member->m01 & 1) && member->hp > -10) {
            if ((member->mE3 & 4) == 0 || RollDice(1, 0x14, 0) < 5) {
                who = gTable325B[i];
                break;
            }
        }
    }
    if (i == end)
        return;
    total = 0;
    attacks = gTable1A3B[index].m03;
    for (i = 0; i < attacks; i++) {
        if (!Obj106B(obj, who))
            continue;
        total += RollDice(gTable1A3B[index].m04[i][0],
                          gTable1A3B[index].m04[i][1],
                          gTable1A3B[index].m04[i][2]);
        if (TickBandageCountdown(member) != 0)
            total = 0;
    }
    if (total <= 0)
        return;
    if (gTable1A3B[index].m0E & 0x80)
        ReportLostItem(who);
    DamageCharacter(who, total);
    if (gTable1A3B[index].m0E & 0x10) {
        ApplyStatusEffect(who, 2, "poisoned", 0, 1, 8, 1);
        gChars[who].mE3 &= ~0x80L;
    }
    if (gTable1A3B[index].m0E & 0x20)
        ApplyStatusEffect(who, 4, "paralyzed", 2, 5, 9, 1);
}
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
/* 08E6:1410: the periodic effect tick. Mode 1 (gF51B == 1) fires when the deadline gDeadline is
   behind g8492 (gDeadline < g8492); mode 2 fires when gF540 has reached gFB32; mode 3 fires when
   either holds; any other mode does nothing. Firing runs the 122E entry with (0, 0x20), then, when
   no object of the 30 (from index 1) has a count left, runs 0EC3 on the first object whose
   17A6:0F61(id, gPartyCell) is over 3. Either way it resets the deadline to g8492 + gF510 and
   clears gF540. Called by 1414:04E7 and overlay 6. The unit's -B flag is what keeps the mode-3
   low-word branch at 1410:005A the right way round that a -O build inverts. */
/*@ 08E6:1410 */ void Obj1410(void)
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
    RunMemberScript(0, 0x20);
    any = 0;
    for (i = 1; i < 30; i++) {
        any += gObjs[i].count <= 0;
    }
    if (any == 0) {
        for (i = 1; i < 30; i++) {
            if (Distance(gObjs[i].id, gPartyCell) > 3) {
                Obj0EC3(&gObjs[i]);
                break;
            }
        }
    }
    gDeadline = g8492 + gF510;
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
   finds; otherwise the first object 189D accepts for (key, field). Called by 0D1C:0A43. The unit's
   -B flag is what keeps the two identical `*list++` tails at 17FD:0058 and 17FD:0082 apart that a -O
   build cross-jumps. */
/*@ 08E6:17FD */ void Obj17FD(int far *list, int key, int field, char flags, char scanAll, char scanFiltered)
{
    struct Obj28 far *obj;
    int i;

    if (scanAll) {
        for (i = 0, obj = gObjs; i < 30; i++, obj++) {
            if (obj->id == key)
                *list++ = i;
        }
    } else {
        if (scanFiltered) {
            *list++ = Obj1775(key, field, flags);
        } else {
            for (i = 0, obj = gObjs; i < 30; i++, obj++) {
                if (Obj189D(obj, key, field)) {
                    *list++ = i;
                    break;
                }
            }
        }
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
