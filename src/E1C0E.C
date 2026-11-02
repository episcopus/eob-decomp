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

#include <dos.h>

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
extern int WaitGameKey(void);                      /*@ 1AAD:053A */

/* The screen routines and font the error dialog uses. */
extern void (far *gRestoreRect)(int x, int y, int w, int h, void far *buf); /*@ ds:804E: buffer back to the screen (E1BBE.ASM) */
extern void (far *gSaveRect)(int x, int y, int w, int h, void far *buf);    /*@ ds:80AE: screen into the buffer (E1BBE.ASM) */
extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour);   /*@ ds:8062: line routine (E1547.C) */
extern int (far *g80C6)(int width, int height);    /*@ ds:80C6: the save-under buffer size in bytes (E1BB1.ASM) */
extern void far *gFE5A;                            /*@ ds:FE5A: the 8x8 font loaded from Font8.fnt (E07C8.C) */
extern void far *gFont1A47;                        /*@ ds:FE4E: the current font (E1A47.C) */

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
extern signed char gLastDrive;           /*@ ds:FF00: highest drive FindLastDrive found */
extern int gSavedDrive;                  /*@ ds:FEFE: the drive getdisk returned at init */
extern unsigned char gSearchAllDrives;   /*@ ds:81EA: nonzero: 1C0E:1310 also scans other drives */
extern long gRecordLength;               /*@ ds:FF02: 1C0E:172E leaves the record's length here */
extern long gRecordOffset;               /*@ ds:FF06: 1C0E:172E leaves the record's offset here */
extern char far *gErrorText[];           /*@ ds:8196: the file error messages, indexed by FatalError code */
extern void far *gMsgBuf;                /*@ ds:81E6: the saved screen rectangle of the error dialog */
extern int gMsgLink;                     /*@ ds:FF0A: ResolveFileLink's result for the file in the dialog */
extern int g80FE;                        /*@ ds:80FE: the text region the error dialog switches to; 1 in the image */

#define MAX_FILES      10
#define SLOT_FREE      0x3fff   /* gFileHandles[i]: the slot holds no file */
#define HANDLE_MASK    0x3fff   /* the low bits of a gFileHandles entry are the DOS handle */
#define FILE_IN_MEMORY 0x8000   /* the slot's bytes are in gFiles[i].data, not a DOS handle */
#define FILE_LINKED    0x4000   /* the slot's name is an alias of another entry */
#define FLAG_KEEP      0x80     /* gFiles[i].flags: the entry is preloaded at init */
#define FLAG_BITS      0x3f
#define LINK_INDEX     0x7fff

/* open() flags and create mode (fcntl.h, STAT.H), used by 1C0E:0A9D and the file locators. */
#define O_RDONLY       0x0001
#define O_WRONLY       0x0002
#define O_RDWR         0x0004
#define O_CREAT        0x0100
#define O_BINARY       0x8000
#define S_IREAD        0x0100
#define S_IWRITE       0x0080

/* Functions defined later in the file but called earlier. */
int FindFile(char far *name);            /*@ 1C0E:1645: defined later in the file */
int IsFileInMemory(char far *name);      /*@ 1C0E:1161: defined later in the file */
int FindLoadedFile(char far *name);      /*@ 1C0E:11D3: defined later in the file */
int LocateOverlayFile(char far *name);   /*@ 1C0E:1310: defined later in the file */
int LoadFileSlot(int index);               /*@ 1C0E:144E: defined later in the file */
long ReadFromMemory(int index, void far *dst, long len); /*@ 1C0E:1240: defined later in the file */
long LookupFileEntry(int handle, char far *name); /*@ 1C0E:172E: defined later in the file */
void ExpandTextWindow(int cells);         /*@ 1C0E:13BB: defined later in the file */
int ResolveFileLink(char far *name);      /*@ 1C0E:168E: defined later in the file */

