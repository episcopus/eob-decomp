/* Unit at EOB.EXE 122E:0001 (7775 bytes, 40 functions). The game's action script interpreter: the
   driver at 122E:1DA8 walks a member's compiled script with a table of 26 far handler pointers and
   dispatches each opcode byte to one of the handlers below. The handlers take the current script
   pointer (a far char pointer) and return how many bytes the opcode consumed, or jump the pointer;
   the driver stops when a handler sets gA9EC. 147F is the big dispatcher with its own in-segment jump
   table. Names are placeholders until evidence names them. */

/*@flags -ml -O -Z */

extern signed char gB894[]; /*@ ds:B894 */     /* nine-byte per-object records (src/E08E6.C) */
extern signed char gTable18BF[]; /*@ ds:18BF */ /* per-object-kind table (Purpose unknown) */
extern int gEFD2; /*@ ds:EFD2 */               /* the mode the driver runs a script in (src/E0ACD.C) */
extern char gA9EC; /*@ ds:A9EC */              /* set by a handler to stop the driver's script walk */
extern int gF50E; /*@ ds:F50E */               /* the depth of the far pointer stack the driver pushes to */
extern void far *gScriptStack[]; /*@ ds:F444 */ /* the ten-deep far pointer stack the driver pushes to */
extern void far *gBuf8DCA; /*@ ds:8DCA */      /* start-up far allocation (src/E07A9.C) */
extern int gFBC8; /*@ ds:FBC8 */               /* the current action (src/E0ACD.C) */
extern unsigned long gActionFlags[]; /*@ ds:FBD4 */ /* per-action 32-bit masks, indexed by gFBC8 */
extern unsigned long gFC04; /*@ ds:FC04 */     /* effect flags (src/E08E6.C) */
extern char gB899[]; /*@ ds:B899 */            /* the nine-byte record list head (src/E0ACD.C) */
extern int gFBCE; /*@ ds:FBCE */               /* the item index shown by the mouse (src/E07A9.C) */
extern int gB800; /*@ ds:B800 */               /* the member whose script is running (122E:1DA8) */
extern char gDragActive; /*@ ds:5C77 */        /* nonzero while a member is picked for arming (122E:0154) */
extern int gDragMember; /*@ ds:5C74 */         /* the member picked for arming (122E:0154) */
extern signed char g5C76; /*@ ds:5C76 */       /* the mode byte 03B4 reads (src/E1414.C) */
extern void ArmEffectWith(int idx, int arg, int sub, int active); /*@ 1414:055B */
extern void AdjustMemberStatus(int who); /*@ 1414:03B4 */
extern void Obj1505(int kind, int id, int frame, int orient, int b0, int m07, int type, int timer, int m12, int m14); /*@ 08E6:1505 */
extern int gFBCC; /*@ ds:FBCC */               /* a page number 0..3 (src/E08E6.C) */
extern char gEFB2; /*@ ds:EFB2 */              /* set when the sound page changes (src/E0ACD.C) */
extern char gDF4C; /*@ ds:DF4C */              /* set to 1 when an action took effect (src/E08E6.C) */

/* The 18-byte effect records at ds:DD2C, 10 of them (src/E1414.C). */
struct DDRec {
    char active;                                /* +00: 1 when the record is live */
    char pad01;
    int m02;                                    /* +02 */
    int m04;                                    /* +04: a character or object index */
    char m06;                                   /* +06 */
    signed char m07;                            /* +07 */
    char pad08[0x12 - 0x08];
};

extern struct DDRec gDD2C[]; /*@ ds:DD2C */    /* 10 records, 18 bytes each */

extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void SetMouseItem(int n); /*@ 07A9:010C */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */

