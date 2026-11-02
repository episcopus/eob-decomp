/* Unit at EOB.EXE 1C0E:000C (6099 bytes, 31 functions): the overlay file cache. This is the file
   layer the VROOMM overlay manager and the data loaders go through. FileOpen (1C0E:00DB) hands out
   a slot in a ten entry table of cached files (the records at ds:01FF, the DOS handles at ds:FEEA),
   FileRead (1C0E:0400) and FileWrite (1C0E:0578) move bytes in and out of the slot's memory copy or
   its open handle, FileClose (1C0E:03BB) releases it, and 1C0E:1645/1161/11D3/168E look a name up
   and follow the link field that groups aliased entries. 1C0E:13E7/:13FD initialise and tear the
   cache down at startup (E07C8.C), 1C0E:1310 finds the program file on disk (E1A72.ASM), and
   1C0E:0E5B/0DC3 choose a slot to evict under a size limit. Placeholder DGROUP names mark the fields
   whose exact role is inferred from this code only. */

/*@flags -ml -O -Z -G */

/* Runtime library (Turbo C++ 1.00, large model). */
extern int open(char far *path, int access, ...);   /*@ 0000:6443: runtime OPEN */
extern int close(int fd);                          /*@ 0000:50C5: runtime CLOSE */
extern int read(int fd, void far *buf, unsigned len); /*@ 0000:65E1: runtime READ, text mode */
extern int write(int fd, void far *buf, unsigned len); /*@ 0000:5BFE: runtime WRITE, text mode */
extern long lseek(int fd, long offset, int whence); /*@ 0000:61FF: runtime LSEEK */
extern long filelength(int fd);                    /*@ 0000:5F26: runtime FILELENGTH */
extern int chsize(int handle, long size);          /*@ 0000:5E41: runtime CHSIZE */
extern char far *strchr(char far *s, int c);       /*@ 0000:6760: runtime STRCHR */
extern char far *strupr(char far *s);              /*@ 0000:686A: runtime STRUPR */
extern int strcmp(char far *a, char far *b);       /*@ 0000:679D: runtime STRCMP */
extern int remove(char far *path);                 /*@ 0000:6B57: runtime REMOVE */
extern int getdisk(void);                          /*@ 0000:5E11: runtime GETDISK */
extern int setdisk(int drive);                     /*@ 0000:5E17: runtime SETDISK */

/* Other units. */
extern void ShutdownGame(void);                    /*@ 07C8:058A */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void MemFree(void far *p);                  /*@ 19BC:0137 */
extern long LargestFreeBlock(void);                /*@ 19BC:023D */
extern void far *f1BA8_0072(void far *p);          /*@ 1BA8:0072 */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern int IsLastLogicalDrive(int drive);          /*@ 1A19:003B */
extern void FindLastDrive(void);                   /*@ 1A19:0006 */
extern void LockHidePointer(void);                 /*@ 1BBE:0307 */
extern void LockShowPointer(void);                 /*@ 1BBE:0332 */
extern void SetTextWindow(int region);             /*@ 1DE7:0009 */
extern void ClearTextRegion(void);                 /*@ 1DE7:0344 */
extern void DrawText(char far *text);              /*@ 1DE7:060A */
extern void f1A47_0169(void far *font);            /*@ 1A47:0169 */
extern void FlushKeyBuffer(void);                  /*@ 1AAD:0480 */
extern void WaitGameKey(void);                     /*@ 1AAD:053A */