/* A cached file, a 19-byte record at ds:01FF (index * 19 + ds:01FF). The name is a C string; an
   empty name ends the table. */
struct FileEntry {
    char far *name;             /* +0x00: the file name, upper cased */
    long size;                  /* +0x04: the file's length on disk */
    char far *data;             /* +0x08: the in-memory copy when the slot is loaded */
    int link;                   /* +0x0C: < 0: LINK_INDEX is the index of the entry this aliases */
    long pos;                   /* +0x0E: the read/write position in the file */
    signed char flags;          /* +0x12: FLAG_KEEP and the low FLAG_BITS */
};

extern struct FileEntry gFiles[];        /*@ ds:01FF */
extern int gFileHandles[];               /*@ ds:FEEA: DOS handle or SLOT_FREE per slot */
extern long gSlotOffsets[];              /*@ ds:FEC2: the record's start offset per slot */
extern long gSlotLengths[];              /*@ ds:FF0C: the record's length per slot */

/* 1C0E:000C: resize the file in slot `index` to `size` bytes and return 1 when that succeeded, 0
   when the slot is loaded in memory (nothing to resize) or chsize failed. */
/*@ 1C0E:000C */ int SetFileSize(int index, long size)
{
    int handle = gFileHandles[index];

    if (!(handle & FILE_IN_MEMORY))
        return chsize(handle & HANDLE_MASK, size) != -1;
    return 0;
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

/* 1C0E:00DB: open the file `name` in one of three modes and give it a cache slot. Mode 1 (0x8001,
   read) reuses an in-memory copy or checks and loads the file from disk; mode 2 (0x8002, write) and
   mode 3 (0x8004, read/write) delete and recreate the file, retrying on failure; any other mode
   reports code 1 then a fatal code 0xC. An existing in-memory copy is dropped unless mode is 1, the
   file is located (following the alias link when its record points at another entry), opened with
   retry, and its directory entry recorded for a linked file (LookupFileEntry). Finally the first
   free slot takes the handle (FILE_IN_MEMORY | index for a loaded file, else the DOS handle with
   FILE_LINKED set) with the record's offset and length, and its index is returned. */
/*@ 1C0E:00DB */ int FileOpen(char far *name, int mode)
{
    int fd;
    int access;
    int slot;
    int found;
    int i;

    switch (mode) {
    case 1:
        if (!IsFileInMemory(name)) {
            while (!CheckFile(name))
                if (!RetryFile(2, name))
                    return -1;
            DropFile(name);
        }
        access = FILE_IN_MEMORY | O_RDONLY;
        break;
    case 2:
        access = FILE_IN_MEMORY | O_WRONLY;
        DeleteFile(name);
        ResetFile(name);
        /* fall through */
    case 3:
        if (!CheckFile(name))
            while (!ResetFile(name))
                if (!RetryFile(0, name))
                    return -1;
        if (mode == 3)
            access = FILE_IN_MEMORY | O_RDWR;
        break;
    default:
        RetryFile(1, name);
        FatalError(0xc, name);
        ShutdownGame();
        break;
    }

    if (IsFileInMemory(name) && mode != 1) {
        slot = FindLoadedFile(name);
        MemFree(gFiles[slot].data);
        gFiles[slot].data = 0;
    }

    if (!IsFileInMemory(name)) {
        found = FindFile(name);
        if (found != -1) {
            if (gFiles[found].link < 0)
                name = gFiles[gFiles[found].link & LINK_INDEX].name;
            else
                found = -1;
        }
        fd = open(name, access, S_IREAD | S_IWRITE);
        while (fd == -1) {
            fd = open(name, access, S_IREAD | S_IWRITE);
            if (fd == -1)
                if (!RetryFile(2, name))
                    return -1;
        }
        if (found != -1) {
            LookupFileEntry(fd, gFiles[found].name);
            fd |= FILE_LINKED;
        }
    } else {
        fd = FindLoadedFile(name);
        gFiles[fd].pos = 0;
        fd |= FILE_IN_MEMORY;
    }

    slot = MAX_FILES;
    for (i = 0; i < MAX_FILES; i++)
        if (gFileHandles[i] == SLOT_FREE) {
            slot = i;
            break;
        }
    if (slot == MAX_FILES)
        FatalError(3, name);
    gFileHandles[slot] = fd;
    gSlotOffsets[slot] = gRecordOffset;
    gSlotLengths[slot] = gRecordLength;
    return slot;
}

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

/* 1C0E:0400: read up to `len` bytes from the file in cache slot `index` into `buf` and return the
   number of bytes read (0 when a read failed). A file loaded in memory is copied through
   1C0E:1240; a linked record (FILE_LINKED) is clamped to what is left of the record from the
   current position. The bytes are read in 0xFA00 byte chunks, each buffer normalised through
   1BA8:0072, and after every chunk `buf` is advanced by that many bytes with
   MK_FP(FP_SEG(buf), FP_OFF(buf) + n): only the offset moves and the segment is kept, which is the
   form the original shows (1C0E:0519 loads the offset, adds, reloads and stores the segment). */
/*@ 1C0E:0400 */ long FileRead(int index, char far *buf, unsigned long len)
{
    int handle;
    unsigned long total;
    long remaining;
    long pos;
    unsigned n;

    handle = gFileHandles[index];
    if (handle == SLOT_FREE)
        FatalError(5, 0);
    if (handle & FILE_IN_MEMORY)
        return ReadFromMemory(handle & HANDLE_MASK, buf, len);
    if (handle & FILE_LINKED) {
        pos = lseek(handle & HANDLE_MASK, 0, 1);
        pos -= gSlotOffsets[index];
        remaining = gSlotLengths[index] - pos;
        if (len > remaining)
            len = remaining;
    }
    handle &= HANDLE_MASK;
    total = 0;
    while (len > 0xfa00) {
        buf = f1BA8_0072(buf);
        n = read(handle, buf, 0xfa00);
        len -= 0xfa00;
        if (n == -1)
            return 0;
        total = total + n;
        buf = MK_FP(FP_SEG(buf), FP_OFF(buf) + n);
    }
    buf = f1BA8_0072(buf);
    n = read(handle, buf, len);
    if (n == -1)
        return 0;
    return total + n;
}
/* 1C0E:0578: write up to `len` bytes from `buf` to the file in cache slot `index` and return the
   number of bytes written (0 on a short write). A file loaded in memory cannot be written (code 9);
   a linked record (FILE_LINKED) is clamped to what is left of the record from the current position.
   The bytes are written in 0xFA00 byte chunks, each buffer normalised through 1BA8:0072, and after
   every full chunk `buf` is advanced by 0xFA00 with MK_FP(FP_SEG(buf), FP_OFF(buf) + 0xFA00): only
   the offset moves and the segment is kept, the form the original shows (1C0E:067D). */
/*@ 1C0E:0578 */ long FileWrite(int index, char far *buf, unsigned long len)
{
    int handle;
    unsigned long total;
    long pos;
    long remaining;
    unsigned n;

    handle = gFileHandles[index];
    if (handle == SLOT_FREE)
        FatalError(6, 0);
    if (handle & FILE_IN_MEMORY)
        FatalError(9, 0);
    if (handle & FILE_LINKED) {
        pos = lseek(handle & HANDLE_MASK, 0, 1);
        pos -= gSlotOffsets[index];
        remaining = gSlotLengths[index] - pos;
        if (len > remaining)
            len = remaining;
    }
    handle &= HANDLE_MASK;
    total = 0;
    while (len > 0xfa00) {
        buf = f1BA8_0072(buf);
        n = write(handle, buf, 0xfa00);
        if (n != 0xfa00)
            return 0;
        len -= 0xfa00;
        total += 0xfa00;
        buf = MK_FP(FP_SEG(buf), FP_OFF(buf) + 0xfa00);
    }
    buf = f1BA8_0072(buf);
    n = write(handle, buf, len);
    if (n != len)
        return 0;
    return total + len;
}
/* 1C0E:06E5: move the read/write position of the file in cache slot `index`. A file loaded in
   memory has its slot position set (SEEK_SET clamps to the size, SEEK_CUR and SEEK_END add `offset`
   and clamp to [0, size]) and the new position is returned; an open file lseeks, shifting `offset`
   by the record's start and length for a linked record, and returns the result. `whence` 0, 1 and 2
   are the runtime's SEEK_SET, SEEK_CUR and SEEK_END; any other reports code 8. The in-memory switch
   lists SEEK_END before SEEK_CUR so its body falls through into the SEEK_CUR clamp, and the
   non-memory branch passes `whence` itself as lseek's third argument, which is why the original
   pushes [bp+0xc] instead of a literal 0/1/2 (1C0E:0870, :0890, :08AF). */
/*@ 1C0E:06E5 */ long SeekFile(int index, long offset, int whence)
{
    register int handle;
    int record;
    long pos;
    long result;

    handle = gFileHandles[index];
    if (handle == SLOT_FREE)
        FatalError(7, 0);
    if (handle & FILE_IN_MEMORY) {
        record = handle & HANDLE_MASK;
        pos = gFiles[record].pos;
        switch (whence) {
        case 0:
            if (gFiles[record].size < offset)
                pos = gFiles[record].size;
            else
                pos = offset;
            break;
        case 2:
            pos = gFiles[record].size;
            /* fall through: SEEK_END is the size, then the SEEK_CUR clamp below */
        case 1:
            if (pos + offset > gFiles[record].size)
                pos = gFiles[record].size;
            else if (pos + offset < 0)
                pos = 0;
            else
                pos += offset;
            break;
        default:
            FatalError(8, 0);
            break;
        }
        gFiles[record].pos = pos;
        return pos;
    }
    switch (whence) {
    case 0:
        if (handle & FILE_LINKED)
            offset += gSlotOffsets[index];
        result = lseek(handle & HANDLE_MASK, offset, whence);
        break;
    case 1:
        result = lseek(handle & HANDLE_MASK, offset, whence);
        break;
    case 2:
        if (handle & FILE_LINKED) {
            offset -= gSlotOffsets[index];
            offset += gSlotLengths[index];
        }
        result = lseek(handle & HANDLE_MASK, offset, whence);
        break;
    default:
        FatalError(8, 0);
        break;
    }
    return result;
}
/* 1C0E:091B: locate and verify the file `name` (upper cased in place) without caching it: follow a
   negative link in the slot's record to the name of the entry it aliases, find the file on one of
   the drives (1C0E:1310), open it and, for a positively linked record (link > 0), check its length
   against the record (a zero length in the record is always accepted); a wrong length reports code
   0xE, a missing file (open failed) reports 0xF. Returns 1 when the file is present and its length
   is right. */
/*@ 1C0E:091B */ int CheckFile(char far *name)
{
    int index = -1;
    int fd;

    name = strupr(name);
    index = FindFile(name);
    if (index != -1 && gFiles[index].link < 0)
        name = gFiles[gFiles[index].link & LINK_INDEX].name;
    if (!LocateOverlayFile(name))
        return 0;
    fd = open(name, O_RDONLY | O_BINARY);
    if (fd == -1)
        FatalError(0xf, name);
    if (index == -1) {
        close(fd);
        return 1;
    }
    if (gFiles[index].link > 0) {
        if (filelength(fd) != gFiles[index].size && gFiles[index].size != 0) {
            close(fd);
            FatalError(0xe, name);
            return 0;
        }
    }
    close(fd);
    return 1;
}
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
    fd = open(name, O_CREAT | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (fd == -1)
        return 0;
    close(fd);
    return 1;
}
/* 1C0E:0ADD: the file error dialog. It hides the mouse pointer, saves the status-line flag, the
   window region and the current font, switches to the 8x8 font gFE5A and the region g80FE, and
   frames the window with four 0xf lines through gDrawLine. It shows the message gErrorText[code]
   (the array of FatalError texts at ds:8196); when `code` is 2 ("Could not find") it also shows the
   file `name`, wrapped in gErrorText[18] ("\r "), the "Insert disk " and "into any drive." literals
   and, when ResolveFileLink(name) is not -1, its value as a disk digit (0x31 + the link). It waits
   for a key; ESC clears the box, shows gErrorText[12] ("A fatal disk error has occurred. Now
   returning to DOS.") and shuts the game down. Afterwards the saved attributes, region, font and
   status flag are restored, the saved screen rectangle is written back (gRestoreRect) and the
   buffer is freed. Returns 1. `code` is a FatalError code (1C0E:00AA) or a RetryFile mode
   (1C0E:0046): the handlers at ds:8104 and ds:8111 are initialised to 0D9B and to this function. */
/*@ 1C0E:0ADD */ int DisplayFileError(int code, char far *name)
{
    int saved811F;
    int savedAttr1;
    int savedAttr2;
    int key;
    int width;
    int height;
    int right;
    int bottom;
    unsigned bufSize;
    char far *title;
    char far *savedFont;
    char text[3];
    int saved8115;
    int x, y;

    bufSize = 0;
    LockHidePointer();
    saved8115 = g8115;
    g8115 = 0;
    title = gErrorText[code];
    saved811F = g811F;
    savedFont = gFont1A47;
    f1A47_0169(gFE5A);
    SetTextWindow(g80FE);
    x = gTextLeft * 8;
    y = gTextTop;
    width = gTextWidth * 8;
    height = gTextHeight;
    right = x + width - 1;
    bottom = y + height - 1;
    if (gMsgBuf == 0) {
        bufSize = g80C6(gTextWidth, height);
        gMsgBuf = MemAlloc(bufSize, 0);
    }
    gSaveRect(gTextLeft, gTextTop, gTextWidth, height, gMsgBuf);
    ClearTextRegion();
    gDrawLine(x, y, right, y, 0xf);
    gDrawLine(x, bottom, right, bottom, 0xf);
    gDrawLine(x, y, x, bottom, 0xf);
    gDrawLine(right, y, right, bottom, 0xf);
    ExpandTextWindow(-1);
    gMsgLink = ResolveFileLink(name);
    if (code == 2) {
        savedAttr1 = gTextAttr1;
        savedAttr2 = gTextAttr2;
        gTextAttr1 = 0xf;
        gTextAttr2 = 0xc;
        DrawText(title);
        DrawText(gErrorText[18]);
        DrawText(name);
        DrawText(gErrorText[18]);
        DrawText("Insert disk ");
        if (gMsgLink != -1) {
            text[0] = (char)gMsgLink + 0x31;
            text[1] = ' ';
            text[2] = 0;
            DrawText(text);
        }
        DrawText("into any drive.");
    } else {
        savedAttr1 = gTextAttr1;
        savedAttr2 = gTextAttr2;
        gTextAttr1 = 0xf;
        gTextAttr2 = 0xc;
        DrawText(title);
    }
    FlushKeyBuffer();
    key = WaitGameKey();
    if (key == 0x1b) {
        ClearTextRegion();
        DrawText(gErrorText[12]);
        WaitGameKey();
        ShutdownGame();
    }
    gTextAttr1 = savedAttr1;
    gTextAttr2 = savedAttr2;
    ExpandTextWindow(1);
    gRestoreRect(gTextLeft, gTextTop, gTextWidth, height, gMsgBuf);
    SetTextWindow(saved811F);
    f1A47_0169(savedFont);
    if (bufSize != 0) {
        MemFree(gMsgBuf);
        gMsgBuf = 0;
    }
    g8115 = saved8115;
    LockShowPointer();
    return 1;
}


/* 1C0E:0D9B: clear the status-line flag ds:8115 while 1C0E:0ADD draws its message box, then put
   the old value back and return 1. */
/*@ 1C0E:0D9B */ int ShowMessage(int index, char far *name)
{
    int saved = g8115;

    g8115 = 0;
    DisplayFileError(index, name);
    g8115 = saved;
    return 1;
}

/* 1C0E:0DC3: return the index of the loaded slot to evict for a `minSize` byte request: the one
   whose data is in memory, whose size is at least `minSize`, whose flags are at most `maxFlags`
   and whose flags are the smallest of those candidates; -1 when there is none. */
/*@ 1C0E:0DC3 */ int FindEvictableFile(int minSize, unsigned char maxFlags)
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
        if (gFiles[i].size >= minSize) {
            if (gFiles[i].flags <= maxFlags) {
                if (slot == -1)
                    slot = i;
                else if (gFiles[i].flags < gFiles[slot].flags)
                    slot = i;
            }
        }
    }
    return slot;
}

