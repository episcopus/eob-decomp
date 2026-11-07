/* Module at EOB.EXE 0CF7:0007 (591 bytes, the whole code segment): links static records into
   singly linked lists. Each BuildList* seeds a list with the first record of a table and links the
   rest with ListAdd. Every `gList = ListAdd(gList, x)` leaves gList unchanged, because ListAdd
   returns its first argument (see its note below), so after each BuildList gList is still the
   table's first record, the one ListInit set. Still unknown: the purpose of gFlag (and so of the a
   and b table sets), and what BuildList1 to BuildList5 number. */

#include "EOB.H"

extern char gFlag; /*@ ds:F3B9 */ /* selects the "a" table set when nonzero (the longer lists for
                                     tables 1 and 2, the shorter for table 3). Three writers: 1 at
                                     0EF3:2281 in the function at 0EF3:21E0, just before it calls
                                     BuildList1 at 0EF3:2286; 0 at 0EF3:2544 in the function at
                                     0EF3:2541 that calls BuildList1 to BuildList3; and 0 at
                                     0ACD:0FA9 in EnterMapScreen (0ACD:0F72), where two more
                                     stores (ds:A99A and ds:DCAA) come before its BuildList1 call
                                     at 0ACD:0FB8. Readers: BuildList1 to BuildList3 test it
                                     here; at 0EF3:2257 the function at
                                     0EF3:21E0 tests it first and, when it is zero, calls through the
                                     far pointer at ds:80AE (0EF3:2269..227A) before the 1 store, so
                                     that path writes 1 too. Purpose unknown. */
extern struct Node far *gList; /*@ ds:FB48 */ /* head of the screen node list walked by the key
                                                 dispatcher DispatchKeyWait (E16A5.C:156-160,
                                                 16A5:005A); 0ACD:0E6C and overlays 0 and 1 pass it
                                                 at ds:FB48 too. See the ListAdd note below. */
/* Table base addresses and record counts. The first entry of tables 1a, 1b, 2a, 2b, 3a and 3b is
   the same record, ds:4836, so the a and b sets differ only after their first node; tables 4 and 5
   start with the records at ds:3E9A and ds:4B7E. One count constant per table ties each loop to
   its table. The 1a/1b/2a/2b/3a/3b suffixes are placeholders that follow BuildList1 to BuildList3,
   not evidence of meaning (name them by address if the numbering is dropped). */
#define TABLE1A_COUNT 52
#define TABLE1B_COUNT 38
#define TABLE2A_COUNT 59
#define TABLE2B_COUNT 47
#define TABLE3A_COUNT 19
#define TABLE3B_COUNT 32
#define TABLE4_COUNT  7
#define TABLE5_COUNT  7
extern struct Node far *gTable1a[]; /*@ ds:5102 */ /* 52 records, TABLE1A_COUNT */
extern struct Node far *gTable1b[]; /*@ ds:506A */ /* 38 records, TABLE1B_COUNT */
extern struct Node far *gTable2a[]; /*@ ds:52DA */ /* 59 records, TABLE2A_COUNT */
extern struct Node far *gTable2b[]; /*@ ds:51D2 */ /* 47 records, TABLE2B_COUNT */
extern struct Node far *gTable3a[]; /*@ ds:528E */ /* 19 records, TABLE3A_COUNT */
extern struct Node far *gTable3b[]; /*@ ds:53C6 */ /* 32 records, TABLE3B_COUNT */
extern struct Node far *gTable4[]; /*@ ds:5446 */ /* 7 records, TABLE4_COUNT */
extern struct Node far *gTable5[]; /*@ ds:5462 */ /* 7 records, TABLE5_COUNT */

/* Walks head to its tail (the first node whose link is null), links item after that tail, clears
   item's own link, and returns head. The return is always its first argument, so a caller that
   assigns the result back to its head keeps the same head. */
extern struct Node far *ListAdd(struct Node far *head, struct Node far *item); /*@ 1658:001E */

