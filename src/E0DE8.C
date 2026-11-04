/* Unit at EOB.EXE 0DE8:0007 (4279 bytes, 20 functions): the combat, damage and item-effect
   module. It rolls to-hit and damage for a character against a creature or an item (0007, 011A,
   01FF, 02D4), applies and records the resulting hit points (09B9, 0ADB), awards experience and
   levels a character up (0B45, 0C2B, 0C82), computes a character's armour class (0D5E), saves
   against an effect (0F98, 0FCA, 101A), runs the party-wipe ending (093B) and uses carried items
   (0359 ApplyItemAttack, 0474 MoveCarriedItem, 0537 UseCarriedItem, 05BB UseItemByKind, 0763
   UseItem). It walks the 243-byte character records at ds:F580 (struct CharRec, src/E0A72.C), the
   28-byte object records at ds:8A82 (struct Monster, src/E08E6.C), the item records gItems (ds:8DD0)
   (struct ItemRecord, src/E0D1C.C), the 27-byte records at ds:1A3B (struct MonsterType, src/E0D1C.C) and
   the 16-byte item types at ds:A9F0 (struct ItemType, src/EOV2.C). Most meanings are not
   established, so most function names say what a function does rather than what the game calls
   it. */

/*@flags -ml -O -Z */

struct CharRec {            /* the 243-byte (0xF3) character records at ds:F580; see src/E0A72.C */
    char pad00;
    unsigned char status;   /* +01: status bits (0D1C:0670, 0DE8:09B9) */
    char name[0x0B];        /* +02: the character's name, NUL padded (0DE8:0C82) */
    char pad0D[0x15 - 0x0D];
    signed char dexterity;  /* +15: one of the five further stat bytes (src/E0A72.C); indexes the
                               per-stat table g349E for the armour-class base (GetArmourClassBase, 0A72:019D)
                               and the per-stat table ds:3474 (GetCharM15Value, 0A72:0181) */
    char pad16;
    signed char constitution;        /* +17: an index into g54F6 (0D1C:059B); also the second argument of
                               RollHitPoints (0A72:01AE), which LevelUpSlot passes there */
    char pad18[0x1B - 0x18];
    signed char hp;         /* +1B: current hit points */
    signed char maxHp;      /* +1C: the maximum hit points (src/EOV5.C:32) */
    signed char armorClass; /* +1D: the armour-class byte 0D5E builds */
    signed char m1E;        /* +1E: a per-slot bit UseItem toggles itself (:522) */
    signed char race;       /* +1F */
    signed char classIndex; /* +20: an index into a class table */
    char pad21[3];
    signed char levels[3];  /* +24: three per-class-slot levels (AddExperience passes levels[i] + 1,
                               src/E0A72.C:276-281) */
    unsigned long experience[3];   /* +27: three per-class-slot experience totals (0DE8:0B45) */
    char pad33[0x77 - 0x33];
    int items[27];          /* +77: 27 carried-item slots (0D1C:00F1) */
    char padAD[0xDF - 0xAD];
    signed char mDF;        /* +DF: the descending armour-class penalty (DamageCharacter :589-594) */
    char padE0[0xE2 - 0xE0];
    signed char mE2;        /* +E2: the damage-reduction value (DamageCharacter :573-576) */
    long effectFlags;       /* +E3: effect flags (a 32-bit mask; 09B9 clears bit 2 only) */
    unsigned char lastDamage;      /* +E7: the last damage amount (DamageCharacter :579) */
    unsigned char lastAction[0xF3 - 0xE8]; /* +E8: the last action result per carried slot (UseItem :523) */
};

struct Monster {              /* the 28-byte object records at ds:8A82; see src/E08E6.C */
    char typeIndex;                /* +0: an index into the ds:1A3B table */
    char pad01;
    int cell;                /* +2 */
    char pad04;
    char orient;               /* +5 */
    char pad06[0x16 - 0x06];
    char m16;               /* +16: bit 0 set on every 0007 call (the object was attacked) */
    char pad17[0x1C - 0x17];
};

struct MonsterType {            /* the 27-byte records at ds:1A3B; see src/E0D1C.C */
    signed char m00;        /* +0: the target's class byte for GetHitTarget (0A72:0522) */
    char m01;               /* +1 */
    signed char m02;        /* +2: a class byte read by 0FCA */
    char pad03[0x0E - 0x03];
    signed char m0E;        /* +0E: bit 0 tested by 011A */
    int m0F;                /* +0F */
    char pad11[0x19 - 0x11];
    signed char m19;        /* +19 */
    signed char m1A;        /* +1A */
};

