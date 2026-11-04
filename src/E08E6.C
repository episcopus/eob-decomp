/* Unit at EOB.EXE 08E6:000B (6331 bytes, 36 functions); the monster update dispatch and the
   per-mode behaviour routines of the monsters: UpdateMonsters walks the 30 28-byte monster
   records at ds:8A82 (struct Monster, indexed by the 27-byte monster-type records at ds:1A3B, struct
   MonsterType) and dispatches each live monster by its mode (+08) to the 00C3/03FE/0AB8/0B34/0B94/0CA6
   behaviours. All 36 functions are at L2. The unit is compiled with -v (debug information): that is
   what keeps the identical jump tails of 0765, 0B94, 17FD and 1410 that a -O build without it merges
   (whitepaper 2.1.1).
   Whitespace and empty-body stubs keep the siblings in address order. */

/*@flags -ml -O -Z -v */

struct Monster {              /* 28-byte records, 30 of them at ds:8A82 */
    char typeIndex;                /* +0: index into the ds:1A3B monster-type table */
    signed char updateGroup;       /* +1: the update group UpdateMonsters(kind) matches: InitMonster
                               stores the level file's group * 2 plus the slot's parity, and
                               timed effects 2 to 5 (ds:5BC8) run the groups (ovl3:0A7C) */
    int cell;                 /* +2: the monster's map cell */
    char cellPos;             /* +4: where in its cell it stands: 0..3 a quarter, 4 the centre (058A, 0EC3) */
    char orient;            /* +5: 0..3, its facing N E S W (times 2 is a DirectionFromPoints direction) */
    char animPhase;            /* +6: the walk animation phase ovl4:0D1B picks the shape row by
                               (+06 << 4); flipped each update unless the mode is 4 or 7 (0CC8) */
    char altShapes;               /* +7: nonzero draws it from its second set of 18 shapes (ovl4:0D1B adds 0x12) */
    signed char mode;       /* +8: the behaviour mode UpdateMonsters switches on: 0 hunts its
                               target cell, 1..3 walk, 4 stands, 5 and 6 follow a wall, 7 and 10
                               wait out the timer, 8 wakes with its group (0055), 9 is never
                               alerted (00C3) */
    signed char state;      /* +9: StepMonsterAlongWall's state (-1, 0, 1); StepMonsterToward resets it when over 6 */
    char attackFrame;               /* +A: the attack frame: 0xfe then 0xff while 05EA draws a melee swing, shown by ovl4:0D1B
                               as shapes 5 and 6 in the nearest row; else 0 */
    char timer;             /* +B: counted down by TickMonsterTimer in modes 7 and 10; the hold spells set 15 with
                               mode 10 (0EF3:1530, 0EF3:1A30) */
    int maxHp;          /* +C: the starting hit points, the roll InitMonster also gives +E; not read in src/ */
    int hp;              /* +E: the current hit points: DamageMonster subtracts from them, 0 or less means dead */
    int targetCell;                /* +10: the target cell the monster heads for (0AB8 passes it to 0430) */
    int dropItem1;                /* +12: an item id dropped at death (0EC3 rolls 1d2, copies it on a 1) */
    int dropItem2;                /* +14: a second item id dropped at death (0EC3 copies it with no roll) */
    char flags;               /* +16: flags: 1 attacked (0FBE, 03FE), 2 hit flash (0DCA), 4 swing phase (05EA),
                               8 fleeing (000B, 0430), 0x10 turn undead tried (0D1C:0A43) */
    char drawOffset;               /* +17: a draw offset, x in the high nibble and y in the low (ovl4:0D1B); 1414:0CE7 re-rolls it */
    char actionDelay;               /* +18: the action delay TryMonsterRangedAttack counts down, reset to 5 */
    char actionsLeft;               /* +19: the remaining action count TryMonsterRangedAttack decrements; 131D sets it from the record's m18 */
    char m1A;               /* +1A: Purpose unknown: not read by this unit */
    char m1B;               /* +1B: Purpose unknown: not read by this unit */
};

struct MonsterType {            /* 27-byte records at ds:1A3B, indexed by Monster.typeIndex (src/E0D1C.C) */
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
    unsigned char status;   /* +01: status bits (src/E0DE8.C) */
    char name[0x0B];        /* +02: the character's name, NUL padded (src/E0DE8.C) */
    char pad0D[0x1B - 0x0D];
    signed char hp;         /* +1B: current hit points (src/E0DE8.C) */
    char pad1C;
    signed char armorClass; /* +1D: the armour-class byte (src/E0DE8.C) */
    char pad1E[0xE3 - 0x1E];
    unsigned long effectFlags;      /* +E3: effect flags, a 32-bit mask (src/E0DE8.C) */
    char padE7[0xF3 - 0xE7];
};

struct ItemRecord {
    signed char nameIndex;        /* +0 (ds:8DD0): indexes the item-name table at segment 0x1F28 */
    signed char identifiedNameIndex;        /* +1 (ds:8DD1): the second name index */
    unsigned char flags;    /* +2 (ds:8DD2): bit 0x20 gates 0BD2:0AF2/0C8E, 0x40 is set by
                               0BD2:040E and tested by 0BD2:05B5; src/E07A9.C calls it flags */
    signed char icon;       /* +3 (ds:8DD3): the icon number */
    signed char type;        /* +4 (ds:8DD4): the item type */
    signed char cellPos;        /* +5 (ds:8DD5): 01D0 stores its 4th argument here and 02B9 matches it
                               against its `key`. No caller shows a sound (01D0's callers pass a
                               slot kind, E0ACD.C:561-568, a roll, E08E6.C:807, or an effect kind | 4,
                               0BD2:07DE), so Purpose unknown: a per-entry tag */
    int key;                /* +6 (ds:8DD6): the key; -1 when the record is free */
    int prevLink;                /* +8 (ds:8DD8): one link of the circular list; 0BD2:01D0 splices a
                               new item in after this record through it */
    int nextLink;                /* +0xa (ds:8DDA): the other link; the list walks (0BD2:0385, 040E,
                               04AA) follow it */
    signed char level;        /* +0xc (ds:8DDC): gCurLevel or -1, set by 01D0 */
    signed char value;        /* +0xd (ds:8DDD): the item's bonus or effect value: the signed bonus
                               printed as "%s +%d" and "Cursed %s %d", the spell index into ds:1278
                               and ds:12E0, the wand effect index gWandSpells[m0B], and the scroll and
                               potion values (0BD2:05B5, 0D2D, 0EE8, 1001) */
};

extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct ItemRecord gItems[]; /*@ ds:8DD0 */

extern struct Monster gMonsters[]; /*@ ds:8A82 */
extern signed char gTurnForMode1To3[]; /*@ ds:3237 */
extern signed char gTurnForMode5To6[]; /*@ ds:3234 */
extern signed char gMonsterFrontPos[]; /*@ ds:323B */
extern unsigned char gMaze[]; /*@ ds:B894 */   /* nine-byte records per map cell: +0..+3 the walls
                                                  by side, +4 bits 0..2 the monster count, +5 the item
                                                  list head (ds:B898 and ds:B899 are cell 0's; cell
                                                  n's record starts at n * 9) */
