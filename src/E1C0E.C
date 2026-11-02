/* Unit at EOB.EXE 1C0E:000C (6099 bytes, 31 functions): the overlay file cache. This is the file
   layer the VROOMM overlay manager and the data loaders go through. FileOpen (1C0E:00DB) takes a
   DOS handle slot from a ten entry handle table (gFileHandles at ds:FEEA, MAX_FILES) for a record
   in the file table at ds:01FF, whose records run to the first empty name (203 of them in this
   image; gFiles). FileRead (1C0E:0400) and FileWrite (1C0E:0578) move bytes in and out of the
   record's memory copy or its open handle, FileClose (1C0E:03BB) releases it, and FileOpen
   (1C0E:00DB), IsFileInMemory (1C0E:1161) and FindLoadedFile (1C0E:11D3) look a record up by name;
   only ResolveFileLink (1C0E:168E) follows the link field that groups aliased records.
   1C0E:13E7/:13FD initialise and tear the cache down at startup (E07C8.C), 1C0E:1310 finds the
   named file (any overlay or data file, not the program) on a drive (called from the INT 3Fh hook
   1A72:0006 and CheckFile 1C0E:091B), and 1C0E:0E5B/0DC3 choose a record to evict under a size
   limit. Placeholder DGROUP names mark the fields whose exact role is inferred from this code
   only. */

/*@flags -ml -O -Z -G */

#include <dos.h>

/* Runtime library (Turbo C++ 1.00, large model). */
extern int open(char far *path, int access, ...);   /*@ 0000:6443: runtime OPEN */
extern int close(int fd);                          /*@ 0000:50C5: runtime CLOSE */
extern int read(int fd, void far *buf, unsigned len); /*@ 0000:65E1: runtime READ (handles are opened O_BINARY) */
extern int write(int fd, void far *buf, unsigned len); /*@ 0000:5BFE: runtime WRITE (handles are opened O_BINARY) */
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
extern int (far *gSaveRectSize)(int width, int height);                     /*@ ds:80C6: the save-under buffer size in bytes (E1BB1.ASM) */
extern void far *gFont8x8;                                                  /*@ ds:FE5A: the 8x8 font loaded from Font8.fnt (E07C8.C) */
extern void far *gFont1A47;                                                 /*@ ds:FE4E: the current font (E1A47.C) */

/* DGROUP globals and indirect far functions. */
extern int gStatusLine;                        /*@ ds:8115: nonzero while a status line is up (E082C.C) */
extern int gTextRegion;                        /*@ ds:811F */
extern int gGraphicsChoice;                        /*@ ds:8102: the video mode the game initialised (E07A9.C) */
extern int gTextLeft;                    /*@ ds:FE64: the text window's left edge, 8-pixel columns */
extern int gTextTop;                     /*@ ds:FE66: the text window's top edge, pixel rows */
extern int gTextWidth;                   /*@ ds:FE62: the text window's width, 8-pixel columns */
extern int gTextHeight;                  /*@ ds:FE60: the text window's height, pixel rows */
extern int gTextAttr1;                   /*@ ds:811D */
extern int gTextAttr2;                   /*@ ds:811B */
extern void (far *gFatalHandler)(int code, char far *name); /*@ ds:8104: the fatal file error handler */
extern int (far *gRetryHandler)(int mode, char far *name);  /*@ ds:8111: the retry file error handler */
extern signed char gLastDrive;           /*@ ds:FF00: highest drive FindLastDrive found */
extern int gSavedDrive;                  /*@ ds:FEFE: the drive getdisk returned at init */
extern unsigned char gSearchAllDrives;   /*@ ds:81EA: nonzero: 1C0E:1310 also scans other drives */
extern long gRecordLength;               /*@ ds:FF02: 1C0E:172E leaves the record's length here */
extern long gRecordOffset;               /*@ ds:FF06: 1C0E:172E leaves the record's offset here */
extern char far *gErrorText[];           /*@ ds:8196: the file error messages, indexed by FatalError code */
extern void far *gMsgBuf;                /*@ ds:81E6: the saved screen rectangle of the error dialog */
extern int gMsgLink;                     /*@ ds:FF0A: ResolveFileLink's result for the file in the dialog */
extern int g80FE;                        /*@ ds:80FE: the text region the error dialog switches to; 1 in the image */