struct ItemRecord {
    signed char nameIndex;        /* +0 (ds:8DD0): indexes the item-name table at segment 0x1F28 */
    signed char identifiedNameIndex;        /* +1 (ds:8DD1): the second name index */
    unsigned char flags;    /* +2 (ds:8DD2): bit 0x20 gates 0BD2:0AF2/0C8E, 0x40 is set by
                               0BD2:040E and tested by 0BD2:05B5; src/E07A9.C calls it flags */
    signed char icon;       /* +3 (ds:8DD3): the icon number */
    signed char type;        /* +4 (ds:8DD4): the item type */
    signed char tag;        /* +5 (ds:8DD5): 01D0 stores its 4th argument here and 02B9 matches it
                               against its `key`. No caller shows a sound (01D0's callers pass a
                               slot kind, E0ACD.C:561-568, a roll, E08E6.C:807, or an effect kind | 4,
                               0BD2:07DE), so Purpose unknown: a per-entry tag */
    int key;                /* +6 (ds:8DD6): the key; -1 when the record is free */
    int prevLink;                /* +8 (ds:8DD8): one link of the circular list; 0BD2:01D0 splices a
                               new item in after this record through it */
    int nextLink;                /* +0xa (ds:8DDA): the other link; the list walks (0BD2:0385, 040E,
                               04AA) follow it */
    signed char m0A;        /* +0xc (ds:8DDC): gFBC8 or -1, set by 01D0 */
    signed char value;        /* +0xd (ds:8DDD): the item's bonus or effect value: the signed bonus
                               printed as "%s +%d" and "Cursed %s %d", the spell index into ds:1278
                               and ds:12E0, the wand effect index g38FA[m0B], and the scroll and
                               potion values (0BD2:05B5, 0D2D, 0EE8, 1001) */
};

struct Projectile {              /* the records at ds:DD2C that 1414:0172 walks; see src/E1414.C */
    char active;            /* +0 */
    char pad01;
    int m02;                /* +2 */
    int cell;                /* +4 */
    char m06;               /* +6 */
    signed char dir;        /* +7 */
    int item;                /* +8: an item slot, indexed into gItems (01FF, 02D4) */
    char m0A;               /* +0A: an item type, indexed into gItemTypes (01FF, 02D4) */
    char m0B;               /* +0B: the attacker (TryRecordAttack :318) */
    char m0C;               /* +0C */
    unsigned char m0D;      /* +0D */
    int (far *fn)(struct Projectile far *rec); /* +0E */
};

struct ItemType {           /* the 16-byte item types at ds:A9F0; see src/EOV2.C */
    signed char slotMask;        /* +0: bit 4 tested by 0D1C:0BEB */
    signed char m01;        /* +1 */
    char pad02[2];
    signed char m04;        /* +4 */
    signed char m05;        /* +5 */
    char pad06;
    signed char m07;        /* +7 */
    signed char m08;        /* +8 */
    signed char m09;        /* +9 */
    signed char m0A;        /* +0A */
    signed char m0B;        /* +0B */
    char pad0C[2];
    short m0E;              /* +0E: low 7 bits tested by 01D7; also UseItem's dispatch kind (:495)
                               and ComputeArmourClass's test (:726) */
};

extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct Monster gMonsters[]; /*@ ds:8A82 */
extern struct MonsterType gMonsterTypes[]; /*@ ds:1A3B */
extern struct ItemRecord gItems[]; /*@ ds:8DD0 */
extern struct ItemType gItemTypes[]; /*@ ds:A9F0 */
extern signed char g1A1B[]; /*@ ds:1A1B */   /* per-page sound table (src/E0D1C.C) */
extern signed char g1A2B[]; /*@ ds:1A2B */   /* per-page byte table used by 01FF */
extern signed char g115F[]; /*@ ds:115F */   /* a byte table indexed by a class, used by 0B45 */
extern signed char g1231[]; /*@ ds:1231 */   /* a byte table indexed by the class classIndex (:725;
                                                src/E0BD2.C:102) */
extern signed char g552F[]; /*@ ds:552F */   /* a byte table indexed by the item's m02 (UseItemByKind
                                                :432) */
extern signed char g553E[]; /*@ ds:553E */   /* the four items slot indexes ComputeArmourClass walks
                                                (:719) */
extern signed char g54F6[]; /*@ ds:54F6 */   /* byte table indexed by constitution; used by 0D1C:059B
                                                (src/E0D1C.C:437, :441) */
extern unsigned char gFBD2; /*@ ds:FBD2 */   /* bit 0 gates the icon colour row (src/E07A9.C);
                                                RollToHit reads bits 4 and 5 (:225) */
extern int gPartyDir; /*@ ds:FBCC */             /* the party's facing, 0..3 north, east, south, west */
extern int gPartyCell; /*@ ds:FBCA */      /* the party's map cell (src/E177D.C:92) */
extern signed char gF57E; /*@ ds:F57E */     /* a colour read by 0D1C:0C21 */
extern signed char gFB4C; /*@ ds:FB4C */     /* a colour read by 0D1C:0C21 */
extern signed char gF53A; /*@ ds:F53A */     /* a colour read by 0D1C:0C21 */
extern char gPanelMode; /*@ ds:FB44 */            /* a mode byte: 0 and 1 gate 0D1C:01CA's redraws
                                                (src/E0D1C.C:108); DamageCharacter redraws the member
                                                when it is 0 and the portrait otherwise (:596-599) */
extern signed char gA9C6; /*@ ds:A9C6 */     /* an offset within a nine-byte gMaze record (:348-350;
                                                0ACD:0644) */
