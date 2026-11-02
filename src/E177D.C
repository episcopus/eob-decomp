/* Unit at EOB.EXE 177D:000A (646 bytes, 4 functions): the sound driver loader and the routines
   that play a game sound through it. LoadSoundDriver (177D:000A) opens adlib.dat or pcgamesn.dat
   by gSoundCard (ds:B892), reads the 120-byte game-number to driver-number map into gSoundMap
   (ds:DCAC) and the driver's 32C8h data block into segment 0000 (_TEXT) at offset 0416 (f0_03AA,
   0000:03AA, returns cs:0416; f0_03FA, 0000:03FA, adds the 0416 base to the block's 12Ch word
   table). PlaySound (177D:0070) plays the driver sound a game number maps to; PlaySoundVolume
   (177D:0106) plays the sound recorded by the previous call at a volume, and PlaySoundDefault
   (177D:00F3) calls it at the global volume gSoundVolume (ds:FBCA). gDrvReg (0000:3B36) is the
   driver's busy flag: the program waits for it to clear before it writes the sound registers and
   sets it to 0Bh after a start. Whitepaper 2.7 (AdLib) and 4.4 (ADLIB.DAT / PCGAMESN.DAT).
   Compiled -ml -O -Z. */

#include <dos.h>

/* Meaningful constants of this unit. */
/* the driver data block's size, the FileRead length at 177D:004C (whitepaper 4.4) */
#define DRIVER_BLOCK_SIZE 0x32C8L
/* a game-sound map value from here up is ignored (whitepaper 4.4, 177D:008C) */
#define SOUND_MAP_IGNORED 0x96
/* the low seven bits of the driver sound, the speaker driver routine index (177D:00D2, 177D:0240) */
#define SPEAKER_ROUTINE_MASK 0x7f
/* gCurVolume from here up skips the AdLib play (re/globals.toml ds:5E24, 177D:012B) */
#define VOLUME_ADLIB_LIMIT 0x10
/* the level register's full attenuation (whitepaper 2.7 "40h to 55h = 3Fh", 177D:01AF) */
#define LEVEL_FULL_ATTENUATION 0x3f
/* Purpose unknown: the base the AdLib amplitude write subtracts gCurVolume from (177D:01DA);
   20h is the OPL register block 20h (waveform and multiplier), so this may not be an amplitude */
#define AMP_BASE 0x20
/* Purpose unknown: the value stored in the busy byte gDrvReg after a start (177D:0201) */
#define DRIVER_BUSY 0x0b

extern unsigned char far *gSoundMap; /*@ ds:DCAC */  /* the 120-byte u8 game-number to driver-number
                                                        map, read by LoadSoundDriver (177D:000A) and
                                                        indexed by PlaySound (177D:0070); whitepaper 4.4 */
extern int gSoundOn; /*@ ds:80E6 */  /* the sound-on flag from argv[2] (whitepaper 2.6); PlaySound and
                                        PlaySoundVolume return at once when it is zero */
extern unsigned char gSoundCard; /*@ ds:B892 */  /* the sound choice from argv[2] minus '1' (whitepaper
                                                    2.6): 0 opens adlib.dat (the AdLib path), nonzero
                                                    pcgamesn.dat; 1 is the PC speaker and 2 the Tandy
                                                    speaker (src/E07C8.C:281-287). The value is 0..2;
                                                    src/E07C8.C:28 declares it char */
extern unsigned char gSpeakerMode; /*@ ds:009E */  /* 1 only on the Tandy path (src/E07C8.C:307), where
                                                      the speaker path of PlaySound/PlaySoundVolume drives
                                                      gSpeakerOn; Purpose unknown beyond that */
extern unsigned char gSpeakerOn; /*@ ds:8A67 */  /* the PC speaker / Tandy driver select applied by
                                                    1948:0458 (E1948.ASM: 1 is Tandy, 0 is PC speaker);
                                                    PlaySound and PlaySoundVolume only set it to 1 */
extern int gCurSound; /*@ ds:5E22 */  /* the driver sound number recorded by the previous call of
                                          PlaySoundVolume; nonzero guards the retune, and 177D:0106
                                          restarts this recorded number, not the new one */
