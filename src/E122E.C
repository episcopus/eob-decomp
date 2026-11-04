/* Unit at EOB.EXE 122E:0001 (7775 bytes, 40 functions of the map). The game's action script
   interpreter: the driver at 122E:1DA8 (RunCellScript) walks a member's compiled script with a
   table of 26 far handler pointers and dispatches each opcode byte to one of the handlers below.
   The handlers take the current script pointer (a far char pointer) and return how many bytes the
   opcode consumed, or jump the pointer; the driver stops when a handler sets gA9EC. 147F is the big
   sub-dispatcher with its own in-segment jump table; the map folded the 122E:1194 play-sound handler
   into 1097 and the 122E:1A3B drain-item handler into 147F, and both are separate functions here.
   Every function of the file matches. */

/*@flags -ml -O -Z -v */

extern signed char gMaze[]; /*@ ds:B894 */     /* nine-byte per-object records (src/E08E6.C) */
extern int gEvalStack[]; /*@ ds:A95C */         /* the script-expression value stack (122E:147F) */
extern int gEvalTop; /*@ ds:A9EE */             /* the index into gEvalStack (122E:147F) */
extern char gClassBaseBits[]; /*@ ds:1231 */             /* per-class byte table (src/E0BD2.C) */
extern signed char gWallDecorHead[]; /*@ ds:18BF */ /* per-object-kind table (Purpose unknown) */
extern int gEFD2; /*@ ds:EFD2 */               /* the mode the driver runs a script in (src/E0ACD.C) */
extern char gA9EC; /*@ ds:A9EC */              /* set by a handler to stop the driver's script walk */
extern int gF50E; /*@ ds:F50E */               /* the depth of the far pointer stack the driver pushes to */
extern void far *gScriptStack[]; /*@ ds:F444 */ /* the ten-deep far pointer stack the driver pushes to */
extern void far *gBuf8DCA; /*@ ds:8DCA */      /* start-up far allocation (src/E07A9.C) */
extern int gCurLevel; /*@ ds:FBC8 */               /* the current action (src/E0ACD.C) */
extern unsigned long gActionFlags[]; /*@ ds:FBD4 */ /* per-action 32-bit masks, indexed by gCurLevel */
extern unsigned long gGameFlags; /*@ ds:FC04 */     /* effect flags (src/E08E6.C) */
extern char gMazeItems[]; /*@ ds:B899 */            /* the nine-byte record list head (src/E0ACD.C) */
extern int gFBCE; /*@ ds:FBCE */               /* the item index shown by the mouse (src/E07A9.C) */
extern int gScriptCell; /*@ ds:B800 */               /* the cell whose script is running (122E:1DA8) */
extern int gFB36; /*@ ds:FB36 */               /* the driver mode the script is running in (122E:1DA8) */
extern char gDragActive; /*@ ds:5C77 */        /* nonzero while a member is picked for arming (122E:0154) */
extern int gDragMember; /*@ ds:5C74 */         /* the member picked for arming (122E:0154) */
extern signed char g5C76; /*@ ds:5C76 */       /* the mode byte 03B4 reads (src/E1414.C) */
extern void ArmEffectWith(int idx, int arg, int sub, int active); /*@ 1414:055B */
extern void AdjustMemberStatus(int who); /*@ 1414:03B4 */
extern void Obj1505(int kind, int id, int frame, int orient, int b0, int m07, int type, int timer, int m12, int m14); /*@ 08E6:1505 */
extern void MarkListItems(void far *head); /*@ 0BD2:040E */
extern int ScanListItem(void far *head, int target, int type, int count); /*@ 0BD2:0385 */
extern void DrainListItem(int cell, int type); /*@ 0BD2:04AA */
extern int TakeItemFromSlot(int who, int row); /*@ 0BD2:0106 */
extern int RemoveListItem(void far *head, int key, int item); /*@ 0BD2:02B9 */
extern void PushListItem(int who, int item, int value); /*@ 0BD2:0458 */
extern int AddDDEffect(int who, int item, int cell, int kind, int page, int type); /*@ 0BD2:07DE */
extern void StartCellEffect(char a, int b, int c, int d, int e); /*@ 0EF3:097B */
extern char gClericSpellMode; /*@ ds:F43D */              /* cleared around the StartCellEffect call (122E:0C4D) */
extern char g5A40[]; /*@ ds:5A40 */            /* byte table indexed by two script bytes (122E:0C4D) */
extern void DrawItemRow(int who, int slot, int one); /*@ 0BD2:0917 */
extern int GetCurrentItem(void); /*@ 0D1C:02DC */
extern int gMouseDisabled; /*@ ds:814A */      /* nonzero disables the mouse (src/E07A9.C) */
extern char gCursorSlot; /*@ ds:F51A */              /* the selected action row: 0..2 in the action menu
                                                  and 0x10 for the character's own item list (tested
                                                  in ItemSlotAction, 122E:06D0) (src/E0ACD.C) */
extern signed char gInventoryMember; /*@ ds:A99A */       /* the selected character index (src/E0ACD.C) */
extern signed char g181F[]; /*@ ds:181F */     /* byte table indexed by an item icon (src/E1414.C) */
extern int ClickInRect(int left, int top, int right, int bottom); /*@ 082C:0A5D */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void PlaySoundAtCell(int sound, int cell); /*@ 177D:0106 */
extern char gFB76; /*@ ds:FB76 */              /* set by opcode 0xFFE4, cleared when the driver starts (122E:1097, 122E:1DA8) */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern char gFileNameBuf[]; /*@ ds:FE6E */     /* the text buffer sprintf formats into (src/E082C.C) */
extern int gForceDoorRoll[]; /*@ ds:59B0 */    /* force-door roll thresholds, indexed by the strength score CharRec.strength (122E:0325) */
extern signed char g1905[]; /*@ ds:1905 */     /* per-object-kind handler selector (src/E0DE8.C) */
extern char gPanelMode; /*@ ds:FB44 */              /* a mode byte: 0, 1 and 2 gate the action handlers
                                                  (src/E0ACD.C); MoveCarriedItemOp (122E:0E9B) skips
                                                  slots 0 and 1 when it is 0 and exits when it is
                                                  nonzero and the member is on screen */
extern char gF542[]; /*@ ds:F542 */            /* a message buffer (src/E0ACD.C) */
extern char gFlag; /*@ ds:F3B9 */              /* a mode byte (src/E0ACD.C) */
extern int Action01E3(void far *action); /*@ 0ACD:01E3 */
extern void RedrawPartyPanel(char far *text); /*@ 0EF3:2541 */
extern void LoadMapScreen(int n); /*@ 210C:007A */  /* overlay 3 entry, ovl3:0A7C */
extern void SaveLevelState(int level); /*@ 210C:002F */  /* overlay 3 entry, ovl3:11AF */
extern void ResetLevel(void); /*@ 210C:007F */  /* overlay 3 entry, ovl3:19F7 */
extern void NoOpAfterMapLoad(void); /*@ 2116:0043 */  /* overlay 4 entry, ovl4:2039, a no-op (src/EOV4.C) */
extern void LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode); /*@ 082C:051B */
extern char gText5BA1[]; /*@ ds:5BA1 */        /* the "INVENT" screen name (122E:1AAB) */
extern unsigned char far gSheet2067[]; /*@ 2067:0000 */ /* the sheet at segment 2067 (src/E0ACD.C) */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern int gTextAttr1; /*@ ds:811D */          /* the first text attribute (src/E07C8.C) */
extern int gTextAttr2; /*@ ds:811B */          /* the second text attribute (src/E07C8.C) */

/* The 52-byte click-region records at ds:E06B, one chain per object kind (122E:0055 walks `next`).
   `kind` indexes gRectKinds; bit 0 of `flags` mirrors `x` about the 176-pixel screen width. */
