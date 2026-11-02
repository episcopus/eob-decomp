/* Unit at EOB.EXE 177D:000A (646 bytes, 4 functions): the sound driver loader and the routines
   that play a game sound through it. LoadSoundDriver (177D:000A) opens adlib.dat or pcgamesn.dat
   by gSoundCard (ds:B892), reads the 120-byte game-number to driver-number map into gSoundMap
   (ds:DCAC, a far pointer to the 120-byte map StartupAlloc allocates, src/E07A9.C:216) and the
   driver's 32C8h data block into segment 0000 (_TEXT) at offset 0416 (f0_03AA, 0000:03AA, returns
   cs:0416; f0_03FA, 0000:03FA, adds the 0416 base to the block's 300-word table). PlaySound
   (177D:0070) plays the driver sound a game number maps to; PlaySoundAtCell (177D:0106) on AdLib
   retunes and restarts the sound recorded by the previous call, at the attenuation that call
   recorded, and records the new sound and cell only for a later call; PlaySoundDefault (177D:00F3)
   passes the party's map cell gPartyCell (ds:FBCA) as the sound's cell. A call's attenuation is the
   distance between gPartyCell and that cell, Distance(gPartyCell, cell) (177D:0261 to 0279). gDrvReg
   (0000:3B36) is the driver's busy countdown: set to 0Bh after a start and counted down by the
   driver's timer; the AdLib path waits for it to reach 0 before it writes registers. Whitepaper
   2.7 (AdLib) and 4.4 (ADLIB.DAT / PCGAMESN.DAT). Compiled -ml -O -Z. */

/* kept for the unit convention (src/E07C8.C:10, src/E082C.C:12) */
#include <dos.h>

/* Meaningful constants of this unit. */
/* the length read for the driver block (at most this many bytes, whitepaper 4.4); the FileRead
   length at 177D:004C */
#define DRIVER_BLOCK_SIZE 0x32C8L
/* the length of the 120-byte sound map StartupAlloc allocates (src/E07A9.C:216); the FileRead
   length at 177D:004C */
#define SOUND_MAP_SIZE 120L
/* a game-sound map value from here up is ignored (whitepaper 4.4, 177D:008C) */
#define SOUND_MAP_IGNORED 0x96
/* the low seven bits of the driver sound, the speaker driver routine index (177D:00D2, 177D:0240) */
#define SPEAKER_ROUTINE_MASK 0x7f
/* the recorded attenuation gCurAttenuation from here up skips the AdLib play (177D:012B) */
#define ATTENUATION_ADLIB_LIMIT 0x10
/* the level register's full attenuation (whitepaper 2.7 "40h to 55h = 3Fh", 177D:01AF) */
#define LEVEL_FULL_ATTENUATION 0x3f
/* Purpose unknown: 177D:01DA writes driver register 1 with the value 20h - gCurAttenuation; what
   that value means is not known */
#define DRIVER_REG1_BASE 0x20
/* Purpose unknown: the value stored in the busy byte gDrvReg after a start (177D:0201). The INT 08h
   routine at 0000:3E66 (installed by 0000:3F77: mov dx,3E66h; int 21h 2508h at 0000:3F97 to 3F9D)
   counts gDrvReg down (0000:3EAF to 3EB6: cmp [3B36], 0; je; dec byte [3B36]), so 0Bh is a number of
   timer passes the program waits out */
#define DRIVER_BUSY 0x0b

extern unsigned char far *gSoundMap; /*@ ds:DCAC */  /* a far pointer to the 120-byte u8 game-number
                                                        to driver-number map that StartupAlloc
                                                        allocates (gSoundMap = MemAlloc(120L, 0),
                                                        src/E07A9.C:216); read by LoadSoundDriver
                                                        (177D:000A) and indexed by PlaySound
                                                        (177D:0070) and PlaySoundAtCell
                                                        (177D:0283); whitepaper 4.4 */
extern int gSoundOn; /*@ ds:80E6 */  /* the sound-on flag from argv[2] (whitepaper 2.6); PlaySound and
                                        PlaySoundAtCell return at once when it is zero */
extern unsigned char gSoundCard; /*@ ds:B892 */  /* the sound choice from argv[2] minus '1' (whitepaper
                                                    2.6): 0 opens adlib.dat (the AdLib path), nonzero
                                                    pcgamesn.dat; 1 is the PC speaker and 2 the Tandy
                                                    speaker, from the whitepaper 2.4 menu order (AdLib or
                                                    Sound Blaster, PC speaker, Tandy speaker) and the PC
                                                    speaker / Tandy driver at src/E1948.ASM:1-2, with the
                                                    Tandy path at src/E07C8.C:306-307. The value is 0..2;
                                                    src/E07C8.C:28 declares it char */