extern char gDF4C; /*@ ds:DF4C */            /* set to 1 when an action took effect (0DE8:0359,
                                                0DE8:0537, 0DE8:05BB) */
extern char gDCA4; /*@ ds:DCA4 */            /* the target object 0359 records (:361); UseItem damages
                                                gMonsters[gDCA4] (:520) */
extern unsigned char g1905[]; /*@ ds:1905 */   /* a 256-byte table gMaze indexes into (0359) */
extern char gMaze[]; /*@ ds:B894 */          /* the nine-byte records at ds:B894 indexed by row * 9
                                                (:348-350); the +4 byte is read at 0D1C:0080 */
extern char gFileNameBuf[]; /*@ ds:FE6E */   /* the text buffer sprintf formats into (src/EOV2.C) */

extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int GetStrengthToHitBonus(int who); /*@ 0A72:00CF */
extern int GetStrengthDamageBonus(int who); /*@ 0A72:0128 */
extern int GetCharM15Value(int who); /*@ 0A72:0181 */
extern char GetHitTarget(char who, char classByte); /*@ 0A72:0522 */
extern char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */
extern long GetClassSlotExpThreshold(char row, char slot, char level); /*@ 0A72:04B7 */
extern int GetArmourClassBase(char m15); /*@ 0A72:019D */
extern int GetIndexedByte(char index, char limit, char offset); /*@ 0A72:0006 */
extern int RollHitPoints(char classIndex, char m17, char countA, char countB, char countC); /*@ 0A72:01AE */
extern int MemberIs(int who, int mask); /*@ 0D1C:0670 */
extern int StepCell(int cell, int dir); /*@ 0D1C:00DA */
extern int f0D1C_0BAB(int obj); /*@ 0D1C:0BAB */
extern int f0D1C_0BEB(int obj, int flag); /*@ 0D1C:0BEB */
extern int f0D1C_059B(int who, int attack); /*@ 0D1C:059B */
extern int f0D1C_07DB(int who, int flag); /*@ 0D1C:07DB */
extern int RemoveListItem(void far *head, int key, int target); /*@ 0BD2:02B9 */
extern int AddDDEffect(int who, int item, int volume, int kind, int page, int type); /*@ 0BD2:07DE */
extern int Obj15B4(char who, int value); /*@ 08E6:15B4 */
extern int GetClassSlot24Value(int who); /*@ 0EF3:04F7 */
extern void DamageObject(struct Monster far *obj, int damage); /*@ 08E6:0FBE */
extern void ClearEffectFlag4(struct CharRec far *rec); /*@ 0EF3:12A1 */
extern void RecomputeArmourClass(struct CharRec far *rec); /*@ 0EF3:0F86 */
extern void ApplyCharEffect(char who, long interval, char value, char firstEmpty); /*@ 1414:05ED */
extern void RedrawMember(int who); /*@ 1547:0991 */
extern void DrawMemberHp(int who); /*@ 1547:033F */
extern void DrawMemberPortrait(int who); /*@ 1547:0544 */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int c1, int c2, int c3); /*@ 082C:09D5 */
extern void DelaySecondsThenKey(int n); /*@ 082C:0B43 */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

void DamageCharacter(int who, int what);  /* forward: 01FF reaches it with a far call */
void LevelUpSlot(int who, int cls, int slot);  /* forward: 0B45 reaches it with a far call */

/* 0DE8:0007: the to-hit roll of character `who` against object `obj`. It derives an attack bonus
   from the item record `slot` (an index into gItems; its m02 item type index selects the m0B
   weapon value when it is 1..3), adds either the attacker's strength to-hit bonus
   (GetStrengthToHitBonus, 0A72:00CF) or the dexterity char value depending on `useStrength`, sets
   gMonsters[obj].m16 bit 0 on every call (the object was attacked), whatever the outcome, and compares
   a d20 roll (raised by gFBD2 bits 4 and 5 and by gChars[who].effectFlags bit 6, clamped to 1..20) against
   GetHitTarget(who, gMonsterTypes[gMonsters[obj].typeIndex].m00) minus the bonus. Returns 1 when the roll
   reaches that to-hit target. Called by 0DE8:02D4, 0DE8:0359, 0EF3:0A3F DamageTargetCell and
   0EF3:0E1B CastMeleeEffect. */
/*@ 0DE8:0007 */ int RollToHit(int who, int obj, int slot, int useStrength)
{
    int entry;
    int toHit;
    int itemType;
    int bonus;
    int roll;

    if (slot != 0)
        itemType = gItems[slot].type;
    else
        itemType = 0;
    if (itemType >= 1 && itemType <= 3)
        bonus = gItems[slot].value;
    else
        bonus = 0;
    if (useStrength != 0)
        bonus += GetStrengthToHitBonus(who);
    else
        bonus += GetCharM15Value(who);
    entry = gMonsters[obj].typeIndex;
    gMonsters[obj].m16 |= 1;
    toHit = GetHitTarget(who, gMonsterTypes[entry].m00) - bonus;
    roll = RollDice(1, 20, 0);
    if (gFBD2 & 0x30)
        roll++;
    if ((gChars[who].effectFlags & 0x40) != 0)
        roll++;
    roll = roll < 20 ? roll : 20;
    roll = roll > 1 ? roll : 1;
    return roll >= toHit;
}

