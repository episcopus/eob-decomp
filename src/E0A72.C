/* Unit at EOB.EXE 0A72:0006 (1458 bytes, 15 functions): accessors for a character record and the
   per-class tables that go with it. A character is a 243-byte (0xF3) record in DGROUP from ds:F580
   (struct CharRec below; the callers compute `index * 0xF3 + 0xF580`, 0D1C:059B); this unit reads
   its fields and derives stats through the far-pointer and byte tables at ds:3406, ds:341E,
   ds:3424, ds:342A, ds:3430, ds:3435, ds:3439, ds:3453, ds:3458, ds:345C, ds:3474, ds:349E,
   ds:35BC, ds:35D4, ds:3610, ds:3614, ds:3618 and ds:116E. Called from the party, combat and
   inventory modules (0DE8, 0D1C, 0EF3, 08E6) and from overlays 0, 1 and 6. Most names are
   placeholders until the record fields are identified. */

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
extern unsigned char g35D4[]; /*@ ds:35D4 */   /* a per-index byte table, indexed [a*4 + b] */
extern int g3474[]; /*@ ds:3474 */             /* a word table indexed by a stat class */
extern int g349E[]; /*@ ds:349E */             /* a word table indexed by a class byte */
extern signed char g3618[]; /*@ ds:3618 */     /* a 2D byte table, 0x0E bytes per row */

/*@ 0A72:0006 stub */ void f0A72_0006(void) {}
/*@ 0A72:007B stub */ void f0A72_007B(void) {}
/*@ 0A72:00CF stub */ void f0A72_00CF(void) {}
/*@ 0A72:0128 stub */ void f0A72_0128(void) {}
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
/*@ 0A72:01AE stub */ void f0A72_01AE(void) {}
/*@ 0A72:02B3 stub */ void f0A72_02B3(void) {}
/*@ 0A72:03A4 stub */ void f0A72_03A4(void) {}
/*@ 0A72:0468 stub */ void f0A72_0468(void) {}
/*@ 0A72:04B7 stub */ void f0A72_04B7(void) {}
/* 0A72:0509: the byte ds:35D4[row][slot] for a 4-wide table. Purpose unknown: 0A72:007B scans
   the four slots looking for a match and 0A72:04B7 uses the result as a row index (in 0D1C:059B
   the result is compared with 4 and 5). */
/*@ 0A72:0509 */ char GetClassSlotByte(char row, char slot)
{
    return g35D4[row * 4 + slot];
}
/*@ 0A72:0522 stub */ void f0A72_0522(void) {}
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