extern struct MonsterType gMonsterTypes[]; /*@ ds:1A3B */
extern signed char gWallTypeFlags[]; /*@ ds:194B */
extern signed char gDirCellOffset[]; /*@ ds:1790 */
extern int gDirSearchLeftFirst; /*@ ds:321E */
extern int gDirSearchRight[]; /*@ ds:31FE */
extern int gDirSearchLeft[]; /*@ ds:320E */
extern char gFrontLeftPosByDir[]; /*@ ds:31F2 */
extern char gPosSearchOrder[]; /*@ ds:31F6 */
extern int gPartyCell; /*@ ds:FBCA */   /* the party's map cell (src/E177D.C:92): this unit reads
                                           the word as a cell at :219, :222, :334, :403, :469, :545,
                                           :549 and :1012. MoveParty (122E:0001) sets it on every
                                           step (settled 2026-11-03, playbook 65) */
extern char gQuietMonsterUpdate; /*@ ds:F442 */
extern unsigned long gGameFlags; /*@ ds:FC04 */
extern unsigned long gFrameWaitTicks; /*@ ds:849A */
extern int gTimerEnableFlags; /*@ ds:849E */
extern char gViewDirty; /*@ ds:DF4C */
extern int gDeferMonsterHits; /*@ ds:F518 */
extern int gHpMinOrHitFlag; /*@ ds:DD10 */
extern int gGraphicsChoice; /*@ ds:8102 */
extern char gHitFlashRemap[]; /*@ ds:1CD3 */
extern char gHitFlashRemapCga[]; /*@ ds:1CE3 */
extern void (far *gSetRemapTable)(char far *map); /*@ ds:80D2 */
extern char gNearSideByDir[]; /*@ ds:1991 */
extern char gProjectileHitOrder[]; /*@ ds:32C3 */
extern char gTargetPosOrder[]; /*@ ds:329B */
extern char gNearPosPair[]; /*@ ds:32BB */
extern signed char gAttackSideByDir[]; /*@ ds:324B */   /* (gPartyDir, facing) to the side of the party attacked */
extern signed char gAttackTargetOrder[]; /*@ ds:325B */   /* per side, two halves of six party members in attack order */
extern signed char gRightSidePosByDir[]; /*@ ds:328B */   /* (facing, cellPos) to the half of the side group */
extern int gPartyDir; /*@ ds:FBCC */
extern unsigned char gPartySpellFlags; /*@ ds:FBD2 */
extern unsigned long gDeadline; /*@ ds:F512 */   /* a 32-bit deadline at ds:F512:F514 */
extern unsigned long gGameTicks; /*@ ds:8492 */       /* the 32-bit tick counter (src/E0ACD.C) */
extern int gLevelTimerPeriod; /*@ ds:F510 */
extern char gLevelTimerMode; /*@ ds:F51B */
extern int gStepCount; /*@ ds:F540 */
extern int gLevelTimerSteps; /*@ ds:FB32 */

extern void UpdateDDEffects(void); /*@ 1414:0D63 */
extern void StartCellEffect(char who, int mode, int cell, int m06, int m07); /*@ 0EF3:097B */
extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int Distance(int x, int y); /*@ 17A6:0F61 */
extern int MonsterResistsMagic(int obj); /*@ 0D1C:0BAB */
extern int GetSavingThrowTarget(char index, char limit, char offset); /*@ 0A72:0006 */
extern int SubtractInts(int minuend, int subtrahend); /*@ 0A72:058C */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void AddPartyExperience(int amount); /*@ 0DE8:0C2B */
extern void RunCellScript(int cell, int mode); /*@ 122E:1DA8 */
extern void WinnerPanel(void); /*@ 211E:007F */   /* overlay 5 entry, ovl5:160A */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void far *memset(void far *dst, int c, unsigned n); /*@ 0000:6384 */

void KillMonster(struct Monster far *obj);   /* forward: FinishDeferredHits reaches it before its definition */
/* two packed map cells (y * 32 + x); returns a direction 0..7 or -1 */
extern int DirectionFromPoints(int point, int cell); /*@ 17A6:0314 */
extern int IsCellInView(int value); /*@ 0D1C:0006 */
extern int StepCell(int cell, int dir); /*@ 0D1C:00DA */
extern void DrawViewFrame(int n); /*@ 2116:005C */   /* overlay 4 entry, ovl4:1F51 */
extern void PlaySoundAtCell(int sound, int cell); /*@ 177D:0106 */
extern void PlaySoundDefault(int sound); /*@ 177D:00F3 */
extern int MemberIs(int who, int mask); /*@ 0D1C:0670 */
extern void DamageCharacter(int who, int amount); /*@ 0DE8:09B9 */
extern void ApplyStatusEffect(int who, int mask, char far *text, int attack, int count, int value, int flag); /*@ 0DE8:101A */
extern void RedrawMember(int dir); /*@ 1547:0991 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern int AddDDEffect(int who, int item, int cell, int kind, int page, int type); /*@ 0BD2:07DE */
extern char gFileNameBuf[]; /*@ ds:FE6E */
extern signed char gBossSpellModes[]; /*@ ds:3220 */
extern signed char gBossSpellSounds[]; /*@ ds:3223 */
extern signed char gSpellSequenceModes[]; /*@ ds:3226 */
extern signed char gSpellSequenceSounds[]; /*@ ds:322F */
extern int TickBandageCountdown(struct CharRec far *who); /*@ 0EF3:18A2 */
extern void ReportLostItem(int who); /*@ 0BD2:000E */
void ResolveMonsterMelee(struct Monster far *obj);   /* forward: 05EA reaches it before its definition */

/* 08E6:000B: make a monster flee, unless its mode is 7, 8 or 0xa: set the mode to 0, face it away
   from the party ((DirectionFromPoints(cell, gPartyCell) ^ 4) >> 1) and set m16 bit 3, which makes
   StepMonsterToward (0430) head away from its target. Called by 0EF3:1702 CastFear. */
/*@ 08E6:000B */ void MakeMonsterFlee(struct Monster far *obj)
{
    if (obj->mode == 7 || obj->mode == 8 || obj->mode == 0xa)
        return;
    obj->mode = 0;
    obj->orient = (DirectionFromPoints(obj->cell, gPartyCell) ^ 4) >> 1;
    obj->flags |= 8;
}

/* 08E6:0055: wake the mode-8 monsters: set every one to mode 0 with the party cell as its target
   cell (targetCell), remembering the last one; when that monster's typeIndex is 7 set bit 18 of
   gGameFlags, when it is 0xc set bit 27. Called by 03FE, when a mode-8 monster was attacked, and
   0EC3, when one dies. Purpose unknown: what the two gGameFlags bits gate. `last` is set only inside
   the loop, but both callers test mode == 8 first, so one monster is always found. */
/*@ 08E6:0055 */ void AlertMode8Monsters(void)
{
    struct Monster far *obj, *last;
    int i;

    for (obj = gMonsters, i = 0; i < 30; i++, obj++) {
        if (obj->mode == 8) {
            obj->mode = 0;
            obj->targetCell = gPartyCell;
            last = obj;
        }
    }
    if (last->typeIndex == 7)
        gGameFlags |= 0x40000L;
    if (last->typeIndex == 0xc)
        gGameFlags |= 0x8000000L;
}