struct SlotRegion {
    unsigned char kind;                         /* +00: index into gRectKinds */
    char pad01[8];
    signed char next;                           /* +09: next record, 0 or negative ends the walk */
    char flags;                                 /* +0A: bit 0 mirrors x */
    char pad0B[2];
    int x;                                      /* +0D: left edge */
    char pad0F[0x12];
    int y;                                      /* +21: top edge */
    char pad23[0x11];
};

struct RectKind {                               /* the per-kind size record gRectKinds points at */
    char pad00;
    signed char height;                         /* +1: base height in pixels */
    signed char width;                          /* +2: base width in eighths */
};

extern struct SlotRegion gSlotRegions[]; /*@ ds:E06B */
extern struct RectKind far *gRectKinds[]; /*@ ds:EFD4 */
extern char far *gEF9E; /*@ ds:EF9E */         /* a far byte array the clicked member row indexes (122E:04CA) */
extern signed char gViewFrontSide; /*@ ds:A9C6 */       /* the member row the mouse is over (src/E0ACD.C) */
extern char g59F8[]; /*@ ds:59F8 */            /* maps a member row (gEF9E[gViewFrontSide]) to a gDoorButtons entry (122E:04CA) */
extern char g59D8[]; /*@ ds:59D8 */            /* maps a member row to a column offset (122E:04CA) */
extern int g1E75; /*@ ds:1E75 */               /* the viewport x origin added to a slot's x (122E:04CA) */

/* The six-byte slot-position records at ds:A99C (src/EOV3.C calls them Triple6): a far pointer to
   a size record, then the slot's x and y. */
struct SlotSize {                               /* the 3-byte size record a gDoorButtons entry points at */
    char pad00;
    signed char height;                         /* +1: base height in pixels */
    signed char width;                          /* +2: base width in eighths */
};
struct SlotPos {
    struct SlotSize far *rec;                   /* +0: the far size record */
    unsigned char x;                            /* +4 */
    unsigned char y;                            /* +5 */
};
extern struct SlotPos gDoorButtons[]; /*@ ds:A99C */

struct CharRec {                                /* the 243-byte character records at ds:F580 */
    char pad00;
    unsigned char status;                       /* +01: bit 0 is live */
    char name[0x0B];                            /* +02: the character's name, NUL padded */
    signed char strength;                       /* +0D: the strength score, the index 122E:0325
                                                   picks gForceDoorRoll by (src/E0A72.C) */
    char pad0E;
    signed char strengthPct;                    /* +0F: the exceptional strength percentile (src/E0A72.C) */
    char pad10[0x1B - 0x10];
    signed char hp;                             /* +1B: 122E:0325 skips a character at or below 0 */
    char pad1C[0x1F - 0x1C];
    signed char race;                           /* +1F: the race index, halved and matched by 122E:147F (src/EOV0.C) */
    signed char classIndex;                     /* +20: indexes gClassBaseBits (122E:147F) */
    char pad21[0x77 - 0x21];
    int items[0x1b];                            /* +77: 27 item slots */
    char padAD[0xE3 - 0xAD];
    long effectFlags;                           /* +E3: bit 2 tested by 122E:147F */
    char padE7[0xf3 - 0xE7];
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
    signed char m0A;        /* +0xc (ds:8DDC): gCurLevel or -1, set by 01D0 */
    signed char value;        /* +0xd (ds:8DDD): the item's bonus or effect value: the signed bonus
                               printed as "%s +%d" and "Cursed %s %d", the spell index into ds:1278
                               and ds:12E0, the wand effect index g38FA[m0B], and the scroll and
                               potion values (0BD2:05B5, 0D2D, 0EE8, 1001) */
};

extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct ItemRecord gItems[]; /*@ ds:8DD0 */
extern int gPartyDir; /*@ ds:FBCC */               /* the party's facing, 0..3 north, east, south, west */
extern char gPickViewPos; /*@ ds:EFB2 */              /* set when the sound page changes (src/E0ACD.C) */
extern char gViewDirty; /*@ ds:DF4C */              /* set to 1 when an action took effect, which this
                                                  unit calls marking the action dirty (src/E08E6.C) */

/* The 18-byte effect records at ds:DD2C, 10 of them (src/E1414.C). */
struct Projectile {
    char active;                                /* +00: 1 when the record is live */
    char pad01;
    int m02;                                    /* +02 */
    int cell;                                    /* +04: a character or object index */
    char m06;                                   /* +06 */
    signed char dir;                            /* +07 */
    char pad08[0x12 - 0x08];
};

extern struct Projectile gProjectiles[]; /*@ ds:DD2C */    /* 10 records, 18 bytes each */

struct Monster {                                  /* the 28-byte object records at ds:8A82 (src/E08E6.C) */
    char pad00[2];
    int cell;                                     /* +2 (ds:8A84): the object id */
    char cellPos;                                 /* +4 (ds:8A86) */
    char pad05[0x08 - 5];
    signed char type;                           /* +8 (ds:8A8A): the object type, cleared by 122E:1097 */
    char pad09[0x16 - 9];
    char m16;                                   /* +0x16 (ds:8A98): a bit flags byte set by 122E:1097 */
    char pad17[0x1c - 0x17];
};
extern struct Monster gMonsters[]; /*@ ds:8A82 */
extern void MoveObjectToId(struct Monster far *obj, int id, int orient); /*@ 08E6:0297 */

extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void SetMouseItem(int n); /*@ 07A9:010C */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */

extern void AddPartyExperience(int amount); /*@ 0DE8:0C2B */
extern void HealCharacter(int who, int amount); /*@ 0DE8:0ADB */
extern void DamageCharacter(int who, int amount); /*@ 0DE8:09B9 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int f0D1C_0163(int who, int fieldB); /*@ 0D1C:0163 */
extern void CharOverlay(int mode); /*@ 211E:0057 */   /* overlay 5 entry, ovl5:1A82 (src/EOV5.C) */
extern void Obj0150(void); /*@ 08E6:0150 */
extern void UpdateDDEffects(void); /*@ 1414:0D63 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern int IsMouseInRect(int left, int top, int right, int bottom); /*@ 1A72:02A6 */
extern int StepCell(int cell, int dir); /*@ 0D1C:00DA */
extern int gPartyCell; /*@ ds:FBCA */        /* the party's map cell: MoveParty (122E:0001)
                                                sets it, RunRowHandler (122E:0933) steps from it
                                                with StepCell, LoadLevelOp (122E:1AAB) stores the
                                                level's start cell here */
extern int gStepCount; /*@ ds:F540 */               /* counts the party's steps (MoveParty) */
extern char gWallTypeFlags[]; /*@ ds:194B */             /* per-object bit flags (src/E08E6.C) */
extern char gStrNewline[]; /*@ ds:5AB5 */        /* a carriage return, drawn on every step (122E:0001) */
extern char gText5B78[]; /*@ ds:5B78 */        /* message drawn for a highlighted member slot (122E:0847) */
void RunCellScript(int cell, int mode);  /* forward: 1DA8 comes after its callers */

/* 122E:0001: moves the party to map cell `cell`: runs Obj0150 around the move, runs the old
   cell's script on leaving (mode 2) and the new cell's on entering (mode 1), counts the step,
   resets gPickViewPos to 0xFF, sets gViewDirty, scrolls the message area and
   refreshes the effects. The movement keys (0ACD:04F8 to 05FF) call it with StepIfOpen's cell. */
/*@ 122E:0001 */ void MoveParty(int cell)
{
    int old;

    Obj0150();
    old = gPartyCell;
    gPartyCell = cell;
    RunCellScript(old, 2);
    RunCellScript(cell, 1);
    Obj0150();
    gStepCount++;
    gPickViewPos = (char)0xff;
    gViewDirty = 1;
    DrawText(gStrNewline);
    UpdateDDEffects();
}
/* 122E:0055: hit-test the click regions of object kind `kind` against the last click. Only the
   script-driving mode 0x40 has regions (any other mode returns 1, so every kind is "hit"). `kind` is
   a 1-based record number: the walk starts at it, steps to `i - 1` and reads that record's x/y
   (grown by 3) and its gRectKinds size (width in eighths times 8, both plus 6) to form the
   rectangle, mirrored about the screen width when the record's bit 0 is set. It stops at the first
   hit or a `next` of 0 or less (the record's `next` is a 1-based record number). Returns 1 when any
   region was hit. */