extern int gCurVolume; /*@ ds:5E24 */  /* the volume of gCurSound (0..0x10); zero means stop, 10h or more
                                          skips the AdLib play */
extern int gPrevSound; /*@ ds:5E26 */  /* the sound whose driver registers gSoundLevel and gSoundAmp hold,
                                          restored through 0000:03B0 before the next sound */
extern int gSoundLevel; /*@ ds:5E28 */  /* the driver level written to register 3 of the current sound
                                           (0000:03B0/0000:03CA) */
extern int gSoundAmp; /*@ ds:5E2A */  /* the driver value written to register 1 of the current sound
                                         (0000:03B0/0000:03CA); Purpose unknown: the AdLib write is
                                         20h - gCurVolume (177D:01DA), and 20h is the OPL register
                                         block 20h (waveform and multiplier), not clearly an amplitude */
extern int gSoundVolume; /*@ ds:FBCA */  /* the sound setting PlaySoundDefault passes; PlaySoundVolume
                                           scales it by f17A6_0F61 to get gCurVolume (177D:026A). Also
                                           read by src/E0D1C.C:334, compared with an index by
                                           src/E1414.C:220 and passed to f17A6_0F61 in src/EOV6.C:393 */

extern unsigned char far gDrvReg; /*@ 0000:3B36 */

extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void far *f0_03AA(void); /*@ 0000:03AA */
extern void f0_03B0(int idx, int reg, int value); /*@ 0000:03B0 */
extern int f0_03CA(int idx, int reg); /*@ 0000:03CA */
extern void far *f0_03E3(int idx); /*@ 0000:03E3 */
extern void f0_03FA(void); /*@ 0000:03FA */
extern void f0_400C(int sound); /*@ 0000:400C */
extern void StartSound(void far *p); /*@ 1948:0418 */
extern void SoundDriverResume(void); /*@ 1948:0458 */
extern int f17A6_0F61(int x, int y); /*@ 17A6:0F61 */

/* 177D:000A: opens the driver file chosen by gSoundCard (adlib.dat when 0, pcgamesn.dat
   otherwise), reads its 120-byte sound map into gSoundMap and its 32C8h data block into segment
   0000 (_TEXT) at offset 0416 (f0_03AA returns cs:0416), closes the file and fixes the block's
   12Ch word table through f0_03FA. Called by StartupAlloc at 07A9:00D2. The handle from FileOpen
   is not checked: the data file is assumed to exist. */
/*@ 177D:000A */ void LoadSoundDriver(void)
{
    int handle;

    if (gSoundCard == 0)
        handle = FileOpen("adlib.dat", 1);
    else
        handle = FileOpen("pcgamesn.dat", 1);
    FileRead(handle, gSoundMap, 120L);
    FileRead(handle, f0_03AA(), DRIVER_BLOCK_SIZE);
    FileClose(handle);
    f0_03FA();
}

/* 177D:0070: plays the game sound numbered `sound`: its driver number comes from gSoundMap
   (unsigned char, so 0..0FFh) and a value of 96h or more is ignored. On AdLib (gSoundCard 0) the
   driver number is handed to 0000:400C. On the other cards (1 PC speaker, 2 Tandy speaker) the low
   seven bits select a driver routine through 0000:03E3 and start it through 1948:0418; when
   gSpeakerMode is 1 (the Tandy path, src/E07C8.C:307) a nonzero number sets gSpeakerOn to 1 and
   resumes the driver through 1948:0458 (1 is Tandy, 0 the PC speaker, E1948.ASM:7). QUIRK
   177D:00A8: the `driverSound < 0` test (the `gSpeakerOn = 0` block) is dead, because gSoundMap's
   bytes are unsigned and driverSound is 0..0FFh, so nothing here ever clears gSpeakerOn. Does
   nothing when sound is off (gSoundOn). Called by many game routines to make a sound effect. */
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
            if (driverSound < 0 && gSpeakerOn) {
                gSpeakerOn = 0;
                SoundDriverResume();
            }
            if (driverSound > 0 && gSpeakerOn == 0) {
                gSpeakerOn = 1;
                SoundDriverResume();
            }
        }
        driverSound &= SPEAKER_ROUTINE_MASK;
        routine = f0_03E3(driverSound);
        StartSound(routine);
    }
}
void PlaySoundVolume(int sound, int volume);