/* 08E6:00C3: alert a monster to the party. For a monster whose mode is not 7, 8, 9 or 0xa and
   that has hit points left: when Distance(cell, gPartyCell) is under 4, and the party is not behind
   it (DirectionFromPoints(cell, gPartyCell) minus twice the facing, minus 3, wrapped into 0..7, is
   over 2; 0..2 are the three directions at its back) or is under 2 away, set mode 0 and the party
   cell as its target cell (targetCell). Called by 0150 and UpdateMonsters. The limit tests are an early-return `||` chain,
   not an `&&` chain:
   that is what makes TCC reload `les bx,[bp+6]` at 08E6:00F2 for the Distance argument (a statement
   after a `||` chain is a reload point), matching the original; the `&&` form reuses the bx the five
   tests already hold and comes out 3 bytes short. */
/*@ 08E6:00C3 */ void AlertMonsterToParty(struct Monster far *obj)
{
    int first, dir;

    if (obj->mode == 7 || obj->mode == 8 || obj->mode == 9 || obj->mode == 0xa
            || obj->hp <= 0)
        return;
    first = Distance(obj->cell, gPartyCell);
    if (first >= 4)
        return;
    dir = DirectionFromPoints(obj->cell, gPartyCell) - obj->orient * 2 - 3;
    if (dir < 0)
        dir += 8;
    if (dir <= 2 && first >= 2)
        return;
    obj->mode = 0;
    obj->targetCell = gPartyCell;
}

/* 08E6:0150: run AlertMonsterToParty over every one of the 30 monsters in gMonsters */
/*@ 08E6:0150 */ void AlertNearbyMonsters(void)
{
    struct Monster far *obj;
    int i;

    for (obj = gMonsters, i = 0; i < 30; i++, obj++)
        AlertMonsterToParty(obj);
}

/* 08E6:017E: the cell positions taken in map cell `id`: OR together one bit (1 << cellPos) for
   each of the first (gMaze[id * 9 + 4] & 7) monsters in the cell; -2 when that count is 4 (the cell
   is full), -1 when one of their ds:1A3B records has m15 set, and a monster in the centre (cellPos
   4) first moves to gFrontLeftPosByDir[orient]. */
/*@ 08E6:017E */ int BuildCellPosMask(int id)
{
    int count;
    struct Monster far *obj;
    int i, bits;

    count = gMaze[id * 9 + 4] & 7;
    if (count == 4)
        return -2;
    bits = 0;
    for (obj = gMonsters, i = 0; i < 30; i++, obj++) {
        if (obj->cell == id) {
            if (gMonsterTypes[obj->typeIndex].m15)
                return -1;
            if (obj->cellPos == 4)
                obj->cellPos = gFrontLeftPosByDir[obj->orient];
            bits |= 1 << obj->cellPos;
            if (--count == 0)
                break;
        }
    }
    return bits;
}

/* 08E6:0218: a free cell position for the monster in map cell `id`: walk the four bytes of
   gPosSearchOrder + (orient & 1) * 4 and return the first `orient ^ *row` (the facing XOR the table
   byte, not the table byte) whose bit BuildCellPosMask(id) does not have; -1 when the monster's
   record's m15 is set, the mask is negative (the cell is full) or no position is free. Called by
   0309 and 058A. */
/*@ 08E6:0218 */ int FindFreeCellPos(struct Monster far *obj, int id)
{
    int bit;
    char far *row;
    int bits, i;

    if (gMonsterTypes[obj->typeIndex].m15)
        return -1;
    bits = BuildCellPosMask(id);
    if (bits < 0)
        return -1;
    row = gPosSearchOrder + (obj->orient & 1) * 4;
    for (i = 0; i < 4; i++, row++) {
        bit = (char)(obj->orient ^ *row);
        if (!(bits & (1 << bit)))
            return bit;
    }
    return -1;
}

/* 08E6:0297: move a monster to map cell `id` and facing `orient`: keep the monster counts in gMaze
   (+4, dropping the old cell's when bits 0..2 are set) and report the cells to IsCellInView. Either
   argument may be -1 to leave that field alone; 0EC3 passes cell 0 for a dead monster. Called by
   0309, 0EC3 and 131D (and 0BD2, 122E). */
/*@ 08E6:0297 */ void MoveMonsterToCell(struct Monster far *obj, int id, int orient)
{
    if (id != -1) {
        IsCellInView(obj->cell);
        if (gMaze[obj->cell * 9 + 4] & 7)
            gMaze[obj->cell * 9 + 4]--;
        obj->cell = id;
        gMaze[id * 9 + 4]++;
    }
    if (orient != -1) {
        obj->orient = orient;
        id = obj->cell;
    }
    IsCellInView(id);
}

/* 08E6:0309: move a monster into map cell `id` facing `orient`, or only turn it. When `id` is not
   -1 and differs from its own cell, read the cell's wall on the side facing back (+orient^2): when
   gWallTypeFlags of that byte has no bit 2 (no monster may pass), or `id` is the party's cell, fail
   (0). When the cell already holds monsters (+4 bits 0..2) and `id` is nonzero, take the cell
   position FindFreeCellPos picks (fail if none) and move the monster to (id, orient); for a plain
   turn (`id` is its own cell or -1) only `orient` is applied when it is not -1. Then report the old
   cell to IsCellInView and, unless gQuietMonsterUpdate is set, play the type's m17 sound at its cell
   through PlaySoundAtCell. Returns 1 on success. */
/*@ 08E6:0309 */ int TryMoveMonster(struct Monster far *obj, int id, int orient)
{
    char far *rec;
    char dir;
    char frame;
    int oldId;

    oldId = obj->cell;
    if (obj->cell != id && id != -1) {
        rec = (char far *)&gMaze[id * 9];
        if (orient == -1)
            orient = obj->orient;
        dir = orient ^ 2;
        if ((gWallTypeFlags[rec[dir]] & 4) == 0)
            return 0;
        if (id == gPartyCell)
            return 0;
        if ((rec[4] & 7) && id != 0) {
            frame = FindFreeCellPos(obj, id);
            if (frame == -1)
                return 0;
            obj->cellPos = frame;
        }
        MoveMonsterToCell(obj, id, orient);
        orient = -1;
    }
    if (orient != -1)
        obj->orient = orient;
    IsCellInView(oldId);
    if (!gQuietMonsterUpdate)
        PlaySoundAtCell(gMonsterTypes[obj->typeIndex].m17, obj->cell);
    return 1;
}

/* 08E6:03FE: a monster that was attacked (m16 bit 0, set by 0FBE and 0DE8:0007) and whose mode is
   not 0xa goes after the party: a mode-8 monster wakes its whole group through 0055, any other is set
   to mode 0 with the party cell as its target cell (targetCell). Called by UpdateMonsters. */