/*@ 122E:0055 */ int HitTestObjectKind(int kind)
{
    register int i;
    register int found;
    int left;
    int top;
    int idx;
    int width;
    int height;

    i = kind;
    if (gEFD2 != 0x40)
        return 1;
    found = 0;
    do {
        i--;
        left = gSlotRegions[i].x - 3;
        top = gSlotRegions[i].y - 3;
        idx = gSlotRegions[i].kind;
        width = gRectKinds[idx]->width * 8 + 6;
        height = gRectKinds[idx]->height + 6;
        if (gSlotRegions[i].flags & 1)
            left = 0xb0 - left - width;
        found |= ClickInRect(left, top, left + width, top + height);
        i = gSlotRegions[i].next;
    } while (i > 0 && !found);
    return found;
}

/* 122E:0137: returns bit 0 of the gMaze record `id`'s byte +4 (this file calls +4 the frame-count
   byte; the name is a placeholder). Called by 147F for one sub-opcode, which pushes the result.
   Purpose unknown: the opcode it implements is not identified. */
/*@ 122E:0137 */ int ObjectCountIsOdd(int id)
{
    if (gMaze[id * 9 + 4] & 1)
        return 1;
    return 0;
}

/* 122E:0154: arm effect 0xC on member `who`. While a member is picked (gDragActive), arming either
   matches the picked member (and turns g5C76 on) or does nothing. Otherwise it picks the flag slot
   that bit 3 of the first flag byte selects (0 when set, else 1) and, when that slot's bit 0 is not
   already set, arms the effect twice around AdjustMemberStatus. The flag test is written `!!(x & 1)`:
   the original sign-extends the gWallTypeFlags byte before testing bit 0 (`mov al; cwde; test ax,1`), while a
   plain `x & 1` folds to a memory byte test in our build. */
/*@ 122E:0154 */ void ArmMemberEffect(int who)
{
    int idx;

    if (gDragActive != 0) {
        if (gDragMember == who)
            g5C76 = 1;
        return;
    }
    idx = (gWallTypeFlags[gMaze[who * 9]] & 8) ? 0 : 1;
    if (!!(gWallTypeFlags[gMaze[who * 9 + idx]] & 1))
        return;
    ArmEffectWith(0xc, who, 1, 1);
    AdjustMemberStatus(who);
    ArmEffectWith(0xc, who, 1, 1);
}

/* 122E:01D9: disarm effect 0xC on member `who`. It does nothing while the member still has a frame
   count (gMaze[who * 9 + 4] & 7). While a member is picked, disarming either matches the picked
   member (and sets g5C76 to -1, a mode src/E1414.C:322-324 reads) or does nothing. Otherwise it arms
   the effect with arg -1 twice around AdjustMemberStatus when the flag slot is armed. */
/*@ 122E:01D9 */ void DisarmMemberEffect(int who)
{
    int idx;

    if (gMaze[who * 9 + 4] & 7)
        return;
    if (gDragActive != 0) {
        if (gDragMember == who)
            g5C76 = (signed char)0xff;
        return;
    }
    idx = (gWallTypeFlags[gMaze[who * 9]] & 8) ? 0 : 1;
    if ((gWallTypeFlags[gMaze[who * 9 + idx]] & 1) == 0)
        return;
    ArmEffectWith(0xc, who, 0xffff, 1);
    AdjustMemberStatus(who);
    ArmEffectWith(0xc, who, 0xffff, 1);
}
/* 122E:0271: pick the arm slot for member `who` and arm effect 0xC: a slot of -1 is replaced by the
   free flag slot (0 or 1). While a member is picked, arming either matches the picked member (and
   negates g5C76) or does nothing. When the chosen slot is armed the effect is armed with arg -1
   unless bit 0 of the member's frame byte is set, in which case it does nothing; otherwise with +1,
   twice around AdjustMemberStatus. */
/*@ 122E:0271 */ void ToggleArmEffect(int who, int slot)
{
    int idx;

    if (gDragActive != 0) {
        if (gDragMember == who)
            g5C76 = g5C76 * -1;
        return;
    }
    if (slot == -1)
        slot = (gWallTypeFlags[gMaze[who * 9]] & 8) ? 0 : 1;
    if (gWallTypeFlags[gMaze[who * 9 + slot]] & 1)
        idx = -1;
    else
        idx = 1;
    if (idx == -1 && (gMaze[who * 9 + 4] & 1))
        return;
    ArmEffectWith(0xc, who, idx, 1);
    AdjustMemberStatus(who);
    ArmEffectWith(0xc, who, idx, 1);
}
/* 122E:0325: the force-door action. It only runs when the last click was in the member-slot
   rectangle or the driver is not in mode 0x40. It picks the live member with the highest strength
   strength score (among those with hp above 0), a tie going to the later index; with no candidate
   it formats "Nobody is able to force the door." into gFileNameBuf (not drawn here). Otherwise it
   rolls 1d20: on a pass (roll <= gForceDoorRoll[its strength]) it draws "The party forces the door.",
   formats "%s forces the door!" into gFileNameBuf (not drawn here), rewrites the clicked slot's
   gMaze byte and the opposite slot (slot ^ 2) to 8 when the byte was 0x1e else to 0x12, and arms the
   member; on a miss it draws "The party tries to force the door and fails." All three outcomes share
   one return 1. Returns 0 only when the click missed and the driver is in mode 0x40.
   QUIRK 122E:0325: the pick test adds gChars[who].strengthPct to both sides, so the strengthPct term cancels and the
   test reduces to gChars[who].strength <= gChars[i].strength. */
/*@ 122E:0325 */ int ForceDoorAction(int id, int slot)
{
    register int who;
    register int i;

    if (ClickInRect(0x28, 0x10, 0x88, 0x58) || gEFD2 != 0x40) {
        who = -1;
        for (i = 0; i < 6; i++) {
            if ((gChars[i].status & 1) == 0)
                continue;
            if (gChars[i].hp <= 0)
                continue;
            if (who >= 0) {
                if (gChars[who].strength + gChars[who].strengthPct <= gChars[i].strength + gChars[who].strengthPct)
                    who = i;
            } else
                who = i;
        }
        if (who == -1) {
            sprintf(gFileNameBuf, "Nobody is able to force the door.\r");
        } else if (RollDice(1, 0x14, 0) <= gForceDoorRoll[gChars[who].strength]) {
            DrawText("The party forces the door.\r");
            sprintf(gFileNameBuf, "%s forces the door!\r", gChars[who].name);
            if (gMaze[id * 9 + slot] == 0x1e) {
                gMaze[id * 9 + slot] = 8;
                gMaze[id * 9 + (slot ^ 2)] = 8;
            } else {
                gMaze[id * 9 + slot] = 0x12;
                gMaze[id * 9 + (slot ^ 2)] = 0x12;
            }
            ArmMemberEffect(id);
        } else {
            DrawText("The party tries to force the door and fails.\r");
        }
        return 1;
    }
    return 0;
}
/* 122E:04CA: the member-slot click handler. It turns the clicked row (gEF9E[gViewFrontSide]) into a gDoorButtons
   slot entry (through g59F8), builds the slot's screen rectangle from the entry's x/y and the
   far size record's width (in eighths) and height, all grown by 4 on each side and offset by g1E75,
   and for the item actions (gCurLevel 4, 5, 6) shifts x by the scroll column (row - g59D8[row]) * 9, or
   by 4 when the row is at least 0x1e. A hit, or a driver not in mode 0x40, toggles the arm effect
   and plays sound 6 and returns 1; otherwise it defers to CheckMemberSlotRect and returns its
   result. */