/* 177D:00F3: plays the game sound numbered `sound` at the current global volume gSoundVolume.
   A thin wrapper over PlaySoundVolume for the callers that do not give a volume themselves. */
/*@ 177D:00F3 */ void PlaySoundDefault(int sound)
{
    PlaySoundVolume(sound, gSoundVolume);
}

/* 177D:0106: plays the game sound numbered `sound` at `volume`. When a sound was already recorded by
   the previous call (gCurSound > 0) it first retunes and restarts that recorded sound, then records
   the new sound and its volume at the end (177D:0261 to 0288), so a call plays the sound of the call
   before it and the first call after load (gCurSound 0) plays nothing. On AdLib (gSoundCard 0) and
   only when the recorded sound's volume gCurVolume is below 10h (177D:012B): it waits for the busy
   byte gDrvReg to clear, restores the loaded sound's registers (gPrevSound, gSoundLevel, gSoundAmp)
   through 0000:03B0, reloads them from the recorded sound through 0000:03CA, and, when gCurVolume is
   above zero, fades the level towards full attenuation and writes the amplitude (0000:03B0); then
   starts the recorded sound through 0000:400C and sets gDrvReg to 0Bh. On the other cards (1 PC
   speaker, 2 Tandy speaker) there is no volume test and no restore: the recorded sound starts on
   every call through 0000:03E3 and 1948:0418, and when gSpeakerMode is 1 (the Tandy path) a nonzero
   sound sets gSpeakerOn to 1. QUIRK 177D:0106: the recorded sound gCurSound is played, not the new
   `sound`; the new number is stored only at 177D:0288. QUIRK 177D:0210: the `gCurSound < 0` test
   (the `gSpeakerOn = 0` block) is dead for the same reason as in PlaySound: gCurSound holds an
   unsigned map byte. Finally gCurVolume = volume ? 17A6:0F61(gSoundVolume, volume) : 0 and gCurSound
   = gSoundMap[sound]. Does nothing when sound is off (gSoundOn). */
/*@ 177D:0106 */ void PlaySoundVolume(int sound, int volume)
{
    int level;
    void far *routine;

    if (!gSoundOn)
        return;
    if (gCurSound > 0) {
        if (gSoundCard == 0) {
            if (gCurVolume < VOLUME_ADLIB_LIMIT) {
                while (gDrvReg)      /* wait for the driver's busy byte to clear (177D:0135) */
                    ;
                f0_03B0(gPrevSound, 3, gSoundLevel);
                f0_03B0(gPrevSound, 1, gSoundAmp);
                gPrevSound = gCurSound;
                gSoundLevel = f0_03CA(gCurSound, 3);
                gSoundAmp = f0_03CA(gCurSound, 1);
                level = gSoundLevel = f0_03CA(gCurSound, 3);
                if (gCurVolume > 0) {
                    level = gSoundLevel + ((LEVEL_FULL_ATTENUATION - gSoundLevel) * (gCurVolume - 1) >> 4);
                    f0_03B0(gCurSound, 3, level);
                    f0_03B0(gCurSound, 1, AMP_BASE - gCurVolume);
                }
                f0_400C(gCurSound);
                gDrvReg = DRIVER_BUSY;
            }
        } else {
            if (gSpeakerMode == 1) {
                if (gCurSound < 0 && gSpeakerOn) {
                    gSpeakerOn = 0;
                    SoundDriverResume();
                }
                if (gCurSound > 0 && gSpeakerOn == 0) {
                    gSpeakerOn = 1;
                    SoundDriverResume();
                }
            }
            gCurSound &= SPEAKER_ROUTINE_MASK;
            routine = f0_03E3(gCurSound);
            StartSound(routine);
        }
    }
    gCurVolume = volume ? f17A6_0F61(gSoundVolume, volume) : 0;
    gCurSound = gSoundMap[sound];
}