/*@ 08E6:03FE */ void ChasePartyIfAttacked(struct Monster far *obj)
{
    if ((obj->flags & 1) && obj->mode != 0xa) {
        if (obj->mode == 8) {
            AlertMode8Monsters();
        } else {
            obj->mode = 0;
            obj->targetCell = gPartyCell;
        }
    }
}
/* 08E6:0430: step a monster toward cell `value`, in the direction DirectionFromPoints(id, value)
   reports (away from it while m16 bit 3, fleeing, is set). When the monster's
   state is over 6 it resets it to 0 and toggles gDirSearchLeftFirst, which selects the direction table gDirSearchRight or
   gDirSearchLeft. The target -1 leaves the monster alone; bit 3 of m16 XORs 4 into it. When the target is even
   and id + gDirCellOffset[target >> 1] equals gPartyCell, the object takes one step through TryMoveMonster with
   -1: `dir` becomes orientation + 1 when the difference from 2*orient is nonzero and under 5,
   orientation - 1 when it is nonzero and not under 5, and is left at orient * 2 when the difference
   is 0, so the `dir &= 3` (:406) then wraps orient * 2 mod 4, not the orientation (QUIRK
   08E6:04E5). Otherwise the difference picks a direction and the loop walks the eight orientations
   from 7 down to 0, adding the table byte, skipping odd directions, and turning through
   TryMoveMonster(obj, StepCell(id, dir >> 1), dir >> 1); the first turn that succeeds returns. Called
   by 0AB8. */
/*@ 08E6:0430 */ void StepMonsterToward(struct Monster far *obj, int value)
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
        gDirSearchLeftFirst ^= 1;
    }
    table = gDirSearchRight;
    if (gDirSearchLeftFirst != 0)
        table = gDirSearchLeft;
    dir = obj->orient * 2;
    id = obj->cell;
    target = DirectionFromPoints(id, value);
    if (target == -1)
        return;
    if (obj->flags & 8)
        target ^= 4;
    diff = (target - dir) & 7;
    if (!(target & 1) && id + gDirCellOffset[target >> 1] == gPartyCell) {
        if (diff != 0 && diff < 5)
            dir = obj->orient + 1;
        else if (diff != 0)
            dir = obj->orient - 1;
        dir &= 3;
        TryMoveMonster(obj, -1, dir);
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
            handle = StepCell(id, dir >> 1);
        }
        if (handle != 0 && TryMoveMonster(obj, handle, dir >> 1) != 0)
            return;
    }
}
/* 08E6:058A: give a monster a new cell position: the centre (4) when it is alone in its cell, else
   the free position FindFreeCellPos picks (none: no change); then report its cell to IsCellInView.
   Called by 05EA when the monster faces the party's cell but IsMonsterAtFront is 0. */
/*@ 08E6:058A */ void RepickMonsterCellPos(struct Monster far *obj)
{
    int id;

    id = obj->cell;
    if ((gMaze[id * 9 + 4] & 7) == 1) {
        obj->cellPos = 4;
    } else {
        id = FindFreeCellPos(obj, id);
        if (id < 0)
            return;
        obj->cellPos = id;
    }
    IsCellInView(obj->cell);
}
/* 08E6:05EA: the monster's melee attempt on the cell ahead, `handle` (-1: recomputed as
   StepCell(cell, orient)). When that is not the party's cell nothing happens and 0 is returned. When
   IsMonsterAtFront(obj) is 0 the monster takes the cell position 058A picks and 1 is returned. Else
   m16 bit 2 toggles, so it swings every other update, and while it is clear 1 is returned. A swing
   in view (IsCellInView) plays the type's m16 sound (before the frames for typeIndex 4, after them
   otherwise) and shows the attack frames 0xfe and 0xff (+0A) through the overlay-4 entry; out of
   view it only plays the sound. Then 10F3 resolves the attack, and in view the attack frame clears,
   the animation phase flips, gViewDirty is set, gTimerEnableFlags bit 1 is set and the countdown is
   8. Returns 1. Called by 0AB8. */
/*@ 08E6:05EA */ int TryMonsterMelee(struct Monster far *obj, int handle)
{
    register int found;

    if (handle == -1)
        handle = StepCell(obj->cell, obj->orient);
    if (handle != gPartyCell)
        return 0;
    if (IsMonsterAtFront(obj) != 0) {
        obj->flags ^= 4;
        if (obj->flags & 4) {
            found = IsCellInView(obj->cell);
            if (found != 0) {
                gTimerEnableFlags &= ~2;
                if (obj->typeIndex == 4)
                    PlaySoundDefault(gMonsterTypes[obj->typeIndex].m16);
                obj->attackFrame = 0xfe;
                gFrameWaitTicks = 0;
                DrawViewFrame(1);
                obj->attackFrame = 0xff;
                if (obj->typeIndex != 4)
                    PlaySoundDefault(gMonsterTypes[obj->typeIndex].m16);
                gFrameWaitTicks = 8;
                DrawViewFrame(1);
            } else {
                PlaySoundDefault(gMonsterTypes[obj->typeIndex].m16);
            }
            ResolveMonsterMelee(obj);
            if (found != 0) {
                obj->attackFrame = 0;
                obj->animPhase ^= 1;
                gViewDirty = 1;
                gTimerEnableFlags |= 2;
                gFrameWaitTicks = 8;
            }
        }
    } else {
        RepickMonsterCellPos(obj);
    }
    return 1;
}
/* 08E6:073F: the monster casts: start the spell effect `value` through StartCellEffect (0EF3:097B)
   from its cell, cell position and facing, with caster 0xff where the party's own casts pass the
   casting member (0EF3:1A0F CastHoldMonster). Called by 0765 as LaunchMonsterSpell(obj, value). */
/*@ 08E6:073F */ void LaunchMonsterSpell(struct Monster far *obj, int value)
{
    StartCellEffect(0xff, value, obj->cell, obj->cellPos, obj->orient);
}
/* 08E6:0765: the monster's ranged attack for UpdateMonsters. It returns 0 unless its 1A3B record
   has m0E bit 6, it has actionsLeft left and actionDelay is zero (otherwise actionDelay is decremented
   and 0 is returned). It then checks Distance(id, gPartyCell) is at most 3 and DirectionFromPoints(id,
   gPartyCell) equals orient * 2, and walks StepCell(id, orient), following it until it reaches
   gPartyCell; each step must have gWallTypeFlags[gMaze[value * 9 + (orient ^ 2)]] bits 0..1 set and no
   gMaze[value * 9 + 4] count, else 0 is returned. The action then switches on typeIndex: 4 starts a cell
   effect at 9 through LaunchMonsterSpell and plays 0x1f; 14 plays gSpellSequenceSounds[actionsLeft] and starts a cell effect through
   LaunchMonsterSpell(obj, gSpellSequenceModes[actionsLeft]); 15 frees an item 0x3c and queues AddDDEffect(-1, item, id, frame,
   orient, gItems[item].type), freeing it when that fails; 16 calls 0EF3:097B(0xff, 0, id, frame,
   orient) and plays 0x55; 17 plays 0x53, announces the mind blast and paralyzes every affected
   member; 21 rolls 1d4 and, under 3, starts a cell effect through LaunchMonsterSpell and plays gBossSpellSounds[roll]
   after gBossSpellModes[roll], else announces the wounds spell and hits the first affected member. Every
   action then runs the overlay-4 entry, clears gViewDirty, decrements actionsLeft unless it is 0xff and sets actionDelay
   to 5, returning 1. Called by 0AB8. The unit's -v flag is what keeps case 21's
   PlaySoundAtCell(0x6c, id) at 08E6:0907 inline instead of tail-merging it into the shared block at
   08E6:0A5E. */
