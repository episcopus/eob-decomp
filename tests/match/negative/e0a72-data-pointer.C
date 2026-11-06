/* NEGATIVE FIXTURE (make match-gate): src/E0A72.C with its data (the data marker, playbook 80 step D) and one
   defect, a far pointer naming the wrong table (gSaveTables[0] g339D for g33BB). The matcher must report a difference; a build error or a match fails the gate. */
/* Unit at EOB.EXE 0A72:0006 (1458 bytes, 15 functions): accessors for a character record and the
   per-class tables that go with it. A character is a 243-byte (0xF3) record in DGROUP from ds:F580
   (struct CharRec below; the callers compute `index * 0xF3 + 0xF580`, 0D1C:059B); this unit reads
   its fields and derives stats through the far-pointer and byte tables at ds:3406, ds:341E,
   ds:3424, ds:342A, ds:3430, ds:3435, ds:3439, ds:3453, ds:3458, ds:345C, ds:3474, ds:349E,
   ds:35BC, ds:35D4, ds:3610, ds:3614, ds:3618, ds:116E and the per-class hit dice tables ds:174B,
   ds:175A, ds:1769 and the constitution-indexed multiplier ds:1778. Called from the party, combat,
   inventory and character-screen modules (0DE8, 0D1C, 0EF3, 08E6, 1547) and from overlays 0, 1
   and 6. The meaning of each field and table is not established, so most function names say what a
   function computes rather than what the game calls it. */

#include "EOB.H"

extern struct CharRec gChars[]; /*@ ds:F580 */ /* the 243-byte character records */
extern char far *gSaveTables[]; /*@ ds:3406 */       /* per-index far buffers, read by 0A72:0006 */
extern signed char gSaveMaxLevel[]; /*@ ds:341E */     /* per-index clamp, read by 0A72:0006 */
extern signed char gSaveLevelStep[]; /*@ ds:3424 */     /* per-index divisor, read by 0A72:0006 */
extern signed char gSaveRowLength[]; /*@ ds:342A */     /* per-index multiplier, read by 0A72:0006 */
extern signed char gStrPctHitSteps[]; /*@ ds:3430 */     /* five strength-percentile thresholds {1,51,76,91,100}, used by 0A72:00CF */
extern signed char gStrPctHitBonus[]; /*@ ds:3435 */     /* five values that 0A72:00CF puts in place of the
                                                  base for the matching gStrPctHitSteps threshold */
extern signed char gStrHitBonus[]; /*@ ds:3439 */     /* base value per strength score, used by 0A72:00CF */
extern signed char gStrPctDamageSteps[]; /*@ ds:3453 */     /* five strength-percentile thresholds, used by 0A72:0128 */
extern signed char gStrPctDamageBonus[]; /*@ ds:3458 */     /* the five values 0A72:0128 puts in place of the
                                                  base gStrDamageBonus for the matching gStrPctDamageSteps threshold */
extern signed char gStrDamageBonus[]; /*@ ds:345C */     /* base value per strength score, used by 0A72:0128 */
extern int gDexMissileBonus[]; /*@ ds:3474 */             /* a word table indexed by dexterity (0A72:0181) */
extern int gDexArmourAdjust[]; /*@ ds:349E */             /* a word table indexed by dexterity (0A72:019D) */
extern long far *gExpTables[]; /*@ ds:35BC */       /* per-index far tables of 32-bit values, read by 0A72:04B7 */
extern char gClassSlotBaseClass[]; /*@ ds:35D4 */            /* a 4-wide byte table indexed row * 4 + slot (0A72:0509) */
extern signed char gHitGainPerStep[]; /*@ ds:3610 */     /* per-class multiplier, read by 0A72:0522 */
extern signed char gHitLevelsPerStep[]; /*@ ds:3614 */     /* per-class divisor, read by 0A72:0522 */
extern signed char gTurnUndeadTable[]; /*@ ds:3618 */     /* a 2D byte table, ten 0x0E-byte rows (0A72:0597) */
extern signed char gClassCombatGroup[]; /*@ ds:116E */     /* per-class byte, read by 0A72:0522 */
extern signed char gHitDieSlot0[]; /*@ ds:174B */     /* hit dice sides for group A, per class index (0A72:01AE) */
extern signed char gHitDieSlot1[]; /*@ ds:175A */     /* hit dice sides for group B, per class index (0A72:01AE) */
extern signed char gHitDieSlot2[]; /*@ ds:1769 */     /* hit dice sides for group C, per class index (0A72:01AE) */
extern signed char gConHitPointBonus[]; /*@ ds:1778 */     /* per-stat multiplier, indexed by the constitution byte minus 1 (0A72:01AE) */
extern char gTextBuf[]; /*@ ds:FC4E */         /* static buffer the formatting helpers write into */