/*@ 122E:04CA */ int MemberSlotClickAction(int id, int slot)
{
    register int x;
    register int row;
    int y;
    int idx;
    int width;
    int height;
    int top;
    struct SlotSize far *rec;

    row = gEF9E[gViewFrontSide];
    idx = g59F8[row];
    rec = gDoorButtons[idx].rec;
    top = g1E75;
    x = gDoorButtons[idx].x + top - 4;
    y = gDoorButtons[idx].y - 4;
    width = rec->width * 8 + 8;
    height = rec->height + 8;
    switch (gCurLevel) {
    case 4:
    case 5:
    case 6:
        if (gEF9E[gViewFrontSide] < 0x1e)
            x += (row - g59D8[row]) * 9;
        else
            x += 4;
        break;
    }
    if (ClickInRect(x, y, x + width, y + height) || gEFD2 != 0x40) {
        ToggleArmEffect(id, slot);
        PlaySound(6);
        return 1;
    }
    return CheckMemberSlotRect(id, slot);
}

/* 122E:05DF: look up the object kind in gMaze[id * 9 + slot] and, if HitTestObjectKind accepts it, run the
   driver for `id` in the current mode gEFD2. Returns 1 when it ran, 0 otherwise. */
/*@ 122E:05DF */ int PerformItemSlotAction(int id, int slot)
{
    int kind;

    kind = gWallDecorHead[gMaze[id * 9 + slot]];
    if (HitTestObjectKind(kind)) {
        RunCellScript(id, gEFD2);
        return 1;
    }
    return 0;
}

/* 122E:0620: raise the count in gMaze[id * 9 + slot] when HitTestObjectKind accepts the object kind,
   mark the action dirty and run the driver. Returns 1 when it ran, 0 otherwise. */
/*@ 122E:0620 */ int IncreaseItemSlot(int id, int slot)
{
    int kind;

    kind = gWallDecorHead[gMaze[id * 9 + slot]];
    if (HitTestObjectKind(kind)) {
        gMaze[id * 9 + slot]++;
        gViewDirty = 1;
        RunCellScript(id, gEFD2);
        return 1;
    }
    return 0;
}

/* 122E:0678: lower the count in gMaze[id * 9 + slot] when HitTestObjectKind accepts the object kind,
   mark the action dirty and run the driver. Returns 1 when it ran, 0 otherwise. */
/*@ 122E:0678 */ int DecreaseItemSlot(int id, int slot)
{
    int kind;

    kind = gWallDecorHead[gMaze[id * 9 + slot]];
    if (HitTestObjectKind(kind)) {
        gMaze[id * 9 + slot]--;
        gViewDirty = 1;
        RunCellScript(id, gEFD2);
        return 1;
    }
    return 0;
}
/* 122E:06D0: the item-slot action: a click on the item list of object `id` (slot `slot` of its gMaze
   record names the object kind). It does nothing and returns 0 unless HitTestObjectKind accepts the
   kind. Then, with the mouse disabled and row 0x10 selected, it removes the list's item and either
   drops it into slot 0x10 (items[16]) of the selected character (when the item type is 0x10) or says
   it cannot go there and pushes it back. Otherwise, with nothing shown on the mouse it takes the list's item
   onto the mouse (when the list has one), and with an item shown it inserts that item into the list
   unless its g181F size says it is too large. Returns 1 once the kind is accepted.
   The `if (item != 0)` around the take branch and the one shared `return 1` after the if/else-if
   chain are what the original's layout needs: an early `return 1` for an empty list puts the jump
   on the shared return instead of the last `jmp` stub (122E:083C) the original jumps to. */
/*@ 122E:06D0 */ int ItemSlotAction(int id, int slot)
{
    register int item;
    int kind;

    kind = gWallDecorHead[gMaze[id * 9 + slot]];
    if (HitTestObjectKind(kind)) {
        if (gMouseDisabled != 0 && gCursorSlot == 0x10) {
            item = RemoveListItem(&gMazeItems[id * 9], 8, -1);
            if (gItems[item].type == 0x10) {
                InsertListItem(&gChars[gInventoryMember].items[16], -2, item, 0);
                DrawItemRow(0x10, 1, 0);
            } else {
                DrawText("You can't put that item there.\r");
                PushListItem(id, item, 8);
            }
            return 1;
        }
        if (GetCurrentItem() == 0) {
            item = RemoveListItem(&gMazeItems[id * 9], 8, -1);
            if (item != 0) {
                RunCellScript(id, 8);
                SetMouseItem(item);
                gViewDirty = 1;
            }
        } else if (g181F[gItems[GetCurrentItem()].icon] > 0xe) {
            InsertListItem(&gMazeItems[id * 9], id, GetCurrentItem(), 8);
            RunCellScript(id, 4);
            SetMouseItem(0);
            gViewDirty = 1;
        } else {
            DrawText("The item is too large to fit.\r");
        }
        return 1;
    }
    return 0;
}

/* 122E:0847: when the mouse is over the member slot's rectangle and the slot's object has flag 0x20
   in gWallTypeFlags, draw the slot's message. Returns 1 when it drew, 0 otherwise. */
/*@ 122E:0847 */ int CheckMemberSlotRect(int id, int slot)
{
    if (IsMouseInRect(0x28, 0x10, 0x88, 0x58)) {
        if (gWallTypeFlags[gMaze[id * 9 + slot]] & 0x20) {
            DrawText(gText5B78);
            return 1;
        }
    }
    return 0;
}
/* 122E:0896: the row handler. It flips bit 1 of the slot, looks the object there up in gMaze, and
   when its g1905 selector is nonzero, the driver mode gEFD2 ANDed with the mask
   ((gMaze[id*9+4] & 0xf8) >> 3) | 0xe0 (the member's byte +4 bits 3 to 7, with 0xe0 set) is nonzero
   and the selector minus one is not negative, calls that handler (a local table of far function
   pointers copied from the compiler template at ds:5A18) with (id, slot). Then it clears the driver
   mode and marks the action dirty. Returns 0 when a guard fails, else the handler's result. */
/*@ 122E:0896 */ int DispatchSlotHandler(int id, int slot)
{
    int (far *handlers[10])(int, int) = {
        MemberSlotClickAction, PerformItemSlotAction, IncreaseItemSlot, DecreaseItemSlot,
        ForceDoorAction, CheckMemberSlotRect, 0, 0, 0, ItemSlotAction
    };
    int idx;
    int mask;
    int result;

    slot ^= 2;
    idx = gMaze[id * 9 + slot];
    if (!g1905[idx])
        return 0;
    mask = (((unsigned char)gMaze[id * 9 + 4] & 0xf8) >> 3) | 0xe0;
    if (!(gEFD2 & mask))
        return 0;
    if (g1905[idx] - 1 < 0)
        return 0;
    result = handlers[g1905[idx] - 1](id, slot);
    gEFD2 = 0;
    gViewDirty = 1;
    return result;
}

/* 122E:0933: set the driver mode to 0x40, work out the member's row through StepCell and run the
   row handler DispatchSlotHandler. QUIRK 122E:0933: the argument is raised by 0x3c and then never read. */
/*@ 122E:0933 */ void RunRowHandler(int entry)
{
    int x;

    entry += 0x3c;
    gEFD2 = 0x40;
    x = StepCell(gPartyCell, gPartyDir);
    DispatchSlotHandler(x, gPartyDir);
}
/* 122E:0960: opcode that writes gMaze fields: 0xFFE9 stores a byte into gMaze[id * 9 + index],
   0xFFED sets the page gPartyDir, and 0xFFF7 fills the first four bytes of gMaze[id * 9] with a byte.
   Returns the bytes read. */