/*@ 08E6:0765 */ int TryMonsterRangedAttack(struct Monster far *obj)
{
    struct MonsterType far *rec;
    int value;
    int orient;
    int arg;
    int item;
    int roll;
    int i;

    rec = &gMonsterTypes[obj->typeIndex];
    if ((rec->m0E & 0x40) == 0 || obj->actionsLeft == 0)
        return 0;
    if (obj->actionDelay != 0) {
        obj->actionDelay--;
        return 0;
    }
    if (Distance(obj->cell, gPartyCell) > 3)
        return 0;
    if (DirectionFromPoints(obj->cell, gPartyCell) != obj->orient * 2)
        return 0;
    orient = obj->orient;
    value = StepCell(obj->cell, orient);
    while (value != gPartyCell) {
        if ((gWallTypeFlags[(signed char)gMaze[value * 9 + (orient ^ 2)]] & 3) == 0
                || (gMaze[value * 9 + 4] & 7))
            return 0;
        value = StepCell(value, orient);
    }
    switch (obj->typeIndex) {
    case 21:
        roll = RollDice(1, 4, 0xff);
        if (roll < 3) {
            arg = gBossSpellModes[roll];
            LaunchMonsterSpell(obj, arg);
            PlaySoundAtCell(gBossSpellSounds[roll], obj->cell);
        } else {
            for (i = 0; i < 6; i++) {
                if (MemberIs(i, 3)) {
                    sprintf(gFileNameBuf, "%s is hit by a cause serious wounds spell!\r",
                            (char far *)gChars[i].name);
                    DrawText(gFileNameBuf);
                    DamageCharacter(i, RollDice(2, 8, 1));
                    break;
                }
            }
            PlaySoundAtCell(0x6c, obj->cell);
        }
        break;
    case 4:
        LaunchMonsterSpell(obj, 9);
        PlaySoundAtCell(0x1f, obj->cell);
        break;
    case 14:
        arg = gSpellSequenceModes[obj->actionsLeft];
        PlaySoundAtCell(gSpellSequenceSounds[obj->actionsLeft], obj->cell);
        LaunchMonsterSpell(obj, arg);
        break;
    case 15:
        item = CopyItemToFreeSlot(0x3c);
        if (item != 0) {
            if (AddDDEffect(-1, item, obj->cell, obj->cellPos, obj->orient,
                            gItems[item].type) == 0)
                gItems[item].key = -1;
        }
        break;
    case 17:
        PlaySoundAtCell(0x53, obj->cell);
        DrawText("The party is hit with a psychic mind blast!\r");
        for (i = 0; i < 6; i++) {
            if (MemberIs(i, 3)) {
                ApplyStatusEffect(i, 4, "paralyzed", 1, 5, 9, 1);
                RedrawMember(i);
            }
        }
        break;
    case 16:
        StartCellEffect(0xff, 0, obj->cell, obj->cellPos, obj->orient);
        PlaySoundAtCell(0x55, obj->cell);
        break;
    }
    DrawViewFrame(1);
    gViewDirty = 0;
    if (obj->actionsLeft != -1)
        obj->actionsLeft--;
    obj->actionDelay = 5;
    return 1;
}
/* 08E6:0AB8: the mode-0 (hunting) monster update: run TryMonsterRangedAttack, which returns 1 when
   it acts, then TryMonsterMelee, which returns 1 when handled; on either the monster is done.
   Otherwise clear the attack frame (+0A), step it toward its target cell with StepMonsterToward(obj,
   targetCell) and run TryMonsterMelee with -1 when its record's m0E has bit 3 set. Called by
   UpdateMonsters. */
/*@ 08E6:0AB8 */ void UpdateHuntingMonster(struct Monster far *obj)
{
    int handle;

    handle = StepCell(obj->cell, obj->orient);
    if (TryMonsterRangedAttack(obj) != 0)
        return;
    if (TryMonsterMelee(obj, handle) != 0)
        return;
    obj->attackFrame = 0;
    StepMonsterToward(obj, obj->targetCell);
    if (gMonsterTypes[obj->typeIndex].m0E & 8)
        TryMonsterMelee(obj, -1);
}
/* 08E6:0B34: step the monster one cell ahead through 0309; when that fails, turn its facing by
   `delta` (mod 4) in place. Called by UpdateMonsters for modes 1 to 3 (gTurnForMode1To3 gives 2, -1
   and 1) and by 0B94. */
/*@ 08E6:0B34 */ void StepMonsterOrTurn(struct Monster far *obj, int delta)
{
    int cur;

    cur = StepCell(obj->cell, obj->orient);
    if (TryMoveMonster(obj, cur, -1) == 0) {
        obj->orient += delta;
        obj->orient &= 3;
        TryMoveMonster(obj, -1, obj->orient);
    }
}
/* 08E6:0B94: walk a monster along the wall on its `arg` side (modes 5 and 6 pass -1 and 1). A state below zero falls straight to the second half;
   a state of zero first steps the object through StepMonsterOrTurn(obj, -arg). It then looks up the
   direction dir = (orient + arg) & 3, cur = StepCell(id, dir) and the byte gNearSideByDir[dir]: when the
   gWallTypeFlags record at gMaze[cur * 9 + gNearSideByDir[dir]] has bit 2 clear it leaves a state of zero as -1 and
   returns, and a nonzero state with that bit clear falls through to the second half. Otherwise it
   turns through TryMoveMonster(obj, -1, dir), sets the state to -1 and returns. The second half looks up
   handle = StepCell(id, orient): when TryMoveMonster(obj, handle, -1) fails it turns the other way,
   TryMoveMonster(obj, -1, (orient - arg) & 3), and clears the state, else it sets the state to 1.
   Called by UpdateMonsters. gNearSideByDir[dir] is dir ^ 2 (the bytes of gOppositeSide, ds:547E), the side
   of `cur` facing back, whose bit 2 lets a monster in as in 0309. Purpose unknown: what the state
   values select. The unit's -v
   flag is what keeps the two separate `obj->state = -1` stores at 0B94:0083 and 0B94:00A4 that a -O
   build cross-jumps. */
/*@ 08E6:0B94 */ void StepMonsterAlongWall(struct Monster far *obj, int arg)
{
    int handle;
    int cur;
    int add;
    register int dir;
    register int bits;

    if (obj->state >= 0) {
        if (obj->state == 0)
            StepMonsterOrTurn(obj, -arg);
        dir = (obj->orient + arg) & 3;
        cur = StepCell(obj->cell, dir);
        add = gNearSideByDir[dir];
        bits = gWallTypeFlags[(signed char)gMaze[cur * 9 + add]] & 4;
        if (obj->state == 0) {
            if (bits == 0)
                obj->state = -1;
            return;
        }
        if (bits == 0)
            goto second;
        TryMoveMonster(obj, -1, dir);
        obj->state = -1;
        return;
    }
second:
    handle = StepCell(obj->cell, obj->orient);
    if (TryMoveMonster(obj, handle, -1) == 0) {
        dir = (obj->orient - arg) & 3;
        TryMoveMonster(obj, -1, dir);
        obj->state = 0;
    } else {
        obj->state = 1;
    }
}
/* 08E6:0CA6: count a monster's timer (+0B) down and set its mode to 0 when it reaches zero. Called
   for modes 7 and 10 by UpdateMonsters (08E6:0CC8); the hold spells set mode 10 with 15 (0EF3:1530). */