/*@data */
/* the data of e0a72, ds:337A to ds:36B0 (822 bytes, 12 relocations): drafted by make data-skeleton from the original's bytes; each item at its annotation, in address order. */
char g337A[35] = { /*@ ds:337A */
    10, 9, 7, 6, 5, 4, 2, 14, 13, 11, 10, 9,
    8, 6, 13, 12, 10, 9, 8, 7, 5, 16, 15, 13,
    12, 11, 10, 8, 15, 14, 12, 11, 10, 9, 7
};
char g339D[30] = { /*@ ds:339D */
    13, 12, 11, 10, 9, 8, 14, 12, 10, 8, 6, 7,
    12, 11, 10, 9, 8, 4, 16, 15, 14, 13, 12, 11,
    15, 13, 11, 9, 7, 5
};
char g33BB[50] = { /*@ ds:33BB */
    16, 14, 13, 11, 10, 8, 7, 5, 4, 3, 18, 16,
    15, 13, 12, 10, 9, 7, 6, 5, 17, 15, 14, 12,
    11, 9, 8, 6, 5, 4, 20, 17, 16, 13, 12, 9,
    8, 5, 4, 4, 19, 17, 16, 14, 13, 11, 10, 8,
    7, 6
};
char g33ED[25] = { /*@ ds:33ED */
    14, 13, 11, 10, 8, 11, 9, 7, 5, 3, 13, 11,
    9, 7, 5, 15, 13, 11, 9, 7, 12, 10, 8, 6,
    4
};
char far *gSaveTables[6] = { g339D, g33ED, g337A, g339D, g33BB, g33BB }; /*@ ds:3406 */
signed char gSaveMaxLevel[6] = { 17, 21, 19, 21, 17, 17 }; /*@ ds:341E */
signed char gSaveLevelStep[6] = { 2, 5, 3, 4, 2, 2 }; /*@ ds:3424 */
signed char gSaveRowLength[6] = { 10, 5, 7, 6, 10, 10 }; /*@ ds:342A */
signed char gStrPctHitSteps[5] = { 1, 51, 76, 91, 100 }; /*@ ds:3430 */
signed char gStrPctHitBonus[4] = { 1, 2, 2, 2 }; /*@ ds:3435 */
signed char gStrHitBonus[26] = { /*@ ds:3439 */
    3, -5, -3, -3, -2, -2, -1, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 3, 3, 4, 4, 5,
    6, 7
};
signed char gStrPctDamageSteps[5] = { 1, 51, 76, 91, 100 }; /*@ ds:3453 */
signed char gStrPctDamageBonus[4] = { 3, 3, 4, 5 }; /*@ ds:3458 */
signed char gStrDamageBonus[24] = { /*@ ds:345C */
    6, -4, -2, -1, -1, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 2, 7, 8, 9, 10, 11
};
int gDexMissileBonus[21] = { /*@ ds:3474 */
    3596, -6, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 3, 3, 4, 4, 4, 5
};
int gDexArmourAdjust[25] = { /*@ ds:349E */
    5, 5, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0,
    0, 0, 0, -1, -2, -3, -4, -4, -5, -5, -5, -6,
    -6
};
long g34D0[12] = { 0L, 2500L, 5000L, 10000L, 20000L, 40000L, 60000L, 90000L, 135000L, 250000L, 375000L, -1L }; /*@ ds:34D0 */
long g3500[12] = { 0L, 2000L, 4000L, 8000L, 16000L, 32000L, 64000L, 125000L, 250000L, 500000L, 750000L, -1L }; /*@ ds:3500 */
long g3530[11] = { 0L, 1500L, 3000L, 6000L, 13000L, 27500L, 55000L, 110000L, 225000L, 450000L, -1L }; /*@ ds:3530 */
long g355C[12] = { 0L, 1250L, 2500L, 5000L, 10000L, 20000L, 40000L, 70000L, 110000L, 160000L, 220000L, -1L }; /*@ ds:355C */
long g358C[12] = { 0L, 2250L, 4500L, 9000L, 18000L, 36000L, 75000L, 150000L, 300000L, 600000L, 900000L, -1L }; /*@ ds:358C */
long far *gExpTables[6] = { g3500, g34D0, g3530, g355C, g358C, g358C }; /*@ ds:35BC */
char gClassSlotBaseClass[60] = { /*@ ds:35D4 */
    0, -1, -1, -1, 5, -1, -1, -1, 4, -1, -1, -1,
    1, -1, -1, -1, 2, -1, -1, -1, 3, -1, -1, -1,
    0, 2, -1, -1, 0, 3, -1, -1, 0, 1, -1, -1,
    0, 1, 3, -1, 3, 1, -1, -1, 2, 3, -1, -1,
    0, 2, 1, -1, 5, 2, -1, -1, 2, 1, -1, -1
};
signed char gHitGainPerStep[4] = { 2, 1, 1, 1 }; /*@ ds:3610 */
signed char gHitLevelsPerStep[4] = { 3, 2, 1, 3 }; /*@ ds:3614 */
signed char gTurnUndeadTable[140] = { /*@ ds:3618 */
    10, 7, 4, 0, 0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 13, 10, 7, 4, 0, 0, -1, -1, -1, -1,
    -1, -1, -1, -1, 16, 13, 10, 7, 4, 0, 0, -1,
    -1, -1, -1, -1, -1, -1, 19, 16, 13, 10, 7, 4,
    0, 0, -1, -1, -1, -1, -1, -1, 20, 19, 16, 13,
    10, 7, 4, 0, 0, -1, -1, -1, -1, -1, 99, 20,
    19, 16, 13, 10, 7, 4, 0, 0, 0, -1, -1, 0,
    99, 99, 20, 19, 16, 13, 10, 7, 4, 0, 0, 0,
    0, -1, 99, 99, 99, 20, 19, 16, 13, 10, 7, 4,
    4, 0, 0, 0, 99, 99, 99, 99, 20, 19, 16, 13,
    10, 7, 7, 4, 4, 0, 99, 99, 99, 99, 99, 20,
    19, 16, 13, 10, 10, 7, 7, 4
};
/* ds:36A4-36AC: the code's (a literal or template, '%d/%02d'): written by its function */
/* ds:36AC-36AF: the code's (a literal or template, '%d'): written by its function */
/* ds:36AF: the pad byte to the next module (word alignment) */


extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */

/* 0A72:0006: a byte from the per-index far buffer gSaveTables[index], at position
   min(limit, gSaveMaxLevel[index]) / gSaveLevelStep[index] + gSaveRowLength[index] * offset. The four parallel tables are
   gSaveTables (the buffers), gSaveMaxLevel (the per-index clamps), gSaveLevelStep (the divisors) and gSaveRowLength (the
   multipliers). It is the saving throw target: index the class group (0 for monsters), limit the
   level or hit dice, offset the save kind; the callers 08E6:1634, 0D1C:059B, 0DE8:0FCA and
   0EF3:1702 succeed on a 1d20 at or above it. */
/*@ 0A72:0006 */ int GetSavingThrowTarget(char index, char limit, char offset)
{
    char far *base;

    base = gSaveTables[index];
    if (limit > gSaveMaxLevel[index])
        limit = gSaveMaxLevel[index];
    limit /= gSaveLevelStep[index];
    limit += gSaveRowLength[index] * offset;
    return base[limit];
}

/* 0A72:007B: the slot 0, 1 or 2 whose gClassSlotBaseClass byte for class `classIndex` equals
   `slotId`, or -1.
   `slotId` is the gClassSlotBaseClass slot value searched for; the src/ callers pass 1, 2 and 4 (E0D1C.C:579,
   :586; E0EF3.C:640, :660, :663; EOV6.C:144, :209, :440, :441, :447). Called by 0D1C:0903,
   0EF3:0498, 0EF3:04F7 and overlays 1 and 6. */
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