#define KEY_ESC        0x1b     /* the key the error dialog treats as "abort" */
#define MSG_NOT_FOUND  2        /* gErrorText[2]: "Could not find" (0ADD appends the name) */
#define MSG_DISK_ERROR 12       /* gErrorText[12]: "A fatal disk error has occurred..." */
#define MSG_LINE_BREAK 18       /* gErrorText[18]: "\r " */

#define MAX_FILES      10
#define SLOT_FREE      0x3fff   /* gFileHandles[i]: the slot holds no file */
#define HANDLE_MASK    0x3fff   /* the low bits of a gFileHandles entry are the DOS handle */
#define FILE_IN_MEMORY 0x8000   /* the record's bytes are in gFiles[i].data, not a DOS handle */
#define FILE_LINKED    0x4000   /* the record's name is an alias of another record */
#define FLAG_KEEP      0x80     /* gFiles[i].flags: the record is preloaded at init */
#define FLAG_LOAD      0x40     /* gFiles[i].flags: DropFile loads this record on demand */
#define FLAG_BITS      0x3f     /* the flag bits a caller (EvictFile) may match */
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
   empty name ends the table (203 records in this image). */
struct FileEntry {
    char far *name;             /* +0x00: the file name, upper cased */
    long size;                  /* +0x04: the length on disk; an alias record holds its length inside the PAK until LoadFileSlot replaces it */
    char far *data;             /* +0x08: the in-memory copy when the record is loaded */
    int link;                   /* +0x0C: < 0: LINK_INDEX is the record this aliases; >= 0: a plain file on the disk whose number is link + 1 (DisplayFileError prints link + 1; link 0 is disk 1, as INTRO.EXE and eobdata1.pak have) */
    long pos;                   /* +0x0E: the read/write position in the file */
    signed char flags;          /* +0x12: FLAG_KEEP, FLAG_LOAD and the low FLAG_BITS */
};

extern struct FileEntry gFiles[];        /*@ ds:01FF */
extern int gFileHandles[];               /*@ ds:FEEA: DOS handle or SLOT_FREE per slot */
extern long gSlotOffsets[];              /*@ ds:FEC2: the record's start offset per slot */
extern long gSlotLengths[];              /*@ ds:FF0C: the record's length per slot */

/* 1C0E:000C: resize the file in handle slot `index` to `size` bytes and return 1 when that
   succeeded, 0 when the slot is loaded in memory (nothing to resize) or chsize failed. The packet
   found no direct caller in the EXE; the name rests on the chsize call (0000:5E41). */
/*@ 1C0E:000C */ int SetFileSize(int index, long size)
{
    int handle = gFileHandles[index];

    if (!(handle & FILE_IN_MEMORY))
        return chsize(handle & HANDLE_MASK, size) != -1;
    return 0;
}

/* 1C0E:0046: hand `name` (upper cased; see QUIRK 1C0E:0060) and the error `mode` to the retry
   handler at ds:8111 (the file error dialog, 1C0E:0ADD) and return its result. `mode` is an index
   into gErrorText: 0 "Could not create the file on disk." (FileOpen's ResetFile loop, 1C0E:00DB),
   1 "Illegal MODE in Open_File." (FileOpen's bad-mode branch, 1C0E:00DB), 2 "Could not find"
   (FileOpen's CheckFile loop and its open retry loop, 1C0E:00DB), 0xA "Trying to
   Get_Resident_Index on unknown file." (FindLoadedFile, 1C0E:11D3) and 0xB "You are exiting with
   open files." (FreeAllCachedFiles, 1C0E:0EE7; BuildFileCache, 1C0E:0F6E, calls no RetryFile).
   QUIRK 1C0E:0060: the loop sets `base` to the backslash strchr found and the next strchr(base)
   finds that same backslash, so a name that contains a backslash never leaves the loop; a name
   without one is upper-cased whole (1C0E:0088). No name in the 203 record table holds a backslash,
   so the loop runs once and stops. */
/*@ 1C0E:0046 */ int RetryFile(int mode, char far *name)
{
    char far *base;
    char far *slash;

    slash = base = name;
    while ((slash = strchr(base, '\\')) != 0)
        base = slash;
    strupr(base);
    return gRetryHandler(mode, base);
}