/*@ 08E6:0CA6 */ void TickMonsterTimer(struct Monster far *obj)
{
    if (obj->timer) {
        if (--obj->timer == 0)
            obj->mode = 0;
    }
}

/* 08E6:0CC8: update every live monster (hit points above 0) of update group `kind` (+01): run
   AlertMonsterToParty, ChasePartyIfAttacked when the mode is positive, then a switch on the mode
   (its jump table at 08E6:0DB4 has 11 entries, 0..10, with modes 4, 8 and 9 falling through) and
   flip the animation phase (+06) unless the mode is 4 or 7; finally run UpdateDDEffects. */
/*@ 08E6:0CC8 */ void UpdateMonsters(int kind)
{
    struct Monster far *obj;
    int i;

    for (obj = gMonsters, i = 0; i < 30; i++, obj++) {
        if (obj->updateGroup != kind || obj->hp <= 0)
            continue;
        AlertMonsterToParty(obj);
        if (obj->mode > 0)
            ChasePartyIfAttacked(obj);
        switch (obj->mode) {
        case 0:
            UpdateHuntingMonster(obj);
            break;
        case 1:
        case 2:
        case 3:
            StepMonsterOrTurn(obj, gTurnForMode1To3[obj->mode]);
            break;
        case 5:
        case 6:
            StepMonsterAlongWall(obj, gTurnForMode5To6[obj->mode]);
            break;
        case 7:
        case 10:
            TickMonsterTimer(obj);
            break;
        }
        if (obj->mode != 4 && obj->mode != 7)
            obj->animPhase ^= 1;
    }
    UpdateDDEffects();
}

/* 08E6:0DCA: flash a hit monster: report its cell to IsCellInView and, when gViewDirty is set (the
   cell is in view), clear gTimerEnableFlags bit 1, set m16 bit 1, which makes ovl4:0D1B draw it
   remapped (ds:1CD3 turns every colour but 0 into 15, ds:1CE3 is the CGA row), pass that row to the
   colour callback, reset the countdown and run the overlay-4 entry, clear the bit again, set the
   countdown to 1, set gTimerEnableFlags bit 1 and gViewDirty. Called by 0FBE. */
/*@ 08E6:0DCA */ void FlashMonsterHit(struct Monster far *obj)
{
    IsCellInView(obj->cell);
    if (gViewDirty) {
        gTimerEnableFlags &= ~2;
        obj->flags |= 2;
        if (!gGraphicsChoice)
            gSetRemapTable(gHitFlashRemapCga);
        else
            gSetRemapTable(gHitFlashRemap);
        gFrameWaitTicks = 0;
        DrawViewFrame(1);
        obj->flags &= ~2;
        gFrameWaitTicks = 1;
        gTimerEnableFlags |= 2;
        gViewDirty = 1;
    }
}

/* 08E6:0E48: draw the deferred hit flashes (the overlay-4 entry with 1), then clear m16 bit 1 of
   every monster that has it; when such a monster has no hit points left, run 0EC3. When any was found set gViewDirty and the
   countdown to 1, else clear gViewDirty; finally clear gHpMinOrHitFlag and gDeferMonsterHits. An uninitialized `any` is a
   QUIRK 08E6:0E99 of the original: only monsters with m16 bit 1 set define its value. Called by
   DamageNearRowMonsters (:1109), 0D1C:0A43 TurnUndead and 0EF3:18D8 CastConeOfCold. */
/*@ 08E6:0E48 */ void FinishDeferredHits(void)
{
    struct Monster far *obj = gMonsters;
    int i, any;

    DrawViewFrame(1);
    for (i = 0; i < 30; i++, obj++) {
        if (obj->flags & 2) {
            obj->flags &= ~2;
            any = 1;
            if (obj->hp <= 0)
                KillMonster(obj);
        }
    }
    if (any) {
        gViewDirty = 1;
        gFrameWaitTicks = 1;
    } else {
        gViewDirty = 0;
    }
    gDeferMonsterHits = gHpMinOrHitFlag = 0;
}
/* 08E6:0EC3: a monster's death: pick `roll`, the cell position its drops land on (its cellPos, or
   1d4 - 1 when it stands in the centre); when dropItem1 is set, roll 1d2 and on a 1 copy dropItem1
   through CopyItemToFreeSlot and put the item (`hit`) on its cell's item list (gMaze + cell * 9 + 5)
   through InsertListItem with the roll as the item's cell position; when dropItem2 is set, drop a
   copy the same way with no roll. Then give the party the record's m13 experience, move the monster
   to cell 0, run WinnerPanel and shut the game down when its typeIndex is 0x15, and wake the mode-8
   monsters when it was one. Called by 0E48,
   0FBE and 1410. `hit` is declared register so it takes SI and `roll` DI, as the original. */
/*@ 08E6:0EC3 */ void KillMonster(struct Monster far *obj)
{
    int roll;
    register int hit;

    roll = obj->cellPos;
    if (roll == 4)
        roll = RollDice(1, 4, 0xff);
    if (obj->dropItem1 != 0 && RollDice(1, 2, 0) == 1) {
        hit = CopyItemToFreeSlot(obj->dropItem1);
        InsertListItem(&gMaze[obj->cell * 9 + 5], obj->cell, hit, roll);
    }
    if (obj->dropItem2 != 0) {
        hit = CopyItemToFreeSlot(obj->dropItem2);
        InsertListItem(&gMaze[obj->cell * 9 + 5], obj->cell, hit, roll);
    }
    AddPartyExperience(gMonsterTypes[obj->typeIndex].m13);
    MoveMonsterToCell(obj, 0, -1);
    if (obj->typeIndex == 0x15) {
        WinnerPanel();
        ShutdownGame();
    }
    if (obj->mode == 8)
        AlertMode8Monsters();
}
/* 08E6:0FBE: damage a monster by `damage`: subtract it from its hit points (+0E), clear m16 bit 3
   (it stops fleeing), flash it through 0DCA (or, while gDeferMonsterHits is set, only mark m16 bit 1
   for 0E48 when IsCellInView finds its cell), set m16 bit 0 (attacked, read by 03FE), and when the
   hit points have run out run 0EC3 only when gDeferMonsterHits is 0 or IsCellInView returned 0. Called by
   DamageMonsterWithSave (:1090), 0D1C:09CB, 0DE8:02D4 TryRecordAttack, 0DE8:0763 UseItem, 0EF3:0A3F
   DamageTargetCell, 0EF3:0E1B CastMeleeEffect and 0EF3:18D8 CastConeOfCold. */
/*@ 08E6:0FBE */ void DamageMonster(struct Monster far *obj, int damage)
{
    int id;

    gHpMinOrHitFlag = 1;
    obj->hp -= damage;
    obj->flags &= ~8;
    if (gDeferMonsterHits == 0) {
        FlashMonsterHit(obj);
    } else {
        id = IsCellInView(obj->cell);
        if (id != 0)
            obj->flags |= 2;
    }
    obj->flags |= 1;
    if (obj->hp <= 0) {
        if (gDeferMonsterHits == 0 || id == 0)
            KillMonster(obj);
    }
}
/* 08E6:103D: does the monster stand in the half of its cell that its facing points to? 1 in the
   centre (cellPos 4), else gMonsterFrontPos[orient * 4 + cellPos]. Called by 05EA. */