/* 0A72:00CF: the attacker's strength to-hit bonus. It starts from the base gStrHitBonus[strength]
   (+0D is the
   strength score) and, only when the +0F percentile is nonzero, replaces it with gStrPctHitBonus[i] for
   every i with +0F >= gStrPctHitSteps[i] (so the last threshold that matches wins, and nothing is added or
   raised); when +0F is 0 the base is returned untouched. The evidence that +0D/+0F are a score and
   its percentile: gStrPctHitSteps holds five bytes {1, 51, 76, 91, 100} and gStrPctHitBonus holds {1, 2, 2, 2, 3}
   (ds:3430 and ds:3435), gStrHitBonus is the per-score base {3, -5, -3, ...}, and the only caller
   0DE8:0007 adds the result to the attack bonus when its `useStrength` is nonzero (GetDexMissileBonus is added
   otherwise, E0DE8.C:120-123). */
/*@ 0A72:00CF */ int GetStrengthToHitBonus(int who)
{
    int value;
    int i;

    value = gStrHitBonus[gChars[who].strength];
    if (gChars[who].strengthPct != 0) {
        for (i = 0; i < 5; i++) {
            if (gChars[who].strengthPct >= gStrPctHitSteps[i])
                value = gStrPctHitBonus[i];
        }
    }
    return value;
}

/* 0A72:0128: the attacker's strength damage bonus, the same shape as 0A72:00CF with the second set
   of tables: the base gStrDamageBonus[strength +0D] is replaced by gStrPctDamageBonus[i] for every i with the +0F
   percentile >= gStrPctDamageSteps[i] (the last match wins), and only when +0F is nonzero are the thresholds
   read; gStrPctDamageSteps is again {1, 51, 76, 91, 100}. The only caller 0DE8:011A starts its damage total
   from the result when its `useStrength` is nonzero and `who` is not -1 (E0DE8.C:148). */
/*@ 0A72:0128 */ int GetStrengthDamageBonus(int who)
{
    int value;
    int i;

    value = gStrDamageBonus[gChars[who].strength];
    if (gChars[who].strengthPct != 0) {
        for (i = 0; i < 5; i++) {
            if (gChars[who].strengthPct >= gStrPctDamageSteps[i])
                value = gStrPctDamageBonus[i];
        }
    }
    return value;
}

/* 0A72:0181: the dexterity missile bonus gDexMissileBonus[dexterity] (ds:3474, CharRec +15) for
   character `who`; the only caller RollToHit (0DE8:0007) adds this
   to the attack bonus when its `useStrength` is zero (GetStrengthToHitBonus is added otherwise, E0DE8.C:200-203). */
/*@ 0A72:0181 */ int GetDexMissileBonus(int who)
{
    return gDexMissileBonus[gChars[who].dexterity];
}

