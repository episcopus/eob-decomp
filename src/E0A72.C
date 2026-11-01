/* Unit at EOB.EXE 0A72:0006 (1458 bytes, 15 functions): accessors for a character record and the
   per-class tables that go with it. A character is a 243-byte (0xF3) record in DGROUP from ds:F580
   (struct CharRec below; the callers compute `index * 0xF3 + 0xF580`, 0D1C:059B); this unit reads
   its fields and derives stats through the far-pointer and byte tables at ds:3406, ds:341E,
   ds:3424, ds:342A, ds:3430, ds:3435, ds:3439, ds:3453, ds:3458, ds:345C, ds:3474, ds:349E,
   ds:35BC, ds:35D4, ds:3610, ds:3614, ds:3618 and ds:116E. Called from the party, combat and
   inventory modules (0DE8, 0D1C, 0EF3, 08E6) and from overlays 0, 1 and 6. The meaning of each
   field and table is not established, so most function names say what a function computes rather
   than what the game calls it. */

/* A character record: the 243-byte records at ds:F580 (6 of them in EOBDATA.SAV). Only the fields
   this unit touches are named; everything else is padding. */
struct CharRec {
    char pad00[0x0D];
    signed char m0D;      /* +0D: an index into a class table, read by 0A72:00CF */
    char pad0E;
    signed char m0F;      /* +0F: a level or score tested by 0A72:00CF */
    char pad10[5];
    signed char m15;      /* +15: an index into a stats table, read by 0A72:0181 */
    char pad16[0x0A];
    signed char m20;      /* +20: an index into a class table, read by 0A72:0522 and 0DE8:0B45 */
    char pad21[3];
    signed char m24;      /* +24: a counter, read by 0A72:0522 */
    char pad25[0xF3 - 0x25];
};

extern struct CharRec gChars[]; /*@ ds:F580 */ /* the 243-byte character records */
extern char far *g3406[]; /*@ ds:3406 */       /* per-index far buffers, read by 0A72:0006 */
extern signed char g341E[]; /*@ ds:341E */     /* per-index clamp, read by 0A72:0006 */
extern signed char g3424[]; /*@ ds:3424 */     /* per-index divisor, read by 0A72:0006 */
extern signed char g342A[]; /*@ ds:342A */     /* per-index multiplier, read by 0A72:0006 */
extern signed char g3430[]; /*@ ds:3430 */     /* five class thresholds used by 0A72:00CF */
extern signed char g3435[]; /*@ ds:3435 */     /* five values paired with g3430 */
extern signed char g3439[]; /*@ ds:3439 */     /* base value per class, used by 0A72:00CF */
extern signed char g3453[]; /*@ ds:3453 */     /* five class thresholds used by 0A72:0128 */
extern signed char g3458[]; /*@ ds:3458 */     /* five values paired with g3453 */
extern signed char g345C[]; /*@ ds:345C */     /* base value per class, used by 0A72:0128 */
extern int g3474[]; /*@ ds:3474 */             /* a word table indexed by a stat class */
extern int g349E[]; /*@ ds:349E */             /* a word table indexed by a class byte */
extern long far *g35BC[]; /*@ ds:35BC */       /* per-index far tables of 32-bit values, read by 0A72:04B7 */
extern char g35D4[]; /*@ ds:35D4 */            /* a per-index byte table, indexed [row*4 + slot] */
extern signed char g3610[]; /*@ ds:3610 */     /* per-class multiplier, read by 0A72:0522 */
extern signed char g3614[]; /*@ ds:3614 */     /* per-class divisor, read by 0A72:0522 */
extern signed char g3618[]; /*@ ds:3618 */     /* a 2D byte table, 0x0E bytes per row */
extern signed char g116E[]; /*@ ds:116E */     /* per-class byte, read by 0A72:0522 */
extern signed char g174B[]; /*@ ds:174B */     /* per-character value A, damage dice sides (0A72:01AE) */
extern signed char g175A[]; /*@ ds:175A */     /* per-character value B, damage dice sides (0A72:01AE) */
extern signed char g1769[]; /*@ ds:1769 */     /* per-character value C, damage dice sides (0A72:01AE) */
extern signed char g1778[]; /*@ ds:1778 */     /* per-level multiplier, indexed by level (0A72:01AE) */
extern char gTextBuf[]; /*@ ds:FC4E */         /* static buffer the formatting helpers write into */

extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */

/* 0A72:0006: a byte from the per-index far buffer g3406[index], at position
   min(limit, g341E[index]) / g3424[index] + g342A[index] * offset. Purpose unknown: called by
   08E6:1634, 0D1C:059B, 0DE8:0FCA and 0EF3:1702. */
/*@ 0A72:0006 */ int GetIndexedByte(char index, char limit, char offset)
{
    char far *base;

    base = g3406[index];
    if (limit > g341E[index])
        limit = g341E[index];
    limit /= g3424[index];
    limit += g342A[index] * offset;
    return base[limit];
}

/* 0A72:007B: the slot 0, 1 or 2 whose g35D4 byte for class `classIndex` equals `value`, or -1.
   Called by 0D1C:0903, 0EF3:0498, 0EF3:04F7 and overlays 1 and 6. */
/*@ 0A72:007B */ int FindClassSlot(int value, char classIndex)
{
    if (GetClassSlotByte(classIndex, 0) == value)
        return 0;
    if (GetClassSlotByte(classIndex, 1) == value)
        return 1;
    if (GetClassSlotByte(classIndex, 2) == value)
        return 2;
    return -1;
}

/* 0A72:00CF: a derived per-character value: the base g3439[+0D] raised to g3435[i] for every i with
   +0F >= g3430[i] (the last match wins). Purpose unknown: called by 0DE8:0007 (added to a class
   value for an attack). */
/*@ 0A72:00CF */ int GetDerivedStatA(int who)
{
    int value;
    int i;

    value = g3439[gChars[who].m0D];
    if (gChars[who].m0F != 0) {
        for (i = 0; i < 5; i++) {
            if (gChars[who].m0F >= g3430[i])
                value = g3435[i];
        }
    }
    return value;
}

/* 0A72:0128: the same shape as 0A72:00CF with the second set of class tables: base g345C[+0D],
   raised to g3458[i] for every i with +0F >= g3453[i]. Called by 0DE8:011A. */
/*@ 0A72:0128 */ int GetDerivedStatB(int who)
{
    int value;
    int i;

    value = g345C[gChars[who].m0D];
    if (gChars[who].m0F != 0) {
        for (i = 0; i < 5; i++) {
            if (gChars[who].m0F >= g3453[i])
                value = g3458[i];
        }
    }
    return value;
}

/* 0A72:0181: the word ds:3474[+15 byte] for character `who`. Purpose unknown: the +15 byte indexes
   a small class table; the only caller (0DE8:0007) adds this to a class value when it builds an
   attack value. */
/*@ 0A72:0181 */ int GetCharM15Value(int who)
{
    return g3474[gChars[who].m15];
}

/* 0A72:019D: the word ds:349E[class] for a class byte. Purpose unknown: called by 0DE8:0D5E,
   0EF3:0F0B and overlay 0 (0A72 is the character-record unit; the argument comes from a class
   table). */
/*@ 0A72:019D */ int GetClassWord(char classIndex)
{
    return g349E[classIndex];
}

/* 0A72:01AE: a random damage-like total for character `who`. It rolls the three damage groups
   (RollDice(count, sides) for sides g174B, g175A and g1769 of `who`), scales by the level
   multiplier g1778[level - 1] (capped at 2 while g174B[who] < 10), adds multiplier * count for
   each nonzero group, then divides by 3 or by 2 as the later groups are present. Purpose unknown:
   called by 0DE8:0C82 and overlay 0 at 1BF2. */
/*@ 0A72:01AE */ int RollDamage(char who, char level, char countA, char countB, char countC)
{
    int total;

    level--;
    total = RollDice(countA, g174B[who], 0) + RollDice(countB, g175A[who], 0)
          + RollDice(countC, g1769[who], 0);
    level = g1778[level];
    if (level > 2 && g174B[who] < 10)
        level = 2;
    total += level * countA;
    if (countB != 0 && g175A[who] != 0)
        total += level * countB;
    if (countC != 0 && g1769[who] != 0)
        total += level * countC;
    if (g1769[who] != 0)
        total /= 3;
    else if (g175A[who] != 0)
        total /= 2;
    return total;
}

/* 0A72:02B3: the maximum of the same three damage groups: countA * g174B[who] + countB * g175A[who]
   + countC * g1769[who], plus the level multiplier term. The same divide-by-3-or-2 as 0A72:01AE.
   Called by overlay 0 at 135D. */