/*@ 08E6:103D */ int IsMonsterAtFront(struct Monster far *obj)
{
    if (obj->cellPos == 4)
        return 1;
    return gMonsterFrontPos[obj->orient * 4 + obj->cellPos];
}
/* 08E6:106B: roll 1d20 for a hit against character `who`; a roll of 20 always hits; otherwise the
   target's effectFlags bit 9 costs 2 and gPartySpellFlags bit 5 costs 1. The hit succeeds when the roll reaches
   SubtractInts(the object's record m01, the character's armorClass). Called by 10F3 and 0EF3:0A3F. */
/*@ 08E6:106B */ int RollMonsterToHit(struct Monster far *obj, int who)
{
    int index;
    int roll;
    int need;

    index = obj->typeIndex;
    roll = RollDice(1, 0x14, 0);
    if (roll != 0x14) {
        if (gChars[who].effectFlags & 0x200)
            roll -= 2;
        if (gPartySpellFlags & 0x20)
            roll--;
    }
    need = SubtractInts(gMonsterTypes[index].m01, gChars[who].armorClass);
    return roll >= need || roll == 0x14;
}
/* 08E6:10F3: resolve a monster's melee attack on the party. gAttackSideByDir[gPartyDir * 4 +
   facing] gives the side of the party it attacks, one of the four 12-byte groups of
   gAttackTargetOrder; gRightSidePosByDir[facing * 4 + cellPos] picks the group's first or second
   half (cellPos 4 rolls 1d2 - 1 to choose). It scans the six slots of that half for the first party member whose status bit 0
   is set, whose hp (CharRec +1B) is above -10, and who passes a 1d20 < 5 save when its effectFlags has bit
   2, and remembers that member. It then rolls the monster type's m03 attack routines through
   RollMonsterToHit, adding RollDice(m04[i][0], m04[i][1], m04[i][2]) damage per hit; each time
   0EF3:18A2 TickBandageCountdown on the member returns nonzero the total is reset to 0 and the
   loop goes on to the next attack. When the total is positive it reports a loss (m0E bit 7), damages
   the member, applies "poisoned" (m0E bit 4, clearing effectFlags bit 7) and "paralyzed" (m0E bit 5).
   Called by 05EA. */
/*@ 08E6:10F3 */ void ResolveMonsterMelee(struct Monster far *obj)
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

    index = obj->typeIndex;
    group = gAttackSideByDir[gPartyDir * 4 + obj->orient];
    who = group * 12;
    if (obj->cellPos != 4)
        roll = gRightSidePosByDir[obj->orient * 4 + obj->cellPos];
    else
        roll = RollDice(1, 2, 0xff);
    if (roll != 0)
        who += 6;
    end = who + 6;
    for (i = who; i < end; i++) {
        member = &gChars[gAttackTargetOrder[i]];
        if ((member->status & 1) && member->hp > -10) {
            if ((member->effectFlags & 4) == 0 || RollDice(1, 0x14, 0) < 5) {
                who = gAttackTargetOrder[i];
                break;
            }
        }
    }
    if (i == end)
        return;
    total = 0;
    attacks = gMonsterTypes[index].m03;
    for (i = 0; i < attacks; i++) {
        if (!RollMonsterToHit(obj, who))
            continue;
        total += RollDice(gMonsterTypes[index].m04[i][0],
                          gMonsterTypes[index].m04[i][1],
                          gMonsterTypes[index].m04[i][2]);
        if (TickBandageCountdown(member) != 0)
            total = 0;
    }
    if (total <= 0)
        return;
    if (gMonsterTypes[index].m0E & 0x80)
        ReportLostItem(who);
    DamageCharacter(who, total);
    if (gMonsterTypes[index].m0E & 0x10) {
        ApplyStatusEffect(who, 2, "poisoned", 0, 1, 8, 1);
        gChars[who].effectFlags &= ~0x80L;
    }
    if (gMonsterTypes[index].m0E & 0x20)
        ApplyStatusEffect(who, 4, "paralyzed", 2, 5, 9, 1);
}
/* 08E6:131D: build monster `index` from scratch: clear it (memset), then, unless `id` is 0, set its
   typeIndex, its kind (the argument doubled, plus one when the slot index is odd, :953-955), frame,
   orientation, altShapes, type and timer and its actionsLeft from the record's m18; roll its hit points (+0C, +0E) as the record's
   m02 d8 (or 1d4 when m02 is 0xff); set dropItem1 and dropItem2 and move it to `id`. Called by 1505 and
   ovl3:0A7C LoadLevel. */
/*@ 08E6:131D */ void InitMonster(int index, int kind, int id, int frame, int orient, int b0, int m07, int type, int timer, int m12, int m14)
{
    struct Monster far *obj;

    obj = &gMonsters[index];
    memset(obj, 0, 0x1c);
    if (id == 0)
        return;
    kind <<= 1;
    if (index & 1)
        kind++;
    obj->typeIndex = b0;
    obj->actionsLeft = gMonsterTypes[b0].m18;
    obj->updateGroup = kind;
    obj->cellPos = frame;
    obj->altShapes = m07;
    obj->mode = type;
    obj->timer = timer;
    obj->orient = orient;
    if (gMonsterTypes[b0].m02 != -1)
        obj->maxHp = obj->hp = RollDice(gMonsterTypes[b0].m02, 8, 0);
    else
        obj->maxHp = obj->hp = RollDice(1, 4, 0);
    obj->dropItem1 = m12;
    obj->dropItem2 = m14;
    MoveMonsterToCell(obj, id, orient);
}
/* 08E6:1410: the periodic effect tick. Mode 1 (gLevelTimerMode == 1) fires when the deadline
   gDeadline is
   behind gGameTicks (gDeadline < gGameTicks); mode 2 fires when gStepCount has reached gLevelTimerSteps; mode 3 fires when
   either holds; any other mode does nothing. Firing runs RunCellScript(0, 0x20), then, when no
   monster slot from 1 to 29 is free (every one has hit points above 0), kills through 0EC3
   the first monster whose Distance(cell, gPartyCell) is over 3. After that
   optional 0EC3 the fire path resets the deadline to gGameTicks + gLevelTimerPeriod and clears gStepCount. Called by
   1414:04E7 and overlay 6. The unit's -v flag is what keeps the mode-3 low-word branch at
   1410:005A the right way round that a -O build inverts. */
/*@ 08E6:1410 */ void TickLevelScriptTimer(void)
{
    int i, any;

    if (gLevelTimerMode == 1) {
        if (gDeadline >= gGameTicks)
            ;
        else
            goto fire;
    }
    if (gLevelTimerMode == 2) {
        if (gStepCount >= gLevelTimerSteps)
            goto fire;
    }
    if (gLevelTimerMode == 3) {
        if (gStepCount >= gLevelTimerSteps)
            goto fire;
        if (gDeadline >= gGameTicks)
            goto fire;
        return;
    }
    return;
fire:
    RunCellScript(0, 0x20);
    any = 0;
    for (i = 1; i < 30; i++) {
        any += gMonsters[i].hp <= 0;
    }
    if (any == 0) {
        for (i = 1; i < 30; i++) {
            if (Distance(gMonsters[i].cell, gPartyCell) > 3) {
                KillMonster(&gMonsters[i]);
                break;
            }
        }
    }
    gDeadline = gGameTicks + gLevelTimerPeriod;
    gStepCount = 0;
}
/* 08E6:1505: spawn a monster: when map cell `id` holds no monster (gMaze + 4, bits 0..2), fill the
   first free monster slot (hit points <= 0) through 131D with the ten fields (update group, cell,
   cellPos, facing, typeIndex, shape set, mode, timer, dropItem1, dropItem2). Called by 122E:0B89. */