/* 0A72:019D: the word ds:349E[+15 stat byte] for the `dexterity` argument of a character record. It is
   the base 0DE8:0D5E builds the armour class armorClass from: 0DE8:0D5E does
   `base = GetArmourClassBase(rec->dexterity);` and then `rec->armorClass = base + 10;` (E0DE8.C:684-685),
   before the item modifiers and clamps. Called by 0DE8:0D5E, 0EF3:0F0B (which does
   `base = GetArmourClassBase(gCurRec->dexterity)` and compares armorClass with base + 6, E0EF3.C:1004-1005),
   ovl0:135D EditStatField (`GetArmourClassBase(amount)` under `if (index == 3)`, EOV0.C:1002) and
   ovl0:1BF2 RollCharacterStats (`GetArmourClassBase(scores[3])`, where scores[3] is stored as dexterity
   at EOV0.C:1103, EOV0.C:1106). */
/*@ 0A72:019D */ int GetArmourClassBase(char dexterity)
{
    return gDexArmourAdjust[dexterity];
}

/* 0A72:01AE: a random hit-point total for class `classIndex`. It rolls the three hit dice groups
   (RollDice(count, sides) for the class's dice sides gHitDieSlot0, gHitDieSlot1 and gHitDieSlot2), then looks up the
   per-stat multiplier gConHitPointBonus[constitution - 1] and adds multiplier * count for each group: the first group's
   term is always added, while the second and third are added only when both their count and their
   sides are nonzero. The multiplier is capped at 2 while gHitDieSlot0[classIndex] < 10 (group A's dice are
   small); no reason for that cap is known from src/. Finally it divides by 3 when gHitDieSlot2 is
   nonzero or by 2 when only gHitDieSlot1 is nonzero. The counts are the three class-slot levels levels[0..2],
   one die per level: RollCharacterStats calls RollHitPoints(classIndex, constitution, levels[0], levels[1], levels[2]) for a
   new character (EOV0.C:1107-1108), while LevelUpSlot calls it with one die for the single slot
   that gained a level (E0DE8.C:652-656). `constitution` is decremented in place and then reused to hold
   the multiplier (a separate local does not match: the original shows `dec byte ptr [bp + 8]` at
   0A72:01B2); `constitution` is the character's constitution (CharRec +17, which also indexes gConSaveBonus;
   the caller 0DE8:0C82 passes gChars[who].constitution, E0DE8.C:652), whose hit point bonus per
   die gConHitPointBonus holds. `classIndex` is the character's class index, CharRec +20 (the caller 0DE8:0C82 passes LevelUpSlot's
   `attr`, which AddExperience sets to gChars[who].classIndex, E0DE8.C:652 and :607, and overlay 0 pushes
   +20 at 1BF2:1E78). Called by 0DE8:0C82 and overlay 0 at 1BF2. */
/*@ 0A72:01AE */ int RollHitPoints(char classIndex, char constitution, char countA, char countB, char countC)
{
    int total;

    constitution--;
    total = RollDice(countA, gHitDieSlot0[classIndex], 0) + RollDice(countB, gHitDieSlot1[classIndex], 0)
          + RollDice(countC, gHitDieSlot2[classIndex], 0);
    constitution = gConHitPointBonus[constitution];
    if (constitution > 2 && gHitDieSlot0[classIndex] < 10)
        constitution = 2;
    total += constitution * countA;
    if (countB != 0 && gHitDieSlot1[classIndex] != 0)
        total += constitution * countB;
    if (countC != 0 && gHitDieSlot2[classIndex] != 0)
        total += constitution * countC;
    if (gHitDieSlot2[classIndex] != 0)
        total /= 3;
    else if (gHitDieSlot1[classIndex] != 0)
        total /= 2;
    return total;
}

/* 0A72:02B3: the maximum of the same three hit dice groups for class `classIndex`: countA *
   gHitDieSlot0[classIndex] + countB * gHitDieSlot1[classIndex] + countC * gHitDieSlot2[classIndex], plus the
   per-stat multiplier term. The same divide-by-3-or-2 as 0A72:01AE. Called by overlay 0 at 135D,
   which pushes gChars[gChargenMember].classIndex as `classIndex` (135D:14BE) and gChars[gChargenMember].constitution as `constitution`. */