extern unsigned char gSpeakerMode; /*@ ds:009E */  /* 1 only on the Tandy path (src/E07C8.C:307), where
                                                      the speaker path of PlaySound/PlaySoundAtCell drives
                                                      gTandyOn; Purpose unknown beyond that */
extern unsigned char gTandyOn; /*@ ds:8A67 */  /* the PC speaker / Tandy driver select applied by
                                                    1948:0458 (E1948.ASM: 1 is Tandy, 0 is PC speaker);
                                                    PlaySound and PlaySoundAtCell set it to 1, and
                                                    their 0 stores (177D:00B3, 177D:021E) are dead:
                                                    both sit in the `driverSound < 0` / `gCurSound < 0`
                                                    blocks the QUIRKs name. The live 0 writer is
                                                    ShutdownGame (src/E07C8.C:329). The name stays
                                                    (owner's call): the value selects the driver */
extern int gCurSound; /*@ ds:5E22 */  /* the driver sound number recorded by the previous call of
                                          PlaySoundAtCell; nonzero guards the retune, and 177D:0106
                                          restarts this recorded number, not the new one */
extern int gCurAttenuation; /*@ ds:5E24 */  /* the attenuation of gCurSound: a larger value is
                                          quieter (the level fades toward 3Fh, 177D:01AF to 01C7),
                                          zero means no fade but the sound still plays (177D:01A8
                                          skips only the fade); 10h or more skips the AdLib play
                                          (177D:012B). From Distance(gPartyCell, cell) at
                                          177D:026E, not bounded here; used only in this file */
extern int gPrevSound; /*@ ds:5E26 */  /* the sound whose driver registers gSoundLevel and gSoundReg1 hold,
                                          restored through 0000:03B0 before the next sound */
extern int gSoundLevel; /*@ ds:5E28 */  /* the value read from register 3 of the recorded sound
                                           gCurSound (177D:0170 to 017F, again at 0194 to 01A3);
                                           written back to register 3 of gPrevSound on the next call
                                           (177D:0142 to 014E through 0000:03B0) */
extern int gSoundReg1; /*@ ds:5E2A */  /* the value read from register 1 of the recorded sound
                                         gCurSound (177D:0182 to 0191), written back to register 1
                                         of gPrevSound on the next call (177D:0156 to 0167 through
                                         0000:03B0); Purpose unknown: 177D:01DA writes register 1
                                         with 20h - gCurAttenuation, and what that value means is
                                         not known; the callers do not make this known either */
extern int gPartyCell; /*@ ds:FBCA */  /* the party's map cell: PlaySoundDefault (177D:00F3) passes
                                          it as the sound's cell, and PlaySoundAtCell passes it with
                                          that cell to Distance (17A6:0F61; 177D:026A pushes it,
                                          177D:026E is the call). ResetMapScreen sets it to 0x1EA
                                          (src/E0ACD.C:939); objects and monsters are measured from
                                          and compared with it (src/E08E6.C:221, :336, :467, :540;
                                          src/EOV6.C:410) and it indexes the nine-byte cell records
                                          (src/E0D1C.C:352, :358) */

extern unsigned char far gDrvReg; /*@ 0000:3B36 */  /* the driver's busy countdown: set to DRIVER_BUSY
                                                       after a start and counted down by the driver's
                                                       timer; the AdLib path waits for it to reach 0
                                                       before it writes registers. Not volatile: the
                                                       timer changes it, but volatile cross-jumps
                                                       elsewhere (src/E1414.C:261) */

/* FileOpen returns the cache slot index (not a DOS handle), 1C0E:00DB */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern long FileRead(int index, char far *buf, unsigned long len); /*@ 1C0E:0400 */  /* the returned
                                                                                       byte count is
                                                                                       not used here */
extern void FileClose(int index); /*@ 1C0E:03BB */
extern void far *f0_03AA(void); /*@ 0000:03AA */ /* returns the block's address, dx:ax = cs:0416
                                                    (0000:03AA to 03AF) */
/* writes the byte `value` at driver register `reg` of driver sound `idx`: `idx` indexes the
   block's 300-word table at 0000:0416 and `reg` offsets it (0000:03B7 to 03C4) */
extern void f0_03B0(int idx, int reg, int value); /*@ 0000:03B0 */
/* reads that byte back from the same place, zero-extended (0000:03CA to 03E2) */
extern int f0_03CA(int idx, int reg); /*@ 0000:03CA */
/* returns the driver routine pointer for sound `idx`, the far pointer cs:[0416 + 2*idx]
   (0000:03E3 to 03F8) */
extern void far *f0_03E3(int idx); /*@ 0000:03E3 */
/* makes the block's 300-word table absolute: adds 0416 to each of the words at 0000:0416
   (cx = 12Ch, 0000:03FD to 0410) */