/* 1C0E:00AA: report a file error `code` for `name` through the fatal handler at ds:8104
   (gFatalHandler; first the code itself unless it is already 0xC, then 0xC) and shut the game
   down. `code` indexes gErrorText: 3 "Too many files open.", 4 to 9 the unopened and resident file
   messages, 0xE "File length does not match size listed in FileData[].", 0xF "Internal Error in
   File_Exists()." and 0x11 "Failure in sort code for resident files." */
/*@ 1C0E:00AA */ void FatalError(int code, char far *name)
{
    if (code != 0xc)
        gFatalHandler(code, name);
    gFatalHandler(0xc, name);
    ShutdownGame();
}

/* 1C0E:00DB: open the file `name` in one of three modes and give it a DOS handle slot. Mode 1
   (O_BINARY | O_RDONLY) reuses an in-memory copy or checks and loads the file from disk; mode 2
   (O_BINARY | O_WRONLY, write) deletes the file and recreates it (DeleteFile then ResetFile), mode
   3 (O_BINARY | O_RDWR, read/write) does not delete it; both then run ResetFile, retrying through
   RetryFile mode 0, while CheckFile fails; any other mode reports RetryFile mode 1 ("Illegal MODE
   in Open_File.") then a fatal code 0xC. An existing in-memory copy is dropped unless mode is 1,
   the file is located (following the alias link when its record points at another entry), opened
   with retry, and its directory entry recorded for an aliased record (LookupFileEntry). Finally
   the first free handle slot takes the handle (FILE_IN_MEMORY | the record index for a loaded
   file, else the DOS handle with FILE_LINKED set) with the record's offset and length, and its
   index is returned. The local `fd` holds a DOS handle, or a record index once FILE_IN_MEMORY is
   set; the local `slot` holds a record index in the drop branch and a handle slot index from the
   free-slot search on. */
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
        access = O_BINARY | O_RDONLY;
        break;
    case 2:
        access = O_BINARY | O_WRONLY;
        DeleteFile(name);
        ResetFile(name);
        /* fall through */
    case 3:
        if (!CheckFile(name))
            while (!ResetFile(name))
                if (!RetryFile(0, name))
                    return -1;
        if (mode == 3)
            access = O_BINARY | O_RDWR;
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

/* 1C0E:0400: read up to `len` bytes from the file in handle slot `index` into `buf` and return the
   number of bytes read (0 when a read failed). A file loaded in memory is copied through
   1C0E:1240; an aliased record (FILE_LINKED) is clamped to what is left of the record from the
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
/* 1C0E:0578: write up to `len` bytes from `buf` to the file in handle slot `index` and return the
   number of bytes written (0 on a short write). A file loaded in memory cannot be written (code 9);
   an aliased record (FILE_LINKED) is clamped to what is left of the record from the current
   position. The bytes are written in 0xFA00 byte chunks, each buffer normalised through 1BA8:0072,
   and after every full chunk `buf` is advanced by 0xFA00 with MK_FP(FP_SEG(buf), FP_OFF(buf) +
   0xFA00): only the offset moves and the segment is kept, the form the original shows (1C0E:067D). */
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
/* 1C0E:06E5: move the read/write position of the file in handle slot `index`. A file loaded in
   memory has its record position set (SEEK_SET clamps to the size, SEEK_CUR and SEEK_END add
   `offset` and clamp to [0, size]) and the new position is returned; an open file lseeks and
   returns the result, shifting `offset` for an aliased record: SEEK_SET adds the record's start,
   SEEK_END subtracts the start and adds the length, SEEK_CUR does not shift. `whence` 0, 1 and 2
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
   negative link in the record to the name of the entry it aliases, find the file on one of the
   drives (1C0E:1310), open it and, for a plain file (link > 0), check its length against the
   record (a zero length in the record is always accepted); a wrong length reports code 0xE. A file
   that LocateOverlayFile cannot find makes this function return 0 with no code, so the caller
   retries or gives up; only a file that was located and then fails to open reports 0xF ("Internal
   Error in File_Exists()"). Returns 1 when the file is present and its length is right. */
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
/* 1C0E:0A35: delete the file `name`: free the record's in-memory copy first, then remove it from
   disk. Returns 1 when the file was removed (remove returned 0). Called by 1C0E:00DB and by
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
   window region and the current font, switches to the 8x8 font gFont8x8 and the region g80FE, and
   frames the window with four 0xf lines through gDrawLine. It shows the message gErrorText[code]
   (the array of FatalError texts at ds:8196); when `code` is MSG_NOT_FOUND ("Could not find") it
   also shows the file `name`, wrapped in MSG_LINE_BREAK, the "Insert disk " and "into any drive."
   literals and, when ResolveFileLink(name) is not -1, its value as a disk digit ('1' + the link).
   It waits for a key; ESC clears the box, shows MSG_DISK_ERROR ("A fatal disk error has occurred.
   Now returning to DOS.") and shuts the game down. Afterwards the saved attributes, region, font
   and status flag are restored and the saved screen rectangle is written back (gRestoreRect); the
   buffer is freed and gMsgBuf cleared only when this call allocated it (bufSize != 0), a buffer
   set on entry is kept. Returns 1. `code` is a FatalError code (1C0E:00AA) or a RetryFile mode
   (1C0E:0046): the handlers at ds:8104 and ds:8111 are initialised to 0D9B and to this function. */