extern void AddPartyExperience(int amount); /*@ 0DE8:0C2B */
extern void HealCharacter(int who, int amount); /*@ 0DE8:0ADB */
extern void DamageCharacter(int who, int amount); /*@ 0DE8:09B9 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int f0D1C_0163(int who, int fieldB); /*@ 0D1C:0163 */
extern void fOvl5_1A82(int x); /*@ 211E:0057 */   /* overlay 5 entry, ovl5:1A82 */
extern void Obj0150(void); /*@ 08E6:0150 */
extern void UpdateDDEffects(void); /*@ 1414:0D63 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern int MouseInRect(int left, int top, int right, int bottom); /*@ 1A72:02A6 */
extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern int gSoundVolume; /*@ ds:FBCA */        /* the global sound volume (src/E08E6.C) */
extern int gF540; /*@ ds:F540 */               /* counts sound-volume changes (src/E08E6.C) */
extern char g194B[]; /*@ ds:194B */             /* per-object bit flags (src/E08E6.C) */
extern char gText5AB5[]; /*@ ds:5AB5 */        /* message drawn when the sound volume changes (122E:0001) */
extern char gText5B78[]; /*@ ds:5B78 */        /* message drawn for a highlighted member slot (122E:0847) */
void f122E_1DA8(int a, int b);  /* forward: 1DA8 comes after its callers */

/* 122E:0001: switch the sound volume to `volume`: run Obj0150 around the change, hand the old and
   the new value to the driver (codes 2 and 1), mark the sound page and action dirty, draw the
   volume message and refresh the effects. */
/*@ 122E:0001 */ void SetSoundVolume(int volume)
{
    int old;

    Obj0150();
    old = gSoundVolume;
    gSoundVolume = volume;
    f122E_1DA8(old, 2);
    f122E_1DA8(volume, 1);
    Obj0150();
    gF540++;
    gEFB2 = (char)0xff;
    gDF4C = 1;
    DrawText(gText5AB5);
    UpdateDDEffects();
}
/*@ 122E:0055 stub */ int f122E_0055(int id) { return 0; }

/* 122E:0137: push bit 0 of the frame-count byte of the gB894 record `id`. Called by 147F for one
   sub-opcode. Purpose unknown: the opcode it implements is not identified. */
/*@ 122E:0137 */ int ObjectCountIsOdd(int id)
{
    if (gB894[id * 9 + 4] & 1)
        return 1;
    return 0;
}

/* 122E:0154: arm effect 0xC on member `who`. While a member is picked (gDragActive), arming either
   matches the picked member (and turns g5C76 on) or does nothing. Otherwise it picks the member's
   free flag slot, arms the effect twice around AdjustMemberStatus. */
/*@ 122E:0154 draft */ void ArmMemberEffect(int who)
{
    int idx;

    if (gDragActive != 0) {
        if (gDragMember == who)
            g5C76 = 1;
        return;
    }
    idx = (g194B[gB894[who * 9]] & 8) ? 0 : 1;
    if (g194B[gB894[who * 9 + idx]] & 1)
        return;
    ArmEffectWith(0xc, who, 1, 1);
    AdjustMemberStatus(who);
    ArmEffectWith(0xc, who, 1, 1);
}

/* 122E:01D9: disarm effect 0xC on member `who`. It does nothing while the member still has a frame
   count (gB894[who * 9 + 4] & 7). While a member is picked, disarming either matches the picked
   member (and turns g5C76 off) or does nothing. Otherwise it arms the effect with arg -1 twice
   around AdjustMemberStatus when the flag slot is armed. */
/*@ 122E:01D9 */ void DisarmMemberEffect(int who)
{
    int idx;

    if (gB894[who * 9 + 4] & 7)
        return;
    if (gDragActive != 0) {
        if (gDragMember == who)
            g5C76 = (signed char)0xff;
        return;
    }
    idx = (g194B[gB894[who * 9]] & 8) ? 0 : 1;
    if ((g194B[gB894[who * 9 + idx]] & 1) == 0)
        return;
    ArmEffectWith(0xc, who, 0xffff, 1);
    AdjustMemberStatus(who);
    ArmEffectWith(0xc, who, 0xffff, 1);
}
/*@ 122E:0271 stub */ void f122E_0271(int who, int delta) {}
/*@ 122E:0325 stub */ void f122E_0325(void) {}
/*@ 122E:04CA stub */ void f122E_04CA(void) {}

/* 122E:05DF: look up the object kind in gB894[id * 9 + slot] and, if f122E_0055 accepts it, run the
   driver for `id` in the current mode gEFD2. Returns 1 when it ran, 0 otherwise. */
/*@ 122E:05DF */ int PerformItemSlotAction(int id, int slot)
{
    int kind;

    kind = gTable18BF[gB894[id * 9 + slot]];
    if (f122E_0055(kind)) {
        f122E_1DA8(id, gEFD2);
        return 1;
    }
    return 0;
}