/* 0DE8:011A: the damage a hit deals. It starts from GetStrengthDamageBonus(who) when `useStrength`
   is set and `who` is valid, then adds a damage roll from the item type `itemType` (dice at
   m0A/m0B or m07/m08 with the m09 bonus, the choice depending on whether `obj`'s gMonsterTypes entry
   has m0E bit 0 set) plus the item record `slot`'s m0B (slot is an index into gItems), or the
   bare RollDice(1, 2, 0) when `slot` is 0, and clamps at 0. Called by 0DE8:01FF, 0DE8:02D4 and
   0DE8:0359. */
/*@ 0DE8:011A */ int RollAttackDamage(int who, int obj, int slot, int itemType, char useStrength)
{
    struct ItemType far *item;
    int total;

    if (useStrength != 0 && who != -1)
        total = GetStrengthDamageBonus(who);
    else
        total = 0;
    if (slot != 0) {
        item = &gItemTypes[itemType];
        if (obj != -1 && (gMonsterTypes[gMonsters[obj].typeIndex].m0E & 1))
            total += RollDice(item->m0A, item->m0B, item->m09);
        else
            total += RollDice(item->m07, item->m08, item->m09);
        total += gItems[slot].value;
    } else {
        total += RollDice(1, 2, 0);
    }
    return total < 0 ? 0 : total;
}

/* 0DE8:01D7: returns 1 when the item type `index`'s m0E low seven bits are 2 or 3, otherwise 0.
   Called by 0DE8:01FF and 0DE8:02D4 to decide whether to roll 011A damage. */
/*@ 0DE8:01D7 */ int IsKind2Or3(int index)
{
    int kind;

    kind = gItemTypes[index].m0E & 0x7f;
    return kind == 2 || kind == 3;
}

/* 0DE8:01FF: runs the attack of a ds:DD2C record against the object it names. It takes the
   record's m08 slot (an index into gItems) and m0A item type, reads
   g1A2B[gPartyDir * 4 + (gItems[slot].tag & 3)] as the base member, rolls 011A damage (or 1 when
   IsKind2Or3 is false) and adds 2 to the base when MemberIs(5, 1) or MemberIs(6, 1)
   holds and RollDice(1, 2, -1) is nonzero, then applies DamageCharacter to that member and
   returns 1, else 0. Called by 1414:0172 via TestDDEffect. */
/*@ 0DE8:01FF */ int RunRecordAttack(struct Projectile far *rec)
{
    int slot;
    int itemType;
    int damage;
    int baseMember;
    int hitMember;

    slot = rec->item;
    itemType = rec->m0A;
    baseMember = gItems[slot].tag & 3;
    baseMember = g1A2B[gPartyDir * 4 + baseMember];
    if (IsKind2Or3(itemType))
        damage = RollAttackDamage(-1, -1, slot, itemType, 0);
    else
        damage = 1;
    if ((MemberIs(5, 1) || MemberIs(6, 1)) && RollDice(1, 2, -1))
        hitMember = baseMember + 2;
    else
        hitMember = baseMember;
    if (MemberIs(hitMember, 1)) {
        DamageCharacter(hitMember, damage);
        return 1;
    }
    return 0;
}
/* 0DE8:02D4: tests and runs the attack of a ds:DD2C record against object `idx`. It takes the
   record's m08 slot, m0A item type and m0B attacker; when m0B is valid it first demands the
   0007 to-hit roll, then rolls 011A damage for the item type (or 1 when IsKind2Or3 is false)
   and hands the object to DamageObject. Returns 0 when the to-hit fails, else 1. Called by
   1414:0172 via TestDDEffect. */
/*@ 0DE8:02D4 */ int TryRecordAttack(struct Projectile far *rec, int idx)
{
    int slot;
    int itemType;
    int who;
    int damage;

    slot = rec->item;
    itemType = rec->m0A;
    who = rec->m0B;
    if (who != -1 && !RollToHit(who, idx, slot, 0))
        return 0;
    if (IsKind2Or3(itemType))
        damage = RollAttackDamage(who, idx, slot, itemType, 0);
    else
        damage = 1;
    DamageObject(&gMonsters[idx], damage);
    return 1;
}
/* 0DE8:0359: one item/effect attack by character `who` on the item record `slot` (an index into
   gItems). It rejects a `who` above 1 with -3, then looks the action up through
   StepCell(gPartyCell, gPartyDir) and Obj15B4. A -1 target with g1905[gMaze[row * 9 + gA9C6]] at
   0xFF bumps two gMaze cells and sets gDF4C, returning -2, otherwise -1. A real target demands the
   0007 to-hit roll (returns -1 on a miss), and when the slot's m02 is 0x34..0x38 f0D1C_0BAB can
   also fail it; the surviving hit is rolled through 011A and halved when f0D1C_0BEB says so.
   Returns -3, -2 or -1 as above, else the damage. Called by 0DE8:0763. The gDCA4 byte records the
   target for the effect dispatcher. */