/*@ 122E:0960 */ long WriteObjectFieldOp(char far *script)
{
    char far *start = script;
    int id;
    char value;
    int i;

    switch (*script++) {
    case 0xFFF7:
        id = *(int far *)script;
        script += 2;
        value = *script++;
        for (i = 0; i <= 3; i++)
            gMaze[id * 9 + i] = value;
        break;
    case 0xFFE9:
        id = *(int far *)script;
        script += 2;
        value = *script++;
        gMaze[id * 9 + value] = *script++;
        break;
    case 0xFFED:
        /* the value goes through the local so the increment stays before the cwde/store: with
           `gPartyDir = *script++;` the compiler defers the increment to the shared tail and merges it
           with the 0xFFE9 case's (122E:09F1, 122E:09FE). */
        value = *script++;
        gPartyDir = value;
        break;
    }
    return (long)(script - start);
}
/* 122E:0A16: opcode that writes a run of four gMaze bytes: 0xFFF7 compares the record's first
   byte with a script byte and fills the first four bytes with the second script byte when they match
   (or the first when they do not); 0xFFEA hands a word and -1 to f122E_0271. Returns the bytes read. */
/*@ 122E:0A16 */ long WriteObjectRunOp(char far *script)
{
    int id;
    int first;
    int second;
    char far *start = script;

    switch (*script++) {
    case 0xFFF7:
        id = *(int far *)script;
        script += 2;
        first = *script++;
        second = *script++;
        if (gMaze[id * 9] == first) {
            gMaze[id * 9] = second;
            gMaze[id * 9 + 1] = second;
            gMaze[id * 9 + 2] = second;
            gMaze[id * 9 + 3] = second;
        } else {
            gMaze[id * 9] = first;
            gMaze[id * 9 + 1] = first;
            gMaze[id * 9 + 2] = first;
            gMaze[id * 9 + 3] = first;
        }
        break;
    case 0xFFEA:
        id = *(int far *)script;
        script += 2;
        ToggleArmEffect(id, -1);
        break;
    }
    return (long)(script - start);
}
/* 122E:0B23: opcode that arms member effects: read a member index and hand it to f122E_0154.
   Returns the bytes read. */
/*@ 122E:0B23 */ long ArmMember(char far *script)
{
    char far *start = script;
    int who;

    who = *(int far *)script;
    script += 2;
    ArmMemberEffect(who);
    return (long)(script - start);
}

/* 122E:0B56: opcode that disarms member effects: read a member index and hand it to f122E_01D9.
   Returns the bytes read. */
/*@ 122E:0B56 */ long DisarmMember(char far *script)
{
    char far *start = script;
    int who;

    who = *(int far *)script;
    script += 2;
    DisarmMemberEffect(who);
    return (long)(script - start);
}
/* 122E:0B89: opcode that spawns an object: read an unused byte, then the ten Obj1505 arguments
   (eight bytes/words from the script and the two trailing words) and hand them on. Returns the bytes
   read. */
/*@ 122E:0B89 */ long SpawnObjectOp(char far *script)
{
    int kind;
    int id;
    int frame;
    int orient;
    int b0;
    int m07;
    int type;
    int timer;
    int m12;
    int m14;
    char far *start;

    start = script;
    script++;
    kind = *script++;
    id = *(int far *)script;
    script += 2;
    frame = *script++;
    orient = *script++;
    b0 = *script++;
    m07 = *script++;
    type = *script++;
    timer = *script++;
    m12 = *(int far *)script;
    script += 2;
    m14 = *(int far *)script;
    script += 2;
    Obj1505(kind, id, frame, orient, b0, m07, type, timer, m12, m14);
    return (long)(script - start);
}
/* 122E:0C4D: the 0xDF/other effect-adding opcode. It reads the first byte (0xDF sets `flag`), a
   byte v1, then skips one byte that is never read (part of the record's layout), a word, and two
   more bytes v2 and v3. When the first byte was not 0xDF it copies v1's item into a free slot and
   adds a DD effect with the word as the cell and g5A40[v2 * 2 + v3] as the kind; a zero result
   frees the slot again (m04 = -1). When it was 0xDF it clears gClericSpellMode around a StartCellEffect call
   with the same fields. Returns the bytes read. */
/*@ 122E:0C4D */ long AddEffectOp(char far *script)
{
    char v1;
    char v2;
    char v3;
    char saved;
    char flag;
    char far *start = script;
    int id;
    int item;

    flag = (*script == (char)0xdf);
    script++;
    v1 = *script;
    script += 2;
    id = *(int far *)script;
    script += 2;
    v2 = *script++;
    v3 = *script++;
    if (!flag) {
        item = CopyItemToFreeSlot(v1);
        if (item != 0) {
            if (AddDDEffect(-1, item, id, g5A40[v2 * 2 + v3], v2, gItems[item].type) == 0)
                gItems[item].key = -1;
        }
    } else {
        saved = gClericSpellMode;
        gClericSpellMode = 0;
        StartCellEffect(-1, v1, id, g5A40[v2 * 2 + v3], v2);
        gClericSpellMode = saved;
    }
    return (long)(script - start);
}
/* 122E:0D57: reassign objects, items and effects from one id to another. It reads an opcode (a byte)
   and, for every opcode but 0xFFF1, a source id `srcId`; 0xFFF1 reads only `id`. Then it reads
   `id`. For 0xFFE8 it moves the party to cell `id` (MoveParty), for 0xFFF3 it moves every gMonsters record whose
   id matches `srcId` to `id`, and for 0xFFF5 it moves every item of the gMazeItems list of `srcId` to the
   list of `id` (keeping each item's old m03 key in `oldKey`) and retargets every live gProjectiles effect
   from `srcId` to `id`. Sets gViewDirty. Returns the bytes read. */
/*@ 122E:0D57 */ long ReassignObjectsOp(char far *script)
{
    struct Monster far *obj;
    struct Projectile far *rec;
    int oldKey;
    int op;
    int srcId;
    int item;
    char far *start = script;
    register int id;
    register int i;

    if ((op = *script++) != 0xfff1) {
        srcId = *(int far *)script;
        script += 2;
    }
    id = *(int far *)script;
    script += 2;
    switch (op) {
    case 0xffe8:
        MoveParty(id);
        break;
    case 0xfff3:
        obj = gMonsters;
        for (i = 0; i < 0x1e; i++) {
            if (obj->cell == srcId)
                MoveObjectToId(obj, id, obj->cellPos);
            obj++;
        }
        break;
    case 0xfff5:
        while ((item = ScanListItem(&gMazeItems[srcId * 9], -1, -1, 0)) != 0) {
            oldKey = gItems[item].cellPos;
            RemoveListItem(&gMazeItems[srcId * 9], 0, item);
            InsertListItem(&gMazeItems[id * 9], id, item, oldKey);
        }
        rec = gProjectiles;
        for (i = 0; i < 0xa; i++) {
            if (rec->active == 1 && rec->cell == srcId)
                rec->cell = id;
            rec++;
        }
        break;
    }
    gViewDirty = 1;
    return (long)(script - start);
}
/* 122E:0E9B: move one item out of a party member's carried list. It reads a member index `who` (a
   random live member when -1), a destination list `id` and a trailing byte. It does nothing when
   gPanelMode is nonzero and the member is the one on screen (gInventoryMember == who). Otherwise it scans the
   member's 27 slots from a random start for a carried item whose g181F icon size is at least 0xf:
   when gPanelMode is nonzero every slot is eligible, otherwise slots 0 and 1 are skipped, and slot 0x10
   is always skipped. It clears the found slot and inserts the item into the gMazeItems list of `id`.
   Returns the bytes read. */