/*@ 08E6:1505 */ void SpawnMonster(int kind, int id, int frame, int orient, int b0, int m07, int type, int timer, int m12, int m14)
{
    int i;

    if (gMaze[id * 9 + 4] & 7)
        return;
    for (i = 0; i < 30; i++) {
        if (gMonsters[i].hp <= 0) {
            InitMonster(i, kind, id, frame, orient, b0, m07, type, timer, m12, m14);
            break;
        }
    }
}
/* 08E6:155F: list the monsters in map cell `key` by where they stand: fill a six-entry list with
   -1, then set list[cellPos] = i for every one of the 30 in that cell. Both callers pass an `int
   list[6]` (FindMeleeTarget and DamageNearRowMonsters), which the function fills. Called by 15B4 and
   16D6. */
/*@ 08E6:155F */ void ListMonstersByCellPos(int far *list, int key)
{
    struct Monster far *obj;
    int i;

    for (i = 0; i < 6; i++)
        list[i] = -1;
    for (obj = gMonsters, i = 0; i < 30; i++, obj++) {
        if (obj->cell == key)
            list[obj->cellPos] = i;
    }
}
/* 08E6:15B4: the monster party member `who` hits in map cell `value`: the one in the centre
   (list[4]) when there is one, else the first of the four cell positions in gTargetPosOrder +
   gPartyDir * 8 + (who & 1) * 4 (the near half first, the member's side first) that holds one, else
   -1. Called by 0DE8:0359 and 0EF3:0E1B. */
/*@ 08E6:15B4 */ int FindMeleeTarget(char who, int value)
{
    int list[6];
    char far *row;
    int i;

    ListMonstersByCellPos(list, value);
    if (list[4] != -1)
        return list[4];
    row = gTargetPosOrder + gPartyDir * 8 + (who & 1) * 4;
    for (i = 0; i < 4; i++, row++) {
        if (list[*row] != -1)
            return list[*row];
    }
    return -1;
}
/* 08E6:1634: roll RollDice(count, sides, bonus) against the monster `index`: when `flag` is set and
   a 1d20 reaches GetSavingThrowTarget(0, gMonsterTypes[typeIndex].m02, 4), halve the roll when flag is 1, else
   zero it; then, unless MonsterResistsMagic (0D1C:0BAB) says it resists, damage it by the roll. Called by 16D6. */
/*@ 08E6:1634 */ void DamageMonsterWithSave(char index, char count, char sides, char bonus, char flag)
{
    int roll;
    int cls;
    int save;

    cls = gMonsterTypes[gMonsters[index].typeIndex].m02;
    roll = RollDice(count, sides, bonus);
    if (flag != 0) {
        save = GetSavingThrowTarget(0, cls, 4);
        if (RollDice(1, 0x14, 0) >= save) {
            if (flag == 1)
                roll >>= 1;
            else
                roll = 0;
        }
    }
    if (MonsterResistsMagic(index) == 0)
        DamageMonster(&gMonsters[index], roll);
}
/* 08E6:16D6: list the monsters in map cell `key`, mark gDeferMonsterHits, damage the one in the
   centre (list[4]) and then those at the two cell positions nearest the party,
   gNearPosPair + gPartyDir * 2, through 1634, and run the 0E48 cleanup. Called by 0EF3:0F99. */
/*@ 08E6:16D6 */ void DamageNearRowMonsters(int key, char count, char sides, char bonus, char flag)
{
    int list[6];
    char far *row;
    int i;

    ListMonstersByCellPos(list, key);
    gDeferMonsterHits = 1;
    if (list[4] != -1)
        DamageMonsterWithSave(list[4], count, sides, bonus, flag);
    row = gNearPosPair + gPartyDir * 2;
    for (i = 0; i < 2; i++, row++) {
        if (list[*row] != -1)
            DamageMonsterWithSave(list[*row], count, sides, bonus, flag);
    }
    FinishDeferredHits();
}
/* 08E6:1775: find a monster in map cell `key`: one at cell position `field` is returned at once;
   otherwise scan the four bytes of gProjectileHitOrder + flags * 16 + field * 4 and remember the
   monster whose cellPos comes earliest there. Returns -1 when none. Called by 17FD. Purpose unknown:
   what `flags` selects (every row of ds:32C3 starts with 4, the centre). */
/*@ 08E6:1775 */ int FindMonsterNearPos(int key, int field, int flags)
{
    char far *row;
    int best, bestSlot;
    struct Monster far *obj;
    int i, slot;

    best = -1;
    bestSlot = 5;
    for (i = 0, obj = gMonsters; i < 30; i++, obj++) {
        row = gProjectileHitOrder + flags * 16 + field * 4;
        if (obj->cell == key) {
            if (obj->cellPos == field)
                return i;
            for (slot = 0; slot < 4; slot++) {
                if (obj->cellPos == row[slot] && slot < bestSlot) {
                    bestSlot = slot;
                    best = i;
                }
            }
        }
    }
    return best;
}
/* 08E6:17FD: fill `list` with gMonsters indices and end it with -1: when scanAll is set, every monster
   in map cell key; otherwise when scanFiltered is set, the single index 1775(key, field, flags)
   finds; otherwise the first monster 189D accepts for (key, field). Called by 0D1C:0A43, 0EF3:0A3F,
   0EF3:1530, 0EF3:18D8 and 0EF3:1A30. The unit's -v flag is what keeps the two identical `*list++`
   tails at 17FD:0058 and 17FD:0082 apart that a -O build cross-jumps. */
/*@ 08E6:17FD */ void ListMonstersAtCell(int far *list, int key, int field, char flags, char scanAll, char scanFiltered)
{
    struct Monster far *obj;
    int i;

    if (scanAll) {
        for (i = 0, obj = gMonsters; i < 30; i++, obj++) {
            if (obj->cell == key)
                *list++ = i;
        }
    } else {
        if (scanFiltered) {
            *list++ = FindMonsterNearPos(key, field, flags);
        } else {
            for (i = 0, obj = gMonsters; i < 30; i++, obj++) {
                if (IsMonsterAtPos(obj, key, field)) {
                    *list++ = i;
                    break;
                }
            }
        }
    }
    *list = -1;
}
/* 08E6:189D: does this monster stand in map cell `key`, at cell position `field` or in the
   centre (4)? Returns 1 when it does, else 0. Called by ListMonstersAtCell (:1159) and 1414:0172 TestDDEffect. */
/*@ 08E6:189D */ int IsMonsterAtPos(struct Monster far *obj, int key, int field)
{
    if (obj->cell == key && (obj->cellPos == field || obj->cellPos == 4))
        return 1;
    return 0;
}