/*@ 0DE8:0359 */ int ApplyItemAttack(int who, int slot)
{
    int target;
    int m02;
    int row;
    int damage;

    if (who > 1)
        return -3;
    row = StepCell(gPartyCell, gPartyDir);
    target = Obj15B4(who, row);
    if (target == -1) {
        if (g1905[gMaze[row * 9 + gA9C6]] == 0xff) {
            gMaze[row * 9 + gA9C6]++;
            gMaze[row * 9 + (gA9C6 ^ 2)]++;
            gDF4C = 1;
            return -2;
        }
        return -1;
    }
    if (RollToHit(who, target, slot, 1) == 0)
        goto fail;
    m02 = gItems[slot].type;
    if (m02 > 0x33 && m02 < 0x39 && f0D1C_0BAB(target))
        return -1;
    gDCA4 = target;
    damage = RollAttackDamage(who, target, slot, gItems[slot].type, 1);
    if (f0D1C_0BEB(target, m02))
        damage >>= 1;
    return damage;
fail:
    return -1;
}

/* 0DE8:0474: moves a carried item into gChars[who].items[slot] (slot is the carried-item index).
   With `flag` set and the special slot items[16] occupied, it takes that slot's item through
   RemoveListItem (key 0, target -1). Otherwise it scans slots 24 down to 22 for the first occupied
   item whose gItems m02 is not `type` (or `type` is -1) and moves that item to `slot`,
   clearing the source. Called by 0DE8:0537. */
/*@ 0DE8:0474 */ void MoveCarriedItem(int who, int slot, int type, int flag)
{
    int i;
    int item;

    if (flag != 0 && gChars[who].items[16] != 0) {
        gChars[who].items[slot] = RemoveListItem(&gChars[who].items[16], 0, -1);
    } else {
        for (i = 0x18; i >= 0x16; i--) {
            item = gChars[who].items[i];
            if (item != 0 && (gItems[item].type != type || type == -1)) {
                gChars[who].items[slot] = item;
                gChars[who].items[i] = 0;
                break;
            }
        }
    }
}
/* 0DE8:0537: tries to use the item in the carried slot `slot` (an index into items) of character
   `who` against the item record `field` (an index into gItems). It dispatches through
   AddDDEffect(who, field, gPartyCell as volume, the g1A1B page byte as kind, gPartyDir as page, the
   item's m02 as type); on success it plays sound 0x0B, clears gChars[who].items[slot] and refills
   it through MoveCarriedItem(who, slot, -1, 0), setting gDF4C. Returns 0. Called by 0DE8:0763. */
/*@ 0DE8:0537 */ int UseCarriedItem(int who, int slot, int field)
{
    int idx;

    idx = who;
    if (idx > 3)
        idx -= 2;
    if (AddDDEffect(who, field, gPartyCell, g1A1B[gPartyDir * 4 + idx], gPartyDir,
                   gItems[field].type)) {
        PlaySound(0xb);
        gChars[who].items[slot] = 0;
        MoveCarriedItem(who, slot, -1, 0);
        gDF4C = 1;
    }
    return 0;
}

/* 0DE8:05BB: picks the item for the action `slot` (an index into gItems) and runs it. The
   item's m02 indexes g552F to a `kind`; kind 0x10 takes the first of items[0] and items[1] whose own
   m02 is 0x10 (clearing that slot), else the special items[16] slot (pulled through RemoveListItem,
   with no m02 test); otherwise it scans all 27 slots for the first whose item m02 equals `kind`
   (clearing it and redrawing with RedrawMember when the slot is below 2). A found item is
   dispatched through AddDDEffect; when that returns nonzero it plays sound 0x1A when m02 is 7
   (else 0x0B) and sets gDF4C. Returns 0 once an item was found, whether or not AddDDEffect took
   it, -4 when none was found. Called by 0DE8:0763. */
/*@ 0DE8:05BB */ int UseItemByKind(int who, int slot)
{
    int m02;
    int kind;
    int item;
    int idx;
    int i;

    m02 = gItems[slot].type;
    kind = g552F[m02];
    item = 0;
    if (kind == 0x10) {
        item = gChars[who].items[0];
        if (item == 0 || gItems[item].type != 0x10)
            item = 0;
        else
            gChars[who].items[0] = 0;
        if (item == 0) {
            item = gChars[who].items[1];
            if (item == 0 || gItems[item].type != 0x10)
                item = 0;
            else
                gChars[who].items[1] = 0;
        }
        if (item == 0 && gChars[who].items[16] != 0)
            item = RemoveListItem(&gChars[who].items[16], 0, -1);
    } else {
        for (i = 0; i < 0x1b; i++) {
            if (gItems[gChars[who].items[i]].type == kind) {
                item = gChars[who].items[i];
                gChars[who].items[i] = 0;
                if (i < 2)
                    RedrawMember(who);
                break;
            }
        }
    }
    if (item != 0) {
        idx = who;
        if (idx > 3)
            idx -= 2;
        if (AddDDEffect(who, item, gPartyCell, g1A1B[idx + gPartyDir * 4], gPartyDir, m02)) {
            if (m02 == 7)
                PlaySound(0x1a);
            else
                PlaySound(0xb);
            gDF4C = 1;
        }
        return 0;
    }
    return -4;
}
/* 0DE8:0763: runs the action item record `field` (an index into gItems) for character `who`
   and carried slot `slot` (an index into items). It derives the item's m02, calls ClearEffectFlag4
   when effectFlags bit 2 is set, dispatches on the gItemTypes m0E kind (1 rolls 0359 and forces a miss
   from 0 to -1 with sound 0x20, 2 runs 0537, 3 runs 05BB and redraws), then, on a hit, raises hp
   (capped at 0x7F) for a flag-8 item, clears items[slot] for a flag-16 item and hands gMonsters[gDCA4]
   to DamageObject. It toggles m1E bit `slot`, stores the result in lastAction[slot], redraws and arms
   ApplyCharEffect(who, 0x12, slot, 1) when the result is below -2, else ApplyCharEffect(who, 0x12,
   slot + 2, 1). Called by 0BD2:1001. */