/*@ 122E:0E9B */ long MoveCarriedItemOp(char far *script)
{
    int id;
    int item;
    int count;
    int third;
    char far *start = script;
    register int who;
    register int idx;

    who = *script++;
    id = *(int far *)script;
    script += 2;
    third = *script++;
    if (who == -1)
        who = RollDice(1, 6, -1);
    while (!gChars[who].status) {
        who++;
        if (who > 5)
            who = 0;
    }
    if (gPanelMode != 0 && gInventoryMember == who)
        goto done;
    item = 0;
    idx = RollDice(1, 0x1b, 0);
    for (count = 0; count < 0x1b; count++) {
        if ((gPanelMode != 0 || idx > 1) && idx != 0x10) {
            item = gChars[who].items[idx];
            if (item != 0 && g181F[gItems[item].icon] >= 0xf)
                break;
        }
        idx++;
        if (idx >= 0x1b)
            idx = 0;
    }
    if (count < 0x1b && item != 0) {
        gChars[who].items[idx] = 0;
        InsertListItem(&gMazeItems[id * 9], id, item, third);
    }
done:
    return (long)(script - start);
}
/* 122E:0FDC: the show-text opcode. It skips the script to the text's NUL, then reads two bytes
   into a 5-byte control-code buffer initialised from the compiler template at ds:5A48, hides the
   pointer, draws the control buffer, the text, the buffer with the two saved text attributes and a
   newline, then shows the pointer. Returns the bytes read. */
/*@ 122E:0FDC */ long ShowTextOp(char far *script)
{
    int attr1;
    int attr2;
    char far *start;
    char far *text;
    char buf[5] = { 6, 0x21, 2, 0x21, 0 };

    start = script;
    text = script;
    while (*script != 0)
        script++;
    script++;
    attr1 = gTextAttr1;
    attr2 = gTextAttr2;
    buf[1] = *script++;
    buf[3] = *script++;
    LockHidePointer();
    DrawText(buf);
    DrawText(text);
    buf[1] = attr1;
    buf[3] = attr2;
    DrawText(buf);
    DrawText("\r");
    LockShowPointer();
    return (long)(script - start);
}
/* 122E:1097: opcode that sets action flags. 0xFFEF sets the bit in the gActionFlags entry of the
   current action gCurLevel, 0xFFF0 sets it in gGameFlags, 0xFFF3 sets a bit in gMonsters[index].m16 and clears
   gMonsters[index].type, and 0xFFE4 sets gFB76. Returns the bytes read. */
/*@ 122E:1097 */ long SetActionFlagsOp(char far *script)
{
    int op;
    int arg;
    char far *start = script;
    register int idx;

    op = *script++;
    switch (op) {
    case 0xffef:
        arg = *script++;
        gActionFlags[gCurLevel] |= 1L << arg;
        break;
    case 0xfff0:
        arg = *script++;
        gGameFlags |= 1L << arg;
        break;
    case 0xfff3:
        idx = *script++;
        arg = *script++;
        gMonsters[idx].m16 |= 1 << arg;
        gMonsters[idx].type = 0;
        break;
    case 0xffe4:
        gFB76 = 1;
        break;
    }
    return (long)(script - start);
}

/* 122E:1194: opcode that plays a sound: read a sound byte and a cell word; a nonzero cell plays
   the sound at that map cell, otherwise as a global sound. Returns the bytes read. */
/*@ 122E:1194 */ long PlaySoundOp(char far *script)
{
    char far *start = script;
    char sound;
    int cell;

    sound = *script++;
    cell = *(int far *)script;
    script += 2;
    if (cell != 0)
        PlaySoundAtCell(sound, cell);
    else
        PlaySound(sound);
    return (long)(script - start);
}

/* 122E:11E8: opcode that clears one bit of a 32-bit action mask: 0xFFEF clears the bit in the
   gActionFlags entry of the current action gCurLevel, 0xFFF0 clears it in gGameFlags. The byte after the
   opcode is the bit number. Returns the bytes read. */
/*@ 122E:11E8 */ long ClearActionBit(char far *script)
{
    int op;
    int bit;
    char far *start = script;

    op = *script++;
    bit = *script++;
    switch (op) {
    case 0xFFEF:
        gActionFlags[gCurLevel] &= ~(1L << bit);
        break;
    case 0xFFF0:
        gGameFlags &= ~(1L << bit);
        break;
    }
    return (long)(script - start);
}

/* 122E:126E: opcode that heals: read a member index and an amount. A member index of -1 heals
   all six party members, otherwise only that member. Returns the bytes read. */
/*@ 122E:126E */ long HealPartyMember(char far *script)
{
    char far *start = script;
    int who;
    int amount;

    who = *script++;
    amount = *script++;
    if (who == -1) {
        for (who = 0; who < 6; who++)
            HealCharacter(who, amount);
    } else {
        HealCharacter(who, amount);
    }
    return (long)(script - start);
}

/* 122E:12CE: opcode that damages: read a member index, a dice count, sides and bonus. A member
   index of -1 damages all six members: each rolls (count, sides, bonus) when sides is nonzero, else
   takes the count, and takes that roll as its damage unless f0D1C_0163(who, 3) is nonzero.
   Otherwise the one member rolls the same way but DamageCharacter is handed the dice count, not the
   roll. QUIRK 122E:12CE: the single-member path rolls and discards the result (it passes `count`),
   while the all-members path passes the roll as the amount. Returns the bytes read. */
/*@ 122E:12CE */ long DamagePartyMember(char far *script)
{
    int count;
    int sides;
    int bonus;
    int who;
    int roll;
    char far *start = script;

    who = *script++;
    count = *script++;
    sides = *script++;
    bonus = *script++;
    if (who == -1) {
        for (who = 0; who < 6; who++) {
            if (sides != 0)
                roll = RollDice(count, sides, bonus);
            else
                roll = count;
            if (f0D1C_0163(who, 3) == 0)
                DamageCharacter(who, roll);
        }
    } else {
        if (sides != 0)
            roll = RollDice(count, sides, bonus);
        else
            roll = count;
        DamageCharacter(who, count);
    }
    return (long)(script - start);
}

/* 122E:139D: opcode that jumps the script pointer to a word offset from the script base
   gBuf8DCA. Returns the pointer's movement. */
/*@ 122E:139D */ long JumpToScriptOffset(char far *script)
{
    char far *start = script;

    script = (char far *)gBuf8DCA + *(int far *)script;
    return (long)(script - start);
}

/* 122E:13D1: set the driver's stop flag and advance the script pointer by one; the driver stops its
   walk as soon as gA9EC is set. Returns 0 as an int; the driver's handler table declares long and
   casts the result to int (see RunCellScript). */
/*@ 122E:13D1 */ int ScriptStop(char far *script)
{
    gA9EC = 1;
    script++;
    return 0;
}

/* 122E:13E0: opcode that pops the far pointer stack: when it is not empty, take the top entry as
   the new script pointer. Returns the pointer's movement, 0 when the stack was empty. */
/*@ 122E:13E0 */ long PopScriptPointer(char far *script)
{
    char far *start = script;

    if (gF50E != 0) {
        gF50E--;
        script = gScriptStack[gF50E];
    }
    return (long)(script - start);
}

/* 122E:1422: opcode that pushes the far pointer stack: read a word offset, and when the stack is
   under its ten-deep limit, push the position after the operand and continue at gBuf8DCA + offset.
   Returns the pointer's movement. */
/*@ 122E:1422 */ long PushScriptPointer(char far *script)
{
    char far *start = script;
    int offset;

    offset = *(int far *)script;
    script += 2;
    if (gF50E < 10) {
        gScriptStack[gF50E] = script;
        gF50E++;
        script = (char far *)gBuf8DCA + offset;
    }
    return (long)(script - start);
}
/* 122E:147F: the sub-dispatcher the driver reaches for opcode 0xFFEE (handler index 17 of
   RunCellScript's table). It evaluates a postfix script expression over a
   global word stack (gEvalStack/gEvalTop): most sub-opcodes push a value, the binary sub-opcodes
   pop two and push a comparison or a logical result, and 0xFFEE ends the expression, returning how
   far the script moved (jumping to gBuf8DCA + word when the popped value is 0). A sub-opcode byte
   in 0..0x7f is pushed as a literal; a negative one not in the table is a no-op. Case order follows
   the original's body layout, and the case labels are read from the jump table at 122E:19EF (the
   item-type case is 0xFFE7 and the gMaze byte case is 0xFFE9, not 0xFFE8/0xFFEA). The opcodes are
   not in numerical order: 0xFFF8 is the logical or, 0xFFF9 the logical and, and the relational pair
   near the end is 0xFFFF `==`, 0xFFFE `!=`, 0xFFFD `<`, 0xFFFC `<=`, 0xFFFB `>` and 0xFFFA `>=`
   (from the bodies, each of which stores its result through the shared store-0/store-1 tails). */
