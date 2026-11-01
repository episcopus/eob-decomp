/* Overlay 2 of EOB.EXE (3332 bytes, 11 functions, all entries of the overlay): loading the item
   tables. Two functions decompiled, the rest are stubs. Placeholder names. */

struct ItemType {           /* 16-byte records of ITEMTYPE.DAT, at ds:A9F0 */
    char data[16];
};

extern struct ItemType gItemTypes[]; /*@ ds:A9F0 */

extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */

/*@ ovl2:0000 stub */ void Ovl2_0000(void) {}
/*@ ovl2:005E stub */ void Ovl2_005E(void) {}

/* ovl2:0110: read ITEMTYPE.DAT, a count then 16-byte records */
/*@ ovl2:0110 */ void LoadItemTypes(void)
{
    int n;
    int i;
    int h;

    h = FileOpen("ITEMTYPE.DAT", 1);
    FileRead(h, (void far *)&n, 2L);
    for (i = 0; i < n; i++)
        FileRead(h, (void far *)&gItemTypes[i], 16L);
    FileClose(h);
}

/*@ ovl2:0496 stub */ void Ovl2_0496(void) {}
/*@ ovl2:078D stub */ void Ovl2_078D(void) {}
/*@ ovl2:0846 stub */ void Ovl2_0846(void) {}
/*@ ovl2:0937 stub */ void Ovl2_0937(void) {}

/* ovl2:09CC, entry 2 (called as 2106:002A): load all the item tables */
/*@ ovl2:09CC */ void LoadItemData(void)
{
    Ovl2_0000();
    Ovl2_0496();
    Ovl2_005E();
    LoadItemTypes();
    Ovl2_0937();
    Ovl2_078D();
    Ovl2_0846();
}
