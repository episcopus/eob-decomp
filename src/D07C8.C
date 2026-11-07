/* Module d07c8 of EOB.EXE: data only, ds:009E to ds:3056 (12,216 bytes, 448 relocations), and
   the empty code segment at 07C8:0001 (TLINK's segment table lists it). The game's tables:
   gTandySelected, the text windows (gRegions) and the option lists (gPanels), the file table
   (gFiles, 203 records and the empty name that ends it) and the frame table after it, the CGA
   colour map, the character generation tables (stat, race, class and alignment names, class bits
   and slot counts), the spell names and the spell records with their callbacks in 0EF3 (E0EF3.C),
   the hit dice and constitution bonus, the direction and view cell tables, the inventory slot
   positions, the wall tables, the monster types, the item slot masks, the hit flash remaps and
   the view position tables; then, from ds:1EC9 to ds:3055, the string pool: every literal of the
   tables, in the order the tables give them (each is reached once, so none is a named array).
   Compiled: its data relocations fall in the relocation table, as Turbo C writes a module's
   fixups (TASM writes them upward; playbook 80, step D). Drafted by make data-skeleton; the names
   other files of src/ declare are used, the rest are named by their address until a later round
   names them. */

#include "EOB.H"

/*@data */
/* The file record, as src/E1C0E.C:119-126 defines it (that file keeps the definition to itself). */
struct FileEntry {
    char far *name;             /* +0x00: the file name */
    long size;                  /* +0x04: the length on disk, or inside the PAK for an alias */
    char far *data;             /* +0x08: the in-memory copy when the record is loaded */
    int link;                   /* +0x0C: < 0: the low 15 bits are the PAK record this aliases */
    long pos;                   /* +0x0E: the read/write position in the file */
    signed char flags;          /* +0x12: FLAG_KEEP, FLAG_LOAD, FLAG_BITS (E1C0E.C:92-94) */
};
/* a link to the PAK at gFiles[rec]: bit 15 set, the record in LINK_INDEX (E1C0E.C:95, :292) */
#define IN_PAK(rec) ((int)(0x8000u | (rec)))

/* E0EF3.C:15's view of a projectile record, which the Hit callbacks take */
struct Tgt;
/* the spell callbacks the records name (E0EF3.C); a Hit callback is stored as Spell.ddFn and an End
   callback without a record as Spell.remove, through a cast to the field's type */
extern void far CastArmor(void); /*@ 0EF3:0F0B */
extern void far EndArmor(struct CharRec far *rec); /*@ 0EF3:0F86 */
extern void far CastBurningHands(void); /*@ 0EF3:0F99 */
extern void far CastDetectMagic(void); /*@ 0EF3:1069 */
extern void far EndDetectMagic(void); /*@ 0EF3:10AE */
extern void far CastMagicMissile(void); /*@ 0EF3:10D2 */
extern void far HitMagicMissile(struct Tgt far *rec); /*@ 0EF3:10F2 */
extern void far CastShield(void); /*@ 0EF3:1122 */
extern void far EndShield(struct CharRec far *rec); /*@ 0EF3:1176 */
extern void far CastShockingGrasp(void); /*@ 0EF3:1197 */
extern void far DiscardEquippedItems(struct CharRec far *rec); /*@ 0EF3:1233 */
extern void far CastInvisibility(void); /*@ 0EF3:127F */
extern void far EndInvisibility(struct CharRec far *rec); /*@ 0EF3:12A1 */
extern void far CastAcidArrow(void); /*@ 0EF3:12C2 */
extern void far HitAcidArrow(struct Tgt far *rec); /*@ 0EF3:12E3 */
extern void far CastStinkingCloudNoOp(void); /*@ 0EF3:1315 */
extern void far CastDispelMagic(void); /*@ 0EF3:131A */
extern void far CastFireball(void); /*@ 0EF3:13FD */
extern void far HitFireball(struct Tgt far *rec); /*@ 0EF3:141E */
extern void far CastFlameArrow(void); /*@ 0EF3:144A */
extern void far HitFlameArrow(struct Tgt far *rec); /*@ 0EF3:146B */
extern void far CastHaste(void); /*@ 0EF3:1497 */
extern void far EndHaste(struct CharRec far *rec); /*@ 0EF3:14F9 */
extern void far CastHoldPerson(void); /*@ 0EF3:150F */
extern int far HitHoldPerson(struct Tgt far *rec); /*@ 0EF3:1530 */
extern void far CastInvisibility10(void); /*@ 0EF3:15D0 */
extern void far CastLightningBolt(void); /*@ 0EF3:1609 */
extern void far HitLightningBolt(struct Tgt far *rec); /*@ 0EF3:162A */
extern void far CastVampiricTouch(void); /*@ 0EF3:1656 */
extern void far EndVampiricTouch(struct CharRec far *rec); /*@ 0EF3:16DC */
extern void far CastFear(void); /*@ 0EF3:1702 */
extern void far CastIceStorm(void); /*@ 0EF3:17B7 */
extern int far HitIceStorm(struct Tgt far *rec); /*@ 0EF3:17D8 */
extern void far CastStoneskin(void); /*@ 0EF3:1865 */
extern void far CastCloudkillNoOp(void); /*@ 0EF3:18D3 */
extern void far CastConeOfCold(void); /*@ 0EF3:18D8 */
extern void far CastHoldMonster(void); /*@ 0EF3:1A0F */
extern int far HitHoldMonster(struct Tgt far *rec); /*@ 0EF3:1A30 */
extern void far CastBless(void); /*@ 0EF3:1AD0 */
extern void far EndBless(void); /*@ 0EF3:1B13 */
extern void far CastCureLightWounds(void); /*@ 0EF3:1B20 */
extern void far CastCauseLightWounds(void); /*@ 0EF3:1B43 */
extern void far CastProtFromEvil(void); /*@ 0EF3:1B5A */
extern void far EndProtFromEvil(struct CharRec far *rec); /*@ 0EF3:1B8D */
extern void far CastAid(void); /*@ 0EF3:1BA3 */
extern void far EndAid(struct CharRec far *rec); /*@ 0EF3:1C45 */
extern void far CastFlameBlade(void); /*@ 0EF3:1D38 */
extern void far CastSlowPoison(void); /*@ 0EF3:1C75 */
extern void far EndSlowPoison(struct CharRec far *rec); /*@ 0EF3:1CD2 */
extern void far CastCreateFood(void); /*@ 0EF3:1DC0 */
extern void far CastMagicalVestment(void); /*@ 0EF3:1DDF */
extern void far EndMagicalVestment(struct CharRec far *rec); /*@ 0EF3:1E1D */
extern void far CastPrayer(void); /*@ 0EF3:1E3E */
extern void far EndPrayer(void); /*@ 0EF3:1E6C */
extern void far CastRemoveParalysis(void); /*@ 0EF3:1E7E */
extern void far CastCureSeriousWounds(void); /*@ 0EF3:1ECC */
extern void far CastCauseSeriousWounds(void); /*@ 0EF3:1EF3 */
extern void far CastNeutralizePoison(void); /*@ 0EF3:1F0B */
extern void far CastProtFromEvil10(void); /*@ 0EF3:1F33 */
extern void far EndProtFromEvil10(void); /*@ 0EF3:1F8F */
extern void far CastProtFromLightningNoOp(void); /*@ 0EF3:1FC8 */
extern void far CastCureCriticalWounds(void); /*@ 0EF3:1FD2 */
extern void far CastCauseCriticalWounds(void); /*@ 0EF3:1FF9 */
extern void far CastFlameStrike(void); /*@ 0EF3:2011 */
extern void far HitFlameStrike(struct Tgt far *rec); /*@ 0EF3:2032 */
extern void far CastRaiseDead(void); /*@ 0EF3:2050 */
extern void far CastLayOnHands(void); /*@ 0EF3:21BC */
extern void far RunMelee0(struct Tgt far *rec); /*@ 0EF3:208A */
extern int far DamageOneMember(struct Tgt far *rec); /*@ 0EF3:20A8 */
extern int far DamageParty(struct Tgt far *rec); /*@ 0EF3:2145 */