/* DGROUP globals and indirect far functions. */
extern int g8115;                        /*@ ds:8115: nonzero while a status line is up (E082C.C) */
extern int g811F;                        /*@ ds:811F */
extern int g8102;                        /*@ ds:8102: the video mode the game initialised (E07A9.C) */
extern int gTextLeft;                    /*@ ds:FE64: the text window's left edge, 8-pixel columns */
extern int gTextTop;                     /*@ ds:FE66: the text window's top edge, pixel rows */
extern int gTextWidth;                   /*@ ds:FE62: the text window's width, 8-pixel columns */
extern int gTextHeight;                  /*@ ds:FE60: the text window's height, pixel rows */
extern int gTextAttr1;                   /*@ ds:811D */
extern int gTextAttr2;                   /*@ ds:811B */
extern void (far *g8104)(int code, char far *name); /*@ ds:8104: the fatal file error handler */
extern int (far *g8111)(int mode, char far *name);  /*@ ds:8111: the retry file error handler */
extern signed char gLastDrive;           /*@ ds:FF00: highest drive _FindLastDrive found */
extern int gSavedDrive;                  /*@ ds:FEFE: the drive getdisk returned at init */
extern unsigned char gSearchAllDrives;   /*@ ds:81EA: nonzero: 1C0E:1310 also scans other drives */

#define MAX_FILES      10
#define SLOT_FREE      0x3fff   /* gFileHandles[i]: the slot holds no file */
#define HANDLE_MASK    0x3fff   /* the low bits of a gFileHandles entry are the DOS handle */
#define FILE_IN_MEMORY 0x8000   /* the slot's bytes are in gFiles[i].data, not a DOS handle */
#define FILE_LINKED    0x4000   /* the slot's name is an alias of another entry */
#define FLAG_KEEP      0x80     /* gFiles[i].flags: the entry is preloaded at init */
#define FLAG_BITS      0x3f
#define LINK_INDEX     0x7fff

/* open() flags and create mode (fcntl.h), used by 1C0E:0A9D. */
#define O_RDWR         0x0002
#define O_CREAT        0x0100
#define O_BINARY       0x8000
#define S_IREAD        0x0100
#define S_IWRITE       0x0080

/* Functions defined later in the file but called earlier. */
int FindFile(char far *name);            /*@ 1C0E:1645: defined later in the file */
int IsFileInMemory(char far *name);      /*@ 1C0E:1161: defined later in the file */
int FindLoadedFile(char far *name);      /*@ 1C0E:11D3: defined later in the file */
int f1C0E_144E(int index);               /*@ 1C0E:144E: defined later in the file */

/* A cached file, a 19-byte record at ds:01FF (index * 19 + ds:01FF). The name is a C string; an
   empty name ends the table. */
struct FileEntry {
    char far *name;             /* +0x00: the file name, upper cased */
    long size;                  /* +0x04: the file's length on disk */
    char far *data;             /* +0x08: the in-memory copy when the slot is loaded */
    int link;                   /* +0x0C: < 0: LINK_INDEX is the index of the entry this aliases */
    long pos;                   /* +0x0E: the read/write position in the file */
    unsigned char flags;        /* +0x12: FLAG_KEEP and the low FLAG_BITS */
};

extern struct FileEntry gFiles[];        /*@ ds:01FF */
extern int gFileHandles[];               /*@ ds:FEEA: DOS handle or SLOT_FREE per slot */

/* 1C0E:000C: resize the file in slot `index` to `size` bytes and return 1 when that succeeded, 0
   when the slot is loaded in memory (nothing to resize) or chsize failed. DRAFT: every byte matches
   but the layout of the two `return 0` tails (the original keeps them apart, TCC -O folds the first
   into `je`), no source form found yet. */
/*@ 1C0E:000C draft */ int SetFileSize(int index, long size)
{
    int handle = gFileHandles[index];

    if (handle & FILE_IN_MEMORY)
        return 0;
    if (chsize(handle & HANDLE_MASK, size) == -1)
        return 0;
    return 1;
}

/* 1C0E:0046: upper case from the last backslash of `name` on (so the file name part, leaving a
   directory prefix alone) and hand the result with `mode` to the retry handler at ds:8111,
   returning its result. Called before an overlay open fails (FileOpen modes 0 and 2) and when a
   name lookup misses (1C0E:11D3 mode 0xA, 1C0E:0EE7/0F6E mode 0xB). Purpose unknown: the mode
   codes. */
