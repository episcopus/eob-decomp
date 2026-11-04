/* EOB.H: the records and constants several units share (Phase 6, playbook 65).

   One declaration of each struct the units of EOB.EXE, INTRO.EXE and START1.EXE have in common, so
   a field has one name and one type everywhere. Only types and constants live here: each file
   declares the globals and functions it uses itself, each annotated with its address in the
   original, since INTRO.EXE's addresses differ from EOB.EXE's.

   Turbo C packs structs on bytes (no -a), so an offset is the sum of the sizes before it; a gap no
   unit reads is `padXX`. Pointers are written far: the game units are large model (-ml), and the
   records keep their size in START1.EXE's small model too. `make match` and `make rebuild` inline
   this file where a unit includes it (tools/match/cmatch.py, `expand`), so the compiler sees one
   file as before. Field names change with `make field`, which edits this header too. */

#ifndef EOB_H
#define EOB_H

/* The 14-byte item records of ITEM.DAT, gItems at ds:8DD0 (500 of them): each item in the game,
   linked into the list of the cell or character that holds it. */
struct ItemRecord {
    signed char nameIndex;        /* +0 (ds:8DD0): indexes the item-name table at segment 0x1F28 */
    signed char identifiedNameIndex;        /* +1 (ds:8DD1): the second name index */
    unsigned char flags;    /* +2 (ds:8DD2): bit 0x20 gates 0BD2:0AF2/0C8E, 0x40 is set by
                               0BD2:040E and tested by 0BD2:05B5; src/E07A9.C calls it flags */
    signed char icon;       /* +3 (ds:8DD3): the icon number */
    signed char type;        /* +4 (ds:8DD4): the item type */
    signed char cellPos;        /* +5 (ds:8DD5): 0BD2:01D0 stores its 4th argument in it and 02B9 matches it
                               against its `key`. No caller shows a sound (01D0's callers pass a
                               slot kind, E0ACD.C:561-568, a roll, E08E6.C:807, or an effect kind | 4,
                               0BD2:07DE): the item's position in its cell, 8 a wall niche (122E:06D0) */
    int key;                /* +06: the key; -2 when the record is free (0EF3:0418) */
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

/* The 16-byte text-window records at ds:009F (gRegions, 14 of them): the window's edges, then the
   text state SetTextWindow (1DE7:0009) saves and loads. */
struct Region {
    int left;                   /* +0: left edge in 8-pixel columns */
    int top;                    /* +2: top edge in lines */
    int width;                  /* +4: width in 8-pixel columns */
    int height;                 /* +6: height in lines */
    int textAttr1;              /* first text attribute (foreground), per-window state SetTextWindow saves and loads */
    int textAttr2;              /* second text attribute (fill colour), saved and loaded the same way */
    int textCol;                 /* +c: cursor column (gTextCol) */
    int textRow;                    /* +e: cursor row (gTextRow) */
};

/* A character record: the 243-byte records at ds:F580, six of them (gChars; saved in EOBDATA.SAV).
   Fields no unit reads are padding. */
struct CharRec {
    char index;                   /* +00: the character index ovl0:26C6 writes */
    unsigned char status;   /* +01: status bits: bit 0 alive, bit 1 poisoned, bit 2 paralyzed
                               (src/E0DE8.C:558 keeps only bit 0 at death; src/E1414.C:476-477
                               clears bit 1 at the end of poison and bit 2 at the end of paralysis) */
    char name[0x0B];                            /* +02: the character's name, NUL padded */
    signed char strength;      /* +0D: the strength score, the base gStrHitBonus/strength and gStrDamageBonus/strength of 0A72:00CF
                             and 0A72:0128; E122E.C:364 indexes gForceDoorRoll by it and
                             E0BD2.C:0D2D sets it to 0x16 for a strength item */
    signed char baseStrength;        /* +0E (src/E1414.C) */
    signed char strengthPct;   /* +0F: the exceptional strength percentile: 0A72:00CF and 0A72:0128
                             compare it with {1, 51, 76, 91, 100}, and 100 prints as "00"
                             (FormatStrengthPercent) */
    signed char baseStrengthPct;        /* +10 (src/E1414.C) */
    signed char intelligence;        /* +11: INTELLIGENCE (ds:1195[1]), the first of the five stat bytes
                               after strength (src/EOV0.C:442-444 formats intelligence, wisdom, dexterity, constitution, charisma) */
    signed char m12;        /* +12 */
    signed char wisdom;         /* +13: the score ovl1:097A turns into a bonus-spell row (gClericWisdomBonus) */
    signed char m14;        /* +14 */
    signed char dexterity;     /* +15: one of the five further stat bytes drawn after the strength
                             score (EOV0.C:442-444 formats intelligence, wisdom, dexterity, constitution, charisma); it indexes the
                             per-stat tables ds:3474 (0A72:0181) and ds:349E (0A72:019D) */
    signed char m16;        /* +16 */
    signed char constitution;        /* +17: an index into gConSaveBonus (0D1C:059B); also the second argument of
                               RollHitPoints (0A72:01AE), which LevelUpSlot passes there */
    signed char m18;        /* +18 */
    signed char charisma;   /* +19: CHARISMA (ds:1195[5]) */
    signed char m1A;        /* +1A */
    signed char hp;                             /* +1B: 122E:0325 skips a character at or below 0 */
    signed char maxHp;         /* +1C: maximum hit points; HealCharacter (0DE8:0ADB) clamps
                                  hp at it */
    signed char armorClass; /* +1D: the armour-class byte, drawn on the ARMOR CLASS row
                               (src/E08E6.C:60 and src/E0EF3.C:25 name it the armour-class byte) */
    signed char m1E;        /* +1E: a per-slot bit toggled by UseItem's `m1E ^= 1 << slot`
                               (0DE8:0763); DrawHandSlot tests bit `slot` (src/E1414.C) */
    signed char race;                           /* +1F: the race index, halved and matched by 122E:147F (src/EOV0.C) */
    signed char classIndex;     /* +20: an index into the per-class tables (gClassBaseBits, 0A72:007B) */
    signed char alignment;        /* +21: index into the alignment-name table ds:1240 (src/E1547.C) */
    signed char portraitIndex;                /* +22: the position ovl0:0A5E stores (Purpose unknown) */
    unsigned char food;     /* +23: the hunger/food value, clamped to 100 by EatFood (0ACD:07E0)
                               and decremented each tick for a living member by DecayMemberState
                               (1414:0C82); unsigned (the code tests < 0x21, < 0x14) */
    signed char levels[3];     /* +24: the three class-slot levels: LevelUpSlot increments levels[slot]
                             when a slot gains a level (E0DE8.C:650), E1547.C:570 reads it as
                             `slotValue` and E1547.C:573-574 prints it as the slot's level-like byte
                             (E1547.C:563-564), and 0A72:0522 reads element [0] */
    unsigned long experience[3];         /* +27: the experience of each class slot, which ovl0:1BF2 sets */
    void far *portrait;     /* +33: a far pointer to the character's picture buffer, preserved by
                                   the record swap at 070E (the movmem destination there) */
    signed char mageSpells[30];      /* +37: the mage's spell slots: a spell number, negative while it is
                                   still to be memorised (src/EOV6.C) */
    signed char clericSpells[30]; /* +55: spell slots, positive: a spell ready to cast; negative:
                                  spent or not yet regained (CampLoop turns it positive as
                                  "gained") */
    long spellBook;             /* +73: the mage's spell book, bit i set when spell i + 1 is known */
    int items[27];          /* +77: 27 carried-item slots; slot 16 (+97) is also a list head */
    unsigned long timers[10]; /* +AD: 10 32-bit effect times (07CA, 0B2E, 0C2D, 05ED) */
    signed char mD5[10];      /* +D5: 10 effect type bytes, one per timers slot */
    signed char mDF;        /* +0DF: a descending penalty counter; when it runs out the armour class
                               is recomputed (src/E0DE8.C:563-566, src/E0EF3.C:1179 "penalty byte") */
    signed char mE0;        /* +0E0: a countdown (src/E0EF3.C:39) */
    char padE1;
    signed char mE2;        /* +E2: the damage-reduction value (DamageCharacter :573-576) */
    long effectFlags;       /* +0E3: status/effect flags: a long, so the bit tests compile to the
                               `mov ax,..; and ax,K; or ax,0` form (src/E1414.C, 0D1C:0B17) */
    signed char lastDamage;      /* +E7: the last damage amount (DamageCharacter :579) */
    signed char lastAction[0xF3 - 0xE8]; /* +E8: the last action result per carried slot (UseItem :523) */
};

/* The 28-byte monster records, 30 of them at ds:8A82 (gMonsters). */
struct Monster {
    char typeIndex;                   /* +00: an index into the ds:1A3B table (src/E08E6.C) */
    signed char updateGroup;       /* +1: the update group UpdateMonsters(kind) matches: InitMonster
                               stores the level file's group * 2 plus the slot's parity, and
                               timed effects 2 to 5 (ds:5BC8) run the groups (ovl3:0A7C) */
    int cell;                                     /* +2 (ds:8A84): the monster's map cell */
    char cellPos;             /* +4: where in its cell it stands: 0..3 a quarter, 4 the centre (058A, 0EC3) */
    char orient;            /* +5: the facing 0..3 N E S W (times 2 is a direction), written by 0D1C:09CB */
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
    char m1A;               /* +1A: Purpose unknown: no unit reads it */
    char m1B;               /* +1B: Purpose unknown: no unit reads it */
};

/* The 27-byte monster-type records at ds:1A3B (gMonsterTypes), indexed by Monster.typeIndex. */
struct MonsterType {
    signed char m00;        /* +0: the target's class byte for GetHitTarget (0A72:0522) */
    char m01;               /* +1 (ds:1A3C) */
    char m02;               /* +2 (ds:1A3D) */
    char m03;               /* +3 (ds:1A3E): number of attack routines */
    char m04[3][3];         /* +4 (ds:1A3F): {count, sides, bonus} per routine */
    char pad0D;
    char m0E;               /* +0xE: flag bits: 8 (08E6), 0x40 (08E6), 1 (0DE8), 2 (0D1C) */
    long m0F;               /* +0xF (ds:1A4A), tested by 0D1C:0A43; a `long` because the original
                               emits the and/or bit-test form at 0D1C:0B17 */
    int m13;                /* +0x13 (ds:1A4E) */
    char m15;               /* +0x15: tested nonzero by 08E6 */
    char m16;               /* +0x16 (ds:1A51) */
    char m17;               /* +0x17 (ds:1A52) */
    char m18;               /* +0x18 (ds:1A53) */
    signed char m19;        /* +0x19 (ds:1A54), read by 0D1C:09CB */
    signed char m1A;        /* +0x1A (ds:1A55), read by 0D1C:0BAB */
};

/* The 18-byte records of the thrown items and spell missiles in flight, ten at ds:DD2C
   (gProjectiles); 1414:0172 calls `fn` for a live one. */
struct Projectile {
    char active;              /* +00: the record state: 0 is cleared, 1 and 2 are the live states */
    char pad01;
    int m02;                                    /* +02 */
    int cell;                  /* +04: the map cell the projectile is in (003E moves its item between cell lists) */
    char m06;                 /* +06: its low two bits index ds:7246; ds:7262 is indexed by the whole byte (altShapes * 4 + m06) */
    signed char dir;          /* +07: the page the record is seen from */
    int item;                /* +8: an item slot, indexed into gItems (01FF, 02D4) */
    char m0A;               /* +0A: an item type, indexed into gItemTypes (01FF, 02D4) */
    char m0B;               /* +0B: the attacker (TryRecordAttack :318) */
    char m0C;                /* +0C: flags, bit 0x20 tested by 1414:0285 */
    unsigned char m0D;        /* +0D: a counter decremented by 003E */
    int (far *fn)(struct Projectile far *rec); /* +0E: called by 1414:0172 when active is 2 */
};

/* The 16-byte item-type records of ITEMTYPE.DAT at ds:A9F0 (gItemTypes), indexed by
   ItemRecord.type. src/E0D1C.C and src/EOV5.C view the same table five bytes in (ItemInfo). */
union ItemMask {               /* a word the game reads whole or by its low byte */
    unsigned word;              /* the 16-bit value (0BD2:0AF2 tests the class mask) */
    unsigned char byte;         /* its low byte (0BD2:0C8E) */
};

struct ItemType {
    union ItemMask slotMask;    /* +00: the classes and slots the item fits; bit 4 tested by 0D1C:0BEB;
                                   stored whole by 0EF3:035A from its first argument */
    int m02;                    /* +02: a word, set from 0EF3:035A's second (char) argument */
    unsigned char m04;          /* +04: 0EF3:035A stores the type in it; 0xE2 marks a spell item
                                   (0EF3:0418); 0DE8 adds it to the armour class as a signed byte */
    signed char m05;            /* +05: the class bits, ANDed with gClassBaseBits (0BD2:11FD, 0DE8) */
    signed char m06;            /* +06: cleared by 0EF3:035A; src/E0D1C.C's ItemInfo +01 */
    signed char m07;            /* +07 */
    signed char m08;            /* +08 */
    signed char m09;            /* +09 */
    signed char m0A;            /* +0A */
    signed char m0B;            /* +0B */
    signed char m0C;            /* +0C */
    char pad0D;
    union ItemMask m0E;         /* +0E: bit 0x80 and the low 7 bits, the use kind (0BD2:000E, 05B5;
                                   0DE8:01D7 and UseItem's dispatch) */
};

union NodeData {               /* Node +34, a variant: what the node keeps depends on its owner */
    struct {
        int tile34;             /* +34: the tile id DrawNode draws in the default kind (src/E16A5.C) */
        int tile36;             /* +36: the tile id DrawNode draws in the switched modes */
    } tile;
    struct ButtonDef far *def;  /* +34: overlay 1's button nodes, their definition (ovl1, src/EOV1.C) */
};

/* A screen-list node: buttons, list rows and tiles the dispatcher (src/E16A5.C) walks and DrawNode
   draws. The tables point at records on a 60-byte grid in DGROUP; the units name the first 0x3A
   bytes. */
struct Node {
    struct Node far *next;                       /* +00: next node, or a null pointer at the tail */
    unsigned key;                                /* +04: sort key, compared unsigned */
    unsigned key06;             /* +06: the first key word the dispatcher matches */
    unsigned key08;             /* +08: the second key word the dispatcher matches */
    unsigned char kind;         /* +0A: the drawing kind 0..5 DrawNode switches on */
    unsigned char modeB;        /* +0B: the "on" drawing mode DrawNode switches on */
    unsigned char modeC;        /* +0C: the second drawing mode DrawNode switches on */
    char pad0D;
    unsigned flags;                              /* +0E: the inactive bit 0x0008 and the button-mode
                                                    selectors (see the flags comment below) */
    void far *src10;            /* +10: the default art pointer DrawNode draws */
    void far *src14;            /* +14: the "on" art/callback pointer DrawNode draws */
    void far *src18;            /* +18: the second art/callback pointer DrawNode draws */
    int region;                 /* +1C: index into the region table ds:009F */
    int x;                      /* +1E: box left, in the region's pixel space */
    int y;                      /* +20: box top, in lines */
    unsigned width;             /* +22: box width */
    unsigned height;            /* +24: box height */
    unsigned char color26;      /* +26: text foreground, the default pair */
    unsigned char color27;      /* +27: text background/extra, the default pair */
    unsigned char color28;      /* +28: foreground of the "on" pair */
    unsigned char color29;      /* +29: background/extra of the "on" pair */
    unsigned char color2A;      /* +2A: foreground of the third pair */
    unsigned char color2B;      /* +2B: background/extra of the third pair */
    unsigned char color2C;      /* +2C: foreground of the fourth pair */
    unsigned char color2D;      /* +2D: background/extra of the fourth pair */
    unsigned state;                              /* +2E: the on bit, the previous-on bit, others */
    int (far *callback30)(struct Node far *node); /* +30: optional callback, fired when asked; the
                                                    dispatcher is the only caller that uses its
                                                    result (E16A5.C:475-476) */
    union NodeData data;        /* +34: two tile ids, or a button definition */
    unsigned defFlags;          /* +38: a copy of def->flags */
    char pad3A[2];
};

/* The 3-byte size record a gDoorButtons or gDoorShapes entry points at. The same layout as
   RectKind; whether the two are one record kind is not established. */
struct SlotSize {
    char pad00;
    signed char height;                         /* +1: base height in pixels */
    signed char width;                          /* +2: base width in eighths */
};

/* The 6-byte slot-position records at ds:A99C: a far size record, then x and y (src/E122E.C). */
struct SlotPos {
    struct SlotSize far *rec;                   /* +0: the far size record */
    unsigned char x;                            /* +4 */
    unsigned char y;                            /* +5 */
};

/* The 3-byte size record a gDecorShapes entry (ds:EFD4) points at; the same layout as SlotSize. */
struct RectKind {
    char pad00;
    signed char height;                         /* +1: base height in pixels */
    signed char width;                          /* +2: base width in eighths */
};

/* The request RunOverlayRequest runs (ds:8121 in EOB.EXE): the overlay file's name and a table
   the overlay entry fills. */
struct OverlayRequest {
    char far *name;         /* +0: the overlay file name (word +0 the offset, word +2 the segment) */
    void far *table;        /* +4: a far pointer (offset then segment) to a table the overlay entry's
                               pointer walks (src/E1C00.ASM:121-127); Purpose unknown: nothing in
                               src/ says who writes ds:8125/ds:8127 */
};

/* gItemTypes seen five bytes in: 16-byte records at ds:A9F5, so +00 is ItemType +05, +01 is
   ItemType +06 and +09 is ItemType +0E (read by 0D1C:07DB, 08B1 and 0BEB). */
struct ItemInfo {
    signed char m00;        /* +0 (ds:A9F5), bit 2 tested by 0D1C:0BEB */
    signed char m01;        /* +1 (ds:A9F6), compared by 0D1C:07DB */
    char pad02[7];
    int m09;                /* +9 (ds:A9FE), tested by 0D1C:07DB */
    char pad0B[5];
};

/* The 0x13-byte spell records at ds:1344 (gSpells): the name, the flags, and the callbacks that
   cast the spell, hit with it and end it (Cast<Spell>, Hit<Spell>, End<Spell>). */
struct Spell {
    char *name;                /* +00: a far pointer to the spell's name */
    signed char flags;                      /* +04: bits 0..3 read by 0EF3:0D78 (bits 0..2, bit 3)
                                               and 0EF3:2676 (bit 0, bit 1, bits 1|2, bit 2, bit 3) */
    void (far *apply)(void);                /* +05: run through gCurrentSpellApply when the spell is cast */
    int (far *ddFn)(struct Projectile far *rec); /* +09: copied into Projectile.fn by 0EF3:097B */
    void (far *remove)(struct CharRec far *rec); /* +0D: run when the spell expires */
    signed char sound;                      /* +11: the sound id (ds:1355) */
    signed char m12;                        /* +12: bit 0 gates 0EF3:2676's first flag clear */
};

/* The 16 timed-effect entries at ds:5BA8 (gEffects), 16 bytes each; the count word at ds:5CA8 is
   0x0010. */
struct Effect {
    void (far *fn)(int);      /* +00: the far handler, called with `arg` as its argument */
    unsigned long period;     /* +04: the interval added to the current tick on re-arm */
    unsigned long expiry;     /* +08: the time the effect next runs (gGameTicks) */
    int arg;                        /* +0C: the handler's argument */
    char sub;                 /* +0E: the third argument ArmEffectWith stores (1, 0xffff or idx,
                                 by caller; src/E122E.C:284, :307, :332) */
    char active;                    /* +0F: nonzero while the effect is armed */
};

/* The 16-byte option-list records at ds:017F (gPanels), right after the region table. */
struct Panel {
    int x;                      /* +0: column offset from ds:FE64; eight-pixel cells in both drawing
                                   functions (<< 3 at :201 and :288); the hit rectangle uses the font width (:295) */
    int y;                      /* +2: row offset from ds:FE66 */
    int width;                  /* +4: width in characters */
    int height;                 /* +6: the number of rows the list draws */
    int sel;                    /* +8: the selected row, counted among the selectable entries */
    int colorA;                 /* +a: normal text colour */
    int colorB;                 /* +c: colour of the selected entry */
    char pad0E[2];
};

/* A four-byte sprite-cell definition: x, y, width and height. */
struct Cell4 {
    unsigned char x;
    unsigned char y;
    unsigned char w;
    unsigned char h;
};

/* Constants several units use. */
#define MEMALLOC_ALIGNED 0x20
#define PAGE_64000 0xFA00L    /* a 64000-byte screen/buffer page (320 x 200) */
#define PAGE_65000 0xFDE8L    /* a 65000-byte page (the largest, choice 3) */
#define PAGE_64512 0xFC00L    /* a 64512-byte page */
#define NODE_INACTIVE   0x0008
#define NODE_FLAG_ON    0x0001  /* picks BUTTON_MODE_NODE_ON over BUTTON_MODE_NODE_OFF */
#define NODE_FLAG_ON_5E1C  0x0400 /* with NODE_FLAG_ON set: gLeftButtonMode gets 0x83 unless it is already 3 */
#define NODE_FLAG_ON_5E1E  0x4000 /* with NODE_FLAG_ON set: gRightButtonMode gets 0x83 unless it is already 3 */
#define NODE_FLAG_OFF_5E1C 0x0100 /* with NODE_FLAG_ON clear: gLeftButtonMode gets 0x81 unless it is already 1 */
#define NODE_FLAG_OFF_5E1E 0x1000 /* with NODE_FLAG_ON clear: gRightButtonMode gets 0x81 unless it is already 1 */
#define NODE_STATE_ON   0x0001
#define NODE_STATE_2    0x0002  /* set by DispatchNodeEvent (E16A5.C:316), cleared by 01E6, read
                                   with the flags 0x20 test (E16A5.C:477-478) */
#define NODE_STATE_4    0x0004
#define NODE_STATE_PREV 0x0008
#define NODE_STATE_10   0x0010
#define SOUND_MAP_SIZE 0x78L
#define FORMAT_STORED      0L  /* stored: Unpack0 copies the data */
#define FORMAT_LZW_12      1L  /* 12-bit LZW (Unpack1, 3C00h work buffer) */
#define FORMAT_LZW_14      2L  /* 14-bit LZW (Unpack2, FC00h work buffer) */
#define FORMAT_LITERAL_FILL 3L /* [H] a literal and fill stream (Unpack3) */
#define FORMAT_LCW         4L  /* LCW (Unpack4) */
#define FORMAT1_BUFFER_SIZE 0x3C00
#define FORMAT2_BUFFER_SIZE 0xFC00
#define KEY_ESC        0x1b     /* the key the error dialog treats as "abort" */

#endif
