/* Unit at EOB.EXE 0A72:0006 (1458 bytes, 15 functions): accessors for a character record and the
   per-class tables that go with it. A character is a 243-byte (0xF3) record in DGROUP from ds:F580
   (struct CharRec below; the callers compute `index * 0xF3 + 0xF580`, 0D1C:059B); this unit reads
   its fields and derives stats through the far-pointer and byte tables at ds:3406, ds:341E,
   ds:3424, ds:342A, ds:3430, ds:3435, ds:3439, ds:3453, ds:3458, ds:345C, ds:3474, ds:349E,
   ds:35BC, ds:35D4, ds:3610, ds:3614, ds:3618, ds:116E and the per-class hit dice tables ds:174B,
   ds:175A, ds:1769 and the m17-indexed multiplier ds:1778. Called from the party, combat,
   inventory and character-screen modules (0DE8, 0D1C, 0EF3, 08E6, 1547) and from overlays 0, 1
   and 6. The meaning of each field and table is not established, so most function names say what a
   function computes rather than what the game calls it. */

/* A character record: the 243-byte records at ds:F580 (6 of them in EOBDATA.SAV). Only the fields
   this unit touches are named; everything else is padding. */
struct CharRec {
    char pad00[0x0D];
    signed char m0D;      /* +0D: the strength score, the base g3439/m0D and g345C/m0D of 0A72:00CF
                             and 0A72:0128; E122E.C:364 indexes gForceDoorRoll by it and
                             E0BD2.C:0D2D sets it to 0x16 for a strength item */
    char pad0E;
    signed char m0F;      /* +0F: the exceptional strength percentile: 0A72:00CF and 0A72:0128
                             compare it with {1, 51, 76, 91, 100}, and 100 prints as "00"
                             (FormatStrengthPercent) */
    char pad10[5];
    signed char m15;      /* +15: one of the five further stat bytes drawn after the strength
                             score (EOV0.C:442-444 formats m11, m13, m15, m17, m19); it indexes the
                             per-stat tables ds:3474 (0A72:0181) and ds:349E (0A72:019D) */
    char pad16[0x0A];
    signed char m20;      /* +20: an index into a class table, read by 0A72:0522 and 0DE8:0B45 */
    char pad21[3];
    signed char m24[3];   /* +24: three per-class-slot counters (src/E0D1C.C declares the same
                             array); 0A72:0522 reads element [0] */
    char pad27[0xF3 - 0x27];
};

extern struct CharRec gChars[]; /*@ ds:F580 */ /* the 243-byte character records */
extern char far *g3406[]; /*@ ds:3406 */       /* per-index far buffers, read by 0A72:0006 */
extern signed char g341E[]; /*@ ds:341E */     /* per-index clamp, read by 0A72:0006 */
extern signed char g3424[]; /*@ ds:3424 */     /* per-index divisor, read by 0A72:0006 */
extern signed char g342A[]; /*@ ds:342A */     /* per-index multiplier, read by 0A72:0006 */
extern signed char g3430[]; /*@ ds:3430 */     /* five strength-percentile thresholds {1,51,76,91,100}, used by 0A72:00CF */
extern signed char g3435[]; /*@ ds:3435 */     /* five values that 0A72:00CF puts in place of the
                                                  base for the matching g3430 threshold */
extern signed char g3439[]; /*@ ds:3439 */     /* base value per strength score, used by 0A72:00CF */
extern signed char g3453[]; /*@ ds:3453 */     /* five strength-percentile thresholds, used by 0A72:0128 */
extern signed char g3458[]; /*@ ds:3458 */     /* five values paired with g3453 */
extern signed char g345C[]; /*@ ds:345C */     /* base value per strength score, used by 0A72:0128 */
extern int g3474[]; /*@ ds:3474 */             /* a word table indexed by the +15 stat byte (0A72:0181) */
extern int g349E[]; /*@ ds:349E */             /* a word table indexed by the +15 stat byte (0A72:019D) */
extern long far *g35BC[]; /*@ ds:35BC */       /* per-index far tables of 32-bit values, read by 0A72:04B7 */
extern char g35D4[]; /*@ ds:35D4 */            /* a 4-wide byte table indexed row * 4 + slot (0A72:0509) */
extern signed char g3610[]; /*@ ds:3610 */     /* per-class multiplier, read by 0A72:0522 */
extern signed char g3614[]; /*@ ds:3614 */     /* per-class divisor, read by 0A72:0522 */
extern signed char g3618[]; /*@ ds:3618 */     /* a 2D byte table, ten 0x0E-byte rows (0A72:0597) */
extern signed char g116E[]; /*@ ds:116E */     /* per-class byte, read by 0A72:0522 */
extern signed char g174B[]; /*@ ds:174B */     /* hit dice sides for group A, per class index (0A72:01AE) */
extern signed char g175A[]; /*@ ds:175A */     /* hit dice sides for group B, per class index (0A72:01AE) */
extern signed char g1769[]; /*@ ds:1769 */     /* hit dice sides for group C, per class index (0A72:01AE) */
extern signed char g1778[]; /*@ ds:1778 */     /* per-stat multiplier, indexed by the m17 byte minus 1 (0A72:01AE) */
extern char gTextBuf[]; /*@ ds:FC4E */         /* static buffer the formatting helpers write into */

extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */

/* 0A72:0006: a byte from the per-index far buffer g3406[index], at position
   min(limit, g341E[index]) / g3424[index] + g342A[index] * offset. The four parallel tables are
   g3406 (the buffers), g341E (the per-index clamps), g3424 (the divisors) and g342A (the
   multipliers). Purpose unknown: called by 08E6:1634, 0D1C:059B, 0DE8:0FCA and 0EF3:1702. */
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

/* 0A72:007B: the slot 0, 1 or 2 whose g35D4 byte for class `classIndex` equals `slotId`, or -1.
   `slotId` is the g35D4 slot value searched for (the two src/ callers pass 2 and 4, E0D1C.C:579
   and :586). Called by 0D1C:0903, 0EF3:0498, 0EF3:04F7 and overlays 1 and 6. */
/*@ 0A72:007B */ int FindClassSlot(int slotId, int classIndex)
{
    if (GetClassSlotByte(classIndex, 0) == slotId)
        return 0;
    if (GetClassSlotByte(classIndex, 1) == slotId)
        return 1;
    if (GetClassSlotByte(classIndex, 2) == slotId)
        return 2;
    return -1;
}

/* 0A72:00CF: the attacker's strength to-hit bonus. It starts from the base g3439[m0D] (+0D is the
   strength score) and, only when the +0F percentile is nonzero, replaces it with g3435[i] for
   every i with +0F >= g3430[i] (so the last threshold that matches wins, and nothing is added or
   raised); when +0F is 0 the base is returned untouched. The evidence that +0D/+0F are a score and
   its percentile: g3430 holds five bytes {1, 51, 76, 91, 100} and g3435 holds {1, 2, 2, 2, 3}
   (ds:3430 and ds:3435), g3439 is the per-score base {3, -5, -3, ...}, and the only caller
   0DE8:0007 adds the result to the attack bonus when its `flag` is set (GetCharM15Value is added
   otherwise, E0DE8.C:200-203). */
/*@ 0A72:00CF */ int GetStrengthToHitBonus(int who)
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

/* 0A72:0128: the attacker's strength damage bonus, the same shape as 0A72:00CF with the second set
   of tables: the base g345C[strength +0D] is replaced by g3458[i] for every i with the +0F
   percentile >= g3453[i] (the last match wins), and only when +0F is nonzero are the thresholds
   read; g3453 is again {1, 51, 76, 91, 100}. The only caller 0DE8:011A starts its damage total
   from the result when its `flag` is set (E0DE8.C:227-228). */
/*@ 0A72:0128 */ int GetStrengthDamageBonus(int who)
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

/* 0A72:0181: the word ds:3474[+15 stat byte] for character `who`. Purpose unknown: the +15 byte is
   one of the five further stat bytes (see the struct field); the only caller (0DE8:0007) adds this
   to the attack bonus when its `flag` is zero (GetStrengthToHitBonus is added otherwise, E0DE8.C:200-203). */
/*@ 0A72:0181 */ int GetCharM15Value(int who)
{
    return g3474[gChars[who].m15];
}

/* 0A72:019D: the word ds:349E[+15 stat byte] for the `m15` argument of a character record. It is
   the base 0DE8:0D5E builds the armour class m1D from: 0DE8:0D5E does
   `base = GetArmourClassBase(rec->m15);` and then `rec->m1D = base + 10;` (E0DE8.C:684-685),
   before the item modifiers and clamps. Called by 0DE8:0D5E, 0EF3:0F0B (which does
   `base = GetArmourClassBase(gCurRec->m15)` and compares m1D with base + 6, E0EF3.C:945-946) and
   overlay 0. */
/*@ 0A72:019D */ int GetArmourClassBase(char m15)
{
    return g349E[m15];
}