/* 1C0E:0E5B: first retry before the out-of-memory hook (src/E19BC.C): drop the in-memory copy of
   the least important slot that can hold a `size` byte request (1C0E:0DC3) and return 1; return 0
   when no slot qualifies or any slot's handle happens to equal its own index. */
/*@ 1C0E:0E5B */ int EvictFile(unsigned size, unsigned char flags)
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
    if (!blocked && i == 10) {
        MemFree(gFiles[slot].data);
        gFiles[slot].data = 0;
        return 1;
    }
    return 0;
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

/* 1C0E:0F6E: build the overlay cache table: mark every slot free, upper case every entry's name and
   clear its memory copy, counting the entries flagged FLAG_KEEP; then allocate a table of their
   indices and a table of their flag bits, fill them, sort them by flag bits (descending, the index
   table moved in parallel), and for every kept entry check its index and drop it into memory
   through 1C0E:144E. Nothing happens when no entry is flagged. Called by InitOverlayCache. The two
   tables hold `count` entries, so the index allocation is `keep = MemAlloc(count * 2, 0)` (an int
   each, sign-extended) and the sort reads `keep[m]` and `*(keep + m + 1)`, the form the original
   indexes with (base + m + one int, 1C0E:1042). */
/*@ 1C0E:0F6E */ void BuildFileCache(void)
{
    int count;
    int far *keep;
    unsigned char far *order;
    int j;
    int n;
    int k;
    int savedOrder;
    int savedKeep;
    register int i;
    int idx;

    count = 0;
    for (i = 0; i < MAX_FILES; i++)
        gFileHandles[i] = SLOT_FREE;
    for (i = 0; gFiles[i].name[0] != 0; i++) {
        strupr(gFiles[i].name);
        gFiles[i].data = 0;
        if (gFiles[i].flags & FLAG_KEEP)
            count++;
    }
    if (count == 0)
        return;
    keep = MemAlloc(count * 2, 0);   /* 2 bytes per int */
    order = MemAlloc(count, 0);
    for (j = n = 0; j < i; j++)
        if (gFiles[j].flags & FLAG_KEEP) {
            keep[n] = j;
            order[n] = gFiles[j].flags & FLAG_BITS;
            n++;
        }
    for (k = count - 1; k >= 0; k--)
        for (idx = 0; idx < k; idx++)
            if (order[idx] < order[idx + 1]) {
                savedOrder = order[idx];
                savedKeep = keep[idx];
                order[idx] = order[idx + 1];
                keep[idx] = *(keep + idx + 1);
                order[idx + 1] = savedOrder;
                *(keep + idx + 1) = savedKeep;
            }
    for (j = 0; j < count; j++) {
        if (keep[j] < 0 || keep[j] >= i)
            FatalError(0x11, 0);
        LoadFileSlot(keep[j]);
    }
    MemFree(order);
    MemFree(keep);
}

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
    fd = open(name, O_RDONLY | O_BINARY);
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
        fd = open(name, O_RDONLY | O_BINARY);
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
    BuildFileCache();
}

