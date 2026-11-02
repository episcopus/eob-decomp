/* Unit at EOB.EXE 122E:0001 (7775 bytes, 40 functions). The game's action script interpreter: the
   driver at 122E:1DA8 walks a member's compiled script with a table of 26 far handler pointers and
   dispatches each opcode byte to one of the handlers below. The handlers take the current script
   pointer (a far char pointer) and return how many bytes the opcode consumed, or jump the pointer;
   the driver stops when a handler sets gA9EC. 147F is the big dispatcher with its own in-segment jump
   table. Names are placeholders until evidence names them. */

/*@flags -ml -O -Z */

extern unsigned char gB894[]; /*@ ds:B894 */   /* nine-byte per-object records (src/E08E6.C) */
extern char gA9EC; /*@ ds:A9EC */              /* set by a handler to stop the driver's script walk */
extern int gF50E; /*@ ds:F50E */               /* the depth of the far pointer stack the driver pushes to */

extern void AddPartyExperience(int amount); /*@ 0DE8:0C2B */
extern void HealCharacter(int who, int amount); /*@ 0DE8:0ADB */
extern void fOvl5_1A82(int x); /*@ 211E:0057 */   /* overlay 5 entry, ovl5:1A82 */

/*@ 122E:0001 stub */ void f122E_0001(void) {}
/*@ 122E:0055 stub */ void f122E_0055(void) {}

/* 122E:0137: push bit 0 of the frame-count byte of the gB894 record `id`. Called by 147F for one
   sub-opcode. Purpose unknown: the opcode it implements is not identified. */
/*@ 122E:0137 */ int ObjectCountIsOdd(int id)
{
    if (gB894[id * 9 + 4] & 1)
        return 1;
    return 0;
}

/*@ 122E:0154 stub */ void f122E_0154(void) {}
/*@ 122E:01D9 stub */ void f122E_01D9(void) {}
/*@ 122E:0271 stub */ void f122E_0271(void) {}
/*@ 122E:0325 stub */ void f122E_0325(void) {}
/*@ 122E:04CA stub */ void f122E_04CA(void) {}
/*@ 122E:05DF stub */ void f122E_05DF(void) {}
/*@ 122E:0620 stub */ void f122E_0620(void) {}
/*@ 122E:0678 stub */ void f122E_0678(void) {}
/*@ 122E:06D0 stub */ void f122E_06D0(void) {}
/*@ 122E:0847 stub */ void f122E_0847(void) {}
/*@ 122E:0896 stub */ void f122E_0896(void) {}
/*@ 122E:0933 stub */ void f122E_0933(void) {}
/*@ 122E:0960 stub */ void f122E_0960(void) {}
/*@ 122E:0A16 stub */ void f122E_0A16(void) {}
/*@ 122E:0B23 stub */ void f122E_0B23(void) {}
/*@ 122E:0B56 stub */ void f122E_0B56(void) {}
/*@ 122E:0B89 stub */ void f122E_0B89(void) {}
/*@ 122E:0C4D stub */ void f122E_0C4D(void) {}
/*@ 122E:0D57 stub */ void f122E_0D57(void) {}
/*@ 122E:0E9B stub */ void f122E_0E9B(void) {}
/*@ 122E:0FDC stub */ void f122E_0FDC(void) {}
/*@ 122E:1097 stub */ void f122E_1097(void) {}
/*@ 122E:11E8 stub */ void f122E_11E8(void) {}

/* 122E:126E: sub-opcode that heals: read a member index and an amount. A member index of -1 heals
   all six party members, otherwise only that member. Returns the bytes read. */
/*@ 122E:126E */ long HealPartyMember(char far *p)
{
    char far *start = p;
    int who;
    int amount;

    who = *p++;
    amount = *p++;
    if (who == -1) {
        for (who = 0; who < 6; who++)
            HealCharacter(who, amount);
    } else {
        HealCharacter(who, amount);
    }
    return (long)(p - start);
}

/*@ 122E:12CE stub */ void f122E_12CE(void) {}
/*@ 122E:139D stub */ void f122E_139D(void) {}

/* 122E:13D1: set the driver's stop flag and advance the script pointer by one; the driver stops its
   walk as soon as gA9EC is set. Returns 0. */
/*@ 122E:13D1 */ int ScriptStop(char far *p)
{
    gA9EC = 1;
    p++;
    return 0;
}

/*@ 122E:13E0 stub */ void f122E_13E0(void) {}
/*@ 122E:1422 stub */ void f122E_1422(void) {}
/*@ 122E:147F stub */ void f122E_147F(void) {}
/*@ 122E:1AAB stub */ void f122E_1AAB(void) {}

/* 122E:1B5A: sub-opcode 0xE2: when the byte is 0xE2, read a word and add it as party experience.
   Returns the bytes read. */
/*@ 122E:1B5A */ long AddExperienceOp(char far *p)
{
    char far *start = p;
    int exp;

    if (*p++ == (char)0xE2) {
        exp = *(int far *)p;
        p += 2;
        AddPartyExperience(exp);
    }
    return (long)(p - start);
}

/*@ 122E:1B9A stub */ void f122E_1B9A(void) {}
/*@ 122E:1C16 stub */ void f122E_1C16(void) {}
/*@ 122E:1CAD stub */ void f122E_1CAD(void) {}

/* 122E:1D76: sub-opcode that passes its byte argument to overlay 5 (ovl5:1A82). Returns the bytes
   read. */
/*@ 122E:1D76 */ long CallOverlayOp(char far *p)
{
    char far *start = p;
    fOvl5_1A82(*p++);
    return (long)(p - start);
}

/*@ 122E:1DA8 stub */ void f122E_1DA8(void) {}