/* 0A72:01AE: a random hit-point total for class `classIndex`. It rolls the three hit dice groups
   (RollDice(count, sides) for the class's dice sides g174B, g175A and g1769), scales by the
   per-stat multiplier g1778[m17 - 1] (capped at 2 while g174B[classIndex] < 10), then adds
   multiplier * count: the first group's term is always added, while the second and third are added
   only when both their count and their sides are nonzero. Finally it divides by 3 when g1769 is
   nonzero or by 2 when only g175A is nonzero. `m17` is decremented in place and then reused to hold
   the multiplier (a separate local does not match: the original shows `dec byte ptr [bp + 8]` at
   0A72:01B2); `m17` is the character's +17 byte, an index into g54F6 (the caller 0DE8:0C82 passes
   gChars[who].m17, E0DE8.C:652; E0DE8.C:23), and the evidence does not show it is a character
   level. `classIndex` is a character's class index m20 (the caller 0DE8:0C82 passes LevelUpSlot's
   `attr`, which AddExperience sets to gChars[who].m20, E0DE8.C:652 and :607, and overlay 0 pushes
   +20 at 1BF2:1E78). Called by 0DE8:0C82 and overlay 0 at 1BF2. */
/*@ 0A72:01AE */ int RollHitPoints(char classIndex, char m17, char countA, char countB, char countC)
{
    int total;

    m17--;
    total = RollDice(countA, g174B[classIndex], 0) + RollDice(countB, g175A[classIndex], 0)
          + RollDice(countC, g1769[classIndex], 0);
    m17 = g1778[m17];
    if (m17 > 2 && g174B[classIndex] < 10)
        m17 = 2;
    total += m17 * countA;
    if (countB != 0 && g175A[classIndex] != 0)
        total += m17 * countB;
    if (countC != 0 && g1769[classIndex] != 0)
        total += m17 * countC;
    if (g1769[classIndex] != 0)
        total /= 3;
    else if (g175A[classIndex] != 0)
        total /= 2;
    return total;
}

/* 0A72:02B3: the maximum of the same three hit dice groups for class `classIndex`: countA *
   g174B[classIndex] + countB * g175A[classIndex] + countC * g1769[classIndex], plus the
   per-stat multiplier term. The same divide-by-3-or-2 as 0A72:01AE. Called by overlay 0 at 135D,
   which pushes gChars[gDCB0].m20 as `classIndex` (135D:14BE) and gChars[gDCB0].m17 as `m17`. */
/*@ 0A72:02B3 */ int MaxHitPoints(char classIndex, char m17, char countA, char countB, char countC)
{
    int total;

    m17--;
    total = countA * g174B[classIndex] + countB * g175A[classIndex] + countC * g1769[classIndex];
    m17 = g1778[m17];
    if (m17 > 2 && g174B[classIndex] < 10)
        m17 = 2;
    total += m17 * countA;
    if (countB != 0 && g175A[classIndex] != 0)
        total += m17 * countB;
    if (countC != 0 && g1769[classIndex] != 0)
        total += m17 * countC;
    if (g1769[classIndex] != 0)
        total /= 3;
    else if (g175A[classIndex] != 0)
        total /= 2;
    return total;
}

/* 0A72:03A4: the minimum of the same three hit dice groups for class `classIndex`: countA + countB +
   countC (one per die), plus the per-stat multiplier term, the same divide-by-3-or-2, and finally
   clamped to at least 1. Called by overlay 0 at 135D, which pushes gChars[gDCB0].m20 as
   `classIndex` (135D:1514) and gChars[gDCB0].m17 as `m17`. */
/*@ 0A72:03A4 */ int MinHitPoints(char classIndex, char m17, char countA, char countB, char countC)
{
    int total;

    m17--;
    total = countA + countB + countC;
    m17 = g1778[m17];
    if (m17 > 2 && g174B[classIndex] < 10)
        m17 = 2;
    total += m17 * countA;
    if (countB != 0 && g175A[classIndex] != 0)
        total += m17 * countB;
    if (countC != 0 && g1769[classIndex] != 0)
        total += m17 * countC;
    if (g1769[classIndex] != 0)
        total /= 3;
    else if (g175A[classIndex] != 0)
        total /= 2;
    if (total < 1)
        total = 1;
    return total;
}

/* 0A72:0468: formats a strength score and its exceptional percentile into the static buffer
   gTextBuf and returns it. When `percent` is 0 or less only the score is written, as "%d"; else
   the score and a two-digit percentile are written, as "%d/%02d". QUIRK 0A72:0468: a percentile of
   100 is written as 0 (the 18/100 form cannot be drawn), which a port must keep. The "%d/%02d" and
   "%d" formats are the literals at ds:36A4 and ds:36AC. Called by 1547:0991 (drawn on the STRENGTH
   row of the CHARACTER INFO block, E1547.C:554), ovl0:065B and ovl0:135D (both pass CharRec m0D,
   m0F). Inferred (not from a source in src/): 100 prints as 0 because %02d would widen the field to
   three digits, and the screen field is two digits wide; the only src/ caller (E1547.C:554) gives
   the x position, not the width. */