/*@data */
/* the data of e0cf7, ds:3A62 to ds:547E (6684 bytes, 354 relocations): drafted by make data-skeleton from the original's bytes; each item at its annotation, in address order. */
/* what the pointers name that the file does not declare before them: */
extern int RedrawPartyPanel(struct ActRec far *rec); /*@ 0EF3:2541 */
extern int RestorePortraitPanel(void far *action); /*@ 0ACD:01E3 */
extern void far CancelMemberSwap(void); /*@ 0ACD:0393 */
extern void far CastSpellOnTarget(void); /*@ 0EF3:08CF */
extern void far ClickFloorItem(void); /*@ 0ACD:0644 */
extern void far ClickHandSlot(void); /*@ 0ACD:0415 */
extern void far ClickInventorySlot(void); /*@ 0ACD:01C0 */
extern void far ClickWallAhead(void); /*@ 122E:0933 */
extern void far FeedInventoryMember(void); /*@ 0ACD:08D1 */
extern void far HandleSpellList(void); /*@ 0EF3:2929 */
extern void far MoveBackward(void); /*@ 0ACD:0529 */
extern void far MoveForward(void); /*@ 0ACD:04F8 */
extern void far MoveLeft(void); /*@ 0ACD:05BA */
extern void far MoveRight(void); /*@ 0ACD:05FF */
extern void far OpenInventory(void); /*@ 0ACD:0034 */
extern void far PickOrSwapMember(void); /*@ 0ACD:0264 */
extern void far RunCampMenu(void); /*@ ovl1:1ACD */
extern void far SelectClickedMember(void); /*@ 0ACD:078C */
extern void far SelectNextMember(void); /*@ 0ACD:00EB */
extern void far SelectPrevMember(void); /*@ 0ACD:008C */
extern void far SelectSpellLevel(void); /*@ 0EF3:25BD */
extern void far ThrowMouseItem(void); /*@ 0ACD:0494 */
extern void far ToggleCharacterInfo(void); /*@ 0ACD:018E */
extern void far TurnLeft(void); /*@ 0ACD:056C */
extern void far TurnRight(void); /*@ 0ACD:0593 */
struct Node gNodeTemplate = { /*@ ds:3A62 */
    0, 1, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 0, 0, 0,
    0, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, 0, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g3A9E = { /*@ ds:3A9E */
    0, 1, 0x0070, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 2, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectClickedMember, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g3ADA = { /*@ ds:3ADA */
    0, 2, 0x0071, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 2, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectClickedMember, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g3B16 = { /*@ ds:3B16 */
    0, 3, 0x0072, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 54, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectClickedMember, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g3B52 = { /*@ ds:3B52 */
    0, 4, 0x0073, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 54, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectClickedMember, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g3B8E = { /*@ ds:3B8E */
    0, 5, 0x0030, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 289, 177, 0x001F,
    0x0015, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, RunCampMenu, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g3BCA = { /*@ ds:3BCA */
    0, 6, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 0, 102, 0x0058,
    0x0012, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickFloorItem, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g3C06 = { /*@ ds:3C06 */
    0, 7, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 89, 102, 0x0058,
    0x0012, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickFloorItem, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g3C42 = { /*@ ds:3C42 */
    0, 8, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 0, 72, 0x0058,
    0x001D, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickFloorItem, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g3C7E = { /*@ ds:3C7E */
    0, 9, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 89, 72, 0x0058,
    0x001D, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickFloorItem, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g3CBA = { /*@ ds:3CBA */
    0, 0x000A, 0x0018, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 10, 0x0021,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, OpenInventory, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g3CF6 = { /*@ ds:3CF6 */
    0, 0x000B, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 10, 0x0021,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, OpenInventory, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g3D32 = { /*@ ds:3D32 */
    0, 0x000C, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 62, 0x0021,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, OpenInventory, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g3D6E = { /*@ ds:3D6E */
    0, 0x000D, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 62, 0x0021,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, OpenInventory, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g3DAA = { /*@ ds:3DAA */
    0, 0x000E, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 216, 10, 0x001F,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickHandSlot, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g3DE6 = { /*@ ds:3DE6 */
    0, 0x000F, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 288, 10, 0x001F,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickHandSlot, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g3E22 = { /*@ ds:3E22 */
    0, 0x0010, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 216, 62, 0x001F,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickHandSlot, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g3E5E = { /*@ ds:3E5E */
    0, 0x0011, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 288, 62, 0x001F,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickHandSlot, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g3E9A = { /*@ ds:3E9A */
    0, 0x0012, 0x0170, 0,
    0, 0, 0, 0,
    0x1000, 0, 0, 0,
    0, 184, 2, 0x003F,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, PickOrSwapMember, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g3ED6 = { /*@ ds:3ED6 */
    0, 0x0013, 0x0171, 0,
    0, 0, 0, 0,
    0x1000, 0, 0, 0,
    0, 256, 2, 0x003F,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, PickOrSwapMember, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g3F12 = { /*@ ds:3F12 */
    0, 0x0014, 0x0172, 0,
    0, 0, 0, 0,
    0x1000, 0, 0, 0,
    0, 184, 54, 0x003F,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, PickOrSwapMember, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g3F4E = { /*@ ds:3F4E */
    0, 0x0015, 0x0173, 0,
    0, 0, 0, 0,
    0x1000, 0, 0, 0,
    0, 256, 54, 0x003F,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, PickOrSwapMember, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g3F8A = { /*@ ds:3F8A */
    0, 0x0027, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 230, 116, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g3FC6 = { /*@ ds:3FC6 */
    0, 0x002B, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 278, 116, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g4002 = { /*@ ds:4002 */
    0, 0x0016, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 181, 40, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g403E = { /*@ ds:403E */
    0, 0x0017, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 199, 40, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g407A = { /*@ ds:407A */
    0, 0x0018, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 181, 58, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    4, { 0, 0 }
};
struct Node g40B6 = { /*@ ds:40B6 */
    0, 0x0019, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 199, 58, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    5, { 0, 0 }
};
struct Node g40F2 = { /*@ ds:40F2 */
    0, 0x001A, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 181, 76, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    6, { 0, 0 }
};
struct Node g412E = { /*@ ds:412E */
    0, 0x001B, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 199, 76, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    7, { 0, 0 }
};
struct Node g416A = { /*@ ds:416A */
    0, 0x001C, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 181, 94, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    8, { 0, 0 }
};
struct Node g41A6 = { /*@ ds:41A6 */
    0, 0x001D, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 199, 94, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    9, { 0, 0 }
};
struct Node g41E2 = { /*@ ds:41E2 */
    0, 0x001E, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 181, 112, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x000A, { 0, 0 }
};
struct Node g421E = { /*@ ds:421E */
    0, 0x001F, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 199, 112, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x000B, { 0, 0 }
};
struct Node g425A = { /*@ ds:425A */
    0, 0x0020, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 181, 130, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x000C, { 0, 0 }
};
struct Node g4296 = { /*@ ds:4296 */
    0, 0x0021, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 199, 130, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x000D, { 0, 0 }
};
struct Node g42D2 = { /*@ ds:42D2 */
    0, 0x0022, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 181, 148, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x000E, { 0, 0 }
};
struct Node g430E = { /*@ ds:430E */
    0, 0x0023, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 199, 148, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x000F, { 0, 0 }
};
struct Node g434A = { /*@ ds:434A */
    0, 0x0024, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 225, 55, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0010, { 0, 0 }
};
struct Node g4386 = { /*@ ds:4386 */
    0, 0x0025, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 224, 76, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0011, { 0, 0 }
};
struct Node g43C2 = { /*@ ds:43C2 */
    0, 0x0026, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 225, 96, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0012, { 0, 0 }
};
struct Node g43FE = { /*@ ds:43FE */
    0, 0x0029, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 298, 55, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0013, { 0, 0 }
};
struct Node g443A = { /*@ ds:443A */
    0, 0x002A, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 287, 75, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0014, { 0, 0 }
};
struct Node g4476 = { /*@ ds:4476 */
    0, 0x0028, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 277, 137, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0015, { 0, 0 }
};
struct Node g44B2 = { /*@ ds:44B2 */
    0, 0x002C, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 300, 94, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0016, { 0, 0 }
};
struct Node g44EE = { /*@ ds:44EE */
    0, 0x002D, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 300, 112, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0017, { 0, 0 }
};
struct Node g452A = { /*@ ds:452A */
    0, 0x002E, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 300, 130, 0x0010,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0018, { 0, 0 }
};
struct Node g4566 = { /*@ ds:4566 */
    0, 0x0030, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 236, 37, 0x001F,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, FeedInventoryMember, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g45A2 = { /*@ ds:45A2 */
    0, 0x0031, 0x001A, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 291, 149, 0x0019,
    0x0011, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ToggleCharacterInfo, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g45DE = { /*@ ds:45DE */
    0, 0x0032, 0x006E, 0x0018,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 181, 3, 0x0020,
    0x0020, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, RestorePortraitPanel, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g461A = { /*@ ds:461A */
    0, 0x0033, 0x0060, 0x0160,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 24, 128, 0x0015,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, MoveForward, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g4656 = { /*@ ds:4656 */
    0, 0x0034, 0x0062, 0x0061,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 24, 144, 0x0015,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, MoveBackward, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g4692 = { /*@ ds:4692 */
    0, 0x0035, 0x005C, 0x015C,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 3, 144, 0x0015,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, MoveLeft, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g46CE = { /*@ ds:46CE */
    0, 0x0036, 0x0066, 0x0166,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 45, 144, 0x0015,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, MoveRight, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g470A = { /*@ ds:470A */
    0, 0x0037, 0x005B, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 3, 128, 0x0015,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, TurnLeft, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g4746 = { /*@ ds:4746 */
    0, 0x0038, 0x0065, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 45, 128, 0x0015,
    0x0010, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, TurnRight, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g4782 = { /*@ ds:4782 */
    0, 0x0039, 0x006E, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 0, 0x0088,
    0x0078, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, CancelMemberSwap, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g47BE = { /*@ ds:47BE */
    0, 0x003A, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 0, 8, 0x0058,
    0x0030, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ThrowMouseItem, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g47FA = { /*@ ds:47FA */
    0, 0x003B, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 88, 8, 0x0058,
    0x0030, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ThrowMouseItem, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g4836 = { /*@ ds:4836 */
    0, 0x003C, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 24, 8, 0x0080,
    0x0060, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickWallAhead, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g4872 = { /*@ ds:4872 */
    0, 0x003D, 0x0070, 0x0071,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 274, 35, 0x0014,
    0x000F, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectPrevMember, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g48AE = { /*@ ds:48AE */
    0, 0x003E, 0x0072, 0x0073,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 297, 35, 0x0014,
    0x000F, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectNextMember, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g48EA = { /*@ ds:48EA */
    0, 0x003F, 2, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 71, 122, 0x0014,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectSpellLevel, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g4926 = { /*@ ds:4926 */
    0, 0x0040, 3, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 92, 122, 0x0014,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectSpellLevel, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g4962 = { /*@ ds:4962 */
    0, 0x0041, 4, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 113, 122, 0x0014,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectSpellLevel, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g499E = { /*@ ds:499E */
    0, 0x0042, 5, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 134, 122, 0x0014,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectSpellLevel, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g49DA = { /*@ ds:49DA */
    0, 0x0043, 6, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 155, 122, 0x0014,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectSpellLevel, { { 0, 0 } },
    4, { 0, 0 }
};
struct Node g4A16 = { /*@ ds:4A16 */
    0, 0x0044, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 75, 132, 0x0061,
    6, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, HandleSpellList, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g4A52 = { /*@ ds:4A52 */
    0, 0x0045, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 75, 138, 0x0061,
    6, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, HandleSpellList, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g4A8E = { /*@ ds:4A8E */
    0, 0x0046, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 75, 144, 0x0061,
    6, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, HandleSpellList, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g4ACA = { /*@ ds:4ACA */
    0, 0x0047, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 75, 150, 0x0061,
    6, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, HandleSpellList, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g4B06 = { /*@ ds:4B06 */
    0, 0x0048, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 75, 156, 0x0061,
    6, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, HandleSpellList, { { 0, 0 } },
    4, { 0, 0 }
};
struct Node g4B42 = { /*@ ds:4B42 */
    0, 0x0049, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 75, 162, 0x0061,
    6, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, HandleSpellList, { { 0, 0 } },
    5, { 0, 0 }
};
struct Node g4B7E = { /*@ ds:4B7E */
    0, 0x004A, 0x0070, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 2, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, CastSpellOnTarget, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g4BBA = { /*@ ds:4BBA */
    0, 0x004B, 0x0071, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 2, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, CastSpellOnTarget, { { 0, 0 } },
    1, { 0, 0 }
};
struct Node g4BF6 = { /*@ ds:4BF6 */
    0, 0x004C, 0x0072, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 54, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, CastSpellOnTarget, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node g4C32 = { /*@ ds:4C32 */
    0, 0x004D, 0x0073, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 54, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, CastSpellOnTarget, { { 0, 0 } },
    3, { 0, 0 }
};
struct Node g4C6E = { /*@ ds:4C6E */
    0, 0x0052, 0x0035, 0x0036,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 320, 200, 0,
    0, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, HandleSpellList, { { 0, 0 } },
    6, { 0, 0 }
};
struct Node g4CAA = { /*@ ds:4CAA */
    0, 0x0053, 0x003D, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 320, 200, 0,
    0, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, HandleSpellList, { { 0, 0 } },
    7, { 0, 0 }
};
struct Node g4CE6 = { /*@ ds:4CE6 */
    0, 0x0054, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 114, 0x0021,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, OpenInventory, { { 0, 0 } },
    4, { 0, 0 }
};
struct Node g4D22 = { /*@ ds:4D22 */
    0, 0x0055, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 114, 0x0021,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, OpenInventory, { { 0, 0 } },
    5, { 0, 0 }
};
struct Node g4D5E = { /*@ ds:4D5E */
    0, 0x0056, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 216, 114, 0x001F,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickHandSlot, { { 0, 0 } },
    4, { 0, 0 }
};
struct Node g4D9A = { /*@ ds:4D9A */
    0, 0x0057, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 288, 114, 0x001F,
    0x0021, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickHandSlot, { { 0, 0 } },
    5, { 0, 0 }
};
struct Node g4DD6 = { /*@ ds:4DD6 */
    0, 0x0058, 0x0174, 0,
    0, 0, 0, 0,
    0x1000, 0, 0, 0,
    0, 184, 106, 0x003F,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, PickOrSwapMember, { { 0, 0 } },
    4, { 0, 0 }
};
struct Node g4E12 = { /*@ ds:4E12 */
    0, 0x0059, 0x0175, 0,
    0, 0, 0, 0,
    0x1000, 0, 0, 0,
    0, 256, 106, 0x003F,
    8, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, PickOrSwapMember, { { 0, 0 } },
    5, { 0, 0 }
};
struct Node g4E4E = { /*@ ds:4E4E */
    0, 0x005A, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 227, 135, 0x000A,
    0x000A, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x0019, { 0, 0 }
};
struct Node g4E8A = { /*@ ds:4E8A */
    0, 0x005B, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 239, 135, 0x000A,
    0x000A, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, ClickInventorySlot, { { 0, 0 } },
    0x001A, { 0, 0 }
};
struct Node g4EC6 = { /*@ ds:4EC6 */
    0, 0x005C, 0x0074, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 106, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, CastSpellOnTarget, { { 0, 0 } },
    4, { 0, 0 }
};
struct Node g4F02 = { /*@ ds:4F02 */
    0, 0x005D, 0x0075, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 106, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, CastSpellOnTarget, { { 0, 0 } },
    5, { 0, 0 }
};
struct Node g4F3E = { /*@ ds:4F3E */
    0, 0x005E, 0, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 75, 168, 0x0061,
    6, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, RedrawPartyPanel, { { 0, 0 } },
    0, { 0, 0 }
};
struct Node g4F7A = { /*@ ds:4F7A */
    0, 0x005F, 0x006E, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 75, 168, 0x0061,
    6, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, CastSpellOnTarget, { { 0, 0 } },
    0xFFFF, { 0, 0 }
};
struct Node g4FB6 = { /*@ ds:4FB6 */
    0, 0x0060, 0x0074, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 184, 106, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectClickedMember, { { 0, 0 } },
    4, { 0, 0 }
};
struct Node g4FF2 = { /*@ ds:4FF2 */
    0, 0x0061, 0x0075, 0,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 256, 106, 0x003F,
    0x0032, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectClickedMember, { { 0, 0 } },
    5, { 0, 0 }
};
struct Node g502E = { /*@ ds:502E */
    0, 0x0062, 0x0074, 0x0075,
    0, 0, 0, 0,
    0x1100, 0, 0, 0,
    0, 320, 200, 1,
    1, 0x0C, 0, 0x0F,
    0, 0x0F, 0, 8,
    0, 0, SelectNextMember, { { 0, 0 } },
    2, { 0, 0 }
};
struct Node far *gTable1b[38] = { /*@ ds:506A */
    &g4836, &g3A9E, &g3ADA, &g3B16, &g3B52, &g4FB6, &g4FF2, &g3B8E, &g3BCA, &g3C06, &g3C42, &g3C7E,
    &g3CBA, &g3CF6, &g3D32, &g3D6E, &g4CE6, &g4D22, &g3DAA, &g3DE6, &g3E22, &g3E5E, &g4D5E, &g4D9A,
    &g3E9A, &g3ED6, &g3F12, &g3F4E, &g4DD6, &g4E12, &g461A, &g4656, &g4692, &g46CE, &g470A, &g4746,
    &g47BE, &g47FA
};
struct Node far *gTable1a[52] = { /*@ ds:5102 */
    &g4836, &g48EA, &g4926, &g4962, &g499E, &g49DA, &g4A16, &g4A52, &g4A8E, &g4ACA, &g4B06, &g4B42,
    &g4C6E, &g4CAA, &g4F3E, &g3A9E, &g3ADA, &g3B16, &g3B52, &g4FB6, &g4FF2, &g3B8E, &g3BCA, &g3C06,
    &g3C42, &g3C7E, &g3CBA, &g3CF6, &g3D32, &g3D6E, &g4CE6, &g4D22, &g3DAA, &g3DE6, &g3E22, &g3E5E,
    &g4D5E, &g4D9A, &g3E9A, &g3ED6, &g3F12, &g3F4E, &g4DD6, &g4E12, &g461A, &g4656, &g4692, &g46CE,
    &g470A, &g4746, &g47BE, &g47FA
};
struct Node far *gTable2b[47] = { /*@ ds:51D2 */
    &g4836, &g3F8A, &g3FC6, &g4002, &g403E, &g407A, &g40B6, &g40F2, &g412E, &g416A, &g41A6, &g41E2,
    &g421E, &g425A, &g4296, &g42D2, &g430E, &g434A, &g4386, &g43C2, &g43FE, &g443A, &g4476, &g44B2,
    &g44EE, &g452A, &g4E4E, &g4E8A, &g4566, &g45A2, &g45DE, &g48AE, &g4872, &g502E, &g3B8E, &g3BCA,
    &g3C06, &g3C42, &g3C7E, &g461A, &g4656, &g4692, &g46CE, &g470A, &g4746, &g47BE, &g47FA
};
struct Node far *gTable3a[19] = { /*@ ds:528E */
    &g4836, &g45A2, &g45DE, &g48AE, &g4872, &g502E, &g3B8E, &g3BCA, &g3C06, &g3C42, &g3C7E, &g461A,
    &g4656, &g4692, &g46CE, &g470A, &g4746, &g47BE, &g47FA
};
struct Node far *gTable2a[59] = { /*@ ds:52DA */
    &g4836, &g48EA, &g4926, &g4962, &g499E, &g49DA, &g4A16, &g4A52, &g4A8E, &g4ACA, &g4B06, &g4B42,
    &g4F3E, &g3F8A, &g3FC6, &g4002, &g403E, &g407A, &g40B6, &g40F2, &g412E, &g416A, &g41A6, &g41E2,
    &g421E, &g425A, &g4296, &g42D2, &g430E, &g434A, &g4386, &g43C2, &g43FE, &g443A, &g4476, &g44B2,
    &g44EE, &g452A, &g4E4E, &g4E8A, &g4566, &g45A2, &g45DE, &g48AE, &g4872, &g502E, &g3B8E, &g3BCA,
    &g3C06, &g3C42, &g3C7E, &g461A, &g4656, &g4692, &g46CE, &g470A, &g4746, &g47BE, &g47FA
};
struct Node far *gTable3b[32] = { /*@ ds:53C6 */
    &g4836, &g48EA, &g4926, &g4962, &g499E, &g49DA, &g4A16, &g4A52, &g4A8E, &g4ACA, &g4B06, &g4B42,
    &g4F3E, &g4566, &g45A2, &g45DE, &g48AE, &g4872, &g502E, &g3B8E, &g3BCA, &g3C06, &g3C42, &g3C7E,
    &g461A, &g4656, &g4692, &g46CE, &g470A, &g4746, &g47BE, &g47FA
};
struct Node far *gTable4[7] = { &g3E9A, &g3ED6, &g3F12, &g3F4E, &g4DD6, &g4E12, &g4782 }; /*@ ds:5446 */
struct Node far *gTable5[7] = { &g4B7E, &g4BBA, &g4BF6, &g4C32, &g4EC6, &g4F02, &g4F7A }; /*@ ds:5462 */

/* 0CF7:0007: points gList at head and clears head's link to a null far pointer, making head the
   only node of a new list. */
/*@ 0CF7:0007 */ void ListInit(struct Node far *head)
{
    gList = head;
    gList->next = 0;
}

/* 0CF7:0028: links one table's records into a list. gFlag nonzero picks gTable1a (52 records, the
   first seeds the list); zero picks gTable1b (38 records). */
/*@ 0CF7:0028 */ void BuildList1(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable1a[0]);
        for (i = 1; i < TABLE1A_COUNT; i++)
            /* QUIRK 0CF7:0028: ListAdd returns its first argument, so this leaves gList at the
               head the loop started with, not the node just appended; see its note. */
            gList = ListAdd(gList, gTable1a[i]);
    } else {
        ListInit(gTable1b[0]);
        for (i = 1; i < TABLE1B_COUNT; i++)
            gList = ListAdd(gList, gTable1b[i]);
    }
}

/* 0CF7:00B4: links one table's records into a list. gFlag nonzero picks gTable2a (59 records);
   zero picks gTable2b (47 records). Same shape as BuildList1. */
/*@ 0CF7:00B4 */ void BuildList2(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable2a[0]);
        for (i = 1; i < TABLE2A_COUNT; i++)
            gList = ListAdd(gList, gTable2a[i]);
    } else {
        ListInit(gTable2b[0]);
        for (i = 1; i < TABLE2B_COUNT; i++)
            gList = ListAdd(gList, gTable2b[i]);
    }
}

/* 0CF7:0140: links one table's records into a list. gFlag nonzero picks gTable3a (19 records);
   zero picks gTable3b (32 records). Same shape as BuildList1. */
/*@ 0CF7:0140 */ void BuildList3(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable3a[0]);
        for (i = 1; i < TABLE3A_COUNT; i++)
            gList = ListAdd(gList, gTable3a[i]);
    } else {
        ListInit(gTable3b[0]);
        for (i = 1; i < TABLE3B_COUNT; i++)
            gList = ListAdd(gList, gTable3b[i]);
    }
}

/* 0CF7:01CC: links gTable4's 7 records into a list; no flag, so one fixed table. Same shape as
   BuildList1 without the branch. Its only caller is the function at 0ACD:0264 (call at
   0ACD:02BE). */
/*@ 0CF7:01CC */ void BuildList4(void)
{
    int i;

    ListInit(gTable4[0]);
    for (i = 1; i < TABLE4_COUNT; i++)
        gList = ListAdd(gList, gTable4[i]);
}

/* 0CF7:0211: links gTable5's 7 records into a list; no flag, one fixed table. Same shape as
   BuildList4. The function at 0EF3:059D calls it at 0EF3:05B6. */
/*@ 0CF7:0211 */ void BuildList5(void)
{
    int i;

    ListInit(gTable5[0]);
    for (i = 1; i < TABLE5_COUNT; i++)
        gList = ListAdd(gList, gTable5[i]);
}