/*@ 1C0E:0ADD */ int DisplayFileError(int code, char far *name)
{
    int savedTextRegion;
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
    int savedStatusLine;
    int x, y;

    bufSize = 0;
    LockHidePointer();
    savedStatusLine = gStatusLine;
    gStatusLine = 0;
    title = gErrorText[code];
    savedTextRegion = gTextRegion;
    savedFont = gFont1A47;
    f1A47_0169(gFont8x8);
    SetTextWindow(g80FE);
    x = gTextLeft * 8;
    y = gTextTop;
    width = gTextWidth * 8;
    height = gTextHeight;
    right = x + width - 1;
    bottom = y + height - 1;
    if (gMsgBuf == 0) {
        bufSize = gSaveRectSize(gTextWidth, height);
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
    if (code == MSG_NOT_FOUND) {
        savedAttr1 = gTextAttr1;
        savedAttr2 = gTextAttr2;
        gTextAttr1 = 0xf;
        gTextAttr2 = 0xc;
        DrawText(title);
        DrawText(gErrorText[MSG_LINE_BREAK]);
        DrawText(name);
        DrawText(gErrorText[MSG_LINE_BREAK]);
        DrawText("Insert disk ");
        if (gMsgLink != -1) {
            text[0] = (char)gMsgLink + '1';
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
    if (key == KEY_ESC) {
        ClearTextRegion();
        DrawText(gErrorText[MSG_DISK_ERROR]);
        WaitGameKey();
        ShutdownGame();
    }
    gTextAttr1 = savedAttr1;
    gTextAttr2 = savedAttr2;
    ExpandTextWindow(1);
    gRestoreRect(gTextLeft, gTextTop, gTextWidth, height, gMsgBuf);
    SetTextWindow(savedTextRegion);
    f1A47_0169(savedFont);
    if (bufSize != 0) {
        MemFree(gMsgBuf);
        gMsgBuf = 0;
    }
    gStatusLine = savedStatusLine;
    LockShowPointer();
    return 1;
}


/* 1C0E:0D9B: clear the status-line flag ds:8115 while 1C0E:0ADD draws its message box, then put
   the old value back and return 1. This is the fatal file error handler stored at ds:8104
   (gFatalHandler; the image holds 1C0E:0D9B there, file offset 2BBE4); `code` is the FatalError
   code, the index into gErrorText (1C0E:00AA). */
/*@ 1C0E:0D9B */ int ShowMessage(int code, char far *name)
{
    int saved = gStatusLine;

    gStatusLine = 0;
    DisplayFileError(code, name);
    gStatusLine = saved;
    return 1;
}

/* 1C0E:0DC3: return the index of the in-memory record to evict for a `minSize` byte request: the
   one whose data is in memory, whose size is at least `minSize`, whose flags are at most
   `maxFlags` and whose flags are the smallest of those candidates; -1 when there is none. `flags`
   is a signed char, so a record with FLAG_KEEP (0x80) compares negative and is always a candidate
   and the smallest pick; no record in this image has the bit. */
/*@ 1C0E:0DC3 */ int FindEvictableFile(int minSize, unsigned char maxFlags)
{
    int count;
    int index = -1;
    int i = 0;

    count = 0;
    while (gFiles[count].name[0] != 0)
        count++;
    for (i = 0; i < count; i++) {
        if (gFiles[i].data == 0)
            continue;
        if (gFiles[i].size >= minSize) {
            if (gFiles[i].flags <= maxFlags) {
                if (index == -1)
                    index = i;
                else if (gFiles[i].flags < gFiles[index].flags)
                    index = i;
            }
        }
    }
    return index;
}

/* 1C0E:0E5B: first retry before the out-of-memory hook (src/E19BC.C): free the in-memory copy of
   the least important record that can hold a `size` byte request (1C0E:0DC3) and return 1; return
   0 when no record qualifies or the blocking test below passes. `size` is `unsigned`, so only the
   low word of the record's long size arrives and `flags` is one byte.
   QUIRK 1C0E:14EB: the caller pushes only [bx+4], the low word of entry->size, so a file over 64
   KB (INTRO.EXE, 88000 bytes) is seen as its size modulo 65536.
   Purpose unknown: the loop that sets `blocked` when any handle slot's handle equals its own index
   ((gFileHandles[i] & HANDLE_MASK) == i); it does not look at the victim and nothing says why it
   should block. */
/*@ 1C0E:0E5B */ int EvictFile(unsigned size, unsigned char flags)
{
    int i = 0;
    int index;
    char blocked;

    index = FindEvictableFile(size, flags);
    if (index == -1)
        return 0;
    blocked = 0;
    for (i = 0; i < 10; i++)
        if ((gFileHandles[i] & HANDLE_MASK) == i) {
            blocked = 1;
            i = 10;
        }
    if (!blocked && i == 10) {
        MemFree(gFiles[index].data);
        gFiles[index].data = 0;
        return 1;
    }
    return 0;
}

/* 1C0E:0EE7: free every record's in-memory copy and, for every handle slot still open, run the
   retry handler (gRetryHandler) with mode 0xB. Called by 1C0E:13FD on shutdown.
   QUIRK 1C0E:0EE7: the RetryFile(0xb, 0) call passes a null name, so RetryFile runs strchr and
   strupr on 0000:0000, the interrupt vector table: strupr writes back only the bytes that are
   lowercase letters (the store at 0000:687F, reached only for a to z), and a backslash among those
   bytes would make its loop (QUIRK 1C0E:0060) never end. */
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

/* 1C0E:0F6E: build the file cache table: mark every handle slot free, then upper case every
   record's name and clear its memory copy, counting the records flagged FLAG_KEEP. When none is
   flagged, return after those two loops (the names and copies are still cleared). Otherwise
   allocate a table of their indices and a table of their flag bits, fill them, sort them by flag
   bits (descending, the index table moved in parallel), and for every kept record check its index
   and load it into memory through 1C0E:144E. Called by InitOverlayCache. The two tables hold
   `count` entries, so the index allocation is `keep = MemAlloc(count * 2, 0)` (an int each,
   sign-extended) and the sort reads `keep[m]` and `*(keep + m + 1)`; the original indexes with one
   `les bx, [bp-6]` (1C0E:1041), an `add bx, ax` (1C0E:1044) and a store (1C0E:1049). */
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

/* 1C0E:11D3: return the index of the record named `name` (upper cased in place); when no record
   matches, report it through the retry handler (mode 0xA) and return 0.
   QUIRK 1C0E:11D3: the 0 it returns on a miss is a real record index, so FileOpen (1C0E:00DB)
   then frees and uses record 0. */
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

/* 1C0E:1240: copy up to `len` bytes from the in-memory copy of record `index` (at its position)
   into `dst` through Unpack0's raw copy, clamp `len` to what is left in the file, advance the
   position and return the number of bytes copied. The source pointer is normalized through
   1BA8:0072 first. */
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

/* 1C0E:1310: locate the file `name` (upper cased in place) on disk, any overlay or data file.
   Try the current drive first, then (only when gSearchAllDrives at ds:81EA asks for it) the drives
   from gLastDrive at ds:FF00 down to A, skipping the drives IsLastLogicalDrive (1A19:003B)
   rejects (that skip is the rule of 1A19:003B, see its comment in src/E1A19.ASM); return the drive
   number plus one on success, 0 when the file is nowhere. Called from the INT 3Fh handler
   (1A72:0006) and from CheckFile (1C0E:091B). */
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

/* 1C0E:13BB: grow the text window outward by `cells` cells on every edge (a negative `cells`
   shrinks it back). Left and width (ds:FE64, ds:FE62) are in 8-pixel columns, so left moves out by
   `cells` columns and width grows by 2 * `cells` columns; top (ds:FE66) moves up by `cells * 8`
   pixel rows and height (ds:FE60) grows by `cells * 16` pixel rows per cell. */
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

/* 1C0E:1411: look `name` up; when the record exists and its flags have FLAG_LOAD (0x40) set,
   load it into memory through LoadFileSlot (1C0E:144E) and return 1, otherwise 0. */
/*@ 1C0E:1411 */ int DropFile(char far *name)
{
    int index = FindFile(name);

    if (index == -1)
        return 0;
    if (gFiles[index].flags & FLAG_LOAD) {
        LoadFileSlot(index);
        return 1;
    }
    return 0;
}

/* 1C0E:144E: load the record `index` of gFiles into memory. A record with no size reports code
   0x10; the copy is allocated (through the free-block hook, evicting another record first when the
   largest free block is too small), the file is opened (through the aliased record when link <= 0,
   recording the looked-up record length), its length is checked for a plain file (link > 0), and
   the bytes are read into the copy; the record's data pointer is set and 1 is returned. A failed
   first allocation has no copy to free (`data` is 0) and returns 0 only when EvictFile
   (1C0E:0E5B) also fails, freeing nothing; only a failed open (1C0E:15A0) or read (1C0E:162B)
   frees the copy and returns 0. The size-mismatch path (FatalError(0xe), 1C0E:15E2) also returns 0
   without freeing, which is consistent because FatalError never returns: it shuts the game down
   through ShutdownGame, whose body ends in exit(0) (src/E07C8.C:478).
   QUIRK 1C0E:144E: the own-file test is `entry->link > 0` (1C0E:1520, `jle` at 1C0E:1525), so
   every link <= 0 takes the alias branch: the alias is `gFiles[gFiles[index].link & LINK_INDEX]`
   (the link load at 1C0E:154C, the mask at 1C0E:1550), so with link 0 it is record 0 for every
   record and the open is of record 0's name (only record 0, INTRO.EXE, is its own alias). FileOpen
   and CheckFile test `link < 0` for an alias, so the two tests disagree.
   QUIRK 1C0E:144E: after EvictFile returns 1 the second `MemAlloc(entry->size, 0)` (1C0E:1508) is
   not tested, so a NULL copy goes on to open and read.
   The evict and final read are written as success-if/else-failure blocks so that the original's
   branch polarity and the tail `mov ax, 1` (1C0E:163D, reached by the success path's `jmp 0x163d`
   at 1C0E:1629) come out. */
/*@ 1C0E:144E */ int LoadFileSlot(int index)
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
        if (EvictFile(entry->size, entry->flags))
            data = MemAlloc(entry->size, 0);
        else
            return 0;
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
    if (read(fd, data, entry->size) == entry->size) {
        entry->data = data;
        close(fd);
    } else {
        MemFree(data);
        return 0;
    }
    return 1;
}

/* 1C0E:1645: return the index of the record named `name` exactly, or -1. */
/*@ 1C0E:1645 */ int FindFile(char far *name)
{
    int i;

    for (i = 0; gFiles[i].name[0] != 0; i++)
        if (strcmp(gFiles[i].name, name) == 0)
            return i;
    return -1;
}

/* 1C0E:168E: find the record named `name` (upper cased in place), follow its chain of aliases
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
   offset followed by a NUL-terminated name (the format is established in WHITEPAPER 4.1). Read
   past the records until `name` matches, read the following record's offset, seek back to this
   record's offset and return the record's length (next offset minus this offset), also left in
   ds:FF02 (length) and ds:FF06 (offset) for the caller to store in the handle slot. */
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