/*@ 0DE8:0763 */ void UseItem(int who, int slot, int field)
{
    int m02;
    int result;

    if (field != 0)
        m02 = gItems[field].type;
    else
        m02 = 0;
    result = 0;
    if (gChars[who].effectFlags & 4)
        ClearEffectFlag4(&gChars[who]);
    switch (gItemTypes[m02].m0E & 0x7f) {
    case 1:
        result = ApplyItemAttack(who, field);
        if (result == 0)
            result = -1;
        PlaySound(0x20);
        break;
    case 2:
        result = UseCarriedItem(who, slot, field);
        break;
    case 3:
        result = UseItemByKind(who, field);
        RedrawMember(who);
        break;
    }
    if (result > 0) {
        if (gItems[field].flags & 8) {
            if (gChars[who].hp + result >= 0x80)
                gChars[who].hp = 0x7f;
            else
                gChars[who].hp += result;
            RedrawMember(who);
        }
        if (gItems[field].flags & 0x10)
            gChars[who].items[slot] = 0;
        DamageObject(&gMonsters[gDCA4], result);
    }
    gChars[who].m1E ^= 1 << slot;
    gChars[who].lastAction[slot] = result;
    RedrawMember(who);
    if (result < -2)
        ApplyCharEffect(who, 0x12, slot, 1);
    else
        ApplyCharEffect(who, 0x12, slot + 2, 1);
}
/* 0DE8:093B: the party-wipe ending. It counts how many of the six characters pass the
   MemberIs(n, 5) test (m01 bit 0 and m1B > 0, src/E0D1C.C:445-448); when none does it hides the
   pointer, fills the panel (0, 0x79)-(0x13F, 0xC8), writes "All of your party has been defeated.
   ..." in text window 9, shows the pointer again (LockShowPointer, :550), waits for a key
   (DelaySecondsThenKey(1), :551) and shuts the game down. Called by 0DE8:09B9. */
/*@ 0DE8:093B */ void CheckPartyWiped(void)
{
    int n;
    int alive;

    alive = n = 0;
    for (; n < 6; n++)
        alive += MemberIs(n, 5);
    if (alive != 0)
        return;
    LockHidePointer();
    DrawFilledPanel(0, 0x79, 0x13f, 0xc8, gFB4C, gF53A, gF57E);
    SetTextWindow(9);
    DrawText("All of your party has been defeated.  The minions of evil will be able to carry out "
             "their plans unhindered!\r");
    LockShowPointer();
    DelaySecondsThenKey(1);
    ShutdownGame();
}

/* 0DE8:09B9: applies `amount` points of damage to character `who`. When MemberIs(who, 3) holds
   it derates mE2 (the clamp to 0 is dead: QUIRK 0DE8:09FD), subtracts from hp, stores the amount
   in lastDamage, and at or below -10 sets hp to -10, clears mE2, clears effectFlags bit 2 and keeps only bit 0 of
   m01; it plays sound 0x16 for the death and 0x15 otherwise, runs the mDF descending effect
   through RecomputeArmourClass when it runs out, redraws through RedrawMember (or
   DrawMemberPortrait when gPanelMode is set), triggers the party-wipe ending when hp is at or below -10
   and arms ApplyCharEffect(who, 0x12, 6, 1). Called by 0EF3, 08E6, 122E, 1414, 0DE8:01FF
   RunRecordAttack and overlay 6.
   QUIRK 0DE8:09FD: the mE2 clamp reads `if (&rec->mDF < 0)`, a far-pointer comparison that is
   always false, so the compiler emits `mov ax,[bp-4]; add ax,0xdf; or ax,ax; jae` and the
   following `rec->mE2 = 0` never runs. Kept as written. */