unsigned char gTandySelected = 0; /*@ ds:009E */
struct Region gRegions[14] = { /*@ ds:009F */
    { 0, 0, 40, 200, 15, 12, 0, 0 },
    { 8, 72, 24, 56, 14, 12, 0, 0 },
    { 19, 64, 20, 128, 6, 12, 0, 0 },
    { 29, 120, 8, 64, 15, 13, 0, 0 },
    { 2, 24, 20, 120, 15, 2, 3, 0 },
    { 0, 0, 22, 120, 15, 13, 0, 0 },
    { 10, 168, 21, 24, 15, 12, 0, 0 },
    { 1, 179, 34, 18, 15, 12, 0, 0 },
    { 2, 24, 20, 0, 15, 2, 3, 0 },
    { 1, 125, 38, 63, 15, 0, 3, 0 },
    { 0, 0, 22, 144, 15, 2, 0, 0 },
    { 1, 20, 20, 56, 15, 2, 0, 0 },
    { 1, 4, 20, 160, 15, 2, 0, 0 },
    { 1, 25, 38, 100, 15, 2, 0, 0 }
};
struct Panel gPanels[8] = { /*@ ds:017F */
    { 1, 16, 38, 20, 0, 15, 6, { 0, 0 } },
    { 0, 16, 16, 12, 0, 15, 6, { 0, 0 } },
    { 0, 16, 23, 0, 0, 15, 6, { 0, 0 } },
    { 0, 16, 16, 0, 0, 15, 6, { 0, 0 } },
    { 0, 16, 7, 4, 0, 15, 6, { 0, 0 } },
    { 0, 0, 17, 5, 0, 15, 6, { 0, 0 } },
    { 0, 0, 21, 3, 0, 15, 6, { 0, 0 } },
    { 0, 0, 17, 8, 0, 15, 6, { 0, 0 } }
};
/* The file table: 203 records and the empty name that ends it (src/E1C0E.C:119). */
struct FileEntry gFiles[204] = { /*@ ds:01FF */
    { "INTRO.EXE", 88000L, 0, 0, 0L, 0 },
    { "eobdata1.pak", 0L, 0, 0, 0L, 0 },
    { "ADLIB.DAT", 7489L, 0, IN_PAK(1), 0L, 0 },
    { "PCSOUND.DAT", 1990L, 0, IN_PAK(1), 0L, 0 },
    { "PCGAMESN.DAT", 4757L, 0, IN_PAK(1), 0L, 0 },
    { "ADVENTUR.CMP", 8078L, 0, IN_PAK(1), 0L, 0 },
    { "AVALANCH.CMP", 20524L, 0, IN_PAK(1), 0L, 0 },
    { "HANDS.CMP", 12854L, 0, IN_PAK(1), 0L, 0 },
    { "KING.CMP", 13685L, 0, IN_PAK(1), 0L, 0 },
    { "ORB.CMP", 17252L, 0, IN_PAK(1), 0L, 0 },
    { "TEXT.CMP", 5341L, 0, IN_PAK(1), 0L, 0 },
    { "TITLE-E.CMP", 9568L, 0, IN_PAK(1), 0L, 0 },
    { "TITLE-V.CMP", 16526L, 0, IN_PAK(1), 0L, 0 },
    { "TOWRMAGE.CMP", 19736L, 0, IN_PAK(1), 0L, 0 },
    { "TUNNEL.CMP", 23134L, 0, IN_PAK(1), 0L, 0 },
    { "WTRDP1.CMP", 14403L, 0, IN_PAK(1), 0L, 0 },
    { "WTRDP2.CMP", 21165L, 0, IN_PAK(1), 0L, 0 },
    { "WTRDP3.CMP", 23101L, 0, IN_PAK(1), 0L, 0 },
    { "ZOOMTUNL.CMP", 17825L, 0, IN_PAK(1), 0L, 0 },
    { "EOB.EXE", 0L, 0, 1, 0L, 0 },
    { "LEVELS.TMP", 0L, 0, 1, 0L, 0 },
    { "eobdata2.pak", 0L, 0, 1, 0L, 0 },
    { "DOOR.EGA", 13699L, 0, IN_PAK(21), 0L, 0 },
    { "EOBPAL.COL", 768L, 0, IN_PAK(21), 0L, 0 },
    { "eobdata3.pak", 0L, 0, 2, 0L, 0 },
    { "BRICK1.CPS", 11543L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK2.CPS", 7337L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK3.CPS", 3303L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK1.EGA", 9215L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK2.EGA", 5270L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK3.EGA", 2717L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK.DAT", 4540L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK.VCN", 34847L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK.VMP", 5834L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK.ECN", 20641L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK.EMP", 5834L, 0, IN_PAK(24), 0L, 0 },
    { "BRICK.PAL", 768L, 0, IN_PAK(24), 0L, 0 },
    { "LEVEL1.INF", 1960L, 0, IN_PAK(24), 0L, 0 },
    { "LEVEL1.MAZ", 4102L, 0, IN_PAK(24), 0L, 0 },
    { "KOBOLD.CPS", 4072L, 0, IN_PAK(24), 0L, 0 },
    { "KOBOLD.EGA", 3520L, 0, IN_PAK(24), 0L, 0 },
    { "LEECH.CPS", 4485L, 0, IN_PAK(24), 0L, 0 },
    { "LEECH.EGA", 4144L, 0, IN_PAK(24), 0L, 0 },
    { "LEVEL2.INF", 4118L, 0, IN_PAK(24), 0L, 0 },
    { "LEVEL2.MAZ", 4102L, 0, IN_PAK(24), 0L, 0 },
    { "SKELETON.CPS", 8406L, 0, IN_PAK(24), 0L, 0 },
    { "SKELETON.EGA", 6564L, 0, IN_PAK(24), 0L, 0 },
    { "ZOMBIE.CPS", 7242L, 0, IN_PAK(24), 0L, 0 },
    { "ZOMBIE.EGA", 5801L, 0, IN_PAK(24), 0L, 0 },
    { "LEVEL3.INF", 3829L, 0, IN_PAK(24), 0L, 0 },
    { "LEVEL3.MAZ", 4102L, 0, IN_PAK(24), 0L, 0 },
    { "FLIND.CPS", 7944L, 0, IN_PAK(24), 0L, 0 },
    { "FLIND.EGA", 6206L, 0, IN_PAK(24), 0L, 0 },
    { "KUOTOA.CPS", 9990L, 0, IN_PAK(24), 0L, 0 },
    { "KUOTOA.EGA", 7777L, 0, IN_PAK(24), 0L, 0 },
    { "INVENT.CPS", 2481L, 0, IN_PAK(24), 0L, 0 },
    { "INVENT.EGA", 1998L, 0, IN_PAK(24), 0L, 0 },
    { "TEXT.DAT", 16105L, 0, IN_PAK(24), 0L, 0 },
    { "OUTTAKE.CPS", 21234L, 0, IN_PAK(24), 0L, 0 },
    { "OUTTAKE.EGA", 15270L, 0, IN_PAK(24), 0L, 0 },
    { "INTRO.EGA", 9568L, 0, IN_PAK(24), 0L, 0 },
    { "INTRO.CPS", 16479L, 0, IN_PAK(24), 0L, 0 },
    { "CHARGEN.CPS", 25914L, 0, IN_PAK(24), 0L, 0 },
    { "CHARGEN.EGA", 10466L, 0, IN_PAK(24), 0L, 0 },
    { "CHARGENA.EGA", 15975L, 0, IN_PAK(24), 0L, 0 },
    { "eobdata4.pak", 0L, 0, 3, 0L, 0 },
    { "BLUE.CPS", 22787L, 0, IN_PAK(65), 0L, 0 },
    { "BLUE.EGA", 13946L, 0, IN_PAK(65), 0L, 0 },
    { "BLUE.DAT", 3228L, 0, IN_PAK(65), 0L, 0 },
    { "BLUE.VCN", 40822L, 0, IN_PAK(65), 0L, 0 },
    { "BLUE.VMP", 5834L, 0, IN_PAK(65), 0L, 0 },
    { "BLUE.ECN", 24290L, 0, IN_PAK(65), 0L, 0 },
    { "BLUE.EMP", 5834L, 0, IN_PAK(65), 0L, 0 },
    { "BLUE.PAL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "LEVEL4.INF", 3692L, 0, IN_PAK(65), 0L, 0 },
    { "LEVEL4.MAZ", 4102L, 0, IN_PAK(65), 0L, 0 },
    { "SPIDER1.CPS", 6581L, 0, IN_PAK(65), 0L, 0 },
    { "SPIDER1.EGA", 6050L, 0, IN_PAK(65), 0L, 0 },
    { "LEVEL5.INF", 4599L, 0, IN_PAK(65), 0L, 0 },
    { "LEVEL5.MAZ", 4102L, 0, IN_PAK(65), 0L, 0 },
    { "DWARF.CPS", 5051L, 0, IN_PAK(65), 0L, 0 },
    { "DWARF.EGA", 4114L, 0, IN_PAK(65), 0L, 0 },
    { "LEVEL6.INF", 4234L, 0, IN_PAK(65), 0L, 0 },
    { "LEVEL6.MAZ", 4102L, 0, IN_PAK(65), 0L, 0 },
    { "KENKU.CPS", 7220L, 0, IN_PAK(65), 0L, 0 },
    { "KENKU.EGA", 5902L, 0, IN_PAK(65), 0L, 0 },
    { "MAGE.CPS", 6171L, 0, IN_PAK(65), 0L, 0 },
    { "MAGE.EGA", 5658L, 0, IN_PAK(65), 0L, 0 },
    { "XDEATH1.CPS", 15749L, 0, IN_PAK(65), 0L, 0 },
    { "XDEATH2.CPS", 13040L, 0, IN_PAK(65), 0L, 0 },
    { "XDEATH3.CPS", 1087L, 0, IN_PAK(65), 0L, 0 },
    { "XDEATH1.EGA", 12744L, 0, IN_PAK(65), 0L, 0 },
    { "XDEATH2.EGA", 10237L, 0, IN_PAK(65), 0L, 0 },
    { "XDEATH3.EGA", 773L, 0, IN_PAK(65), 0L, 0 },
    { "CHARGENB.CPS", 7888L, 0, IN_PAK(65), 0L, 0 },
    { "CHARGENB.EGA", 5300L, 0, IN_PAK(65), 0L, 0 },
    { "CHARGENA.CPS", 27474L, 0, IN_PAK(65), 0L, 0 },
    { "ORB.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "PALETTE.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "TITLE-V.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "TOWRMAGE.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "WESTWOOD.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "WTRDP2.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "ZOOMTUNL.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "EOBPAL.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "ORB.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "PALETTE.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "TITLE-V.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "TOWRMAGE.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "WESTWOOD.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "WTRDP2.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "ZOOMTUNL.COL", 768L, 0, IN_PAK(65), 0L, 0 },
    { "SOUND.DAT", 13139L, 0, IN_PAK(65), 0L, 0 },
    { "AND.CMP", 364L, 0, IN_PAK(65), 0L, 0 },
    { "DAND.CMP", 5800L, 0, IN_PAK(65), 0L, 0 },
    { "EOBTITLE.CMP", 6966L, 0, IN_PAK(65), 0L, 0 },
    { "PRESENT.CMP", 547L, 0, IN_PAK(65), 0L, 0 },
    { "SSI.CMP", 2882L, 0, IN_PAK(65), 0L, 0 },
    { "WESTWOOD.CMP", 4765L, 0, IN_PAK(65), 0L, 0 },
    { "DOOR.CPS", 25232L, 0, IN_PAK(65), 0L, 0 },
    { "eobdata5.pak", 0L, 0, 4, 0L, 0 },
    { "GREEN.CPS", 13680L, 0, IN_PAK(120), 0L, 0 },
    { "GREEN.EGA", 10116L, 0, IN_PAK(120), 0L, 0 },
    { "GREEN.DAT", 2820L, 0, IN_PAK(120), 0L, 0 },
    { "GREEN.VCN", 42770L, 0, IN_PAK(120), 0L, 0 },
    { "GREEN.VMP", 5834L, 0, IN_PAK(120), 0L, 0 },
    { "GREEN.ECN", 34728L, 0, IN_PAK(120), 0L, 0 },
    { "GREEN.EMP", 5834L, 0, IN_PAK(120), 0L, 0 },
    { "GREEN.PAL", 768L, 0, IN_PAK(120), 0L, 0 },
    { "LEVEL10.INF", 2342L, 0, IN_PAK(120), 0L, 0 },
    { "LEVEL10.MAZ", 4102L, 0, IN_PAK(120), 0L, 0 },
    { "SHINDIA.CPS", 5639L, 0, IN_PAK(120), 0L, 0 },
    { "SHINDIA.EGA", 4794L, 0, IN_PAK(120), 0L, 0 },
    { "MANTIS1.CPS", 9163L, 0, IN_PAK(120), 0L, 0 },
    { "MANTIS1.EGA", 8324L, 0, IN_PAK(120), 0L, 0 },
    { "LEVEL11.INF", 2482L, 0, IN_PAK(120), 0L, 0 },
    { "LEVEL11.MAZ", 4102L, 0, IN_PAK(120), 0L, 0 },
    { "MFLAYER.CPS", 8837L, 0, IN_PAK(120), 0L, 0 },
    { "MFLAYER.EGA", 7304L, 0, IN_PAK(120), 0L, 0 },
    { "XORN1.CPS", 10862L, 0, IN_PAK(120), 0L, 0 },
    { "XORN1.EGA", 7231L, 0, IN_PAK(120), 0L, 0 },
    { "XANATH1.CPS", 9102L, 0, IN_PAK(120), 0L, 0 },
    { "XANATHA.CPS", 9261L, 0, IN_PAK(120), 0L, 0 },
    { "XANATH1.EGA", 8751L, 0, IN_PAK(120), 0L, 0 },
    { "XANATHA.EGA", 6766L, 0, IN_PAK(120), 0L, 0 },
    { "XANATHA.DAT", 2096L, 0, IN_PAK(120), 0L, 0 },
    { "XANATHA.VCN", 25328L, 0, IN_PAK(120), 0L, 0 },
    { "XANATHA.VMP", 5834L, 0, IN_PAK(120), 0L, 0 },
    { "XANATHA.ECN", 12974L, 0, IN_PAK(120), 0L, 0 },
    { "XANATHA.EMP", 5834L, 0, IN_PAK(120), 0L, 0 },
    { "XANATHA.PAL", 768L, 0, IN_PAK(120), 0L, 0 },
    { "LEVEL12.INF", 2104L, 0, IN_PAK(120), 0L, 0 },
    { "LEVEL12.MAZ", 4102L, 0, IN_PAK(120), 0L, 0 },
    { "GOLEM.CPS", 8973L, 0, IN_PAK(120), 0L, 0 },
    { "GOLEM.EGA", 6070L, 0, IN_PAK(120), 0L, 0 },
    { "PLAYFLD.CPS", 9983L, 0, IN_PAK(120), 0L, 0 },
    { "PLAYFLD.EGA", 4666L, 0, IN_PAK(120), 0L, 0 },
    { "EOBDATA.SAV", 0L, 0, 5, 0L, 0 },
    { "eobdata6.pak", 0L, 0, 5, 0L, 0 },
    { "DROW.CPS", 14722L, 0, IN_PAK(158), 0L, 0 },
    { "DROW.EGA", 8384L, 0, IN_PAK(158), 0L, 0 },
    { "DROW.DAT", 2744L, 0, IN_PAK(158), 0L, 0 },
    { "DROW.VCN", 38539L, 0, IN_PAK(158), 0L, 0 },
    { "DROW.VMP", 5834L, 0, IN_PAK(158), 0L, 0 },
    { "DROW.ECN", 21219L, 0, IN_PAK(158), 0L, 0 },
    { "DROW.EMP", 5834L, 0, IN_PAK(158), 0L, 0 },
    { "DROW.PAL", 768L, 0, IN_PAK(158), 0L, 0 },
    { "LEVEL7.INF", 3663L, 0, IN_PAK(158), 0L, 0 },
    { "LEVEL7.MAZ", 4102L, 0, IN_PAK(158), 0L, 0 },
    { "DROWELF.CPS", 7115L, 0, IN_PAK(158), 0L, 0 },
    { "DROWELF.EGA", 6184L, 0, IN_PAK(158), 0L, 0 },
    { "SKELWAR.CPS", 6974L, 0, IN_PAK(158), 0L, 0 },
    { "SKELWAR.EGA", 6188L, 0, IN_PAK(158), 0L, 0 },
    { "LEVEL8.INF", 3402L, 0, IN_PAK(158), 0L, 0 },
    { "LEVEL8.MAZ", 4102L, 0, IN_PAK(158), 0L, 0 },
    { "HELLHND.CPS", 5595L, 0, IN_PAK(158), 0L, 0 },
    { "HELLHND.EGA", 4339L, 0, IN_PAK(158), 0L, 0 },
    { "DRIDER1.CPS", 8983L, 0, IN_PAK(158), 0L, 0 },
    { "DRIDER1.EGA", 8241L, 0, IN_PAK(158), 0L, 0 },
    { "LEVEL9.INF", 3727L, 0, IN_PAK(158), 0L, 0 },
    { "LEVEL9.MAZ", 4102L, 0, IN_PAK(158), 0L, 0 },
    { "RUST1.CPS", 6929L, 0, IN_PAK(158), 0L, 0 },
    { "RUST1.EGA", 6430L, 0, IN_PAK(158), 0L, 0 },
    { "DISBEAST.CPS", 5159L, 0, IN_PAK(158), 0L, 0 },
    { "DISBEAST.EGA", 4136L, 0, IN_PAK(158), 0L, 0 },
    { "THROWN.CPS", 6393L, 0, IN_PAK(158), 0L, 0 },
    { "THROWN.EGA", 5778L, 0, IN_PAK(158), 0L, 0 },
    { "DECORATE.CPS", 2967L, 0, IN_PAK(158), 0L, 0 },
    { "DECORATE.EGA", 2148L, 0, IN_PAK(158), 0L, 0 },
    { "ITEMICN.CPS", 8655L, 0, IN_PAK(158), 0L, 0 },
    { "ITEML1.CPS", 9645L, 0, IN_PAK(158), 0L, 0 },
    { "ITEMS1.CPS", 6497L, 0, IN_PAK(158), 0L, 0 },
    { "ITEMICN.EGA", 6455L, 0, IN_PAK(158), 0L, 0 },
    { "ITEML1.EGA", 7033L, 0, IN_PAK(158), 0L, 0 },
    { "ITEMRMP.EGA", 1600L, 0, IN_PAK(158), 0L, 0 },
    { "ITEMS1.EGA", 5094L, 0, IN_PAK(158), 0L, 0 },
    { "ITEM.DAT", 9601L, 0, IN_PAK(158), 0L, 0 },
    { "ITEMTYPE.DAT", 914L, 0, IN_PAK(158), 0L, 0 },
    { "ITEMRMP.VGA", 1600L, 0, IN_PAK(158), 0L, 0 },
    { "PORTALA.CPS", 13225L, 0, IN_PAK(158), 0L, 0 },
    { "PORTALB.CPS", 16861L, 0, IN_PAK(158), 0L, 0 },
    { "PORTALA.EGA", 5420L, 0, IN_PAK(158), 0L, 0 },
    { "PORTALB.EGA", 9110L, 0, IN_PAK(158), 0L, 0 },
    { "", 0L, 0, 0, 0L, 0 }
};
/* ds:1123: the frame table gFrameTable points at, 12 words (src/E1DE7.C:132-164 reads it) */
unsigned g1123[12] = { 0, 2, 5, 4, 1, 255, 3, 255, 7, 255, 6, 255 }; /*@ ds:1123 */
unsigned far *gFrameTable[1] = { g1123 }; /*@ ds:113B */
char gCgaMapDefault[30] = { /*@ ds:113F */
    0, 2, 2, 1, 1, 0, 1, 2, 3, 1, 1, 1, 0, 2, 2, 3, 0, 2, 2, 1, 1, 0, 1, 2, 3, 3, 1, 1, 0, 0
};
/* EOV0.C:125 declares ds:115D a table of far string pointers and reads its entries 8 to 13
   (EOV0.C:394-395), which are g117D below; the two bytes at ds:115D itself are not pointers. */
unsigned char gAbilityAbbrevs[2] = { 0x02, 0x03 }; /*@ ds:115D */
signed char gClassSlotCount[15] = { 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 2, 2, 3, 2, 2 }; /*@ ds:115F */
signed char gClassCombatGroup[15] = { 0, 0, 0, 1, 2, 3, 0, 0, 0, 0, 3, 2, 0, 0, 2 }; /*@ ds:116E */
/* the ability abbreviations ovl0:065B draws down the panel's left column, gAbilityAbbrevs[8..13]
   in EOV0.C:395 */
char far *g117D[6] = { "STR", "INT", "WIS", "DEX", "CON", "CHA" }; /*@ ds:117D */
char far *gStatNames[6] = { /*@ ds:1195 */
    "STRENGTH", "INTELLIGENCE", "WISDOM", "DEXTERITY", "CONSTITUTION", "CHARISMA"
};
char far *gRaceNames[12] = { /*@ ds:11AD */
    "HUMAN MALE", "HUMAN FEMALE", "ELF MALE", "ELF FEMALE", "HALF-ELF MALE", "HALF-ELF FEMALE",
    "DWARF MALE", "DWARF FEMALE", "GNOME MALE", "GNOME FEMALE", "HALFLING MALE", "HALFLING FEMALE"
};
char far *gClassNames[15] = { /*@ ds:11DD */
    "FIGHTER", "RANGER", "PALADIN", "MAGE", "CLERIC", "THIEF", "FIGHTER/CLERIC", "FIGHTER/THIEF",
    "FIGHTER/MAGE", "FIGHTER/MAGE/THIEF", "THIEF/MAGE", "CLERIC/THIEF", "FIGHTER/CLERIC/MAGE",
    "RANGER/CLERIC", "CLERIC/MAGE"
};
char far *gBaseClassNames[6] = { /*@ ds:1219 */
    "FIGHTER", "MAGE", "CLERIC", "THIEF", "PALADIN", "RANGER"
};
signed char gClassBaseBits[15] = { 1, 1, 5, 2, 4, 8, 5, 9, 3, 11, 10, 12, 7, 5, 6 }; /*@ ds:1231 */
char far *gAlignmentNames[14] = { /*@ ds:1240 */
    "LAWFUL GOOD", "NEUTRAL GOOD", "CHAOTIC GOOD", "LAWFUL NEUTRAL", "TRUE NEUTRAL",
    "CHAOTIC NEUTRAL", "LAWFUL EVIL", "NEUTRAL EVIL", "CHAOTIC EVIL", "First", "Second", "Third",
    "Fourth", "Fifth"
};
char far *gMageSpellNames[26] = { /*@ ds:1278 */
    "", "Armor", "Burning Hands", "Detect Magic", "Magic Missile", "Read Magic", "Shield",
    "Shocking Grasp", "Invisibility", "Knock", "M's Acid Arrow", "Stinking Cloud", "Dispel Magic",
    "Fireball", "Flame Arrow", "Haste", "Hold Person", "Invisibility 10'", "Lightning Bolt",
    "Vampiric Touch", "Fear", "Ice Storm", "Stoneskin", "Cloudkill", "Cone of Cold", "Hold Monster"
};
char far *gClericSpellNames[25] = { /*@ ds:12E0 */
    "", "Bless", "Cure Light Wnds", "Cause Light Wnds", "Detect Magic", "Protect-Evil", "Aid",
    "Flame Blade", "Hold Person", "Slow Poison", "Create Food", "Dispel Magic", "Magical Vestment",
    "Prayer", "Remove Paralysis", "Cure Serious", "Cause Serious", "Neutral-Poison",
    "Protect-Evil 10'", "Protect-Lightning", "Cure Critical", "Cause Critical", "Flame Strike",
    "Raise Dead", "Lay on Hands"
};
/* The spell records, 0x13 bytes each, 53 in one run: the mage spells 1 to 25, the cleric spells
   (gSpells[26] to gSpells[49]), then three records with no name whose ddFn is RunMelee0,
   DamageOneMember or DamageParty. EOV6.C:19 declares gClericSpells at ds:151F, gSpells[25], the
   base a cleric spell number (from 1) indexes (EOV6.C:182); the run is written as two arrays so
   that the name has its definition, the same bytes as one. E0EF3 reads the sound byte of each
   record as gSpells[i].sound (ds:1355 for record 0). */
struct Spell gSpells[25] = { /*@ ds:1344 */
    { "", 0, 0, 0, 0, 0, 0 },
    { "armor", 1, CastArmor, 0, EndArmor, 92, 0 },
    { "burning hands", 0, CastBurningHands, 0, 0, 87, 1 },
    { "detect magic", 8, CastDetectMagic, 0, (void (far *)())EndDetectMagic, 95, 0 },
    { "magic missile", 0, CastMagicMissile, (int (far *)())HitMagicMissile, 0, 85, 1 },
    { 0, 0, 0, 0, 0, 0, 0 },
    { "shield", 4, CastShield, 0, EndShield, 92, 0 },
    { "shocking grasp", 2, CastShockingGrasp, 0, DiscardEquippedItems, 88, 0 },
    { "invisibility", 1, CastInvisibility, 0, EndInvisibility, 94, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
    { "melf's acid arrow", 0, CastAcidArrow, (int (far *)())HitAcidArrow, 0, 96, 1 },
    { "stinking cloud", 0, CastStinkingCloudNoOp, 0, 0, 0, 1 },
    { "dispel magic", 1, CastDispelMagic, 0, 0, 97, 0 },
    { "fireball", 0, CastFireball, (int (far *)())HitFireball, 0, 98, 1 },
    { "flame arrow", 0, CastFlameArrow, (int (far *)())HitFlameArrow, 0, 99, 1 },
    { "haste", 8, CastHaste, 0, EndHaste, 100, 0 },
    { "hold person", 0, CastHoldPerson, (int (far *)())HitHoldPerson, 0, 101, 1 },
    { "invisibility 10' radius", 8, CastInvisibility10, 0, EndInvisibility, 94, 0 },
    { "lightning bolt", 0, CastLightningBolt, (int (far *)())HitLightningBolt, 0, 31, 1 },
    { "vampiric touch", 2, CastVampiricTouch, 0, EndVampiricTouch, 102, 0 },
    { "fear", 0, CastFear, 0, 0, 103, 1 },
    { "ice storm", 0, CastIceStorm, (int (far *)())HitIceStorm, 0, 89, 1 },
    { "stoneskin", 1, CastStoneskin, 0, 0, 93, 0 },
    { "cloudkill", 0, CastCloudkillNoOp, 0, 0, 0, 1 },
    { "cone of cold", 0, CastConeOfCold, 0, 0, 118, 1 }
};
struct Spell gClericSpells[28] = { /*@ ds:151F */
    { "hold monster", 0, CastHoldMonster, (int (far *)())HitHoldMonster, 0, 101, 1 },
    { "bless", 8, CastBless, 0, (void (far *)())EndBless, 91, 0 },
    { "cure light wounds", 1, CastCureLightWounds, 0, 0, 104, 0 },
    { "cause light wounds", 0, CastCauseLightWounds, 0, 0, 107, 1 },
    { "detect magic", 8, CastDetectMagic, 0, (void (far *)())EndDetectMagic, 95, 0 },
    { "protection from evil", 1, CastProtFromEvil, 0, EndProtFromEvil, 110, 0 },
    { "aid", 1, CastAid, 0, EndAid, 91, 0 },
    { "flame blade", 2, CastFlameBlade, 0, DiscardEquippedItems, 99, 0 },
    { "hold person", 0, CastHoldPerson, 0, 0, 101, 1 },
    { "slow poison", 1, CastSlowPoison, 0, EndSlowPoison, 111, 0 },
    { "create food", 8, CastCreateFood, 0, 0, 112, 0 },
    { "dispel magic", 1, CastDispelMagic, 0, 0, 97, 0 },
    { "magical vestment", 4, CastMagicalVestment, 0, EndMagicalVestment, 113, 0 },
    { "prayer", 8, CastPrayer, 0, (void (far *)())EndPrayer, 91, 0 },
    { "remove paralysis", 8, CastRemoveParalysis, 0, 0, 114, 0 },
    { "cure serious wounds", 1, CastCureSeriousWounds, 0, 0, 105, 0 },
    { "cause serious wounds", 0, CastCauseSeriousWounds, 0, 0, 108, 1 },
    { "neutralize poison", 1, CastNeutralizePoison, 0, 0, 115, 0 },
    { "protection from evil 10' radius", 8, CastProtFromEvil10, 0,
      (void (far *)())EndProtFromEvil10, 110, 0 },
    { "protection from lightning", 4, CastProtFromLightningNoOp, 0, 0, 116, 0 },
    { "cure critical wounds", 1, CastCureCriticalWounds, 0, 0, 106, 0 },
    { "cause critical wounds", 0, CastCauseCriticalWounds, 0, 0, 108, 1 },
    { "flame strike", 0, CastFlameStrike, (int (far *)())HitFlameStrike, 0, 98, 1 },
    { "raise dead", 1, CastRaiseDead, 0, 0, 117, 0 },
    { "lay on hands", 1, CastLayOnHands, 0, 0, 104, 0 },
    { "", 0, 0, (int (far *)())RunMelee0, 0, 117, 0 },
    { "", 0, 0, (int (far *)())DamageOneMember, 0, 117, 0 },
    { "", 0, 0, (int (far *)())DamageParty, 0, 117, 0 }
};
char far *g1733[6] = { /*@ ds:1733 */
    "CHARACTER GENERATION\r\rUSE THE ARROW KEYS TO HIGHLIGHT YOUR SELECTION",
    "OPTIONS\r\rRE-ROLL\rMODIFY\rKEEP\rEXIT", "Do you really wish to delete this character?\r",
    "PREFERENCES\r\rRETURN TO GAME\rLOAD GAME\rSAVE GAME\rSOUNDS ARE OFF\rGRAPHS ARE OFF\rEXIT TO DOS",
    "You have not created four characters yet!  Do you wish to continue anyway?",
    "LOAD GAME IN PROGRESS\rSTART A NEW PARTY\rEXIT TO DOS"
};
signed char gHitDieSlot0[15] = { /*@ ds:174B */
    10, 10, 10, 4, 8, 6, 10, 10, 10, 10, 4, 8, 10, 10, 8
};
signed char gHitDieSlot1[15] = { 0, 0, 0, 0, 0, 0, 8, 6, 4, 4, 6, 6, 8, 8, 4 }; /*@ ds:175A */
signed char gHitDieSlot2[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 4, 0, 0 }; /*@ ds:1769 */
signed char gConHitPointBonus[24] = { /*@ ds:1778 */
    -3, -2, -2, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 6, 6, 7, 7
};
signed char gDirCellOffset[62] = { /*@ ds:1790 */
    -32, 1, 32, -1, -72, 0, 0, 1, -72, 0, 0, 1, -72, 0, 0, 1, 0, 0, 0, 0, 56, 0, 56, 0, 112, 0, 112,
    0, -9, 0, 63, 1, -9, 0, 63, 1, 48, 0, 48, 0, 104, 0, 104, 0, 0, 0, 32, 0, 32, 0, 8, 0, 8, 0, 24,
    0, 32, 0, 16, 0, 16, 0
};
int gInvSlotX[27] = { /*@ ds:17CE */
    230, 278, 181, 199, 181, 199, 181, 199, 181, 199, 181, 199, 181, 199, 181, 199, 225, 224, 225,
    298, 287, 277, 300, 300, 300, 228, 240
};
unsigned char gInvSlotY[27] = { /*@ ds:1804 */
    0x74, 0x74, 0x28, 0x28, 0x3A, 0x3A, 0x4C, 0x4C, 0x5E, 0x5E, 0x70, 0x70, 0x82, 0x82, 0x94, 0x94,
    0x38, 0x4C, 0x60, 0x37, 0x4B, 0x89, 0x5E, 0x70, 0x82, 0x88, 0x88
};
signed char gIconPictureIndex[90] = { /*@ ds:181F */
    0, 0, 0, 4, 1, 2, 3, 4, 5, 6, 7, 7, 18, 6, 15, 18, 20, 23, 19, 17, 21, 13, 9, 9, 22, 31, 12, 35,
    12, 12, 12, 11, 10, 28, 28, 26, 27, 33, 29, 29, 34, 34, 34, 0, 8, 0, 37, 24, 22, 23, 22, 23, 22,
    23, 25, 35, 16, 30, 36, 24, 28, 32, 18, 33, 30, 35, 28, 0, 32, 17, 17, 13, 31, 9, 9, 21, 21, 15,
    5, 12, 30, 30, -1, -1, -1, -1, -1, 27, 27, 0
};
unsigned char gWallVmpSet[70] = { /*@ ds:1879 */
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};
signed char gWallDecorHead[70] = { /*@ ds:18BF */
    0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0
};
unsigned char gWallHandlerIndex[70] = { /*@ ds:1905 */
    0, 0, 0, 1, 1, 1, 1, 1, 6, 0, 0, 0, 0, 1, 1, 1, 1, 1, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};
signed char gWallTypeFlags[70] = { /*@ ds:194B */
    7, 0, 64, -88, -120, -120, -120, -97, -88, -120, -120, -120, -97, -86, -118, -118, -118, -97,
    -86, -118, -118, -118, -97, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
char gNearSideByDir[4] = { 2, 3, 0, 1 }; /*@ ds:1991 */
signed char gRightSideByDir[4] = { 1, 2, 3, 0 }; /*@ ds:1995 */
signed char gLeftSideByDir[4] = { 3, 0, 1, 2 }; /*@ ds:1999 */
signed char gViewCellOffsets[72] = { /*@ ds:199D */
    -99, -98, -97, -96, -95, -94, -93, -66, -65, -64, -63, -62, -33, -32, -31, -1, 0, 1, -93, -61,
    -29, 3, 35, 67, 99, -62, -30, 2, 34, 66, -31, 1, 33, -32, 0, 32, 99, 98, 97, 96, 95, 94, 93, 66,
    65, 64, 63, 62, 33, 32, 31, 1, 0, -1, 93, 61, 29, -3, -35, -67, -99, 62, 30, -2, -34, -66, 31,
    -1, -33, 32, 0, -32
};
int gViewSlotCenterX[18] = { /*@ ds:19E5 */
    -56, -8, 40, 88, 136, 184, 232, -72, 8, 88, 168, 248, -40, 88, 216, -88, 88, 264
};
signed char gViewSlotRow[18] = { /*@ ds:1A09 */
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3
};
signed char gViewToCellPos[16] = { 0, 1, 2, 3, 1, 3, 0, 2, 3, 2, 1, 0, 2, 0, 3, 1 }; /*@ ds:1A1B */
signed char gViewRelativePos[16] = { /*@ ds:1A2B */
    0, 1, 2, 3, 2, 0, 3, 1, 3, 2, 1, 0, 1, 3, 0, 2
};
struct MonsterType gMonsterTypes[22] = { /*@ ds:1A3B */
    { 7, 18, -1, 1, { { 1, 4, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 0, 3L, 15, 0, 2, 75, 2, -1, 0 },
    { 7, 17, 2, 1, { { 1, 4, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 2, 2L, 120, 1, 56, 57, 0, -1, 0 },
    { 5, 17, 1, 1, { { 1, 6, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 1, 12, 4L, 65, 0, 66, 67, 0, 2, 0 },
    { 8, 15, 2, 1, { { 1, 8, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 0, 4L, 65, 0, 73, 74, 0, 3, 0 },
    { 4, 16, 2, 2, { { 1, 4, 1 }, { 1, 4, 1 }, { 0, 0, 0 } }, 0, 66, 0L, 175, 2, 54, 55, 4, -1, 20 },
    { 5, 17, 3, 1, { { 1, 8, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 8, 3L, 65, 0, 47, 48, 0, -1, 0 },
    { 6, 19, 2, 1, { { 1, 8, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 18, 2L, 650, 2, 49, 38, 0, -1, 0 },
    { 3, 12, 7, 1, { { 1, 8, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 0, 3L, 730, 0, 45, 46, 0, -1, 0 },
    { 5, 7, 10, 1, { { 1, 8, 6 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 12, 4L, 1500, 0, 64, 65, 0, 7, 0 },
    { 4, 15, 6, 2, { { 1, 8, 0 }, { 1, 8, 0 }, { 0, 0, 0 } }, 0, 2, 3L, 975, 1, 39, 40, 0, -1, 0 },
    { 5, 13, 6, 3, { { 1, 8, 0 }, { 1, 4, 0 }, { 1, 4, 0 } }, 0, 40, 3L, 1400, 2, 37, 36, 0, -1, 0 },
    { 4, 13, 5, 2, { { 1, 4, 0 }, { 1, 4, 0 }, { 0, 0, 0 } }, 0, 0, 1L, 1250, 0, 62, 63, 0, -1, 0 },
    { 4, 15, 4, 1, { { 1, 8, 4 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 32, 1L, 650, 0, 43, 44, 0, -1, 60 },
    { -1, 7, 10, 1, { { 3, 8, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 2, 2L, 10000, 2, 68, 69, 0, -1, 100 },
    { -1, 7, 9, 1, { { 3, 4, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 66, 3L, 2000, 0, 31, -1, 8, -1, 0 },
    { 3, 13, 7, 1, { { 1, 6, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 64, 2L, 5000, 2, 41, 42, 2, -1, 60 },
    { 5, 13, 7, 1, { { 1, 6, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 64, 2L, 5000, 0, 52, 53, 2, -1, 0 },
    { 5, 11, 8, 1, { { 5, 20, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 66, 0L, 8000, 0, 58, 59, -1, -1, 90 },
    { 2, 15, 5, 1, { { 1, 1, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, -126, 2L, 270, 2, 60, 61, 0, -1, 0 },
    { -2, 13, 8, 1, { { 6, 4, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 2, 2L, 4000, 2, 72, 71, 0, -1, 50 },
    { 4, 13, 7, 1, { { 1, 10, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 2, 2L, 975, 1, 51, 50, 0, -1, 40 },
    { 0, 5, 35, 1, { { 1, 100, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }, 0, 66, 0L, 14000, 2, -1, -1, -1, -1, 100 }
};
int gSlotItemMask[35] = { /*@ ds:1C8D */
    8, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 32, 64, 16, -1, 128, 128,
    256, 256, 2816, 2827, 2827, 2827, 2827, 2827, 2827, 2827
};
char gHitFlashRemap[16] = { /*@ ds:1CD3 */
    0, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};
char gHitFlashRemapCga[4] = { 0, -1, -1, -1 }; /*@ ds:1CE3 */
signed char gSelectFrameColours[32] = { /*@ ds:1CE7 */
    15, -80, -78, -76, -74, -72, -70, -68, 12, -68, -70, -72, -74, -76, -78, -80, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
signed char gItemThrownShape[90] = { /*@ ds:1D07 */
    -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, 3, -1, 5, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
int gViewPosX[1] = { -111 }; /*@ ds:1D61 */
int gViewPosY[137] = { /*@ ds:1D63 */
    -63, -95, -63, -139, -59, -117, -59, -120, -61, -76, -63, -60, -63, -95, -59, -74, -59, -80, -61,
    -43, -63, -27, -63, -53, -59, -31, -59, -40, -61, -8, -63, 8, -63, -10, -59, 10, -59, 0, -61, 27,
    -63, 43, -63, 31, -59, 53, -59, 40, -61, 60, -63, 76, -63, 74, -59, 95, -59, 80, -61, 95, -63,
    111, -63, 117, -59, 139, -59, 120, -61, -118, -53, -92, -53, -152, -45, -120, -45, -118, -50,
    -66, -53, -40, -53, -84, -45, -51, -45, -59, -50, -13, -53, 13, -53, -16, -45, 16, -45, 0, -50,
    40, -53, 66, -53, 51, -45, 84, -45, 59, -50, 92, -53, 118, -53, 120, -45, 152, -45, 118, -50,
    -110, -35, -67, -35, -140, -22, -83, -22, -98, -30, -22, -35, 22, -35, -27, -22, 27, -22
};
int gDoorButtonXBias = 0; /*@ ds:1E75 */
unsigned char g1E77[82] = { /*@ ds:1E77 */
    0xE2, 0xFF, 0x43, 0x00, 0xDD, 0xFF, 0x6E, 0x00, 0xDD, 0xFF, 0x53, 0x00, 0xEA, 0xFF, 0x8C, 0x00,
    0xEA, 0xFF, 0x62, 0x00, 0xE2, 0xFF, 0x80, 0xFF, 0xFC, 0xFF, 0x80, 0x00, 0xFC, 0xFF, 0x80, 0xFF,
    0xBE, 0xFF, 0x80, 0x00, 0xBE, 0xFF, 0x80, 0x00, 0x00, 0x00, 0xDA, 0xFF, 0xFC, 0xFF, 0x26, 0x00,
    0xFC, 0xFF, 0xDA, 0xFF, 0xBE, 0xFF, 0x26, 0x00, 0xBE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF,
    0xFC, 0xFF, 0x80, 0x00, 0xFC, 0xFF, 0x80, 0xFF, 0xBE, 0xFF, 0x80, 0x00, 0xBE, 0xFF, 0x80, 0x00,
    0x00, 0x00
};