/*@ 1C0E:0046 */ int RetryFile(int mode, char far *name)
{
    char far *base;
    char far *slash;

    slash = base = name;
    while ((slash = strchr(base, '\\')) != 0)
        base = slash;
    strupr(base);
    return g8111(mode, base);
}

/* 1C0E:00AA: report a file error `code` for `name` through the fatal handler at ds:8104 (first the
   code itself unless it is already 0xC, then 0xC) and shut the game down. */
/*@ 1C0E:00AA */ void FatalError(int code, char far *name)
{
    if (code != 0xc)
        g8104(code, name);
    g8104(0xc, name);
    ShutdownGame();
}

/*@ 1C0E:00DB stub */ int FileOpen(char far *name, int mode) { }

/* 1C0E:03BB: close the file in slot `index`: report code 4 when the slot is already free, close
   the DOS handle unless the slot is loaded in memory, then mark the slot free. */
/*@ 1C0E:03BB */ void FileClose(int index)
{
    int handle = gFileHandles[index];

    if (handle == SLOT_FREE)
        FatalError(4, 0);
    if (!(handle & FILE_IN_MEMORY))
        close(handle & HANDLE_MASK);
    gFileHandles[index] = SLOT_FREE;
}

/*@ 1C0E:0400 stub */ void FileRead(int index, void far *buf, unsigned long len) { }
/*@ 1C0E:0578 stub */ void FileWrite(int index, void far *buf, unsigned long len) { }
/*@ 1C0E:06E5 stub */ void SeekFile(int index, long offset, int whence) { }
/*@ 1C0E:091B stub */ int f1C0E_091B(char far *name) { }
/* 1C0E:0A35: delete the file `name`: drop any in-memory copy from its slot first, then remove it
   from disk. Returns 1 when the file was removed (remove returned 0). Called by 1C0E:00DB and by
   1C0E:0A9D before recreating the file. */
/*@ 1C0E:0A35 */ int DeleteFile(char far *name)
{
    if (IsFileInMemory(name)) {
        int index = FindLoadedFile(name);

        MemFree(gFiles[index].data);
        gFiles[index].data = 0;
    }
    return !remove(name);
}

/* 1C0E:0A9D: empty the file `name`: delete it (1C0E:0A35) then recreate it with open(), and close
   the new handle. Returns 1 when the file could be recreated. */