extern void f0_03FA(void); /*@ 0000:03FA */
/* queues `sound` for the driver: appends it to a 16-entry ring (head at 0000:3B7F, index masked
   with 0Fh, 0000:400C to 4023) */
extern void f0_400C(int sound); /*@ 0000:400C */
extern void StartSound(void far *routine); /*@ 1948:0418 */
extern void SoundDriverResume(void); /*@ 1948:0458 */
extern int Distance(int x, int y); /*@ 17A6:0F61 */

/* forward declaration: PlaySoundDefault (177D:00F3) calls it before its definition below */
void PlaySoundAtCell(int sound, int cell);

/* 177D:000A: opens the driver file chosen by gSoundCard (adlib.dat when 0, pcgamesn.dat
   otherwise), reads its 120-byte sound map into gSoundMap and its 32C8h data block into segment
   0000 (_TEXT) at offset 0416 (f0_03AA returns cs:0416), closes the file and fixes the block's
   300-word table through f0_03FA. Called by StartupAlloc at 07A9:00D2. The slot from FileOpen
   is not checked: the data file is assumed to exist. */
/*@ 177D:000A */ void LoadSoundDriver(void)
{
    int slot;

    /* FileOpen mode 1 is read (0x8001, src/E1C0E.C:166) */
    if (gSoundCard == 0)
        slot = FileOpen("adlib.dat", 1);
    else
        slot = FileOpen("pcgamesn.dat", 1);
    FileRead(slot, gSoundMap, SOUND_MAP_SIZE);
    FileRead(slot, f0_03AA(), DRIVER_BLOCK_SIZE);
    FileClose(slot);
    f0_03FA();
}

/* 177D:0070: plays the game sound numbered `sound`: its driver number comes from gSoundMap
   (unsigned char, so 0..0FFh) and a value of 96h or more is ignored. On AdLib (gSoundCard 0) the
   driver number is handed to 0000:400C. On the other cards (1 PC speaker, 2 Tandy speaker) the low
   seven bits select a driver routine through 0000:03E3 and start it through 1948:0418; when
   gSpeakerMode is 1 (the Tandy path, src/E07C8.C:307) a nonzero number sets gTandyOn to 1 and
   resumes the driver through 1948:0458, but only when gTandyOn is currently 0 (177D:00C1 to 00CD;
   1 is Tandy, 0 the PC speaker, E1948.ASM:7). QUIRK
   177D:00A8: the `driverSound < 0` test (the `gTandyOn = 0` block) is dead: the map byte is
   zero-extended into driverSound (`mov ah, 0` at 177D:0088), so driverSound is 0..0FFh; nothing here
   ever clears gTandyOn, and the only 0 store in this unit (177D:00B3, in PlaySound) is dead, inside
   that block. Does nothing when sound is off (gSoundOn). Called by many game routines to make a sound
   effect. */
/*@ 177D:0070 */ void PlaySound(int sound)
{
    int driverSound;
    void far *routine;

    if (!gSoundOn)
        return;
    driverSound = gSoundMap[sound];
    if (driverSound < SOUND_MAP_IGNORED) {
        if (gSoundCard == 0) {
            f0_400C(driverSound);
            return;
        }
        if (gSpeakerMode == 1) {
            if (driverSound < 0 && gTandyOn) {
                gTandyOn = 0;
                SoundDriverResume();
            }
            if (driverSound > 0 && gTandyOn == 0) {
                gTandyOn = 1;
                SoundDriverResume();
            }
        }
        driverSound &= SPEAKER_ROUTINE_MASK;
        routine = f0_03E3(driverSound);
        StartSound(routine);
    }
}

/* 177D:00F3: passes the game sound numbered `sound` to PlaySoundAtCell with the party's map cell
   gPartyCell (ds:FBCA) as its cell, for the callers that do not give a cell themselves: the sound
   is then heard from the party's own cell [H], so Distance(gPartyCell, gPartyCell) is 0, the
   attenuation is 0 and the sound plays at full level. Like every PlaySoundAtCell call it does not
   play `sound` now: PlaySoundAtCell plays the sound recorded by the previous call and records
   `sound` for the next (QUIRK 177D:0106). */
/*@ 177D:00F3 */ void PlaySoundDefault(int sound)
{
    PlaySoundAtCell(sound, gPartyCell);
}