/*@ 0DE8:09B9 */ void DamageCharacter(int who, int amount)
{
    struct CharRec far *rec;

    if (!MemberIs(who, 3))
        return;
    rec = &gChars[who];
    if (rec->mE2 != 0) {
        rec->mE2 -= amount;
        if (&rec->mDF < 0)  /* QUIRK 0DE8:09FD: always false (see the comment above) */
            rec->mE2 = 0;
    }
    rec->hp -= amount;
    rec->lastDamage = amount;
    if (rec->hp <= -10) {
        rec->hp = -10;
        rec->mE2 = 0;
        rec->effectFlags &= ~4;
        rec->status &= 1;
        PlaySound(0x16);
    } else {
        PlaySound(0x15);
    }
    if (rec->mDF > 0) {
        rec->mDF -= amount;
        if (rec->mDF <= 0) {
            rec->mDF = 0;
            RecomputeArmourClass(&gChars[who]);
        }
    }
    if (gPanelMode == 0)
        RedrawMember(who);
    else
        DrawMemberPortrait(who);
    if (rec->hp <= -10)
        CheckPartyWiped();
    ApplyCharEffect(who, 0x12, 6, 1);
}
/* 0DE8:0ADB: heals character `who` by `amount` hit points when the MemberIs(who, 3) condition
   holds, clamping at the record's maxHp maximum, then redraws the character through DrawMemberHp
   and RedrawMember. Called by 0BD2, 0EF3 and 122E. */
/*@ 0DE8:0ADB */ void HealCharacter(int who, int amount)
{
    struct CharRec far *rec;

    if (!MemberIs(who, 3))
        return;
    rec = &gChars[who];
    if (rec->hp + amount > rec->maxHp)
        rec->hp = rec->maxHp;
    else
        rec->hp = rec->hp + amount;
    DrawMemberHp(who);
    RedrawMember(who);
}

/* 0DE8:0B45: adds `amount` experience to character `who`'s three class slots. It divides `amount`
   by g115F[cls] (the per-class divisor for classIndex), then for each slot 0..2 whose GetClassSlotByte
   byte is not -1 (the class has that slot), accumulates the share into that slot's experience total,
   reads the slot's next-level threshold through GetClassSlotExpThreshold(cls, i, levels[i] + 1) and
   levels the slot up through LevelUpSlot when the threshold is not -1 and the total has passed it.
   Called by 0DE8:0C2B. */
/*@ 0DE8:0B45 */ void AddExperience(int who, int amount)
{
    int i;
    int cls;
    long value;

    cls = gChars[who].classIndex;
    amount /= g115F[cls];
    for (i = 0; i < 3; i++) {
        if (GetClassSlotByte(cls, i) == -1)
            continue;
        gChars[who].experience[i] += amount;
        value = GetClassSlotExpThreshold(cls, i, gChars[who].levels[i] + 1);
        if (value != -1 && gChars[who].experience[i] > value)
            LevelUpSlot(who, cls, i);
    }
}
/* 0DE8:0C2B: gives `amount` experience to the whole party. It counts the members that pass
   MemberIs(n, 3), divides `amount` by that count and calls AddExperience(n, amount) for each of
   them. Called by 08E6:0EC3 and 122E:1B5A. */
/*@ 0DE8:0C2B */ void AddPartyExperience(int amount)
{
    int n;
    int count;

    count = 0;
    n = 0;
    for (; n < 6; n++) {
        if (MemberIs(n, 3))
            count++;
    }
    if (count <= 0)
        return;
    amount /= count;
    for (n = 0; n < 6; n++) {
        if (MemberIs(n, 3))
            AddExperience(n, amount);
    }
}

/* 0DE8:0C82: levels character `who` up in class slot `slot`. It bumps that levels slot and, for the
   three slots 0/1/2, rolls one die of the matching damage group through RollHitPoints(cls, constitution),
   adding the result to both hp and maxHp, then formats
   "\x06\x01%s has gained a level of experience.\x06\x0f\r" into gFileNameBuf for the
   character's name (0x06 is the control byte that takes the next byte as the first text attribute,
   gTextAttr1 at ds:811D; 0x01 and 0x0f are attribute values, src/E1DE7.C:332, :379), redraws through
   RedrawMember, draws the text and plays sound 0x17. Called by 0DE8:0B45. */
/*@ 0DE8:0C82 */ void LevelUpSlot(int who, int cls, int slot)
{
    int damage;

    gChars[who].levels[slot]++;
    if (slot == 0)
        damage = RollHitPoints(cls, gChars[who].constitution, 1, 0, 0);
    else if (slot == 1)
        damage = RollHitPoints(cls, gChars[who].constitution, 0, 1, 0);
    else if (slot == 2)
        damage = RollHitPoints(cls, gChars[who].constitution, 0, 0, 1);
    gChars[who].hp += damage;
    gChars[who].maxHp += damage;
    sprintf(gFileNameBuf, "\x06\x01%s has gained a level of experience.\x06\x0f\r",
            gChars[who].name);
    RedrawMember(who);
    DrawText(gFileNameBuf);
    PlaySound(0x17);
}
/* 0DE8:0D5E: computes the armour class armorClass of character `who`. It starts from GetArmourClassBase(dexterity) + 10,
   then for the four slots named by g553E adds the item type's m04 and subtracts the slot's m0B when
   the item m02 passes the gItemTypes m0E and g1231[classIndex] tests (slot 2 also needs f0D1C_07DB); for
   slots 1 and 2 only item type 0x1B counts (:728), and the items[25]/items[26] items count only
   when their m0E kind is 0 (:738, :741). When items[17]'s slot m0B is 0 it subtracts the larger m0B
   of those two items. It clamps against base + 6 while mDF holds (when armorClass is not above base + 6
   the else branch clears mDF, :750), caps at 4 when effectFlags bit 1 is set and at
   5 + (GetClassSlot24Value - 5) / 3 when effectFlags bit 8 is set. Called by 0ACD, 0BD2, 0EF3 and overlays
   0 and 5. */