/* 1C0E:13FD: the shutdown half of the overlay cache: free every loaded copy (1C0E:0EE7) and go
   back to the drive the game started on. Called from ShutdownGame (E07C8.C). */
/*@ 1C0E:13FD */ void ShutdownOverlayCache(void)
{
    FreeAllCachedFiles();
    setdisk(gSavedDrive);
}

/* 1C0E:1411: look `name` up; when the slot exists and its flags have 0x40 set, drop the slot
   through 1C0E:144E and return 1, otherwise 0. */
/*@ 1C0E:1411 */ int DropFile(char far *name)
{
    int index = FindFile(name);

    if (index == -1)
        return 0;
    if (gFiles[index].flags & 0x40) {
        LoadFileSlot(index);
        return 1;
    }
    return 0;
}

/* 1C0E:144E: load the file in cache slot `index` into memory. A slot with no size reports code 0x10;
   the copy is allocated (through the free-block hook, evicting another slot first when the largest
   free block is too small), the file is opened (through the aliased entry when the slot's link is
   negative, recording the looked-up record length), its length is checked for a linked record, and
   the bytes are read into the copy; the slot's data pointer is set and 1 is returned. A failed
   allocation, open or read frees the copy and returns 0. DRAFT: every byte matches except the last
   read's success/failure blocks, where the original reloads no far pointer and jumps over the
   failure to a shared `mov ax,1` (1C0E:1603), one byte longer; no source form found yet. */