/* 122E:0620: raise the count in gB894[id * 9 + slot] when f122E_0055 accepts the object kind,
   mark the action dirty and run the driver. Returns 1 when it ran, 0 otherwise. */
/*@ 122E:0620 */ int IncreaseItemSlot(int id, int slot)
{
    int kind;

    kind = gTable18BF[gB894[id * 9 + slot]];
    if (f122E_0055(kind)) {
        gB894[id * 9 + slot]++;
        gDF4C = 1;
        f122E_1DA8(id, gEFD2);
        return 1;
    }
    return 0;
}

/* 122E:0678: lower the count in gB894[id * 9 + slot] when f122E_0055 accepts the object kind,
   mark the action dirty and run the driver. Returns 1 when it ran, 0 otherwise. */
/*@ 122E:0678 */ int DecreaseItemSlot(int id, int slot)
{
    int kind;

    kind = gTable18BF[gB894[id * 9 + slot]];
    if (f122E_0055(kind)) {
        gB894[id * 9 + slot]--;
        gDF4C = 1;
        f122E_1DA8(id, gEFD2);
        return 1;
    }
    return 0;
}
/*@ 122E:06D0 stub */ void f122E_06D0(void) {}

/* 122E:0847: when the mouse is over the member slot's rectangle and the slot's object has flag 0x20
   in g194B, draw the slot's message. Returns 1 when it drew, 0 otherwise. */
/*@ 122E:0847 */ int CheckMemberSlotRect(int id, int slot)
{
    if (MouseInRect(0x28, 0x10, 0x88, 0x58)) {
        if (g194B[gB894[id * 9 + slot]] & 0x20) {
            DrawText(gText5B78);
            return 1;
        }
    }
    return 0;
}
/*@ 122E:0896 stub */ int f122E_0896(int a, int b) { return 0; }

/* 122E:0933: set the driver mode to 0x40, work out the member's row through f0D1C_00DA and run the
   row handler f122E_0896. QUIRK 122E:0933: the argument is raised by 0x3c and then never read. */
/*@ 122E:0933 */ void RunRowHandler(int entry)
{
    int x;

    entry += 0x3c;
    gEFD2 = 0x40;
    x = f0D1C_00DA(gSoundVolume, gFBCC);
    f122E_0896(x, gFBCC);
}
/* 122E:0960: sub-opcode that writes gB894 fields: 0xFFE9 stores a byte into gB894[id * 9 + index],
   0xFFED sets the page gFBCC, and 0xFFF7 fills the first four bytes of gB894[id * 9] with a byte.
   Returns the bytes read. */
/*@ 122E:0960 draft */ long WriteObjectFieldOp(char far *script)
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
            gB894[id * 9 + i] = value;
        break;
    case 0xFFE9:
        id = *(int far *)script;
        script += 2;
        value = *script++;
        gB894[id * 9 + value] = *script++;
        break;
    case 0xFFED:
        gFBCC = *script++;
        break;
    }
    return (long)(script - start);
}
/* 122E:0A16: sub-opcode that writes a run of four gB894 bytes: 0xFFF7 compares the record's first
   byte with a script byte and fills the first four bytes with the second script byte when they match
   (or the first when they do not); 0xFFEA hands a word and -1 to f122E_0271. Returns the bytes read. */
/*@ 122E:0A16 draft */ long WriteObjectRunOp(char far *script)
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
        if (gB894[id * 9] == first) {
            gB894[id * 9] = second;
            gB894[id * 9 + 1] = second;
            gB894[id * 9 + 2] = second;
            gB894[id * 9 + 3] = second;
        } else {
            gB894[id * 9] = first;
            gB894[id * 9 + 1] = first;
            gB894[id * 9 + 2] = first;
            gB894[id * 9 + 3] = first;
        }
        break;
    case 0xFFEA:
        id = *(int far *)script;
        script += 2;
        f122E_0271(id, -1);
        break;
    }
    return (long)(script - start);
}
/* 122E:0B23: sub-opcode that arms member effects: read a member index and hand it to f122E_0154.
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

/* 122E:0B56: sub-opcode that disarms member effects: read a member index and hand it to f122E_01D9.
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
/* 122E:0B89: sub-opcode that spawns an object: read an unused byte, then the ten Obj1505 arguments
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
/*@ 122E:0C4D stub */ void f122E_0C4D(void) {}
/*@ 122E:0D57 stub */ void f122E_0D57(void) {}
/*@ 122E:0E9B stub */ void f122E_0E9B(void) {}
/*@ 122E:0FDC stub */ void f122E_0FDC(void) {}
/*@ 122E:1097 stub */ void f122E_1097(void) {}