/*@ 1C0E:0A9D */ int ResetFile(char far *name)
{
    int fd;

    DeleteFile(name);
    fd = open(name, O_CREAT | O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
    if (fd == -1)
        return 0;
    close(fd);
    return 1;
}
/*@ 1C0E:0ADD stub */ int f1C0E_0ADD(int index, char far *name) { }

/* 1C0E:0D9B: clear the status-line flag ds:8115 while 1C0E:0ADD draws its message box, then put
   the old value back and return 1. */
/*@ 1C0E:0D9B */ int ShowMessage(int index, char far *name)
{
    int saved = g8115;

    g8115 = 0;
    f1C0E_0ADD(index, name);
    g8115 = saved;
    return 1;
}

/* 1C0E:0DC3: return the index of the loaded slot to evict for a `minSize` byte request: the one
   whose data is in memory, whose size is at least `minSize`, whose flags are at most `maxFlags`
   and whose flags are the smallest of those candidates; -1 when there is none. DRAFT: the loops and
   values are right, TCC -O lays the signed size compare (1C0E:0DC3+46) out with the branches in the
   other order; no source form found yet. */
/*@ 1C0E:0DC3 draft */ int FindEvictableFile(int minSize, unsigned char maxFlags)
{
    int count;
    int slot = -1;
    int i = 0;

    count = 0;
    while (gFiles[count].name[0] != 0)
        count++;
    for (i = 0; i < count; i++) {
        if (gFiles[i].data == 0)
            continue;
        if (gFiles[i].size < minSize)
            continue;
        if (gFiles[i].flags > maxFlags)
            continue;
        if (slot == -1 || gFiles[i].flags < gFiles[slot].flags)
            slot = i;
    }
    return slot;
}

/* 1C0E:0E5B: first retry before the out-of-memory hook (src/E19BC.C): drop the in-memory copy of
   the least important slot that can hold a `size` byte request (1C0E:0DC3) and return 1; return 0
   when no slot qualifies or any slot's handle happens to equal its own index. DRAFT: the code is
   right but the three `return 0` tails that the original keeps apart and shares are folded into
   inline zeros by TCC -O; no source form found yet. */
/*@ 1C0E:0E5B draft */ int EvictFile(unsigned size, unsigned char flags)
{
    int i = 0;
    int slot;
    char blocked;

    slot = FindEvictableFile(size, flags);
    if (slot == -1)
        return 0;
    blocked = 0;
    for (i = 0; i < 10; i++)
        if ((gFileHandles[i] & HANDLE_MASK) == i) {
            blocked = 1;
            i = 10;
        }
    if (blocked)
        return 0;
    if (i != 10)
        return 0;
    MemFree(gFiles[slot].data);
    gFiles[slot].data = 0;
    return 1;
}

/* 1C0E:0EE7: free every slot's in-memory copy and, for every slot still open, run the retry
   handler with mode 0xB. Called by 1C0E:13FD on shutdown. */
/*@ 1C0E:0EE7 */ void FreeAllCachedFiles(void)
{
    int i;

    for (i = 0; gFiles[i].name[0] != 0; i++) {
        if (gFiles[i].data != 0) {
            MemFree(gFiles[i].data);
            gFiles[i].data = 0;
        }
    }
    for (i = 0; i < MAX_FILES; i++)
        if (gFileHandles[i] != SLOT_FREE)
            RetryFile(0xb, 0);
}

/*@ 1C0E:0F6E stub */ void f1C0E_0F6E(void) { }

/* 1C0E:1161: 1 when the file `name` (upper cased in place) has an in-memory copy, 0 when it is not
   in the table or not loaded. */
/*@ 1C0E:1161 */ int IsFileInMemory(char far *name)
{
    int i = 0;

    name = strupr(name);
    for (; gFiles[i].name[0] != 0; i++)
        if (strcmp(gFiles[i].name, name) == 0) {
            if (gFiles[i].data != 0)
                return 1;
            return 0;
        }
    return 0;
}

/* 1C0E:11D3: return the index of the slot named `name` (upper cased in place); when no slot
   matches, report it through the retry handler (mode 0xA) and return 0. */
/*@ 1C0E:11D3 */ int FindLoadedFile(char far *name)
{
    int i = 0;

    name = strupr(name);
    for (; gFiles[i].name[0] != 0; i++)
        if (strcmp(gFiles[i].name, name) == 0)
            return i;
    RetryFile(0xa, name);
    return 0;
}

/* 1C0E:1240: copy up to `len` bytes from the in-memory copy of slot `index` (at its position) into
   `dst` through Unpack0's raw copy, clamp `len` to what is left in the file, advance the position
   and return the number of bytes copied. The source pointer is normalized through 1BA8:0072 first. */
/*@ 1C0E:1240 */ long ReadFromMemory(int index, void far *dst, long len)
{
    char far *src;

    if (gFiles[index].pos + len > gFiles[index].size)
        len = gFiles[index].size - gFiles[index].pos;
    src = f1BA8_0072(gFiles[index].data + gFiles[index].pos);
    Unpack0(src, dst, len);
    gFiles[index].pos += len;
    return len;
}

/* 1C0E:1310: locate the file `name` (upper cased in place) on disk. Try the current drive first,
   then (only when ds:81EA asks for it) the drives from ds:FF00 down to A; return the drive number
   plus one on success, 0 when the file is nowhere. Called from the INT 3Fh handler (E1A72.ASM). */
/*@ 1C0E:1310 */ int LocateOverlayFile(char far *name)
{
    int saved = getdisk();
    int fd;
    int drive;

    name = strupr(name);
    fd = open(name, 0x8001);
    if (fd != -1) {
        close(fd);
        return saved + 1;
    }
    if (gSearchAllDrives == 0)
        return 0;
    for (drive = gLastDrive; drive != -1; drive--) {
        if (!IsLastLogicalDrive(drive))
            continue;
        setdisk(drive);
        fd = open(name, 0x8001);
        if (fd != -1) {
            close(fd);
            return drive + 1;
        }
    }
    setdisk(saved);
    return 0;
}

/* 1C0E:13BB: grow the text window outward by `cells` 8-pixel columns on both sides and `cells`
   lines on both edges: left and top move out, width and height grow by twice `cells` (a negative
   `cells` shrinks it back). */
/*@ 1C0E:13BB */ void ExpandTextWindow(int cells)
{
    gTextLeft -= cells;
    gTextTop -= cells * 8;
    gTextWidth += cells * 2;
    gTextHeight += cells * 16;
}

/* 1C0E:13E7: the startup half of the overlay cache: remember the current drive, find the highest
   logical drive and build the cache table (1C0E:0F6E). Called from StartupGame (E07C8.C). */
/*@ 1C0E:13E7 */ void InitOverlayCache(void)
{
    gSavedDrive = getdisk();
    FindLastDrive();
    f1C0E_0F6E();
}

/* 1C0E:13FD: the shutdown half of the overlay cache: free every loaded copy (1C0E:0EE7) and go
   back to the drive the game started on. Called from ShutdownGame (E07C8.C). */
/*@ 1C0E:13FD */ void ShutdownOverlayCache(void)
{
    FreeAllCachedFiles();
    setdisk(gSavedDrive);
}

/* 1C0E:1411: look `name` up; when the slot exists and its flags have 0x40 set, drop the slot
   through 1C0E:144E and return 1, otherwise 0. DRAFT: the values are right, TCC -O folds the two
   `return 0` tails into inline zeros where the original keeps one and falls through the other; no
   source form found yet. */
/*@ 1C0E:1411 draft */ int DropFile(char far *name)
{
    int index = FindFile(name);

    if (index == -1)
        return 0;
    if (!(gFiles[index].flags & 0x40))
        return 0;
    f1C0E_144E(index);
    return 1;
}

/*@ 1C0E:144E stub */ int f1C0E_144E(int index) { }

/* 1C0E:1645: return the index of the slot named `name` exactly, or -1. */
/*@ 1C0E:1645 */ int FindFile(char far *name)
{
    int i;

    for (i = 0; gFiles[i].name[0] != 0; i++)
        if (strcmp(gFiles[i].name, name) == 0)
            return i;
    return -1;
}

/* 1C0E:168E: find the slot named `name` (upper cased in place), follow its chain of aliases
   through the link field while it is negative, and return the terminal link value. */
/*@ 1C0E:168E */ int ResolveFileLink(char far *name)
{
    int i = 0;

    name = strupr(name);
    for (; gFiles[i].name[0] != 0; i++)
        if (strcmp(gFiles[i].name, name) == 0) {
            while (gFiles[i].link < 0)
                i = gFiles[i].link & LINK_INDEX;
            return gFiles[i].link;
        }
    return -1;
}

/* 1C0E:171A: stop 1C0E:1310 from scanning the other drives for a file. */
/*@ 1C0E:171A */ void ClearSearchAllDrives(void)
{
    gSearchAllDrives = 0;
}

/* 1C0E:1724: let the file locator 1C0E:1310 search every logical drive, not only the one the game
   started on. */
/*@ 1C0E:1724 */ void SetSearchAllDrives(void)
{
    gSearchAllDrives = 1;
}

/*@ 1C0E:172E stub */ long f1C0E_172E(int handle, char far *name) { }