/*@ 1C0E:144E draft */ int LoadFileSlot(int index)
{
    void far *data;
    struct FileEntry far *entry;
    struct FileEntry far *alias;
    int fd;

    data = 0;
    entry = &gFiles[index];
    if (entry->size <= 0) {
        FatalError(0x10, entry->name);
        return 0;
    }
    if (LargestFreeBlock() >= entry->size)
        data = MemAlloc(entry->size, 0);
    else
        data = 0;
    if (data == 0) {
        if (!EvictFile(entry->size, entry->flags))
            return 0;
        data = MemAlloc(entry->size, 0);
    }
    if (entry->link > 0) {
        fd = open(entry->name, 0x8001, 0x180);
    } else {
        alias = &gFiles[gFiles[index].link & LINK_INDEX];
        fd = open(alias->name, 0x8001, 0x180);
        entry->size = LookupFileEntry(fd, entry->name);
    }
    if (fd == -1) {
        MemFree(data);
        return 0;
    }
    if (entry->link > 0) {
        if (filelength(fd) != entry->size) {
            FatalError(0xe, entry->name);
            return 0;
        }
    }
    if (read(fd, data, entry->size) != entry->size) {
        MemFree(data);
        return 0;
    }
    entry->data = data;
    close(fd);
    return 1;
}

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

/* 1C0E:172E: walk the directory at the start of the open file `handle`: each record is a 4-byte
   offset followed by a NUL-terminated name. Read past the records until `name` matches, read the
   following record's offset, seek back to this record's offset and return the record's length
   (next offset minus this offset), also left in ds:FF02 (length) and ds:FF06 (offset) for the
   caller to store in the slot. */
/*@ 1C0E:172E */ long LookupFileEntry(int handle, char far *name)
{
    char buf[14];
    long offset;
    long next;
    int i;

    handle &= HANDLE_MASK;
    do {
        read(handle, &offset, 4);
        i = 0;
        do {
            read(handle, buf + i, 1);
        } while (buf[i++] != 0);
    } while (strcmp(buf, name) != 0);
    read(handle, &next, 4);
    lseek(handle, offset, 0);
    next -= offset;
    gRecordLength = next;
    gRecordOffset = offset;
    return next;
}