/*@ 0DE8:0D5E */ void ComputeArmourClass(int who)
{
    int itemType;
    int limit;
    int bonus25;
    int bonus26;
    int value;
    int base;
    int i;
    int item;
    struct CharRec far *rec;

    rec = &gChars[who];
    base = GetArmourClassBase(rec->dexterity);
    rec->armorClass = base + 10;
    for (i = 0; i < 4; i++) {
        item = rec->items[g553E[i]];
        if (item == 0)
            continue;
        if (i == 2 && !f0D1C_07DB(who, 1))
            continue;
        itemType = gItems[item].type;
        value = gItemTypes[itemType].m05 & g1231[rec->classIndex];
        if ((gItemTypes[itemType].m0E & 0x7f) != 0 || value == 0)
            continue;
        if (i >= 1 && i <= 2 && itemType != 0x1b)
            continue;
        rec->armorClass += gItemTypes[itemType].m04;
        rec->armorClass -= gItems[item].value;
    }
    item = rec->items[17];
    if (gItems[item].value == 0) {
        bonus25 = 0;
        bonus26 = 0;
        item = rec->items[25];
        if (item != 0 && (gItemTypes[gItems[item].type].m0E & 0x7f) == 0)
            bonus25 = gItems[item].value;
        item = rec->items[26];
        if (item != 0 && (gItemTypes[gItems[item].type].m0E & 0x7f) == 0)
            bonus26 = gItems[item].value;
        bonus25 = bonus25 > bonus26 ? bonus25 : bonus26;
        rec->armorClass -= bonus25;
    }
    if (rec->mDF > 0) {
        if (rec->armorClass > base + 6)
            rec->armorClass = base + 6;
        else
            rec->mDF = 0;
    }
    if ((rec->effectFlags & 2) != 0 && rec->armorClass > 4)
        rec->armorClass = 4;
    if ((rec->effectFlags & 0x100) != 0) {
        limit = 5;
        if (GetClassSlot24Value(who) > 5)
            limit += (GetClassSlot24Value(who) - 5) / 3;
        if (rec->armorClass > limit)
            rec->armorClass = limit;
    }
}
/* 0DE8:0F98: a saving-throw style keeper: it takes 0D1C:059B's 0/1 result as the threshold
   `chance` and halves `value` when a fresh RollDice(1, 20, 0) is at least it. As 059B returns 0 or
   1 and the roll is 1..20, the test holds on every roll, so it always halves (QUIRK 0DE8:0F98).
   A takes a character index; B takes a pointer to a 28-byte object record (struct Monster) and reads
   that record's typeIndex as the gMonsterTypes index. Called by 0EF3:0A3F and 0EF3:20A8. */
/*@ 0DE8:0F98 */ int HalveSaveDamageA(int who, int value)
{
    int chance;

    chance = f0D1C_059B(who, 4);
    if (RollDice(1, 20, 0) >= chance)
        value >>= 1;
    return value;
}

/* 0DE8:0FCA: the same half-on-success keeper for the object record `objRec`: it reads that
   record's typeIndex as the gMonsterTypes index, derives a class value through gMonsterTypes[typeIndex].m02 and
   GetIndexedByte, then halves `value` on a d20 roll at or above it. Its chance is a real
   threshold (A, by contrast, always halves). A takes a character index. Called by 0EF3:0A3F. */
/*@ 0DE8:0FCA */ int HalveSaveDamageB(struct Monster far *objRec, int value)
{
    int chance;
    int classByte;

    classByte = gMonsterTypes[objRec->typeIndex].m02;
    chance = GetIndexedByte(0, classByte, 4);
    if (RollDice(1, 20, 0) >= chance)
        value >>= 1;
    return value;
}

/* 0DE8:101A: applies the status effect `mask` to character `who` and announces it. It returns
   early when the m01 bit is already set and `skipIfSet` is nonzero, or when the
   f0D1C_059B(who, attack) check succeeds, otherwise ORs `mask` into m01, arms
   ApplyCharEffect(who, count * 0x222, value, 1) and formats "%s is %s!" with the character's
   name and `text`. Called by 08E6:0765, 08E6:10F3 and 0BD2:0D2D. */
/*@ 0DE8:101A */ void ApplyStatusEffect(int who, int mask, char far *text, int attack, int count,
                                 int value, int skipIfSet)
{
    if ((gChars[who].status & mask) != 0 && skipIfSet != 0)
        return;
    if (f0D1C_059B(who, attack))
        return;
    gChars[who].status |= mask;
    ApplyCharEffect(who, count * 0x222, value, 1);
    sprintf(gFileNameBuf, "%s is %s!\r", gChars[who].name, text);
    DrawText(gFileNameBuf);
}