/*@ 0A72:02B3 */ int MaxDamage(char who, char level, char countA, char countB, char countC)
{
    int total;

    level--;
    total = countA * g174B[who] + countB * g175A[who] + countC * g1769[who];
    level = g1778[level];
    if (level > 2 && g174B[who] < 10)
        level = 2;
    total += level * countA;
    if (countB != 0 && g175A[who] != 0)
        total += level * countB;
    if (countC != 0 && g1769[who] != 0)
        total += level * countC;
    if (g1769[who] != 0)
        total /= 3;
    else if (g175A[who] != 0)
        total /= 2;
    return total;
}

/* 0A72:03A4: the minimum of the same three damage groups: countA + countB + countC (one per die),
   plus the level multiplier term, the same divide-by-3-or-2, and finally clamped to at least 1.
   Called by overlay 0 at 135D. */
/*@ 0A72:03A4 */ int MinDamage(char who, char level, char countA, char countB, char countC)
{
    int total;

    level--;
    total = countA + countB + countC;
    level = g1778[level];
    if (level > 2 && g174B[who] < 10)
        level = 2;
    total += level * countA;
    if (countB != 0 && g175A[who] != 0)
        total += level * countB;
    if (countC != 0 && g1769[who] != 0)
        total += level * countC;
    if (g1769[who] != 0)
        total /= 3;
    else if (g175A[who] != 0)
        total /= 2;
    if (total < 1)
        total = 1;
    return total;
}

/* 0A72:0468: format `value` into the static buffer gTextBuf as "%d" when `percent` <= 0, else as
   "%d/%02d" with 100 written as 0, and return the buffer. Called by 1547:0991, overlay 0 at 065B
   and overlay 0 at 135D. The "%d/%02d" and "%d" formats are the literals at ds:36A4 and ds:36AC. */
/*@ 0A72:0468 */ char *FormatRatioText(char value, char percent)
{
    if (percent > 0) {
        if (percent == 100)
            percent = 0;
        sprintf(gTextBuf, "%d/%02d", value, percent);
    } else {
        sprintf(gTextBuf, "%d", value);
    }
    return gTextBuf;
}

/* 0A72:04B7: the 32-bit value at index countC - 1 of the table g35BC[g35D4[indexA][indexB]], or -1
   when that index is -1. Purpose unknown: called by 0DE8:0B45 while it accumulates character
   values. */
/*@ 0A72:04B7 */ long GetClassSlotLong(char indexA, char indexB, char countC)
{
    long far *table;

    indexA = GetClassSlotByte(indexA, indexB);
    if (indexA != -1) {
        table = g35BC[indexA];
        return *(table + countC - 1);
    }
    return -1;
}

/* 0A72:0509: the byte ds:35D4[row][slot] for a 4-wide table. Purpose unknown: 0A72:007B scans
   the four slots looking for a match and 0A72:04B7 uses the result as a row index (in 0D1C:059B
   the result is compared with 4 and 5). */
/*@ 0A72:0509 */ char GetClassSlotByte(char row, char slot)
{
    return g35D4[row * 4 + slot];
}

/* 0A72:0522: 20 - (m24 - 1) / g3614[g116E[m20]] * g3610[g116E[m20]] - x, computed in bytes.
   Purpose unknown: called by 0DE8:0007 with x a class byte from ds:1A3B to build an attack value;
   m20 indexes ds:116E and m24 is the character's counter at +24. Draft: with -O our build keeps
   `cls` in CL across the division, the original spills it to [bp-2] and reloads it for both table
   reads. Tried a volatile, an extra local, initializers, char casts, and inlining the m20 lookup
   (which the compiler then keeps in a register); all either promote `cls` or promote `num`. */
/*@ 0A72:0522 draft */ char GetScaledValue(char who, char x)
{
    char num;
    char cls;

    num = gChars[who].m24 - 1;
    cls = g116E[gChars[who].m20];
    return 20 - num / g3614[cls] * g3610[cls] - x;
}

/* 0A72:058C: the difference of two ints. Purpose unknown: its only caller 08E6:106B uses it as a
   comparison helper. */
/*@ 0A72:058C */ int SubtractInts(int minuend, int subtrahend)
{
    return minuend - subtrahend;
}

/* 0A72:0597: the byte ds:3618[row * 0x0E + col] with col clamped to 0x0E. Purpose unknown: called
   by 0D1C:09CB with a class byte from ds:1A54 as the row. */
/*@ 0A72:0597 */ int GetClampedTableByte(int col, int row)
{
    if (col > 0x0E)
        col = 0x0E;
    return g3618[row * 0x0E + col];
}