/*@ 122E:147F */ long EvaluateScriptExpr(char far *script)
{
    int op;
    int value;
    int t1;
    int t2;
    int i;
    register int lhs;
    register int rhs;
    char far *start;

    start = script;
    while ((op = *script++) != 0xFFEE) {
        switch (op) {
        case 0xFFE0:
            gEvalStack[gEvalTop++] = gFB36;
            break;
        case 0xFFED:
            gEvalStack[gEvalTop++] = gPartyDir;
            break;
        case 0xFFF7:
            value = *(int far *)script;
            script += 2;
            gEvalStack[gEvalTop++] = (signed char)gMaze[value * 9];
            break;
        case 0xFFF5:
            t2 = *script++;
            value = *(int far *)script;
            script += 2;
            /* the two arms are one call in source form (`ScanListItem(&gMazeItems[value * 9], -1, t2, 1)`,
               equal when t2 is -1); the split reproduces the original's two call sites (matcher
               notes). */
            if (t2 == -1)
                value = ScanListItem(&gMazeItems[value * 9], -1, -1, 1);
            else
                value = ScanListItem(&gMazeItems[value * 9], -1, t2, 1);
            gEvalStack[gEvalTop++] = value;
            break;
        case 0xFFF3:
            value = *(int far *)script;
            script += 2;
            gEvalStack[gEvalTop++] = ObjectCountIsOdd(value);
            break;
        case 0xFFF2:
            t1 = *(int far *)script;
            script += 2;
            value = *(int far *)script;
            script += 2;
            gEvalStack[gEvalTop++] = ScanListItem(&gMazeItems[value * 9], t1, -1, 0);
            break;
        case 0xFFF1:
            value = *(int far *)script;
            script += 2;
            gEvalStack[gEvalTop++] = (gPartyCell == value);
            break;
        case 0xFFF0:
            /* The original writes the two arms out (each its own store) rather than `... = (cond)
               != 0`: the `!= 0` form makes TCC share the store-0 tail, where the original inlines
               store 0 in each case and shares store 1. */
            value = *script++;
            if (gGameFlags & (1L << value))
                gEvalStack[gEvalTop++] = 1;
            else
                gEvalStack[gEvalTop++] = 0;
            break;
        case 0xFFEF:
            value = *script++;
            if (gActionFlags[gCurLevel] & (1L << value))
                gEvalStack[gEvalTop++] = 1;
            else
                gEvalStack[gEvalTop++] = 0;
            break;
        case 0xFFE7:
            t1 = GetCurrentItem();
            if (*script == (char)0xE1)
                gEvalStack[gEvalTop++] = gItems[t1].type;
            else if (*script == (char)0xF5)
                gEvalStack[gEvalTop++] = t1;
            else
                gEvalStack[gEvalTop++] = gItems[t1].value;
            script++;
            break;
        case 0xFFE9:
            t1 = *script++;
            value = *(int far *)script;
            script += 2;
            gEvalStack[gEvalTop++] = (signed char)gMaze[value * 9 + t1];
            break;
        case 0xFFDD:
            t1 = *script++;
            /* `lhs = i = 0` makes TCC zero AX once and store it to i and then to SI (the register
               scratch lhs), as the original does at 122E:16CD; `lhs = 0; for (i = 0; ...)` instead
               emits `xor si,si; mov word ptr [bp-10],0`. */
            lhs = i = 0;
            for (; i < 6; i++) {
                if ((gChars[i].status & 1) && (gChars[i].race >> 1) == t1) {
                    lhs = 1;
                    break;
                }
            }
            gEvalStack[gEvalTop++] = lhs;
            break;
        case 0xFFDC:
            t1 = *script++;
            lhs = i = 0;
            for (; i < 6; i++) {
                if ((gChars[i].status & 1) && (gClassBaseBits[gChars[i].classIndex] & t1)) {
                    lhs = 1;
                    break;
                }
            }
            gEvalStack[gEvalTop++] = lhs;
            break;
        case 0xFFDB:
            value = *script++;
            t2 = *script++;
            t1 = *script++;
            gEvalStack[gEvalTop++] = RollDice(value, t2, t1);
            break;
        case 0xFFFF:
            rhs = gEvalStack[--gEvalTop];
            lhs = gEvalStack[--gEvalTop];
            gEvalStack[gEvalTop++] = (lhs == rhs);
            break;
        case 0xFFFE:
            rhs = gEvalStack[--gEvalTop];
            lhs = gEvalStack[--gEvalTop];
            gEvalStack[gEvalTop++] = (lhs != rhs);
            break;
        case 0xFFFD:
            rhs = gEvalStack[--gEvalTop];
            lhs = gEvalStack[--gEvalTop];
            gEvalStack[gEvalTop++] = (lhs < rhs);
            break;
        case 0xFFFC:
            rhs = gEvalStack[--gEvalTop];
            lhs = gEvalStack[--gEvalTop];
            gEvalStack[gEvalTop++] = (lhs <= rhs);
            break;
        case 0xFFFB:
            rhs = gEvalStack[--gEvalTop];
            lhs = gEvalStack[--gEvalTop];
            gEvalStack[gEvalTop++] = (lhs > rhs);
            break;
        case 0xFFFA:
            rhs = gEvalStack[--gEvalTop];
            lhs = gEvalStack[--gEvalTop];
            gEvalStack[gEvalTop++] = (lhs >= rhs);
            break;
        case 0xFFF9:
            rhs = gEvalStack[--gEvalTop];
            lhs = gEvalStack[--gEvalTop];
            gEvalStack[gEvalTop++] = (lhs != 0 && rhs != 0);
            break;
        case 0xFFF8:
            rhs = gEvalStack[--gEvalTop];
            lhs = gEvalStack[--gEvalTop];
            gEvalStack[gEvalTop++] = (lhs != 0 || rhs != 0);
            break;
        case 0xFFDA:
            /* The original reuses `lhs` as the loop counter and `rhs` as the flag here (counter in
               SI, flag in DI), unlike the FFDD/FFDC loops (counter in memory, flag in SI), and its
               status test is `(status & 1) == 1` (`mov al; mov ah,0; and ax,1; cmp ax,1`), not the
               byte test `status & 1` those loops use. */
            rhs = 0;
            for (lhs = 0; lhs < 6; lhs++) {
                if ((gChars[lhs].status & 1) == 1 && (gChars[lhs].effectFlags & 4) == 0) {
                    rhs = 1;
                    break;
                }
            }
            gEvalStack[gEvalTop++] = !rhs;
            break;
        default:
            /* the original reloads script[-1] (the byte the loop test consumed) into the register
               scratch lhs, not op: `les bx,[bp+6]; mov al,es:[bx-1]; cwde; mov si,ax` at 122E:197A. */
            lhs = script[-1];
            if (lhs >= 0 && lhs < 0x80)
                gEvalStack[gEvalTop++] = lhs;
            break;
        }
    }
    value = gEvalStack[--gEvalTop];
    if (value != 0)
        script += 2;
    else
        script = (char far *)gBuf8DCA + *(int far *)script;
    return (long)(script - start);
}

/* 122E:1A3B: the drain-item opcode. It reads a byte: 0xFFFF clears slot -1 of member 0, 0xFFFE
   drains every item of cell `cell`'s list (the word read), and any other byte drains that list
   of items of that type. Returns the bytes read. */