/*@ 0A72:0468 */ char *FormatStrengthPercent(char score, char percent)
{
    if (percent > 0) {
        if (percent == 100)
            percent = 0;
        sprintf(gTextBuf, "%d/%02d", score, percent);
    } else {
        sprintf(gTextBuf, "%d", score);
    }
    return gTextBuf;
}

/* 0A72:04B7: the 32-bit experience threshold for reaching level `level` in the class-slot table
   g35BC[index], where index is the g35D4 byte at row `row`, slot `slot` (the class's slot index),
   or -1 when that byte is -1 (the class has no such slot); the byte replaces `row` in place, the
   original uses one local for both. `level` is the level being reached, so the entry for level 1
   is at index 0: the caller 0DE8:0B45 passes gChars[who].m24[i] + 1 (E0DE8.C:613), adds the share
   to the slot's experience total m27[i] and levels the slot up when that total passes this value
   (E0DE8.C:612-615). Called by 0DE8:0B45 (AddExperience). */
/*@ 0A72:04B7 */ long GetClassSlotExpThreshold(char row, char slot, char level)
{
    long far *table;

    row = GetClassSlotByte(row, slot);
    if (row != -1) {
        table = g35BC[row];
        return *(table + level - 1);
    }
    return -1;
}

/* 0A72:0509: the byte ds:35D4[row * 4 + slot] for a 4-wide table. A slot's byte is -1 (0xFF) when
   the class has no such slot: 0A72:007B scans slots 0, 1 and 2 for a match, 0A72:04B7 uses the
   byte as an index into g35BC and returns -1 when it is -1, and the callers test for -1
   (E1547.C:566, E0DE8.C:610; E0DE8.C:614 tests 0A72:04B7's long result, not this byte)
   (0D1C:059B compares the result with 4 and 5). Purpose unknown. */
/*@ 0A72:0509 */ char GetClassSlotByte(char row, char slot)
{
    return g35D4[row * 4 + slot];
}

/* 0A72:0522: 20 - (m24[0] - 1) / g3614[g116E[m20]] * g3610[g116E[m20]] - classByte, computed in
   bytes. It is the d20 number a to-hit roll must reach before the attacker bonus: its only caller
   0DE8:0007 computes `ac = GetHitTarget(who, gTable1A3B[b0].m00) - bonus` (E0DE8.C:206) and then
   compares a d20 roll against it (`return roll >= ac;`, E0DE8.C:214). `classByte` is the +0 field
   of the target's ds:1A3B entry (E0DE8.C:57), m20 is the character's class index into ds:116E,
   and m24[0] is the character's first per-class-slot counter at +24. Build note: `result` is what
   keeps the two lookups in memory; without it -O promotes `cls` (or `num`) to CL and the spills
   differ from the original. */
/*@ 0A72:0522 */ char GetHitTarget(char who, char classByte)
{
    char num;
    char cls;
    char result;

    num = gChars[who].m24[0] - 1;
    cls = g116E[gChars[who].m20];
    result = 20 - num / g3614[cls] * g3610[cls];
    result -= classByte;
    return result;
}

/* 0A72:058C: the difference of two ints. Purpose unknown: its only caller 08E6:106B takes
   `need = SubtractInts(gTable1A3B[index].m01, gChars[who].m1D)` and hits when a d20 roll reaches
   `need` (E08E6.C:861-862). */
/*@ 0A72:058C */ int SubtractInts(int minuend, int subtrahend)
{
    return minuend - subtrahend;
}

/* 0A72:0597: the byte ds:3618[row * 0x0E + col] with col clamped to 0x0E. The table is ten
   0x0E-byte rows and ends at ds:36A4, where the "%d/%02d" literal begins, so a row holds columns
   0x00..0x0D and 0x0E is one past the row. QUIRK 0A72:0597: the clamp admits col 0x0E, which
   reads ds:3618[row * 0x0E + 0x0E], the first byte of the next row (and, for the last row, the
   '%' byte at ds:36A4); a port must keep this aliasing. Purpose unknown: called by 0D1C:09CB with
   the +19 field of the object's ds:1A3B entry as the row. */
/*@ 0A72:0597 */ int GetClampedTableByte(int col, int row)
{
    if (col > 0x0E)
        col = 0x0E;
    return g3618[row * 0x0E + col];
}