/* 122E:11E8: sub-opcode that clears one bit of a 32-bit action mask: 0xFFEF clears the bit in the
   gActionFlags entry of the current action gFBC8, 0xFFF0 clears it in gFC04. The byte after the
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
        gActionFlags[gFBC8] &= ~(1L << bit);
        break;
    case 0xFFF0:
        gFC04 &= ~(1L << bit);
        break;
    }
    return (long)(script - start);
}

/* 122E:126E: sub-opcode that heals: read a member index and an amount. A member index of -1 heals
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

/* 122E:12CE: sub-opcode that damages: read a member index, a dice count, sides and bonus. A member
   index of -1 damages all six members (each must pass f0D1C_0163(who, 3) first), otherwise the one
   member. QUIRK 122E:12CE: the single-member path passes the dice count, not the roll, to
   DamageCharacter, so the roll only decides whether the damage happens for the whole party.
   Returns the bytes read. */
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

/* 122E:139D: sub-opcode that jumps the script pointer to a word offset from the script base
   gBuf8DCA. Returns the pointer's movement. */
/*@ 122E:139D */ long JumpToScriptOffset(char far *script)
{
    char far *start = script;

    script = (char far *)gBuf8DCA + *(int far *)script;
    return (long)(script - start);
}

/* 122E:13D1: set the driver's stop flag and advance the script pointer by one; the driver stops its
   walk as soon as gA9EC is set. Returns 0. */
/*@ 122E:13D1 */ int ScriptStop(char far *script)
{
    gA9EC = 1;
    script++;
    return 0;
}

/* 122E:13E0: sub-opcode that pops the far pointer stack: when it is not empty, take the top entry as
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

/* 122E:1422: sub-opcode that pushes the far pointer stack: read a word offset, and when the stack is
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
/*@ 122E:147F stub */ void f122E_147F(void) {}
/*@ 122E:1AAB stub */ void f122E_1AAB(void) {}

/* 122E:1B5A: sub-opcode 0xE2: when the byte is 0xE2, read a word and add it as party experience.
   Returns the bytes read. */
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

/* 122E:1B9A: sub-opcode that copies an item into a free slot and either shows it on the mouse (when
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
            InsertListItem(&gB899[dest * 9], dest, item, *script);
        }
    }
    script++;
    return (long)(script - start);
}

/* 122E:1C16: sub-opcode that turns a page: 0xFFF1 rotates gFBCC by the byte argument (modulo 4) and
   marks the sound page and action dirty; 0xFFF5 rotates the orientation (m07) of every live effect
   record that names the running member. Returns the bytes read. */
/*@ 122E:1C16 */ long TurnPageOp(char far *script)
{
    int op;
    int delta;
    struct DDRec far *rec;
    char far *start = script;
    int i;

    op = *script++;
    delta = *script++;
    switch (op) {
    case 0xFFF1:
        gFBCC = (gFBCC + delta) & 3;
        gEFB2 = (char)0xff;
        gDF4C = 1;
        break;
    case 0xFFF5:
        rec = gDD2C;
        for (i = 0; i < 10; i++) {
            if (rec->active != 0 && rec->m04 == gB800)
                rec->m07 = (rec->m07 + delta) & 3;
            rec++;
        }
        break;
    }
    return (long)(script - start);
}
/*@ 122E:1CAD stub */ void f122E_1CAD(void) {}

/* 122E:1D76: sub-opcode that passes its byte argument to overlay 5 (ovl5:1A82). Returns the bytes
   read. */
/*@ 122E:1D76 */ long CallOverlayOp(char far *script)
{
    char far *start = script;
    fOvl5_1A82(*script++);
    return (long)(script - start);
}

/*@ 122E:1DA8 stub */ void f122E_1DA8(int a, int b) {}