/*@ 0A72:02B3 */ int MaxHitPoints(char classIndex, char constitution, char countA, char countB, char countC)
{
    int total;

    constitution--;
    total = countA * gHitDieSlot0[classIndex] + countB * gHitDieSlot1[classIndex] + countC * gHitDieSlot2[classIndex];
    constitution = gConHitPointBonus[constitution];
    if (constitution > 2 && gHitDieSlot0[classIndex] < 10)
        constitution = 2;
    total += constitution * countA;
    if (countB != 0 && gHitDieSlot1[classIndex] != 0)
        total += constitution * countB;
    if (countC != 0 && gHitDieSlot2[classIndex] != 0)
        total += constitution * countC;
    if (gHitDieSlot2[classIndex] != 0)
        total /= 3;
    else if (gHitDieSlot1[classIndex] != 0)
        total /= 2;
    return total;
}

/* 0A72:03A4: the minimum of the same three hit dice groups for class `classIndex`: countA + countB +
   countC (one per die), plus the per-stat multiplier term, the same divide-by-3-or-2, and finally
   clamped to at least 1. Called by overlay 0 at 135D, which pushes gChars[gChargenMember].classIndex as
   `classIndex` (135D:1514) and gChars[gChargenMember].constitution as `constitution`. */
/*@ 0A72:03A4 */ int MinHitPoints(char classIndex, char constitution, char countA, char countB, char countC)
{
    int total;

    constitution--;
    total = countA + countB + countC;
    constitution = gConHitPointBonus[constitution];
    if (constitution > 2 && gHitDieSlot0[classIndex] < 10)
        constitution = 2;
    total += constitution * countA;
    if (countB != 0 && gHitDieSlot1[classIndex] != 0)
        total += constitution * countB;
    if (countC != 0 && gHitDieSlot2[classIndex] != 0)
        total += constitution * countC;
    if (gHitDieSlot2[classIndex] != 0)
        total /= 3;
    else if (gHitDieSlot1[classIndex] != 0)
        total /= 2;
    if (total < 1)
        total = 1;
    return total;
}