/*@ 122E:1A3B */ long DrainItemOp(char far *script)
{
    char far *start = script;
    char op;
    register int cell;

    op = *script++;
    switch (op) {
    case 0xffff:
        TakeItemFromSlot(0, -1);
        break;
    case 0xfffe:
        cell = *(int far *)script;
        script += 2;
        DrainListItem(cell, -1);
        break;
    default:
        cell = *(int far *)script;
        script += 2;
        DrainListItem(cell, op);
        break;
    }
    return (long)(script - start);
}
/* 122E:1AAB: the load-level opcode. It refreshes the display: Action01E3(gF542) when gPanelMode is
   nonzero and RedrawPartyPanel(gF542) when gFlag is nonzero. It reads a byte, a word and a byte,
   stores the word as gPartyCell and the byte as gPartyDir (unless 0xFF), resets and saves the level,
   runs ovl3:0A7C, loads the INVENT screen (gText5BA1) with the 2067:0000 sheet, runs ovl4:2039,
   moves the party to its cell and stops the script driver (gA9EC). Returns 0 as an int; the driver's
   handler table declares long and casts the result to int (see RunCellScript). */
/*@ 122E:1AAB */ int LoadLevelOp(char far *script)
{
    char v1;
    char v2;
    int id;

    if (gPanelMode != 0)
        Action01E3(gF542);
    if (gFlag != 0)
        RedrawPartyPanel(gF542);
    script++;
    v1 = *script++;
    id = *(int far *)script;
    script += 2;
    v2 = *script++;
    gPartyCell = id;
    if (v2 != (char)0xff)
        gPartyDir = v2;
    ResetLevel();
    SaveLevelState(gCurLevel);
    LoadMapScreen(v1);
    LoadScreenImage(gText5BA1, (char far *)gSheet2067, 5, 3, 2);
    NoOpAfterMapLoad();
    MoveParty(id);
    gA9EC = 1;
    return 0;
}

/* 122E:1B5A: opcode that adds party experience: when the byte after the opcode is 0xE2, read a
   word and add it as party experience. Returns the bytes read. */
/*@ 122E:1B5A */ long AddExperienceOp(char far *script)
{
    char far *start = script;
    int exp;

    if (*script++ == (char)0xE2) {
        exp = *(int far *)script;
        script += 2;
        AddPartyExperience(exp);
    }
    return (long)(script - start);
}

/* 122E:1B9A: opcode that copies an item into a free slot and either shows it on the mouse (when
   the destination is -1 and nothing is shown yet) or inserts it into that member's list with the
   trailing sound byte. Returns the bytes read. */
/*@ 122E:1B9A */ long TakeItemOp(char far *script)
{
    char far *start = script;
    int item;
    int dest;

    item = *(int far *)script;
    script += 2;
    dest = *(int far *)script;
    script += 2;
    item = CopyItemToFreeSlot(item);
    if (item != 0) {
        if (dest == -1 && gFBCE == 0) {
            SetMouseItem(item);
        } else {
            InsertListItem(&gMazeItems[dest * 9], dest, item, *script);
        }
    }
    script++;
    return (long)(script - start);
}

/* 122E:1C16: opcode that turns a page: 0xFFF1 rotates gPartyDir by the byte argument (modulo 4) and
   marks the sound page and action dirty; 0xFFF5 rotates dir (called the orientation here; the name
   is a placeholder, src/E1414.C owns the Projectile record) of every live effect record that names the
   running member. Returns the bytes read. */
/*@ 122E:1C16 */ long TurnPageOp(char far *script)
{
    int op;
    int delta;
    struct Projectile far *rec;
    char far *start = script;
    int i;

    op = *script++;
    delta = *script++;
    switch (op) {
    case 0xFFF1:
        gPartyDir = (gPartyDir + delta) & 3;
        gPickViewPos = (char)0xff;
        gViewDirty = 1;
        break;
    case 0xFFF5:
        rec = gProjectiles;
        for (i = 0; i < 10; i++) {
            if (rec->active != 0 && rec->cell == gScriptCell)
                rec->dir = (rec->dir + delta) & 3;
            rec++;
        }
        break;
    }
    return (long)(script - start);
}
/* 122E:1CAD: opcode that marks a member's item list: read a member/character index and, when it
   matches gPartyCell (see its declaration for the open reading), set flag 0x40 on every carried item
   slot (items[0..26]) of each of the six party records (gChars[0..5]) that has bit 0 of its status
   set, and mark each record's list (rec + 0x97). Always marks the gMazeItems list of the index. Returns
   the bytes read. */
/*@ 122E:1CAD */ long MarkMemberItemsOp(char far *script)
{
    int item;
    struct CharRec far *rec;
    int id;
    register int i;
    register int j;
    char far *start = script;

    id = *(int far *)script;
    script += 2;
    if (id == gPartyCell) {
        /* QUIRK 122E:1CAD: `rec = gChars` and the loop's `rec++` are dead, overwritten by the body's
           `rec = &gChars[i]`; they keep the original's register shape (matcher notes). */
        rec = gChars;
        for (i = 0; i < 6; i++, rec++) {
            rec = &gChars[i];
            if (rec->status & 1) {
                for (j = 0; j < 0x1b; j++) {
                    item = rec->items[j];
                    if (item != 0)
                        gItems[item].flags |= 0x40;
                }
                MarkListItems((char far *)rec + 0x97);
            }
        }
    }
    MarkListItems(&gMazeItems[id * 9]);
    return (long)(script - start);
}

/* 122E:1D76: opcode that passes its byte argument to overlay 5 (ovl5:1A82). Returns the bytes
   read. */
/*@ 122E:1D76 */ long CallOverlayOp(char far *script)
{
    char far *start = script;
    CharOverlay(*script++);
    return (long)(script - start);
}

/* 122E:1DA8: the script driver. Given a map cell `cell` and a driver mode, it takes the cell's
   script offset from gMaze[cell * 9 + 7] and returns without running anything when that offset is 0
   or the mode misses the cell's ((gMaze[cell*9+4] & 0xf8) >> 3) | 0xe0 mask. Otherwise it walks
   the far script at gBuf8DCA + offset, dispatching each negative opcode (0xFFE6..0xFFFF) through the
   26-entry far handler table to a handler that returns how many bytes it consumed, until a handler
   sets gA9EC. Records gScriptCell (the cell), gFB36 (mode) and clears gFB76.
   The table is `long (far *)(char far *)`; ScriptStop and LoadLevelOp return int (they always
   return 0, so only AX is set) and the driver casts the result to int. */
/*@ 122E:1DA8 */ void RunCellScript(int cell, int mode)
{
    long (far *handlers[26])(char far *) = {
        WriteObjectFieldOp, WriteObjectRunOp, ArmMember, DisarmMember, SpawnObjectOp,
        ReassignObjectsOp, MoveCarriedItemOp, ShowTextOp, SetActionFlagsOp, PlaySoundOp,
        ClearActionBit, HealPartyMember, DamagePartyMember, JumpToScriptOffset, ScriptStop,
        PopScriptPointer, PushScriptPointer, EvaluateScriptExpr, DrainItemOp, LoadLevelOp,
        AddExperienceOp, TakeItemOp, AddEffectOp, TurnPageOp, MarkMemberItemsOp, CallOverlayOp
    };
    char far *script;
    unsigned char mask;
    register int offset;
    register int op;

    offset = *(int *)(gMaze + cell * 9 + 7);
    if (offset == 0)
        return;
    mask = (((unsigned char)gMaze[cell * 9 + 4] & 0xf8) >> 3) | 0xe0;
    if (!(mode & mask))
        return;
    gFB76 = 0;
    gScriptCell = cell;
    gFB36 = mode;
    gA9EC = 0;
    script = (char far *)gBuf8DCA + offset;
    do {
        op = *script++;
        if (op < 0 && op > -0x1b)
            script += (int)handlers[-(op + 1)](script);
    } while (!gA9EC);
}