/* 177D:0106: records the game sound numbered `sound` and its source cell `cell` for a later call;
   `cell` becomes the attenuation gCurAttenuation = cell ? Distance(gPartyCell, cell) : 0, the
   distance between the party's cell and the sound's cell (177D:0261 to 0279: 177D:0267 pushes
   `cell`, 177D:026A pushes gPartyCell, 177D:026E is the Distance call); a `cell` of 0 skips the
   call and leaves the attenuation 0 (why 0 is special is not known). On AdLib, when a sound was
   already recorded by the previous call (gCurSound > 0), it first retunes and restarts that recorded
   sound at the attenuation that call recorded; the speaker cards restart it with no retune and no
   attenuation (below). So a call plays the sound of the call before it, and the
   new `sound` and `cell` are read only at the end (the map byte for `sound` is read at 177D:0283,
   zero-extended at 0286 and stored into gCurSound at 177D:0288); the first call after load
   (gCurSound 0) plays nothing. On AdLib (gSoundCard 0) and only when the recorded attenuation
   gCurAttenuation is below 10h (177D:012B): it waits for the busy byte gDrvReg to clear, restores
   the loaded sound's registers (gPrevSound, gSoundLevel, gSoundReg1) through 0000:03B0, reloads them
   from the recorded sound through 0000:03CA, and, when gCurAttenuation is above zero, fades the
   level towards full attenuation and writes the driver register-1 value 20h - gCurAttenuation
   (0000:03B0, 177D:01DA; whether that value is an amplitude is not known); then starts the recorded
   sound through 0000:400C and sets gDrvReg to 0Bh. On the other cards (1 PC speaker, 2 Tandy
   speaker) there is no attenuation test and no restore: the recorded sound starts on every call
   through 0000:03E3 and 1948:0418, and when gSpeakerMode is 1 (the Tandy path) the recorded sound sets
   gTandyOn to 1 and resumes the driver through 1948:0458, but only when gTandyOn is currently 0
   (177D:022F to 023B). QUIRK 177D:0106: the recorded sound gCurSound is played, not the new
   `sound`; the new number is stored only at 177D:0288. QUIRK 177D:0210: the `gCurSound < 0` test
   (the `gTandyOn = 0` block) is dead twice over: it sits inside the `gCurSound > 0` guard
   (177D:0117), so it cannot be reached with a negative gCurSound, and gCurSound is the
   zero-extended map byte (`mov ah, 0` at 177D:0286), so it is never negative anyway. QUIRK 177D:0288:
   unlike PlaySound there is no `gSoundMap[sound] < 96h` test, so a map value of 96h or more is
   recorded and played by the next call (through 0000:400C at 177D:01F2 on AdLib, left masked to
   seven bits at 177D:0240 on the speaker cards). Finally gCurAttenuation = cell ?
   Distance(gPartyCell, cell) : 0, then gCurSound = gSoundMap[sound]. Does nothing when sound is off
   (gSoundOn). */
/*@ 177D:0106 */ void PlaySoundAtCell(int sound, int cell)
{
    int level;
    void far *routine;

    if (!gSoundOn)
        return;
    if (gCurSound > 0) {
        if (gSoundCard == 0) {
            if (gCurAttenuation < ATTENUATION_ADLIB_LIMIT) {
                while (gDrvReg)      /* wait for the driver's busy byte to clear (177D:0135) */
                    ;
                f0_03B0(gPrevSound, 3, gSoundLevel);
                f0_03B0(gPrevSound, 1, gSoundReg1);
                gPrevSound = gCurSound;
                gSoundLevel = f0_03CA(gCurSound, 3);
                gSoundReg1 = f0_03CA(gCurSound, 1);
                /* QUIRK 177D:0194: register 3 is read a second time here (177D:0170 and 177D:0194);
                   it is what the original does, not a slip. The store to `level` is dead on every
                   path: it is reassigned at 177D:01C7 before its only read (177D:01C9) */
                level = gSoundLevel = f0_03CA(gCurSound, 3);
                if (gCurAttenuation > 0) {
                    /* the level moves (gCurAttenuation - 1) / 16 of the way from gSoundLevel to
                       LEVEL_FULL_ATTENUATION (177D:01AF to 01C7) */
                    level = gSoundLevel + ((LEVEL_FULL_ATTENUATION - gSoundLevel) * (gCurAttenuation - 1) >> 4);
                    f0_03B0(gCurSound, 3, level);
                    f0_03B0(gCurSound, 1, DRIVER_REG1_BASE - gCurAttenuation);
                }
                f0_400C(gCurSound);
                gDrvReg = DRIVER_BUSY;
            }
        } else {
            if (gSpeakerMode == 1) {
                if (gCurSound < 0 && gTandyOn) {
                    gTandyOn = 0;
                    SoundDriverResume();
                }
                if (gCurSound > 0 && gTandyOn == 0) {
                    gTandyOn = 1;
                    SoundDriverResume();
                }
            }
            /* masks gCurSound in place (177D:0240) before the play; the store at 177D:0288
               overwrites it with gSoundMap[sound] afterwards */
            gCurSound &= SPEAKER_ROUTINE_MASK;
            routine = f0_03E3(gCurSound);
            StartSound(routine);
        }
    }
    gCurAttenuation = cell ? Distance(gPartyCell, cell) : 0;
    gCurSound = gSoundMap[sound];
}