/* 0A72:0468: formats a strength score and its exceptional percentile into the static buffer
   gTextBuf and returns it. When `percent` is 0 or less only the score is written, as "%d"; else
   the score and a two-digit percentile are written, as "%d/%02d". QUIRK 0A72:0468: a percentile of
   100 is written as 0, which a port must keep (inferred: the 18/100 form cannot be drawn). The
   "%d/%02d" and "%d" formats are the literals at ds:36A4 and ds:36AC. Called by 1547:0991 (drawn
   on the STRENGTH row of the CHARACTER INFO block, E1547.C:554), ovl0:065B and ovl0:135D (both
   pass CharRec strength, strengthPct). Inferred (not from a source in src/): 100 prints as 0 because %02d would
   widen the field to three digits, and the screen field is two digits wide; the only src/ caller
   (E1547.C:554) gives the x position, not the width. */
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
   gExpTables[index], where index is the gClassSlotBaseClass byte at row `row`, slot `slot` (the class's slot index),
   or -1 when that byte is -1 (the class has no such slot); the byte replaces `row` in place, the
   original uses one local for both. `level` is the level being reached, so the entry for level 1
   is at index 0: the caller 0DE8:0B45 passes gChars[who].levels[i] + 1 (E0DE8.C:613), adds the share
   to the slot's experience total experience[i] and levels the slot up when that total passes this value
   (E0DE8.C:612-615). Called by 0DE8:0B45 (AddExperience). */
/*@ 0A72:04B7 */ long GetClassSlotExpThreshold(char row, char slot, char level)
{
    long far *table;

    row = GetClassSlotByte(row, slot);
    if (row != -1) {
        table = gExpTables[row];
        return *(table + level - 1);
    }
    return -1;
}

/* 0A72:0509: the byte ds:35D4[row * 4 + slot] for a 4-wide table. A slot's byte is -1 (0xFF) when
   the class has no such slot: 0A72:007B scans slots 0, 1 and 2 for a match, 0A72:04B7 uses the
   byte as an index into gExpTables and returns -1 when it is -1, and the callers test for -1
   (E1547.C:566, E0DE8.C:610; E0DE8.C:614 tests 0A72:04B7's long result, not this byte). A
   non-negative byte indexes gBaseClassNames, the per-class-slot names, at E1547.C:568 (gBaseClassNames is declared at
   E1547.C:124), and gExpTables, and 0D1C:059B passes it to GetSavingThrowTarget as `index`, the gSaveTables buffer,
   after folding 4 and 5 to 0 (E0D1C.C:430-433). Purpose unknown. */
/*@ 0A72:0509 */ char GetClassSlotByte(char row, char slot)
{
    return gClassSlotBaseClass[row * 4 + slot];
}

/* 0A72:0522: 20 - (levels[0] - 1) / gHitLevelsPerStep[gClassCombatGroup[classIndex]] *
   gHitGainPerStep[gClassCombatGroup[classIndex]] - classByte, computed in
   bytes. It is the d20 number a to-hit roll must reach before the attacker bonus is subtracted:
   its only caller 0DE8:0007 computes `toHit = GetHitTarget(who, gMonsterTypes[entry].m00) - bonus;`
   (E0DE8.C:126) and then compares a d20 roll against it (`return roll >= toHit;`, E0DE8.C:134).
   `classByte` is the +0 field of the target's ds:1A3B entry (E0DE8.C:57), classIndex is the character's
   class index into ds:116E, and levels[0] is the character's first class-slot level at +24; this
   function subtracts 1 from it (`num`), while E0D1C.C:433 passes the unchanged levels[0] as
   GetSavingThrowTarget's limit, so the less-one value is used only here. Build note: `result` is what
   keeps the two lookups in memory; without it -O promotes `cls` (or `num`) to CL and the spills
   differ from the original. */
/*@ 0A72:0522 */ char GetHitTarget(char who, char classByte)
{
    char num;
    char cls;
    char result;

    num = gChars[who].levels[0] - 1;
    cls = gClassCombatGroup[gChars[who].classIndex];
    result = 20 - num / gHitLevelsPerStep[cls] * gHitGainPerStep[cls];
    result -= classByte;
    return result;
}

/* 0A72:058C: the difference of two ints, a monster's to-hit target: its only caller 08E6:106B takes
   `need = SubtractInts(gMonsterTypes[index].m01, gChars[who].armorClass)` and hits when a d20 roll reaches
   `need` (E08E6.C:861-862). */
/*@ 0A72:058C */ int SubtractInts(int minuend, int subtrahend)
{
    return minuend - subtrahend;
}

/* 0A72:0597: the byte ds:3618[row * 0x0E + col] with col clamped to 0x0E. The table is ten
   0x0E-byte rows and ends at ds:36A4, where the "%d/%02d" literal begins, so a row holds columns
   0x00..0x0D and 0x0E is one past the row. QUIRK 0A72:0597: the clamp admits col 0x0E, which
   reads ds:3618[row * 0x0E + 0x0E], the first byte of the next row (and, for the last row, the
   '%' byte at ds:36A4); a port must keep this aliasing. It is the turn undead target: TryTurnMonster
   (0D1C:09CB) passes the turner's level as col and the monster type's +19 byte as the row; -1 destroys. */
/*@ 0A72:0597 */ int GetTurnUndeadTarget(int col, int row)
{
    if (col > 0x0E)
        col = 0x0E;
    return gTurnUndeadTable[row * 0x0E + col];
}
