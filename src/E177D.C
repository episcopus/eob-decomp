/* Unit at EOB.EXE 177D:000A (646 bytes, 4 functions): the sound driver loader and the routines
   that play a game sound through it. LoadSoundDriver (177D:000A) opens adlib.dat or pcgamesn.dat
   by gSoundCard (ds:B892), reads the 120-byte game-number to driver-number map into gSoundMap
   (ds:DCAC) and the driver's 32C8h data block into the code segment (the far pointer 0000:03AA
   points at it), then fixes the block's word table (0000:03FA). PlaySound (177D:0070) plays the
   driver sound a game number maps to; PlaySoundVolume (177D:0106) plays it at a volume, first
   stopping or retuning the sound already playing, and PlaySoundDefault (177D:00F3) calls it at
   the global volume gSoundVolume (ds:FBCA). The byte at 0000:3B36 is the loaded driver's register.
   Whitepaper 2.7 (AdLib) and 4.4 (ADLIB.DAT / PCGAMESN.DAT). Compiled -ml -O -Z. */

#include <dos.h>

extern unsigned char far *gSoundMap; /*@ ds:DCAC */
extern int gSoundOn; /*@ ds:80E6 */
extern unsigned char gSoundCard; /*@ ds:B892 */
extern unsigned char gSpeakerMode; /*@ ds:009E */
extern unsigned char gSpeakerOn; /*@ ds:8A67 */
extern int gCurSound; /*@ ds:5E22 */
extern int gCurVolume; /*@ ds:5E24 */
extern int gPrevSound; /*@ ds:5E26 */
extern int gSoundLevel; /*@ ds:5E28 */
extern int gSoundAmp; /*@ ds:5E2A */
extern int gSoundVolume; /*@ ds:FBCA */

extern unsigned char far gDrvReg; /*@ 0000:3B36 */

extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void far *f0_03AA(void); /*@ 0000:03AA */
extern void f0_03B0(int idx, int off, int val); /*@ 0000:03B0 */
extern int f0_03CA(int idx, int off); /*@ 0000:03CA */
extern void far *f0_03E3(int idx); /*@ 0000:03E3 */
extern void f0_03FA(void); /*@ 0000:03FA */
extern void f0_400C(int c); /*@ 0000:400C */
extern void StartSound(void far *p); /*@ 1948:0418 */
extern void SoundDriverResume(void); /*@ 1948:0458 */
extern int f17A6_0F61(int x, int y); /*@ 17A6:0F61 */

/* 177D:000A: opens the driver file chosen by gSoundCard (adlib.dat when 0, pcgamesn.dat
   otherwise), reads its 120-byte sound map into gSoundMap and its data block into the code
   segment, closes the file and fixes the block's word table. Called by StartupAlloc (07A9:000A). */
/*@ 177D:000A */ void LoadSoundDriver(void)
{
    int handle;

    if (gSoundCard == 0)
        handle = FileOpen("adlib.dat", 1);
    else
        handle = FileOpen("pcgamesn.dat", 1);
    FileRead(handle, gSoundMap, 120L);
    FileRead(handle, f0_03AA(), 0x32C8L);
    FileClose(handle);
    f0_03FA();
}

/* 177D:0070: plays the game sound numbered `sound`: its driver number comes from gSoundMap and
   a value of 96h or more is ignored. On AdLib the driver number is handed to 0000:400C; on the
   PC speaker the sign of the number gates the speaker (gSpeakerOn) when gSpeakerMode is 1 and
   the low seven bits select a routine through 0000:03E3. Does nothing when sound is off
   (gSoundOn). Called by many game routines to make a sound effect. */
/*@ 177D:0070 */ void PlaySound(int sound)
{
    int driverSound;
    void far *routine;

    if (!gSoundOn)
        return;
    driverSound = gSoundMap[sound];
    if (driverSound < 0x96) {
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
        driverSound &= 0x7f;
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

/* 177D:0106: plays the game sound numbered `sound` at `volume`. When a sound is already
   playing (gCurSound) it first restores the driver registers of the loaded sound (gPrevSound,
   gSoundLevel, gSoundAmp), then loads the new sound's register values through 0000:03CA and,
   for a volume above zero, fades the level towards full attenuation and writes the amplitude
   (0000:03B0), starts it (0000:400C on AdLib, 0000:03E3 on the PC speaker) and pokes the driver
   register gDrvReg; the PC-speaker path also gates the speaker through gSpeakerOn when
   gSpeakerMode is 1. Finally gCurVolume is computed from `volume` through 17A6:0F61(gSoundVolume,
   volume) and gCurSound is set from gSoundMap[sound]. Does nothing when sound is off (gSoundOn). */
/*@ 177D:0106 */ void PlaySoundVolume(int sound, int volume)
{
    int level;
    void far *routine;

    if (!gSoundOn)
        return;
    if (gCurSound > 0) {
        if (gSoundCard == 0) {
            if (gCurVolume < 0x10) {
                while (gDrvReg)
                    ;
                f0_03B0(gPrevSound, 3, gSoundLevel);
                f0_03B0(gPrevSound, 1, gSoundAmp);
                gPrevSound = gCurSound;
                gSoundLevel = f0_03CA(gCurSound, 3);
                gSoundAmp = f0_03CA(gCurSound, 1);
                level = gSoundLevel = f0_03CA(gCurSound, 3);
                if (gCurVolume > 0) {
                    level = gSoundLevel + ((0x3f - gSoundLevel) * (gCurVolume - 1) >> 4);
                    f0_03B0(gCurSound, 3, level);
                    f0_03B0(gCurSound, 1, 0x20 - gCurVolume);
                }
                f0_400C(gCurSound);
                gDrvReg = 0xb;
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
            gCurSound &= 0x7f;
            routine = f0_03E3(gCurSound);
            StartSound(routine);
        }
    }
    gCurVolume = volume ? f17A6_0F61(gSoundVolume, volume) : 0;
    gCurSound = gSoundMap[sound];
}
