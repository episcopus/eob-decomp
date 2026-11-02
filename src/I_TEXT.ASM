; Unit at INTRO.EXE 0000:020C (20980 bytes), the _TEXT segment of the INTRO.EXE load module between the
; start-up (i_c0) and the Borland runtime library (i_lib): Westwood's AdLib sound driver, code and data in one
; segment, running with DS = CS (WHITEPAPER 2.1.2 gives what segment 0 holds, 2.4 the INTRO.EXE program).
; LoadSoundDriver (E177D.C) reads the 32C8h-byte block of adlib.dat or pcgamesn.dat
; into cs:0416h; the AdLib driver (sound card 0, E177D.C:147-149) plays the sounds in it, the PC speaker and Tandy
; driver is E1948.ASM. This is the INTRO.EXE build of EOB.EXE's e_text (src/E_TEXT.ASM): the same driver, 984 bytes
; larger, with the data laid out 1000 bytes further on (tables 3944 to 3D2C, variables 3B36 to 3F1E) and six
; instructions dropped from the code in three places (the `cld` of the AdLib INT 08h handler, its gBusyCountdown wait
; and the push di/pop di of the command dispatch); the rest of the code is instruction for instruction the EOB version,
; only the addresses move. In address order:
;   020C  the pitch curve tables the script commands read (data), then GetScriptFlag (03A1)
;   03AA  the five far routines LoadSoundDriver, PlaySound and PlaySoundAtCell call to reach the block (E177D.C)
;   0416  the driver block (data: the EXE's image of it), then the driver's tables (3D2C), variables (3F1E),
;         ten voice records (3F7A) and voice tables (4218)
;   424D  the two INT 08h handlers (the AdLib timer, the sample player)
;   4352  the driver: start, stop, tick, the sound queue, the per-voice script interpreter UpdateVoices (456A) and
;         its helpers, and the interpreter's command jump table (536C, data)
; The source uses the names src/E177D.C and src/E07C8.C give the shared routines and names the rest from the
; code. The data is the EXE's own bytes: tables are written out, run-time state is zero or the initial value the
; image holds.

        .MODEL  LARGE
        .CODE
        ASSUME  ds:@code                ; the driver runs with DS = CS: its variables are addressed through DS

; 0000:020C: the pitch curve tables (data, 405 bytes). gCurveTablePtrs holds six near pointers, indexed by 2 * a curve number
; (mov bx,[bx+20Ch] in the script command that starts a pitch effect; GetDriverTable returns 020Ch for its argument 4);
; gCurvePairs steps down by 1 every second byte from 50h to 10h, gCurveRamp counts 00h to 7Fh and gCurveTriples steps down
; every third byte from 40h to 15h. QUIRK 02F9: the ramp has 6Fh where its pattern wants 5Fh (the neighbours are 5Eh and 60h);
; the EXE's byte is kept. gDriverTablePtrs (039C) holds the near pointers SelectDriverTable (44C0) picks the sound table from,
; both 0416h. gScriptFlag (03A0) is the byte the script command C7h sets from its operand (4DFB: mov [3A0h],ah), read back by
; GetScriptFlag.
        PUBLIC  _CurveTables              ; @ 0000:020C
_CurveTables LABEL BYTE
gCurveTablePtrs LABEL WORD
        dw      gCurvePairs, gCurveRamp, gCurvePairs, gCurveRamp, gCurveTriples, gCurveRamp
gCurvePairs LABEL BYTE
        db      50h,50h,4Fh,4Fh,4Eh,4Eh,4Dh,4Dh,4Ch,4Ch,4Bh,4Bh,4Ah,4Ah,49h,49h
        db      48h,48h,47h,47h,46h,46h,45h,45h,44h,44h,43h,43h,42h,42h,41h,41h
        db      40h,40h,3Fh,3Fh,3Eh,3Eh,3Dh,3Dh,3Ch,3Ch,3Bh,3Bh,3Ah,3Ah,39h,39h
        db      38h,38h,37h,37h,36h,36h,35h,35h,34h,34h,33h,33h,32h,32h,31h,31h
        db      30h,30h,2Fh,2Fh,2Eh,2Eh,2Dh,2Dh,2Ch,2Ch,2Bh,2Bh,2Ah,2Ah,29h,29h
        db      28h,28h,27h,27h,26h,26h,25h,25h,24h,24h,23h,23h,22h,22h,21h,21h
        db      20h,20h,1Fh,1Fh,1Eh,1Eh,1Dh,1Dh,1Ch,1Ch,1Bh,1Bh,1Ah,1Ah,19h,19h
        db      18h,18h,17h,17h,16h,16h,15h,15h,14h,14h,13h,13h,12h,12h,11h,11h
        db      10h,10h
gCurveRamp LABEL BYTE
        db      00h,01h,02h,03h,04h,05h,06h,07h,08h,09h,0Ah,0Bh,0Ch,0Dh,0Eh,0Fh
        db      10h,11h,12h,13h,14h,15h,16h,17h,18h,19h,1Ah,1Bh,1Ch,1Dh,1Eh,1Fh
        db      20h,21h,22h,23h,24h,25h,26h,27h,28h,29h,2Ah,2Bh,2Ch,2Dh,2Eh,2Fh
        db      30h,31h,32h,33h,34h,35h,36h,37h,38h,39h,3Ah,3Bh,3Ch,3Dh,3Eh,3Fh
        db      40h,41h,42h,43h,44h,45h,46h,47h,48h,49h,4Ah,4Bh,4Ch,4Dh,4Eh,4Fh
        db      50h,51h,52h,53h,54h,55h,56h,57h,58h,59h,5Ah,5Bh,5Ch,5Dh,5Eh,6Fh
        db      60h,61h,62h,63h,64h,65h,66h,67h,68h,69h,6Ah,6Bh,6Ch,6Dh,6Eh,6Fh
        db      70h,71h,72h,73h,74h,75h,76h,77h,78h,79h,7Ah,7Bh,7Ch,7Dh,7Eh,7Fh
gCurveTriples LABEL BYTE
        db      40h,40h,40h,3Fh,3Fh,3Fh,3Eh,3Eh,3Eh,3Dh,3Dh,3Dh,3Ch,3Ch,3Ch,3Bh
        db      3Bh,3Bh,3Ah,3Ah,3Ah,39h,39h,39h,38h,38h,38h,37h,37h,37h,36h,36h
        db      36h,35h,35h,35h,34h,34h,34h,33h,33h,33h,32h,32h,32h,31h,31h,31h
        db      30h,30h,30h,2Fh,2Fh,2Fh,2Eh,2Eh,2Eh,2Dh,2Dh,2Dh,2Ch,2Ch,2Ch,2Bh
        db      2Bh,2Bh,2Ah,2Ah,2Ah,29h,29h,29h,28h,28h,28h,27h,27h,27h,26h,26h
        db      26h,25h,25h,25h,24h,24h,24h,23h,23h,23h,22h,22h,22h,21h,21h,21h
        db      20h,20h,20h,1Fh,1Fh,1Fh,1Eh,1Eh,1Eh,1Dh,1Dh,1Dh,1Ch,1Ch,1Ch,1Bh
        db      1Bh,1Bh,1Ah,1Ah,1Ah,19h,19h,19h,18h,18h,18h,17h,17h,17h,16h,16h
        db      16h,15h
gDriverTablePtrs LABEL WORD
        dw      _DriverBlock, _DriverBlock
gScriptFlag     db      0

; 0000:03A1: returns dx = cs and ax = the script flag byte (gScriptFlag, 03A0) zero-extended. Far routine, no arguments.
; Purpose unknown: no far call or pointer in the EXE image names 0000:03A1 (it may be reached from the block).
        PUBLIC  _GetScriptFlag              ; @ 0000:03A1
_GetScriptFlag PROC NEAR
        mov       dx, cs
        xor       ah, ah
        mov       al, byte ptr cs:gScriptFlag
        retf
_GetScriptFlag ENDP

; 0000:03AA: returns the driver block pointer dx:ax = cs:0416 (loaded by LoadSoundDriver)
        PUBLIC  _GetDriverBlock              ; @ 0000:03AA
_GetDriverBlock PROC NEAR
        mov       dx, cs
        mov       ax, offset _DriverBlock
        retf
_GetDriverBlock ENDP

; 0000:03B0: writes one byte of the driver block: idx selects one word of the 300-word table at 0416 (the near pointer at cs:0416 + 2*idx), reg offsets into the record that word points to (E177D.C:251 to 252)
        PUBLIC  _SetSoundByte              ; @ 0000:03B0
_SetSoundByte PROC NEAR
        push      bp
        mov       bp, sp
        push      bx
        mov       bx, word ptr [bp + 6]
        shl       bx, 1
        mov       bx, word ptr cs:[bx + _DriverBlock]
        add       bx, word ptr [bp + 8]
        mov       ax, word ptr [bp + 0ah]
        mov       byte ptr cs:[bx], al
        pop       bx
        pop       bp
        retf
_SetSoundByte ENDP

; 0000:03CA: reads one byte of the driver block, zero-extended (E177D.C)
        PUBLIC  _GetSoundByte              ; @ 0000:03CA
_GetSoundByte PROC NEAR
        push      bp
        mov       bp, sp
        push      bx
        mov       bx, word ptr [bp + 6]
        shl       bx, 1
        mov       bx, word ptr cs:[bx + _DriverBlock]
        add       bx, word ptr [bp + 8]
        mov       al, byte ptr cs:[bx]
        xor       ah, ah
        pop       bx
        pop       bp
        retf
_GetSoundByte ENDP

; 0000:03E3: returns the driver routine pointer dx = cs and ax = the word at cs:[0416 + 2*idx] (E177D.C)
        PUBLIC  _GetSoundData              ; @ 0000:03E3
_GetSoundData PROC NEAR
        push      bp
        mov       bp, sp
        push      bx
        mov       ax, word ptr [bp + 6]
        mov       bx, ax
        shl       bx, 1
        add       bx, offset _DriverBlock
        mov       ax, word ptr cs:[bx]
        mov       dx, cs
        pop       bx
        pop       bp
        retf
_GetSoundData ENDP

; 0000:03FA: makes the driver block's offset table absolute: adds 0416h to each of the 300 words at cs:0416h (cx = 12Ch, 03FD to 0410),
; so the block's file offsets become near pointers into cs. Called once by LoadSoundDriver (E177D.C) after it reads the 32C8h
; block from adlib.dat or pcgamesn.dat into cs:0416h; saves and restores cx, di and ds. Far routine, no arguments.
        PUBLIC  _RebaseDriverBlock              ; @ 0000:03FA
_RebaseDriverBlock PROC NEAR
        push      cx
        push      di
        push      ds
        mov       cx, 12ch
        mov       ax, cs
        mov       ds, ax
        xor       di, di
        mov       ax, offset _DriverBlock
L_0409:
        add       word ptr ds:[di + _DriverBlock], ax
        add       di, 2
        loop    L_0409
        pop       ds
        pop       di
        pop       cx
        retf
_RebaseDriverBlock ENDP

; 0000:0416: the driver block (data, 14614 bytes to 3D2B). LoadSoundDriver reads the 32C8h bytes of adlib.dat or pcgamesn.dat
; over 0416 to 36DD at run time (E177D.C:152) and RebaseDriverBlock turns its first 300 words into near pointers; the bytes
; here are the EXE's image of it and are overwritten before they are read: zeros but for the word 066Eh at 0416, the word
; 0671h at 0542 (gPatchTable's first word) and 12 bytes at 0670 (10 nonzero): Purpose unknown. gBlockSpare (36DE, 1614 bytes to 3D2C)
; follows the block, zero and unreferenced in this file.
        PUBLIC  _DriverBlock              ; @ 0000:0416
_DriverBlock LABEL BYTE
        dw      066Eh
        db      298 dup (0)
; gPatchTable (0542): the near pointers the script command 90h reads at [bx+gPatchTable] (line 1017); GetDriverTable's arg 3 returns it (line 713)
gPatchTable LABEL WORD
        dw      0671h
        db      300 dup (0)
        db      088h,001h,001h,008h,001h,000h,018h,000h,054h,0F2h,060h,046h
        db      12386 dup (0)
gBlockSpare LABEL BYTE
        db      1614 dup (0)

; 0000:3D2C: the driver's tables and variables (data, 590 bytes). The tables, 498 bytes: gEntryTable (3D2C) lists the eleven near entry points of the driver in the
; order the original links them [H]: no reader in this file (Purpose unknown; the block's code may use it [H]). gLevelCurvePtrs
; (3D42) points to the fourteen 32-byte level curves from 3D5E: monotone maps of a level 0 to 1Fh to a larger one (UpdateVoices
; reads them with mov si,[bx+3D42h] at 4A1E and 4A3B, SetVoicePitch at 4E6E and 4E8B). QUIRK 3E6E: curve 8 has 1Ah, 19h where its neighbours rise (17h,1Ah,
; 19h,1Ah,1Ch); QUIRK 3EFB: curve 12 has 34h at 3EFB (3EFC holds 41h) between 3Bh and 41h (the pattern wants about 3Eh); the EXE's bytes are kept.
        PUBLIC  _DriverTables              ; @ 0000:3D2C
_DriverTables LABEL BYTE
gEntryTable LABEL WORD
        dw      _StartAdlibDriver, _StopAdlibDriver, _SoundDriverTick, _QueueSound, _StartSample, _GetVoiceState
        dw      _GetSampleOffset, _GetDriverTable, _SetNote, _SetSamplePlaying, _SelectDriverTable
gLevelCurvePtrs LABEL WORD
        dw      gLevelCurve0, gLevelCurve1, gLevelCurve2, gLevelCurve3, gLevelCurve4, gLevelCurve5, gLevelCurve6, gLevelCurve7, gLevelCurve8, gLevelCurve9, gLevelCurve10, gLevelCurve11, gLevelCurve12, gLevelCurve13
gLevelCurve0 LABEL BYTE
        db      00h,01h,02h,03h,04h,05h,06h,08h,09h,0Ah,0Bh,0Ch,0Dh,0Eh,0Fh,10h
        db      11h,12h,13h,14h,15h,16h,17h,19h,1Ah,1Bh,1Ch,1Dh,1Eh,1Fh,20h,21h
gLevelCurve1 LABEL BYTE
        db      00h,01h,02h,03h,04h,06h,07h,09h,0Ah,0Bh,0Ch,0Dh,0Eh,0Fh,10h,11h
        db      12h,13h,14h,15h,16h,17h,18h,1Ah,1Bh,1Ch,1Dh,1Eh,1Fh,20h,22h,24h
gLevelCurve2 LABEL BYTE
        db      00h,01h,02h,03h,04h,06h,08h,09h,0Ah,0Ch,0Dh,0Eh,0Fh,11h,12h,13h
        db      14h,15h,16h,17h,19h,1Ah,1Ch,1Dh,1Eh,1Fh,20h,21h,22h,24h,25h,26h
gLevelCurve3 LABEL BYTE
        db      00h,01h,02h,03h,04h,06h,08h,0Ah,0Bh,0Ch,0Dh,0Eh,0Fh,11h,12h,13h
        db      14h,15h,16h,17h,18h,1Ah,1Ch,1Dh,1Eh,1Fh,20h,21h,23h,25h,27h,28h
gLevelCurve4 LABEL BYTE
        db      00h,01h,02h,03h,04h,06h,08h,0Ah,0Bh,0Ch,0Dh,0Eh,0Fh,11h,13h,15h
        db      16h,17h,18h,19h,1Bh,1Dh,1Fh,20h,21h,22h,23h,24h,25h,26h,28h,2Ah
gLevelCurve5 LABEL BYTE
        db      00h,01h,02h,03h,05h,07h,09h,0Bh,0Ch,0Dh,0Eh,0Fh,10h,11h,13h,15h
        db      16h,17h,18h,19h,1Bh,1Dh,1Fh,20h,21h,22h,23h,25h,27h,29h,2Bh,2Dh
gLevelCurve6 LABEL BYTE
        db      00h,01h,02h,03h,05h,07h,09h,0Bh,0Ch,0Dh,0Eh,0Fh,10h,11h,13h,15h
        db      16h,17h,18h,1Ah,1Ch,1Eh,21h,24h,25h,26h,27h,29h,2Bh,2Dh,2Fh,30h
gLevelCurve7 LABEL BYTE
        db      00h,01h,02h,04h,06h,08h,0Ah,0Ch,0Dh,0Eh,0Fh,10h,11h,13h,15h,18h
        db      19h,1Ah,1Ch,1Dh,1Fh,21h,23h,25h,26h,27h,29h,2Bh,2Dh,2Fh,30h,32h
gLevelCurve8 LABEL BYTE
        db      00h,01h,02h,04h,06h,08h,0Ah,0Dh,0Eh,0Fh,10h,11h,12h,14h,17h,1Ah
        db      19h,1Ah,1Ch,1Eh,20h,22h,25h,28h,29h,2Ah,2Bh,2Dh,2Fh,31h,33h,35h
gLevelCurve9 LABEL BYTE
        db      00h,01h,03h,05h,07h,09h,0Bh,0Eh,0Fh,10h,12h,14h,16h,18h,1Ah,1Bh
        db      1Ch,1Dh,1Eh,20h,22h,24h,26h,29h,2Ah,2Ch,2Eh,30h,32h,34h,36h,39h
gLevelCurve10 LABEL BYTE
        db      00h,01h,03h,05h,07h,09h,0Bh,0Eh,0Fh,10h,12h,14h,16h,19h,1Bh,1Eh
        db      1Fh,21h,23h,25h,27h,29h,2Bh,2Dh,2Eh,2Fh,31h,32h,34h,36h,39h,3Ch
gLevelCurve11 LABEL BYTE
        db      00h,01h,03h,05h,07h,0Ah,0Ch,0Fh,10h,11h,13h,15h,17h,19h,1Bh,1Eh
        db      1Fh,20h,22h,24h,26h,28h,2Bh,2Eh,2Fh,30h,32h,34h,36h,39h,3Ch,3Fh
gLevelCurve12 LABEL BYTE
        db      00h,02h,04h,06h,08h,0Bh,0Dh,10h,11h,12h,14h,16h,18h,1Bh,1Eh,21h
        db      22h,23h,25h,27h,29h,2Ch,2Fh,32h,33h,34h,36h,38h,3Bh,34h,41h,44h
gLevelCurve13 LABEL BYTE
        db      00h,02h,04h,06h,08h,0Bh,0Dh,11h,12h,13h,15h,17h,1Ah,1Dh,20h,23h
        db      24h,25h,27h,29h,2Ch,2Fh,32h,35h,36h,37h,39h,3Bh,3Eh,41h,44h,47h

; The variables from 3F1E, 92 bytes, DS = CS, with the EXE image's initial values. Names come from the code that uses them (the
; UpdateVoices commands, the INT 08h handlers, E177D.C); the ones with no use in this file are spare.
        db      4 dup (0)               ; 3F1E to 3F21: spare
gNoteDuration   db      0               ; 3F22: StartVoiceNote stores the note's duration byte; zero lets the voice's script go on
        db      2 dup (0)               ; 3F23 to 3F24: spare
gVoiceIndex     db      0               ; 3F25: the voice UpdateVoices is serving, 9 down to 0 (the register offset of voice n is n)
gUpdateEnable   db      0               ; 3F26: UpdateVoices runs only while nonzero (set when a sound starts, cleared by reset)
gRhythmKeyBits  db      0               ; 3F27: the rhythm mode byte: C1h sets it to 20h (line 1481), C2h ORs its operand into it (lines 1502 to 1503), C3h clears it (line 1512); VoiceKeyOff (line 1829) and VoiceSilence (line 2282) test it for nonzero
gTempoRate      db      0               ; 3F28: the script's speed byte: added to gTempoAccum each tick by TickTempo; written by A6h (line 1111); copied into a voice's speed byte [di+25h] by BAh (line 1340) and, when [di+40h] is set, by the UpdateVoices copy (lines 781 to 784)
gEffectMode     db      0               ; 3F29: the pitch effect in force, 0 none: L_4AAC sets it; ProcessSoundQueue, ResetVoices, the
                                        ;   command at 4923 and L_4AF8 clear it (_AdlibTimerIsr acts while it is nonzero)
gEffectPeriod   db      1               ; 3F2A: the reload of gEffectCounter
gEffectCounter  db      1               ; 3F2B: counted down by _AdlibTimerIsr
gEffectIndex    dw      0FFFFh          ; 3F2C: the index into the effect's two curves (SetNote stores its argument here; L_4AAC clears it only for effect type 2, lines 1378 to 1381)
gRandomSeed     dw      1234h           ; 3F2E: SoundRandom's state, 1234h from ResetVoices on
gToggleBit      db      0               ; 3F30: _AdlibTimerIsr inverts it; its bit 5 becomes the key-on bit of voice 0
gRegBDDepth     db      0               ; 3F31: bits 7 and 6 of register BDh (amplitude and vibrato depth)
gOperatorOffset db      0               ; 3F32: the operator register offset of the voice being served (gOperatorOffsets)
gRhythmLevelA   db      5 dup (0)       ; 3F33: three five-byte sets of rhythm operator level terms, one byte per operator in the order of
gRhythmLevelB   db      5 dup (0)       ; 3F38:   the level registers 53h, 54h, 51h, 52h, 55h. A is the level byte of the patch L_4B01 loads
gRhythmLevelC   db      5 dup (0)       ; 3F3D:   for the rhythm voices, B is stored by L_4CC7 and also by C6h (L_4D6B, lines 1648, 1659, 1670, 1681 and 1692); C by L_4C23; both commands write the
                                        ;   clamped sum of the operand and the three terms to the operator's level register
gTempoReload    db      0               ; 3F42: half the tempo operand (the command at L_47EA)
gTempoCounter   db      0               ; 3F43: counted down from gTempoReload each time gTempoAccum carries
gTempoAccum     db      0FFh            ; 3F44: TickTempo adds gTempoRate; a carry steps the counter
gTempoBeat      db      0               ; 3F45: the beat count: TickTempo adds one each time gTempoCounter reaches zero
gTempoWait      db      0               ; 3F46: the wait-for-beat command's phase flag (L_4806)
gEffectCurveA   dw      0               ; 3F47: the near pointers the effect command picks from gCurveTablePtrs
gEffectCurveB   dw      0               ; 3F49
gSoundTable     dw      0               ; 3F4B: the sound-number table ProcessSoundQueue indexes (SelectDriverTable, StartAdlibDriver)
gSavedTimerOff  dw      0               ; 3F4D: the BIOS INT 08h vector StartAdlibDriver saved, offset
gSavedTimerSeg  dw      0               ; 3F4F: and segment; StopAdlibDriver restores it, _AdlibTimerIsr chains to it
gSampleSavedOff dw      0               ; 3F51: the INT 08h vector StartSample saved (424D's), offset
gSampleSavedSeg dw      0               ; 3F53: and segment; _SampleTimerIsr puts it back
gTimerPasses12  db      0Ch             ; 3F55: _AdlibTimerIsr calls the tick when it counts down to zero
gTimerPasses48  db      30h             ; 3F56: and chains to the BIOS when this one does
        db      4 dup (0)               ; 3F57 to 3F5A: spare
gSampleNibbles  db      0               ; 3F5B: the sample's pass counter (bit 0 selects the nibble)
gSampleLevel    db      0               ; 3F5C: the sample's running level, set to 8 by StartSample
gSampleLength   dw      0               ; 3F5D: its length in bytes
gSamplePtrOff   dw      0               ; 3F5F: the sample's read pointer, offset (zero when none plays: GetSampleOffset)
gSamplePtrSeg   dw      0               ; 3F61: and segment
gSampleStart    dw      0               ; 3F63: the sample's first offset
gSampleEnable   db      0               ; 3F65: nonzero while a sample plays (SetSamplePlaying, StartSample)
gQueueWrite     dw      0               ; 3F66: the sound queue's write index, 0 to 0Fh (_QueueSound)
gQueueRead      dw      0               ; 3F68: its read index (ProcessSoundQueue)
gQueueRing      db      16 dup (0)      ; 3F6A: the queued driver sound numbers, one byte each (a word is stored at each index)

; 0000:3F7A: the ten voice records (data, 670 bytes, 43h each): the state of the nine AdLib voices (0 to 8) and the rhythm voice 9
; as UpdateVoices and its helpers read it through di. Zero in the image. The 43h-byte layout, by offset, with the code that proves
; each field (a byte unless it says otherwise; the script commands are the cases of UpdateVoices at 536C):
;   +00h  level adjustment added to both operator levels: script ACh sets it, ADh adds to it; VoiceCarrierLevel and VoiceModulatorLevel add it
;   +03h  script pointer (word): ProcessSoundQueue stores it, UpdateVoices reads and rewrites it
;   +05h  note delay counter: StartVoiceNote sets it, UpdateVoices counts it down, GetVoiceDelay returns it
;   +06h  loop counter: script 80h sets it, 81h decrements it
;   +07h  transpose, added to the octave by SetVoicePitch and by the B9h command
;   +08h  priority: ProcessSoundQueue compares and stores it, ClearVoice and the sound-end handler clear it
;   +09h  block depth: script 85h pushes, 86h pops
;   +0Ah  block return stack: the word at +0Ah + 2*depth (85h pushes, 86h pops)
;   +12h  detune, added to the semitone by SetVoicePitch and the B9h command
;   +13h  pitch slide rate: script 91h sets it, L_52D8 adds it to the phase
;   +14h  pitch slide phase accumulator: 91h sets it to FFh, L_52D8 adds +13h and carries
;   +15h  pitch slide amount (word): 91h stores it, L_52D8 adds it to the F-number
;   +17h  oscillator delta (word): VoiceKeyOn computes it, L_5256 negates it at a carry
;   +19h  oscillator shift: script 95h sets it, VoiceKeyOn shifts the F-number by it
;   +1Ah  oscillator count: 95h stores it, L_5256 reloads it from +1Bh
;   +1Bh  oscillator reload: 95h stores it, L_5256 reloads +1Ah from it
;   +1Ch  oscillator wait: 95h stores it, VoiceKeyOn copies it to +1Fh
;   +1Dh  oscillator rate: 95h sets it, L_5256 adds it to +1Eh
;   +1Eh  oscillator phase accumulator: L_5256 adds +1Dh and carries
;   +1Fh  oscillator delay counter: L_5256 counts it down before stepping the phase
;   +20h  operator level offset: script 9Eh sets it, the B0h command adds to it, the level routines add it
;   +21h  key-off count: StartVoiceNote computes it, UpdateVoices keys the voice off at it
;   +24h  fine tune, added to the F-number by SetVoicePitch
;   +25h  speed: scripts A9h, BAh and BDh set it, UpdateVoices copies gTempoRate into it while +40h is set, ClearVoice sets it to FFh
;   +26h  phase accumulator: UpdateVoices adds +25h each tick and starts the script on a carry
;   +27h  register A0h image (F-number low byte): SetVoicePitch stores it, the B9h command adjusts it
;   +28h  register B0h image (F-number high bits and key-on bit 20h): the INT 08h handler at 424D, SetVoicePitch, VoiceKeyOn,
;         VoiceKeyOff and the B5h command read and write it
;   +29h  per-tick handler pointer (word): UpdateVoices calls it every tick; scripts 91h, 92h, 95h and B6h and ClearVoice set it
;   +2Bh  per-voice handler pointer (word): UpdateVoices calls it every tick; scripts 8Dh and A7h and ClearVoice set it
;   +2Dh  key-off multiplier: script A4h sets it, StartVoiceNote uses it
;   +2Eh  patch modulator level byte: LoadVoicePatch stores it, VoiceModulatorLevel reads it
;   +2Fh  patch carrier level byte: LoadVoicePatch stores it, VoiceCarrierLevel reads it
;   +30h  second operator level offset: script ABh sets it, the level routines add it
;   +31h  connection bit: LoadVoicePatch keeps bit 0 of the patch C0h byte here, the level routines test it
;   +32h  modulator level shift: script C9h sets it, VoiceModulatorLevel shifts by it
;   +33h  carrier level shift: script C9h sets it, VoiceCarrierLevel shifts by it
;   +34h  decay level: scripts A0h and A1h store it, the level routines shift it
;   +35h  second key-off count: script 83h sets it, ClearVoice sets it to 1, UpdateVoices keys off at it
;   +36h  random-delay mask: script BCh sets it, StartVoiceNote masks SoundRandom with it
;   +39h  pitch envelope rate: script 8Dh sets it, L_52AF adds it to the accumulator
;   +3Ah  pitch envelope accumulator: 8Dh sets it, L_52AF adds +39h and carries
;   +3Bh  pitch envelope reload: 8Dh stores it, L_52AF reloads +3Ch from it
;   +3Ch  pitch envelope count: 8Dh stores it, L_52AF counts it down
;   +3Dh  pitch envelope register offset: 8Dh stores it, L_52AF adds gOperatorOffset to it
;   +3Eh  pitch envelope table pointer (word): 8Dh stores it, L_52AF indexes it with xlatb
;   +40h  follow-tempo flag: script C8h sets it, UpdateVoices copies gTempoRate into +25h while it is set
;   +41h  note byte (octave high nibble, semitone low nibble): SetVoicePitch stores its argument here, the B9h command reads it
;   +42h  note effect amount: script B9h sets it, SetVoicePitch bends the level by it
        PUBLIC  _VoiceRecords              ; @ 0000:3F7A
_VoiceRecords LABEL BYTE
gVoiceRecord0 db 43h dup (0)
gVoiceRecord1 db 43h dup (0)
gVoiceRecord2 db 43h dup (0)
gVoiceRecord3 db 43h dup (0)
gVoiceRecord4 db 43h dup (0)
gVoiceRecord5 db 43h dup (0)
gVoiceRecord6 db 43h dup (0)
gVoiceRecord7 db 43h dup (0)
gVoiceRecord8 db 43h dup (0)
gVoiceRecord9 db 43h dup (0)

; 0000:4218: the voice tables (data, 53 bytes). gVoiceRecordPtrs: the near pointer of each voice record, indexed by 2 * voice.
; gOperatorOffsets: the AdLib modulator operator offset of each of voices 0 to 8 (the carrier is 3 above it; register 20h + offset
; and so on). gFNumbers: the twelve F-numbers (register A0h and the low bits of B0h) of the notes of an octave.
        PUBLIC  _VoiceTables              ; @ 0000:4218
_VoiceTables LABEL BYTE
gVoiceRecordPtrs LABEL WORD
        dw      gVoiceRecord0, gVoiceRecord1, gVoiceRecord2, gVoiceRecord3, gVoiceRecord4, gVoiceRecord5, gVoiceRecord6, gVoiceRecord7, gVoiceRecord8, gVoiceRecord9
gOperatorOffsets LABEL BYTE
        db      000h,001h,002h,008h,009h,00Ah,010h,011h,012h
gFNumbers LABEL WORD
        dw      0134h,0147h,015Ah,016Fh,0184h,019Ch,01B4h,01CEh,01E9h,0207h,0225h,0246h

; 0000:424D: the AdLib driver's INT 08h handler, installed by StartAdlibDriver (4352: mov dx,424Dh; int 21h 2508h, 4372 to 4378).
; The PIT runs at divisor 0555h there (437D to 438B), 874 Hz if its clock is the usual 1193182 Hz [H], and this handler divides it down:
;   - while gEffectMode is nonzero, counts gEffectCounter down; at zero it toggles the key-on bit (20h) of voice 0's register B0h image
;     (the byte at +28h of gVoiceRecord0, which VoiceKeyOn also sets), writes the register to the card, and reloads gEffectCounter from
;     gEffectPeriod;
;   - every 0Ch passes (72.8 Hz) calls SoundDriverTick (43CF), counted by gTimerPasses12;
;   - every 30h passes (18.2 Hz, gTimerPasses48) chains to the BIOS handler StartAdlibDriver saved in gSavedTimerOff and gSavedTimerSeg;
;     that handler sends the end of interrupt, so this path does not; on every other pass it sends EOI itself.
; Saves and restores ax, bx, dx and ds; DS is set to CS, where the driver keeps its variables.
        PUBLIC  _AdlibTimerIsr              ; @ 0000:424D
_AdlibTimerIsr PROC NEAR
        push      ax
        push      bx
        push      dx
        push      ds
        mov       ax, cs
        mov       ds, ax
        cmp       byte ptr ds:[gEffectMode], 0
        je    L_4295
        dec       byte ptr ds:[gEffectCounter]
        jne    L_4295
        mov       bx, offset gVoiceRecord0
        mov       al, byte ptr [bx + 28h]
        mov       ah, al
        and       al, 1fh
        xor       byte ptr ds:[gToggleBit], 0ffh
        mov       ah, byte ptr ds:[gToggleBit]
        and       ah, 20h
        or        al, ah
        mov       byte ptr [bx + 28h], al
        mov       bl, al
        mov       dx, 388h
        mov       al, 0b0h
        out       dx, al
        call    _AdlibWaitAfterIndex
        mov       al, bl
        inc       dx
        out       dx, al
        call    _AdlibWaitAfterData
        mov       al, byte ptr ds:[gEffectPeriod]
        mov       byte ptr ds:[gEffectCounter], al
L_4295:
        dec       byte ptr ds:[gTimerPasses12]
        jne    L_42A3
        mov       byte ptr ds:[gTimerPasses12], 0ch
        call    _SoundDriverTick
L_42A3:
        dec       byte ptr ds:[gTimerPasses48]
        jne    L_42B7
        mov       byte ptr ds:[gTimerPasses48], 30h
        pop       ds
        pop       dx
        pop       bx
        pop       ax
        jmp       dword ptr cs:[gSavedTimerOff]
L_42B7:
        mov       al, 20h
        out       20h, al
        pop       ds
        pop       dx
        pop       bx
        pop       ax
        iret
_AdlibTimerIsr ENDP

; 0000:42C0: the INT 08h handler that plays a sample, installed by StartSample (4F57: mov dx,42C0h; int 21h 2508h, 5021 to 5027)
; in place of 424D, with the PIT at divisor 004Bh (5029 to 5039), about 15.9 kHz on the same clock [H]. Each pass reads one 4-bit
; delta from the sample at the far pointer gSamplePtrOff/gSamplePtrSeg (the low nibble of a byte first, then the high one; the count
; gSampleNibbles selects), adds it to gSampleLevel when bit 3 is clear and subtracts (value and 7) when it is set, and writes the
; level to the AdLib data port 389h (the register StartSample selected last, 40h, at 5003 to 500F). The sample ends when the pointer
; is gSampleLength bytes past gSampleStart, or at once when gSampleEnable (set by StartSample and SetSamplePlaying) is zero: then it puts
; the PIT back to 0555h, clears gSamplePtrOff, restores the INT 08h vector StartSample saved in gSampleSavedOff/Seg (the vector 424D
; was installed as) and clears gUpdateEnable (UpdateVoices runs only while it is nonzero).
; QUIRK 42C0: it always sends EOI itself and never chains to the BIOS handler, so the BIOS clock loses the ticks of a sample.
        PUBLIC  _SampleTimerIsr              ; @ 0000:42C0
_SampleTimerIsr PROC NEAR
        push      ax
        push      dx
        push      ds
        push      si
        cmp       byte ptr cs:[gSampleEnable], 0
        je    L_431B
        lds       si, dword ptr cs:[gSamplePtrOff]
        cld
        lodsb
        mov       ah, byte ptr cs:[gSampleLevel]
        test      byte ptr cs:[gSampleNibbles], 1
        je    L_42E9
        shr       al, 1
        shr       al, 1
        shr       al, 1
        shr       al, 1
        inc       si
L_42E9:
        dec       si
        and       al, 0fh
        test      al, 8
        jne    L_42F4
        add       ah, al
        jmp short    L_42F8
L_42F4:
        and       al, 7
        sub       ah, al
L_42F8:
        mov       al, ah
        mov       byte ptr cs:[gSampleLevel], ah
        mov       dx, 389h
        out       dx, al
        dec       byte ptr cs:[gSampleNibbles]
        mov       word ptr cs:[gSamplePtrOff], si
        mov       ax, si
        sub       ax, word ptr cs:[gSampleStart]
        cmp       ax, word ptr cs:[gSampleLength]
        jb    L_4349
L_431B:
        mov       al, 36h
        out       43h, al
        mov       al, 55h
        out       40h, al
        mov       al, 5
        out       40h, al
        mov       word ptr cs:[gSamplePtrOff], 0
        mov       si, 20h
        xor       ax, ax
        mov       ds, ax
        mov       dx, word ptr cs:[gSampleSavedOff]
        mov       ax, word ptr cs:[gSampleSavedSeg]
        mov       word ptr [si], dx
        mov       word ptr [si + 2], ax
        mov       byte ptr cs:[gUpdateEnable], 0
L_4349:
        mov       al, 20h
        out       20h, al
        pop       si
        pop       ds
        pop       dx
        pop       ax
        iret
_SampleTimerIsr ENDP

; 0000:4352: the AdLib driver on: installs the INT 08h handler at 424D, resets the AdLib voices (4516), programs the PIT and resets the queue indices gQueueRead and gQueueWrite (the ring's bytes are left as they were)
        PUBLIC  _StartAdlibDriver              ; @ 0000:4352
_StartAdlibDriver PROC NEAR
        push      ds
        push      es
        push      si
        push      di
        mov       ax, cs
        mov       ds, ax
        xor       bx, bx
        mov       bx, word ptr [bx + gDriverTablePtrs]
        mov       word ptr ds:[gSoundTable], bx
        cli
        mov       ax, 3508h
        int       21h
        mov       word ptr ds:[gSavedTimerSeg], es
        mov       word ptr ds:[gSavedTimerOff], bx
        mov       dx, offset _AdlibTimerIsr
        mov       ax, 2508h
        int       21h
        call    _ResetVoices
        mov       al, 36h
        out       43h, al
        jmp    $+2
L_4383:
        mov       al, 55h
        out       40h, al
        jmp    $+2
L_4389:
        mov       al, 5
        out       40h, al
        in        al, 61h
        and       al, 0feh
        out       61h, al
        mov       word ptr ds:[gQueueRead], 0
        mov       word ptr ds:[gQueueWrite], 0
        sti
        pop       di
        pop       si
        pop       es
        pop       ds
        retf
_StartAdlibDriver ENDP

; 0000:43A5: the AdLib driver off: restores the saved INT 08h vector, resets the AdLib voices and stops the PIT. Callers: ShutdownGame at 0892:0835 (src/I0892.C:444, calls it at line 454; the extern is at I0892.C:149); EOB's E07C8.C:451.
        PUBLIC  _StopAdlibDriver              ; @ 0000:43A5
_StopAdlibDriver PROC NEAR
        push      ds
        mov       ax, cs
        mov       ds, ax
        push      ds
        cli
        mov       dx, word ptr ds:[gSavedTimerOff]
        mov       ax, word ptr ds:[gSavedTimerSeg]
        mov       ds, ax
        mov       ax, 2508h
        int       21h
        sti
        pop       ds
        call    _ResetVoices
        mov       al, 36h
        out       43h, al
        jmp    $+2
L_43C5:
        mov       al, 0
        out       40h, al
        jmp    $+2
L_43CB:
        out       40h, al
        pop       ds
        retf
_StopAdlibDriver ENDP

; 0000:43CF: the driver tick, called from the INT 08h handler at 424D every 0ch passes: processes the ring, the voices and the tempo
        PUBLIC  _SoundDriverTick              ; @ 0000:43CF
_SoundDriverTick PROC NEAR
        push      cx
        push      dx
        push      di
        push      si
        push      ds
        mov       ax, cs
        mov       ds, ax
        call    _ProcessSoundQueue
        call    _UpdateVoices
        call    _TickTempo
        pop       ds
        pop       si
        pop       di
        pop       dx
        pop       cx
        ret
_SoundDriverTick ENDP

; 0000:43E7: queues a driver sound number in the 16-entry ring gQueueRing at the write index gQueueWrite, then advances the index
; (masked with 0Fh); E177D.C calls it with the AdLib number. Far routine, one argument (stored as a word, so its high byte lands in the next
; ring byte; for index 15 the high byte lands at 3F7A, gVoiceRecord0 byte 0: QUIRK 43E7; ACh, ADh and VoiceCarrierLevel read or write that byte)
        PUBLIC  _QueueSound              ; @ 0000:43E7
_QueueSound PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        mov       bx, cs
        mov       ds, bx
        mov       bx, word ptr ds:[gQueueWrite]
        mov       ax, word ptr [bp + 6]
        mov       word ptr [bx + gQueueRing], ax
        inc       bx
        and       bx, 0fh
        mov       word ptr ds:[gQueueWrite], bx
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_QueueSound ENDP

; 0000:4407: drains the 16-entry ring gQueueRing from the read index gQueueRead up to the write index gQueueWrite: for each queued sound
; takes its data pointer from gSoundTable; the first word of the data is the voice number (low byte) and the priority (high byte), and
; when the priority is not below that of the voice's current sound (record byte 8) it clears the voice, loads the script that follows and
; starts it. A sound for voice 0 also clears gEffectMode; VoiceSilence is skipped for voice 9 (the doubled voice number 12h at line 608)
        PUBLIC  _ProcessSoundQueue              ; @ 0000:4407
_ProcessSoundQueue PROC NEAR
        push      ds
        push      si
        push      di
        mov       bx, cs
        mov       ds, bx
L_440E:
        mov       bx, word ptr ds:[gQueueRead]
        cmp       bx, word ptr ds:[gQueueWrite]
        jne    L_441A
        jmp short    L_4475
L_441A:
        mov       bx, word ptr [bx + gQueueRing]
        sub       bh, bh
        shl       bx, 1
        add       bx, word ptr ds:[gSoundTable]
        mov       si, word ptr [bx]
        mov       bx, word ptr cs:[si]
        cmp       bl, 0
        jne    L_4435
        mov       byte ptr ds:[gEffectMode], 0
L_4435:
        mov       al, bh
        mov       cl, bl
        sub       bh, bh
        shl       bx, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        cmp       al, byte ptr [di + 8]
        jl    L_446A
        call    _ClearVoice
        mov       byte ptr [di + 8], al
        inc       si
        inc       si
        mov       word ptr [di + 3], si
        mov       byte ptr [di + 25h], 0ffh
        mov       byte ptr [di + 26h], 0ffh
        mov       byte ptr [di + 5], 1
        mov       byte ptr ds:[gUpdateEnable], 1
        cmp       bl, 12h
        je    L_446A
        call    _VoiceSilence
L_446A:
        inc       word ptr ds:[gQueueRead]
        and       word ptr ds:[gQueueRead], 0fh
        jmp short    L_440E
L_4475:
        pop       di
        pop       si
        pop       ds
        ret
_ProcessSoundQueue ENDP

; 0000:4479: stores the note argument in gEffectIndex, then writes register A0 with the byte of the effect curve gEffectCurveB at that
; index (gEffectCurveA's byte becomes the period gEffectPeriod). Far routine, one argument; the ninth entry of the entry table (3D2C)
        PUBLIC  _SetNote              ; @ 0000:4479
_SetNote PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        push      si
        push      di
        mov       ax, word ptr [bp + 6]
        mov       bx, cs
        mov       ds, bx
        mov       word ptr ds:[gEffectIndex], ax
        and       ax, 7fffh
        mov       bx, ax
        mov       si, word ptr ds:[gEffectCurveA]
        mov       al, byte ptr [bx + si]
        mov       byte ptr ds:[gEffectPeriod], al
        mov       si, word ptr ds:[gEffectCurveB]
        mov       al, byte ptr [bx + si]
        mov       ah, 0a0h
        call    _AdlibWrite
        pop       di
        pop       si
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_SetNote ENDP

; 0000:44A9: sets gSampleEnable, the flag _SampleTimerIsr tests, to the argument (a byte). Far routine, one argument
        PUBLIC  _SetSamplePlaying              ; @ 0000:44A9
_SetSamplePlaying PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        push      si
        push      di
        mov       ax, word ptr [bp + 6]
        mov       bx, cs
        mov       ds, bx
        mov       byte ptr ds:[gSampleEnable], al
        pop       di
        pop       si
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_SetSamplePlaying ENDP

; 0000:44C0: selects the sound-number table: gSoundTable = gDriverTablePtrs[argument] (both entries are 0416h), and returns it. Far routine,
; one argument
        PUBLIC  _SelectDriverTable              ; @ 0000:44C0
_SelectDriverTable PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        mov       bx, cs
        mov       ds, bx
        mov       bx, word ptr [bp + 6]
        shl       bx, 1
        mov       bx, word ptr [bx + gDriverTablePtrs]
        mov       word ptr ds:[gSoundTable], bx
        mov       ax, word ptr ds:[gSoundTable]
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_SelectDriverTable ENDP

; 0000:44DD: returns a driver table pointer for arg 0 to 4: arg 0 returns cs (a segment); args 1 and 2 both return gSoundTable; arg 3 returns gPatchTable (0542h); arg 4 returns gCurveTablePtrs (020Ch)
        PUBLIC  _GetDriverTable              ; @ 0000:44DD
_GetDriverTable PROC NEAR
        push      bp
        mov       bp, sp
        cmp       word ptr [bp + 6], 0
        jne    L_44EA
        mov       ax, cs
        jmp short    L_4512
L_44EA:
        cmp       word ptr [bp + 6], 1
        jne    L_44F6
        mov       ax, word ptr cs:[gSoundTable]
        jmp short    L_4512
L_44F6:
        cmp       word ptr [bp + 6], 2
        jne    L_4500
        mov       ax, word ptr cs:[gSoundTable]
L_4500:
        cmp       word ptr [bp + 6], 3
        jne    L_4509
        mov       ax, offset gPatchTable
L_4509:
        cmp       word ptr [bp + 6], 4
        jne    L_4512
        mov       ax, offset gCurveTablePtrs
L_4512:
        mov       sp, bp
        pop       bp
        retf
_GetDriverTable ENDP

; 0000:4516: writes the driver reset registers, writes 3Fh to registers 40h and 43h to silence voices 0 to 8 (lines 740 to 750), and clears all ten voice records (calls 516D)
        PUBLIC  _ResetVoices              ; @ 0000:4516
_ResetVoices PROC NEAR
        mov       byte ptr ds:[gUpdateEnable], 0
        mov       word ptr ds:[gRandomSeed], 1234h
        mov       ax, 120h
        call    _AdlibWrite
        mov       ax, 800h
        call    _AdlibWrite
        mov       ax, 0bd00h
        call    _AdlibWrite
        mov       byte ptr ds:[gEffectMode], 0
        mov       cx, 0ah
        mov       bh, 0
L_453D:
        mov       bl, cl
        dec       bl
        cmp       bl, 9
        je    L_455E
        mov       ah, byte ptr [bx + gOperatorOffsets]
        add       ah, 40h
        mov       al, 3fh
        call    _AdlibWrite
        mov       ah, byte ptr [bx + gOperatorOffsets]
        add       ah, 43h
        mov       al, 3fh
        call    _AdlibWrite
L_455E:
        shl       bl, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        call    _ClearVoice
        loop    L_453D
        ret
_ResetVoices ENDP

; 0000:456A: the per-voice script interpreter: loops voice 9 down to 0 and runs its command through the 536C jump table
        PUBLIC  _UpdateVoices              ; @ 0000:456A
_UpdateVoices PROC NEAR
        cmp       byte ptr ds:[gUpdateEnable], 0
        jne    L_4572
        ret
L_4572:
        mov       byte ptr ds:[gVoiceIndex], 9
L_4577:
        mov       bl, byte ptr ds:[gVoiceIndex]
        mov       bh, 0
        shl       bl, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        mov       al, byte ptr [di + 5]
        or        al, al
        jne    L_458D
        jmp    L_4640
L_458D:
        mov       bx, offset gOperatorOffsets
        add       bl, byte ptr ds:[gVoiceIndex]
        mov       al, byte ptr [bx]
        mov       byte ptr ds:[gOperatorOffset], al
        cmp       byte ptr [di + 40h], 0
        je    L_45A5
        mov       al, byte ptr ds:[gTempoRate]
        mov       byte ptr [di + 25h], al
L_45A5:
        mov       al, byte ptr [di + 26h]
        add       al, byte ptr [di + 25h]
        mov       byte ptr [di + 26h], al
        jb    L_45B3
        jmp    L_463A
L_45B3:
        dec       byte ptr [di + 5]
        je    L_45D4
        mov       al, byte ptr [di + 5]
        cmp       al, byte ptr [di + 21h]
        jne    L_45C3
        call    _VoiceKeyOff
L_45C3:
        cmp       al, byte ptr [di + 35h]
        jne    L_45D2
        cmp       byte ptr ds:[gVoiceIndex], 9
        je    L_45D2
        call    _VoiceKeyOff
L_45D2:
        jmp short    L_463A
L_45D4:
        mov       si, word ptr [di + 3]
L_45D7:
        lodsw
        or        al, al
        jns    L_45DE
        jmp short    L_4625
L_45DE:
        call    _SetVoicePitch
        call    _VoiceKeyOn
        call    _StartVoiceNote
        cmp       word ptr [di + 32h], 0
        je    L_45EF
        jne    L_45F5
L_45EF:
        cmp       word ptr [di + 33h], 0
        je    L_4619
L_45F5:
        lodsb
        mov       byte ptr [di + 34h], al
        call    _VoiceCarrierLevel
        mov       dx, 388h
        mov       ah, byte ptr ds:[gOperatorOffset]
        add       ah, 43h
        call    _AdlibWrite
        call    _VoiceModulatorLevel
        mov       dx, 388h
        mov       ah, byte ptr ds:[gOperatorOffset]
        add       ah, 40h
        call    _AdlibWrite
L_4619:
        mov       al, byte ptr ds:[gNoteDuration]
        or        al, al
        je    L_45D7
        mov       word ptr [di + 3], si
        jmp short    L_463A
L_4625:
        and       al, 7fh
        cmp       al, 70h
        jle    L_462E
        jmp    L_46E9
L_462E:
        mov       bl, al
        sub       bh, bh
        shl       bl, 1
        mov       bx, word ptr ds:[bx + _CommandTable]
        jmp       bx
L_463A:
        call      word ptr [di + 29h]
        call      word ptr [di + 2bh]
L_4640:
        dec       byte ptr ds:[gVoiceIndex]
        js    L_4649
        jmp    L_4577
L_4649:
        ret
L_464A:                 ; script byte 80h
        mov       byte ptr [di + 6], ah
        jmp short    L_45D7
L_464F:                 ; script byte 81h
        dec       byte ptr [di + 6]
        jne    L_46AC
        inc       si
        jmp    L_45D7
L_4657:                 ; script byte 82h
        push      si
        push      di
        push      bx
        mov       bl, ah
        cmp       bl, 0ffh
        jb    L_4663
        jmp short    L_46A0
L_4663:
        sub       bh, bh
        shl       bx, 1
        add       bx, word ptr ds:[gSoundTable]
        mov       si, word ptr [bx]
        mov       bx, word ptr cs:[si]
        mov       al, bh
        mov       cl, bl
        sub       bh, bh
        shl       bx, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        cmp       al, byte ptr [di + 8]
        jl    L_46A0
        call    _ClearVoice
        mov       byte ptr [di + 8], al
        inc       si
        inc       si
        mov       word ptr [di + 3], si
        mov       byte ptr [di + 25h], 0ffh
        mov       byte ptr [di + 26h], 0ffh
        mov       byte ptr [di + 5], 1
        mov       byte ptr ds:[gUpdateEnable], 1
        call    _VoiceSilence
L_46A0:
        pop       bx
        pop       di
        pop       si
        jmp    L_45D7
L_46A6:                 ; script byte 83h
        mov       byte ptr [di + 35h], ah
        jmp    L_45D7
L_46AC:                 ; script byte 84h
        mov       cl, ah
        lodsb
        xchg      ah, al
        mov       al, cl
        mov       si, ax
        jmp    L_45D7
L_46B8:                 ; script byte 85h
        dec       si
        mov       bl, byte ptr [di + 9]
        mov       bh, 0
        shl       bl, 1
        mov       ax, si
        inc       ax
        inc       ax
        mov       word ptr [bx + di + 0ah], ax
        inc       byte ptr [di + 9]
        lodsw
        mov       si, ax
        jmp    L_45D7
L_46D0:                 ; script byte 86h
        dec       si
        dec       byte ptr [di + 9]
        mov       bl, byte ptr [di + 9]
        mov       bh, 0
        shl       bl, 1
        mov       ax, word ptr [bx + di + 0ah]
        mov       si, ax
        jmp    L_45D7
L_46E3:                 ; script byte 87h
        mov       byte ptr [di + 7], ah
        jmp    L_45D7
; the end-of-script handler: 20 entries of the command table name it (the commands that have no handler of their own), and so does
; the range check of the dispatch (script bytes above F0h). It clears the voice's priority and script pointer and keys the voice off
L_46E9:
        mov       byte ptr [di + 8], 0
        mov       word ptr [di + 3], 0
        cmp       byte ptr ds:[gVoiceIndex], 9
        je    L_46FC
        call    _VoiceKeyOff
L_46FC:
        jmp    L_4640
L_46FF:                 ; script byte 89h
        call    _StartVoiceNote
        mov       word ptr [di + 3], si
        cmp       byte ptr ds:[gVoiceIndex], 9
        je    L_470F
        call    _VoiceKeyOff
L_470F:
        mov       al, byte ptr ds:[gNoteDuration]
        or        al, al
        je    L_4719
        jmp    L_463A
L_4719:
        jmp    L_45D7
L_471C:                 ; script byte 8Ah
        lodsb
        call    _AdlibWrite
        jmp    L_45D7
L_4723:                 ; script byte 8Bh
        xchg      ah, al
        call    _SetVoicePitch
        lodsb
        xchg      ah, al
        call    _StartVoiceNote
        mov       word ptr [di + 3], si
        mov       al, byte ptr ds:[gNoteDuration]
        or        al, al
        je    L_473B
        jmp    L_463A
L_473B:
        jmp    L_45D7
L_473E:                 ; script byte 8Ch
        mov       byte ptr [di + 12h], ah
        jmp    L_45D7
L_4744:                 ; script byte 8Dh
        mov       byte ptr [di + 3ah], ah
        mov       byte ptr [di + 39h], ah
        lodsb
        mov       byte ptr [di + 3bh], al
        mov       byte ptr [di + 3ch], al
        lodsb
        mov       byte ptr [di + 3dh], al
        lodsw
        mov       word ptr [di + 3eh], ax
        mov       word ptr [di + 2bh], offset L_52AF
        jmp    L_45D7
L_4761:                 ; script byte 8Eh
        mov       al, byte ptr ds:[gVoiceIndex]
        push      ax
        push      di
        mov       bl, ah
        mov       bh, 0
        shl       bl, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        mov       byte ptr [di + 5], 0
        mov       byte ptr [di + 8], 0
        mov       word ptr [di + 3], 0
        pop       di
        pop       ax
        mov       byte ptr ds:[gVoiceIndex], al
        jmp    L_45D7
L_4785:                 ; script byte 90h
        push      si
        mov       cl, byte ptr ds:[gOperatorOffset]
        sub       bx, bx
        mov       bl, ah
        shl       bx, 1
        mov       si, word ptr [bx + gPatchTable]
        call    _LoadVoicePatch
        pop       si
        jmp    L_45D7
L_479B:                 ; script byte 91h
        mov       byte ptr [di + 13h], ah
        lodsb
        xchg      ah, al
        lodsb
        mov       word ptr [di + 15h], ax
        mov       word ptr [di + 29h], offset L_52D8
        mov       byte ptr [di + 14h], 0ffh
        jmp    L_45D7
L_47B1:                 ; script byte 92h
        dec       si
        mov       word ptr [di + 29h], offset L_5255
        mov       word ptr [di + 15h], 0
        jmp    L_45D7
L_47BF:                 ; script byte 93h
        mov       byte ptr [di + 24h], ah
        jmp    L_45D7
L_47C5:                 ; script byte 95h
        mov       byte ptr [di + 1dh], ah
        lodsb
        mov       byte ptr [di + 19h], al
        lodsb
        mov       byte ptr [di + 1ah], al
        inc       byte ptr [di + 1ah]
        shl       al, 1
        mov       byte ptr [di + 1bh], al
        lodsb
        mov       byte ptr [di + 1ch], al
        mov       word ptr [di + 29h], offset L_5256
        jmp    L_45D7
L_47E4:                 ; script byte 9Ah
        mov       byte ptr [di + 8], ah
        jmp    L_45D7
L_47EA:                 ; script byte 9Ch
        shr       ah, 1
        mov       byte ptr ds:[gTempoReload], ah
        mov       byte ptr ds:[gTempoCounter], ah
        mov       byte ptr ds:[gTempoAccum], 0ffh
        mov       ah, 0
        mov       byte ptr ds:[gTempoBeat], ah
        mov       byte ptr ds:[gTempoWait], ah
        jmp    L_45D7
L_4806:                 ; script byte 9Dh
        mov       al, byte ptr ds:[gTempoWait]
        or        al, al
        jne    L_4819
        and       ah, byte ptr ds:[gTempoBeat]
        jne    L_4827
        inc       byte ptr ds:[gTempoWait]
        jmp short    L_4827
L_4819:
        and       ah, byte ptr ds:[gTempoBeat]
        je    L_4827
        mov       byte ptr ds:[gTempoWait], 0
        jmp    L_45D7
L_4827:
        dec       si
        dec       si
        mov       byte ptr [di + 5], 1
        mov       word ptr [di + 3], si
        jmp    L_4640
        jmp    L_45D7
L_4836:                 ; script byte 9Eh
        mov       byte ptr [di + 20h], ah
        call    _WriteVoiceLevels
        jmp    L_45D7
L_483F:                 ; script byte A0h
        call    _StartVoiceNote
        mov       word ptr [di + 3], si
        mov       al, byte ptr ds:[gNoteDuration]
        or        al, al
        je    L_484F
        jmp    L_463A
L_484F:
        jmp    L_45D7
L_4852:                 ; script byte A1h
        call    _StartVoiceNote
        call    _VoiceKeyOn
        mov       word ptr [di + 3], si
        mov       al, byte ptr ds:[gNoteDuration]
        or        al, al
        je    L_4865
        jmp    L_463A
L_4865:
        jmp    L_45D7
L_4868:                 ; script byte A4h
        and       ah, 7
        mov       byte ptr [di + 2dh], ah
        jmp    L_45D7
L_4871:                 ; script byte A6h
        mov       byte ptr ds:[gTempoRate], ah
        jmp    L_45D7
L_4878:                 ; script byte A7h
        dec       si
        mov       word ptr [di + 2bh], offset L_5255
        jmp    L_45D7
L_4881:                 ; script byte A9h
        mov       byte ptr [di + 25h], ah
        jmp    L_45D7
L_4887:                 ; script byte ABh
        mov       byte ptr [di + 30h], ah
        jmp    L_45D7
L_488D:                 ; script byte ACh
        mov       al, byte ptr ds:[gVoiceIndex]
        push      ax
        push      di
        lodsb
        mov       byte ptr ds:[gVoiceIndex], ah
        mov       bl, ah
        mov       bh, 0
        shl       bl, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        mov       byte ptr [di], al
        call    _WriteVoiceLevels
        pop       di
        pop       ax
        mov       byte ptr ds:[gVoiceIndex], al
        jmp    L_45D7
L_48AE:                 ; script byte ADh
        mov       al, byte ptr ds:[gVoiceIndex]
        push      ax
        push      di
        mov       byte ptr ds:[gVoiceIndex], ah
        sub       bx, bx
        mov       bl, ah
        lodsb
        shl       bl, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        add       al, byte ptr [di]
        mov       byte ptr [di], al
        call    _WriteVoiceLevels
        pop       di
        pop       ax
        mov       byte ptr ds:[gVoiceIndex], al
        jmp    L_45D7
L_48D1:                 ; script byte AEh
        and       ah, 1
        mov       cl, 7
        shl       ah, cl
        mov       bl, byte ptr ds:[gRegBDDepth]
        and       bl, 7fh
        or        bl, ah
        mov       byte ptr ds:[gRegBDDepth], bl
        mov       ah, 0bdh
        mov       al, bl
        call    _AdlibWrite
        jmp    L_45D7
L_48EF:                 ; script byte AFh
        and       ah, 1
        mov       cl, 6
        shl       ah, cl
        mov       bl, byte ptr ds:[gRegBDDepth]
        and       bl, 0bfh
        or        bl, ah
        mov       byte ptr ds:[gRegBDDepth], bl
        mov       ah, 0bdh
        mov       al, bl
        call    _AdlibWrite
        jmp    L_45D7
L_490D:                 ; script byte B0h
        add       ah, byte ptr [di + 20h]
        mov       byte ptr [di + 20h], ah
        call    _WriteVoiceLevels
        jmp    L_45D7
L_4919:                 ; script byte B3h
        mov       al, byte ptr ds:[gVoiceIndex]
        push      ax
        push      di
        cmp       ah, 0
        jne    L_4928
        mov       byte ptr ds:[gEffectMode], 0
L_4928:
        mov       byte ptr ds:[gVoiceIndex], ah
        mov       bl, ah
        mov       bh, 0
        shl       bl, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        mov       byte ptr [di + 5], 0
        mov       byte ptr [di + 8], 0
        mov       word ptr [di + 3], 0
        mov       byte ptr [di], 0
        cmp       byte ptr ds:[gVoiceIndex], 9
        je    L_497D
        mov       bl, ah
        xor       bh, bh
        mov       cl, byte ptr [bx + gOperatorOffsets]
        mov       ah, 0c0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, 0
        call    _AdlibWrite
        mov       ah, 43h
        add       ah, cl
        mov       al, 3fh
        call    _AdlibWrite
        mov       ah, 83h
        add       ah, cl
        mov       al, 0ffh
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, 0
        call    _AdlibWrite
L_497D:
        pop       di
        pop       ax
        mov       byte ptr ds:[gVoiceIndex], al
        jmp    L_45D7
L_4985:                 ; script byte B5h
        lodsb
        mov       bx, ax
        call    _SoundRandom
        and       ax, bx
        mov       cl, byte ptr [di + 28h]
        mov       bh, cl
        and       cl, 20h
        mov       bl, byte ptr [di + 27h]
        and       bh, 1fh
        add       bx, ax
        or        bh, cl
        mov       ah, 0a0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, bl
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, bh
        call    _AdlibWrite
        jmp    L_45D7
L_49B8:                 ; script byte B6h
        dec       si
        mov       word ptr [di + 29h], offset L_5255
        jmp    L_45D7
L_49C1:                 ; script byte B9h
        mov       byte ptr [di + 42h], ah
        mov       al, byte ptr [di + 41h]
        push      ax
        mov       ch, al
        and       ch, 0f0h
        mov       bl, byte ptr [di + 7]
        add       ch, bl
        and       al, 0fh
        mov       bl, byte ptr [di + 12h]
        add       al, bl
        cmp       al, 0ch
        jge    L_49E8
        cmp       al, 0
        jge    L_49ED
        add       al, 0ch
        sub       ch, 10h
        jmp short    L_49ED
L_49E8:
        sub       al, 0ch
        add       ch, 10h
L_49ED:
        mov       bl, al
        mov       bh, 0
        shl       bl, 1
        mov       ax, word ptr [bx + gFNumbers]
        sub       dx, dx
        mov       dl, byte ptr [di + 24h]
        add       ax, dx
        shr       ch, 1
        shr       ch, 1
        and       ch, 1ch
        or        ch, ah
        mov       cl, al
        cmp       byte ptr [di + 42h], 0
        jl    L_4A30
        xor       bh, bh
        mov       bl, byte ptr [di + 41h]
        and       bl, 0fh
        inc       bl
        inc       bl
        shl       bx, 1
        push      si
        mov       si, word ptr [bx + gLevelCurvePtrs]
        xor       bh, bh
        mov       bl, byte ptr [di + 42h]
        mov       al, byte ptr [bx + si]
        pop       si
        xor       ah, ah
        add       cx, ax
        jmp short    L_4A52
L_4A30:
        xor       bh, bh
        mov       bl, byte ptr [di + 41h]
        and       bl, 0fh
        shl       bx, 1
        push      si
        mov       si, word ptr [bx + gLevelCurvePtrs]
        xor       bh, bh
        mov       bl, byte ptr [di + 42h]
        xor       bl, 0ffh
        inc       bl
        mov       al, byte ptr [bx + si]
        pop       si
        xor       ah, ah
        sub       cx, ax
        jmp    $+2
L_4A52:
        mov       ah, 0a0h
        mov       al, byte ptr [di + 28h]
        and       al, 20h
        or        ch, al
        mov       byte ptr [di + 28h], ch
        mov       byte ptr [di + 27h], cl
        mov       ah, 0a0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, cl
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, ch
        call    _AdlibWrite
        pop       ax
        jmp    L_45D7
L_4A7B:                 ; script byte BAh
        dec       si
        mov       al, byte ptr ds:[gTempoRate]
        mov       byte ptr [di + 25h], al
        jmp    L_45D7
L_4A85:                 ; script byte BBh
        dec       si
        jmp    L_45D7
L_4A89:                 ; script byte BCh
        mov       byte ptr [di + 36h], ah
        jmp    L_45D7
L_4A8F:                 ; script byte BDh
        or        ah, ah
        js    L_4A9C
        add       ah, byte ptr [di + 25h]
        jae    L_4AA6
        mov       ah, 0ffh
        jmp short    L_4AA6
L_4A9C:
        add       ah, byte ptr [di + 25h]
        cmp       ah, byte ptr [di + 25h]
        jb    L_4AA6
        mov       ah, 1
L_4AA6:
        mov       byte ptr [di + 25h], ah
        jmp    L_45D7
L_4AAC:                 ; script byte BFh
        mov       byte ptr ds:[gEffectMode], ah
        lodsb
        push      si
        sub       bh, bh
        mov       bl, al
        shl       bx, 1
        mov       bx, word ptr [bx + gCurveTablePtrs]
        mov       word ptr ds:[gEffectCurveA], bx
        inc       al
        sub       bh, bh
        mov       bl, al
        shl       bx, 1
        mov       bx, word ptr [bx + gCurveTablePtrs]
        mov       word ptr ds:[gEffectCurveB], bx
        cmp       ah, 2
        jne    L_4AF4
        mov       bx, 0
        mov       word ptr ds:[gEffectIndex], bx
        mov       si, word ptr ds:[gEffectCurveA]
        mov       al, byte ptr [bx + si]
        mov       byte ptr ds:[gEffectPeriod], al
        mov       si, word ptr ds:[gEffectCurveB]
        mov       al, byte ptr [bx + si]
        mov       ah, 0a0h
        call    _AdlibWrite
        pop       si
        jmp    L_45D7
L_4AF4:
        pop       si
        jmp    L_45D7
L_4AF8:                 ; script byte C0h
        dec       si
        mov       byte ptr ds:[gEffectMode], 0
        jmp    L_45D7
L_4B01:                 ; script byte C1h
        mov       al, byte ptr ds:[gVoiceIndex]
        push      ax
        mov       al, byte ptr ds:[gOperatorOffset]
        push      ax
        push      si
        mov       byte ptr ds:[gVoiceIndex], 6
        mov       bx, offset gOperatorOffsets
        add       bl, 6
        mov       al, byte ptr [bx]
        mov       byte ptr ds:[gOperatorOffset], al
        mov       dx, 388h
        mov       cl, byte ptr ds:[gOperatorOffset]
        sub       bx, bx
        mov       bl, ah
        shl       bx, 1
        mov       si, word ptr [bx + gPatchTable]
        mov       al, byte ptr [si + 6]
        mov       byte ptr ds:[gRhythmLevelA], al
        call    _LoadVoicePatch
        pop       si
        lodsb
        push      si
        mov       byte ptr ds:[gVoiceIndex], 7
        mov       bx, offset gOperatorOffsets
        add       bl, 7
        mov       ah, byte ptr [bx]
        mov       byte ptr ds:[gOperatorOffset], ah
        mov       dx, 388h
        mov       cl, byte ptr ds:[gOperatorOffset]
        sub       bx, bx
        mov       bl, al
        shl       bx, 1
        mov       si, word ptr [bx + gPatchTable]
        mov       al, byte ptr [si + 5]
        mov       byte ptr ds:[gRhythmLevelA+2], al
        mov       al, byte ptr [si + 6]
        mov       byte ptr ds:[gRhythmLevelA+1], al
        call    _LoadVoicePatch
        pop       si
        lodsb
        push      si
        mov       byte ptr ds:[gVoiceIndex], 8
        mov       bx, offset gOperatorOffsets
        add       bl, 8
        mov       ah, byte ptr [bx]
        mov       byte ptr ds:[gOperatorOffset], ah
        mov       dx, 388h
        mov       cl, byte ptr ds:[gOperatorOffset]
        sub       bx, bx
        mov       bl, al
        shl       bx, 1
        mov       si, word ptr [bx + gPatchTable]
        mov       al, byte ptr [si + 5]
        mov       byte ptr ds:[gRhythmLevelA+3], al
        mov       al, byte ptr [si + 6]
        mov       byte ptr ds:[gRhythmLevelA+4], al
        call    _LoadVoicePatch
        pop       si
        lodsb
        and       al, 2fh
        mov       byte ptr ds:[gVoiceRecord6+28h], al
        mov       ah, 0b6h
        call    _AdlibWrite
        lodsb
        mov       ah, 0a6h
        call    _AdlibWrite
        lodsb
        and       al, 2fh
        mov       byte ptr ds:[gVoiceRecord7+28h], al
        mov       ah, 0b7h
        call    _AdlibWrite
        lodsb
        mov       ah, 0a7h
        call    _AdlibWrite
        lodsb
        and       al, 2fh
        mov       byte ptr ds:[gVoiceRecord8+28h], al
        mov       ah, 0b8h
        call    _AdlibWrite
        lodsb
        mov       ah, 0a8h
        call    _AdlibWrite
        mov       byte ptr ds:[gRhythmKeyBits], 20h
        pop       ax
        mov       byte ptr ds:[gOperatorOffset], al
        pop       ax
        mov       byte ptr ds:[gVoiceIndex], al
        jmp    L_45D7
L_4BE0:                 ; script byte C2h
        mov       dx, 388h
        mov       al, 0bdh
        out       dx, al
        call    _AdlibWaitAfterIndex
        inc       dx
        xchg      ah, al
        push      ax
        and       al, 1fh
        xor       al, 0ffh
        and       al, byte ptr ds:[gRhythmKeyBits]
        or        al, 20h
        out       dx, al
        call    _AdlibWaitAfterData
        pop       ax
        or        al, byte ptr ds:[gRhythmKeyBits]
        mov       byte ptr ds:[gRhythmKeyBits], al
        or        al, byte ptr ds:[gRegBDDepth]
        or        al, 20h
        out       dx, al
        call    _AdlibWaitAfterData
        jmp    L_45D7
L_4C10:                 ; script byte C3h
        dec       si
        mov       al, byte ptr ds:[gRegBDDepth]
        mov       byte ptr ds:[gRhythmKeyBits], 0
        and       al, 0c0h
        mov       ah, 0bdh
        call    _AdlibWrite
        jmp    L_45D7
L_4C23:                 ; script byte C4h
        lodsb
        mov       bx, ax
        test      ah, 1
        je    L_4C44
        mov       byte ptr ds:[gRhythmLevelC+2], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+2]
        add       al, byte ptr ds:[gRhythmLevelB+2]
        add       al, byte ptr ds:[gRhythmLevelC+2]
        call    _ClampLevel
        mov       ah, 51h
        call    _AdlibWrite
L_4C44:
        mov       ax, bx
        test      ah, 2
        je    L_4C64
        mov       byte ptr ds:[gRhythmLevelC+4], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+4]
        add       al, byte ptr ds:[gRhythmLevelB+4]
        add       al, byte ptr ds:[gRhythmLevelC+4]
        call    _ClampLevel
        mov       ah, 55h
        call    _AdlibWrite
L_4C64:
        mov       ax, bx
        test      ah, 4
        je    L_4C84
        mov       byte ptr ds:[gRhythmLevelC+3], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+3]
        add       al, byte ptr ds:[gRhythmLevelB+3]
        add       al, byte ptr ds:[gRhythmLevelC+3]
        call    _ClampLevel
        mov       ah, 52h
        call    _AdlibWrite
L_4C84:
        mov       ax, bx
        test      ah, 8
        je    L_4CA4
        mov       byte ptr ds:[gRhythmLevelC+1], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+1]
        add       al, byte ptr ds:[gRhythmLevelB+1]
        add       al, byte ptr ds:[gRhythmLevelC+1]
        call    _ClampLevel
        mov       ah, 54h
        call    _AdlibWrite
L_4CA4:
        mov       ax, bx
        test      ah, 10h
        je    L_4CC4
        mov       byte ptr ds:[gRhythmLevelC], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA]
        add       al, byte ptr ds:[gRhythmLevelB]
        add       al, byte ptr ds:[gRhythmLevelC]
        call    _ClampLevel
        mov       ah, 53h
        call    _AdlibWrite
L_4CC4:
        jmp    L_45D7
L_4CC7:                 ; script byte C5h
        lodsb
        mov       bx, ax
        test      ah, 1
        je    L_4CE8
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+2]
        add       al, byte ptr ds:[gRhythmLevelB+2]
        add       al, byte ptr ds:[gRhythmLevelC+2]
        call    _ClampLevel
        mov       byte ptr ds:[gRhythmLevelB+2], al
        mov       ah, 51h
        call    _AdlibWrite
L_4CE8:
        mov       ax, bx
        test      ah, 2
        je    L_4D08
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+4]
        add       al, byte ptr ds:[gRhythmLevelB+4]
        add       al, byte ptr ds:[gRhythmLevelC+4]
        call    _ClampLevel
        mov       byte ptr ds:[gRhythmLevelB+4], al
        mov       ah, 55h
        call    _AdlibWrite
L_4D08:
        mov       ax, bx
        test      ah, 4
        je    L_4D28
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+3]
        add       al, byte ptr ds:[gRhythmLevelB+3]
        add       al, byte ptr ds:[gRhythmLevelC+3]
        call    _ClampLevel
        mov       byte ptr ds:[gRhythmLevelB+3], al
        mov       ah, 52h
        call    _AdlibWrite
L_4D28:
        mov       ax, bx
        test      ah, 8
        je    L_4D48
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+1]
        add       al, byte ptr ds:[gRhythmLevelB+1]
        add       al, byte ptr ds:[gRhythmLevelC+1]
        call    _ClampLevel
        mov       byte ptr ds:[gRhythmLevelB+1], al
        mov       ah, 54h
        call    _AdlibWrite
L_4D48:
        mov       ax, bx
        test      ah, 10h
        je    L_4D68
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA]
        add       al, byte ptr ds:[gRhythmLevelB]
        add       al, byte ptr ds:[gRhythmLevelC]
        call    _ClampLevel
        mov       byte ptr ds:[gRhythmLevelB], al
        mov       ah, 53h
        call    _AdlibWrite
L_4D68:
        jmp    L_45D7
L_4D6B:                 ; script byte C6h
        lodsb
        mov       bx, ax
        test      ah, 1
        je    L_4D88
        mov       byte ptr ds:[gRhythmLevelB+2], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+2]
        add       al, byte ptr ds:[gRhythmLevelC+2]
        call    _ClampLevel
        mov       ah, 51h
        call    _AdlibWrite
L_4D88:
        mov       ax, bx
        test      ah, 2
        je    L_4DA4
        mov       byte ptr ds:[gRhythmLevelB+4], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+4]
        add       al, byte ptr ds:[gRhythmLevelC+4]
        call    _ClampLevel
        mov       ah, 55h
        call    _AdlibWrite
L_4DA4:
        mov       ax, bx
        test      ah, 4
        je    L_4DC0
        mov       byte ptr ds:[gRhythmLevelB+3], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+3]
        add       al, byte ptr ds:[gRhythmLevelC+3]
        call    _ClampLevel
        mov       ah, 52h
        call    _AdlibWrite
L_4DC0:
        mov       ax, bx
        test      ah, 8
        je    L_4DDC
        mov       byte ptr ds:[gRhythmLevelB+1], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA+1]
        add       al, byte ptr ds:[gRhythmLevelC+1]
        call    _ClampLevel
        mov       ah, 54h
        call    _AdlibWrite
L_4DDC:
        mov       ax, bx
        test      ah, 10h
        je    L_4DF8
        mov       byte ptr ds:[gRhythmLevelB], al
        xor       ah, ah
        add       al, byte ptr ds:[gRhythmLevelA]
        add       al, byte ptr ds:[gRhythmLevelC]
        call    _ClampLevel
        mov       ah, 53h
        call    _AdlibWrite
L_4DF8:
        jmp    L_45D7
L_4DFB:                 ; script byte C7h
        mov       byte ptr ds:[gScriptFlag], ah
        jmp    L_45D7
L_4E02:                 ; script byte C8h
        mov       byte ptr [di + 40h], ah
        jmp    L_45D7
L_4E08:                 ; script byte C9h
        mov       byte ptr [di + 32h], ah
        lodsb
        mov       byte ptr [di + 33h], al
        jmp    L_45D7
_UpdateVoices ENDP

; 0000:4E12: builds the voice frequency and octave from its record and writes registers A0 and B0; on entry AL is the note, stored at [di+41h] (line 1718), and the pitch-curve adjust from [di+42h] follows (lines 1749 to 1781)
        PUBLIC  _SetVoicePitch              ; @ 0000:4E12
_SetVoicePitch PROC NEAR
        push      ax
        mov       byte ptr [di + 41h], al
        mov       ch, al
        and       ch, 0f0h
        mov       bl, byte ptr [di + 7]
        add       ch, bl
        and       al, 0fh
        mov       bl, byte ptr [di + 12h]
        add       al, bl
        cmp       al, 0ch
        jge    L_4E36
        cmp       al, 0
        jge    L_4E3B
        add       al, 0ch
        sub       ch, 10h
        jmp short    L_4E3B
L_4E36:
        sub       al, 0ch
        add       ch, 10h
L_4E3B:
        mov       bl, al
        mov       bh, 0
        shl       bl, 1
        mov       ax, word ptr [bx + gFNumbers]
        sub       dx, dx
        mov       dl, byte ptr [di + 24h]
        add       ax, dx
        shr       ch, 1
        shr       ch, 1
        and       ch, 1ch
        or        ch, ah
        mov       cl, al
        cmp       byte ptr [di + 42h], 0
        je    L_4EA0
        jl    L_4E80
        xor       bh, bh
        mov       bl, byte ptr [di + 41h]
        and       bl, 0fh
        inc       bl
        inc       bl
        shl       bx, 1
        push      si
        mov       si, word ptr [bx + gLevelCurvePtrs]
        xor       bh, bh
        mov       bl, byte ptr [di + 42h]
        mov       al, byte ptr [bx + si]
        pop       si
        xor       ah, ah
        add       cx, ax
        jmp short    L_4EA0
L_4E80:
        xor       bh, bh
        mov       bl, byte ptr [di + 41h]
        and       bl, 0fh
        shl       bx, 1
        push      si
        mov       si, word ptr [bx + gLevelCurvePtrs]
        xor       bh, bh
        mov       bl, byte ptr [di + 42h]
        xor       bl, 0ffh
        inc       bl
        mov       al, byte ptr [bx + si]
        pop       si
        xor       ah, ah
        sub       cx, ax
L_4EA0:
        mov       ah, 0a0h
        mov       al, byte ptr [di + 28h]
        and       al, 20h
        or        ch, al
        mov       byte ptr [di + 28h], ch
        mov       byte ptr [di + 27h], cl
        mov       ah, 0a0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, cl
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, ch
        call    _AdlibWrite
        pop       ax
        ret
_SetVoicePitch ENDP

; 0000:4EC7: writes register B0 for the voice key-on gate and computes its period
        PUBLIC  _VoiceKeyOn              ; @ 0000:4EC7
_VoiceKeyOn PROC NEAR
        push      ax
        mov       ah, 0b0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, byte ptr [di + 28h]
        or        al, 20h
        mov       byte ptr [di + 28h], al
        call    _AdlibWrite
        mov       cl, byte ptr [di + 19h]
        mov       al, 9
        sub       al, cl
        mov       cl, al
        mov       ax, word ptr [di + 27h]
        and       ax, 3ffh
        shr       ax, cl
        mov       ah, 0
        mov       word ptr [di + 17h], ax
        mov       cl, byte ptr [di + 1ch]
        mov       byte ptr [di + 1fh], cl
        pop       ax
        ret
_VoiceKeyOn ENDP

; 0000:4EF7: clears the key-on bit of the voice and writes register B0; while rhythm mode is on (gRhythmKeyBits nonzero) it skips voices 6 to 9, and voice 9 gets no B0 write
        PUBLIC  _VoiceKeyOff              ; @ 0000:4EF7
_VoiceKeyOff PROC NEAR
        cmp       byte ptr ds:[gRhythmKeyBits], 0
        je    L_4F06
        cmp       byte ptr ds:[gVoiceIndex], 6
        jl    L_4F06
        ret
L_4F06:
        cmp       byte ptr ds:[gVoiceIndex], 9
        je    L_4F1E
        mov       ah, 0b0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, byte ptr [di + 28h]
        and       al, 0dfh
        mov       byte ptr [di + 28h], al
        call    _AdlibWrite
L_4F1E:
        ret
_VoiceKeyOff ENDP

; 0000:4F1F: starts the note of a voice: stores AH (the note's duration) in gNoteDuration and in the voice's delay counter [di+5], adding a random part when [di+36h] is set; the 2dh shift multiplies into the key-off point [di+21h]
        PUBLIC  _StartVoiceNote              ; @ 0000:4F1F
_StartVoiceNote PROC NEAR
        mov       byte ptr ds:[gNoteDuration], ah
        cmp       byte ptr [di + 36h], 0
        mov       bl, ah
        je    L_4F37
        call    _SoundRandom
        and       al, byte ptr [di + 36h]
        add       bl, al
        mov       byte ptr [di + 5], bl
        ret
L_4F37:
        cmp       byte ptr [di + 2dh], 0
        je    L_4F53
        mov       bl, ah
        shr       ah, 1
        shr       ah, 1
        shr       ah, 1
        mov       cl, byte ptr [di + 2dh]
        xor       ch, ch
        xor       al, al
L_4F4C:
        add       al, ah
        loop    L_4F4C
        mov       byte ptr [di + 21h], al
L_4F53:
        mov       byte ptr [di + 5], bl
        ret
_StartVoiceNote ENDP

; 0000:4F57: the AdLib sample driver on: takes a far sample pointer at [bp+6] and a length at [bp+0ah], programs the PIT (divisor 4Bh) and the sample's AdLib registers (40h = 0), and installs the INT 08h handler at 42C0
        PUBLIC  _StartSample              ; @ 0000:4F57
_StartSample PROC NEAR
        push      bp
        mov       bp, sp
        push      es
        push      ds
        push      si
        mov       al, 36h
        out       43h, al
        xor       ax, ax
        out       40h, al
        out       40h, al
        mov       ax, 2021h
        call    _AdlibWrite
        mov       ax, 60f0h
        call    _AdlibWrite
        mov       ax, 80f0h
        call    _AdlibWrite
        mov       ax, 0c001h
        call    _AdlibWrite
        mov       ax, 0e000h
        call    _AdlibWrite
        mov       ax, 433fh
        call    _AdlibWrite
        mov       ax, 0b000h
        call    _AdlibWrite
        mov       ax, 0a000h
        call    _AdlibWrite
        mov       ax, 0a08fh
        call    _AdlibWrite
        mov       ax, 0b02eh
        call    _AdlibWrite
        cli
        mov       al, 0
        out       43h, al
        in        al, 40h
        mov       bl, al
        in        al, 40h
        mov       bh, al
L_4FB0:
        mov       al, 0
        out       43h, al
        in        al, 40h
        mov       cl, al
        in        al, 40h
        mov       ch, al
        neg       cx
        add       cx, bx
        cmp       cx, 952h
        jb    L_4FB0
        mov       ax, 0b020h
        call    _AdlibWrite
        mov       ax, 0a000h
        call    _AdlibWrite
        sti
        lds       si, [bp + 6]
        mov       ax, si
        mov       word ptr cs:[gSamplePtrOff], ax
        mov       word ptr cs:[gSampleStart], ax
        mov       ax, ds
        mov       word ptr cs:[gSamplePtrSeg], ax
        mov       cx, word ptr [bp + 0ah]
        mov       word ptr cs:[gSampleLength], cx
        mov       byte ptr cs:[gSampleLevel], 8
        mov       byte ptr cs:[gSampleNibbles], 0
        mov       ax, cs
        mov       ds, ax
        mov       byte ptr ds:[gSampleEnable], 1
        mov       dx, 388h
        mov       al, 40h
        out       dx, al
        call    _AdlibWaitAfterIndex
        mov       al, 0
        inc       dx
        out       dx, al
        call    _AdlibWaitAfterData
        cli
        mov       ax, 3508h
        int       21h
        mov       word ptr ds:[gSampleSavedSeg], es
        mov       word ptr ds:[gSampleSavedOff], bx
        mov       dx, offset _SampleTimerIsr
        mov       ax, 2508h
        int       21h
        mov       al, 36h
        out       43h, al
        call    _AdlibWaitAfterIndex
        mov       al, 4bh
        out       40h, al
        call    _AdlibWaitAfterIndex
        mov       al, 0
        out       40h, al
        sti
        pop       si
        pop       ds
        pop       es
        mov       bp, sp
        pop       bp
        retf
_StartSample ENDP

; 0000:5043: writes the value AL to the AdLib register AH: the register number to port 388h, a short wait, the value to port 389h, a
; long wait (AdlibWaitAfterIndex, AdlibWaitAfterData). Clobbers dx; keeps ax. Every register write of the driver goes through here.
        PUBLIC  _AdlibWrite              ; @ 0000:5043
_AdlibWrite PROC NEAR
        mov       dx, 388h
        xchg      al, ah
        out       dx, al
        call    _AdlibWaitAfterIndex
        inc       dx
        xchg      al, ah
        out       dx, al
        call    _AdlibWaitAfterData
        ret
_AdlibWrite ENDP

; 0000:5054: clamps the signed level in AX to 0 to 3Fh, the range of an AdLib level register's total level bits; the result is in AL
        PUBLIC  _ClampLevel              ; @ 0000:5054
_ClampLevel PROC NEAR
        cmp       ax, 0
        jl    L_505F
        cmp       ax, 3fh
        jg    L_5062
        ret
L_505F:
        mov       al, 0
        ret
L_5062:
        mov       al, 3fh
        ret
_ClampLevel ENDP

; 0000:5065: advances the tempo counters at 3F44/3F43 and the beat count gTempoBeat (3F45)
        PUBLIC  _TickTempo              ; @ 0000:5065
_TickTempo PROC NEAR
        mov       al, byte ptr ds:[gTempoAccum]
        add       al, byte ptr ds:[gTempoRate]
        mov       byte ptr ds:[gTempoAccum], al
        jb    L_5072
        ret
L_5072:
        dec       byte ptr ds:[gTempoCounter]
        je    L_5079
        ret
L_5079:
        mov       al, byte ptr ds:[gTempoReload]
        mov       byte ptr ds:[gTempoCounter], al
        inc       byte ptr ds:[gTempoBeat]
        ret
_TickTempo ENDP

; 0000:5084: returns ax = the sample read pointer's offset (the word at 3F5F), zero when no sample plays. Far routine, no arguments;
; the seventh entry of the driver's entry table (3D2C).
        PUBLIC  _GetSampleOffset              ; @ 0000:5084
_GetSampleOffset PROC NEAR
        mov       ax, word ptr cs:[gSamplePtrOff]
        retf
_GetSampleOffset ENDP

; 0000:5089: returns the byte at +5 of the voice record of the voice given as the argument (0 to 9), the voice's note delay counter
; (StartVoiceNote sets it, UpdateVoices counts it down): nonzero while the voice is sounding. Far routine, one argument; the sixth entry of
; the entry table (3D2C)
        PUBLIC  _GetVoiceState              ; @ 0000:5089
_GetVoiceState PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        push      di
        mov       ax, cs
        mov       ds, ax
        mov       bx, word ptr [bp + 6]
        sub       bh, bh
        shl       bx, 1
        mov       di, word ptr [bx + gVoiceRecordPtrs]
        mov       al, byte ptr [di + 5]
        xor       ah, ah
        pop       di
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_GetVoiceState ENDP

; 0000:50A8: the wait after an AdLib data write: reads the status port 388h 17h (23) times; keeps ax, cx and dx
        PUBLIC  _AdlibWaitAfterData              ; @ 0000:50A8
_AdlibWaitAfterData PROC NEAR
        push      ax
        push      cx
        push      dx
        mov       dx, 388h
        mov       cx, 17h
L_50B1:
        in        al, dx
        loop    L_50B1
        pop       dx
        pop       cx
        pop       ax
        ret
_AdlibWaitAfterData ENDP

; 0000:50B8: the wait after an AdLib register number write: reads the status port 388h 4 times; keeps ax, cx and dx
        PUBLIC  _AdlibWaitAfterIndex              ; @ 0000:50B8
_AdlibWaitAfterIndex PROC NEAR
        push      ax
        push      cx
        push      dx
        mov       dx, 388h
        mov       cx, 4
L_50C1:
        in        al, dx
        loop    L_50C1
        pop       dx
        pop       cx
        pop       ax
        ret
_AdlibWaitAfterIndex ENDP

; 0000:50C8: writes register 43h (carrier level) of the voice's operator (gOperatorOffsets, 422C) and, when record byte 31h (the connection bit) is set, register 40h (modulator level, an additive voice)
        PUBLIC  _WriteVoiceLevels              ; @ 0000:50C8
_WriteVoiceLevels PROC NEAR
        call    _VoiceCarrierLevel
        mov       bx, offset gOperatorOffsets
        add       bl, byte ptr ds:[gVoiceIndex]
        mov       cl, byte ptr [bx]
        mov       ah, cl
        add       ah, 43h
        call    _AdlibWrite
        cmp       byte ptr [di + 31h], 0
        je    L_50F6
        call    _VoiceModulatorLevel
        mov       bx, offset gOperatorOffsets
        add       bl, byte ptr ds:[gVoiceIndex]
        mov       cl, byte ptr [bx]
        mov       ah, cl
        add       ah, 40h
        call    _AdlibWrite
L_50F6:
        ret
_WriteVoiceLevels ENDP

; 0000:50F7: computes the value of the carrier operator's level register (43h plus the operator offset) for the voice record at di:
; the level bits (3Fh) of record byte 2Fh (the patch's carrier byte, set by LoadVoicePatch) plus the bytes at 20h, 00h and 30h, less the high
; byte of byte 34h shifted left by byte 33h + 1 when byte 33h is nonzero, clamped to 0 to 3Fh; the top two bits (key scale) of byte 2Fh are
; kept. Result in AL
        PUBLIC  _VoiceCarrierLevel              ; @ 0000:50F7
_VoiceCarrierLevel PROC NEAR
        mov       al, byte ptr [di + 2fh]
        and       al, 3fh
        add       al, byte ptr [di + 20h]
        add       al, byte ptr [di]
        add       al, byte ptr [di + 30h]
        cmp       byte ptr [di + 33h], 0
        je    L_5118
        mov       cl, byte ptr [di + 33h]
        inc       cl
        xor       bh, bh
        mov       bl, byte ptr [di + 34h]
        shl       bx, cl
        sub       al, bh
L_5118:
        cmp       al, 3fh
        jbe    L_5126
        cmp       al, 0
        jl    L_5124
        mov       al, 3fh
        jmp short    L_5126
L_5124:
        mov       al, 0
L_5126:
        mov       bl, byte ptr [di + 2fh]
        and       bl, 0c0h
        or        al, bl
        ret
_VoiceCarrierLevel ENDP

; 0000:512F: the same for the modulator operator's level register (40h plus the operator offset), from record byte 2Eh: the bytes at 20h,
; 00h and 30h are added only when byte 31h (the patch's connection bit, additive: both operators sound) is set, and the shift is by byte
; 32h + 1. Result in AL
        PUBLIC  _VoiceModulatorLevel              ; @ 0000:512F
_VoiceModulatorLevel PROC NEAR
        mov       al, byte ptr [di + 2eh]
        and       al, 3fh
        cmp       byte ptr [di + 31h], 0
        je    L_5142
        add       al, byte ptr [di + 20h]
        add       al, byte ptr [di]
        add       al, byte ptr [di + 30h]
L_5142:
        cmp       byte ptr [di + 32h], 0
        je    L_5156
        mov       cl, byte ptr [di + 32h]
        inc       cl
        xor       bh, bh
        mov       bl, byte ptr [di + 34h]
        shl       bx, cl
        sub       al, bh
L_5156:
        cmp       al, 3fh
        jbe    L_5164
        cmp       al, 0
        jl    L_5162
        mov       al, 3fh
        jmp short    L_5164
L_5162:
        mov       al, 0
L_5164:
        mov       bl, byte ptr [di + 2eh]
        and       bl, 0c0h
        or        al, bl
        ret
_VoiceModulatorLevel ENDP

; 0000:516D: zeroes the 40h-byte voice record from offset 3 (the script pointer [di+3] is in the zeroed range) and sets the per-tick handlers [di+29h] and [di+2Bh] to L_5255 (a ret, lines 2210 to 2211)
        PUBLIC  _ClearVoice              ; @ 0000:516D
_ClearVoice PROC NEAR
        push      es
        push      ax
        push      cx
        mov       cx, ds
        mov       es, cx
        mov       cx, 40h
        sub       ax, ax
        push      di
        add       di, 3
        shr       cx, 1
        repne stosw
        adc       cx, cx
        repne stosb
        pop       di
        mov       byte ptr [di + 25h], 0ffh
        mov       byte ptr [di + 8], 0
        mov       word ptr [di + 29h], offset L_5255
        mov       word ptr [di + 2bh], offset L_5255
        mov       byte ptr [di + 35h], 1
        pop       cx
        pop       ax
        pop       es
        ret
_ClearVoice ENDP

; 0000:51A0: writes a voice's patch (instrument), the 11 bytes at si, to the AdLib: registers 20h, 23h (tremolo, vibrato, sustain hold, key scale rate
; and multiplier of the two operators), C0h plus the voice (feedback and connection, whose bit 0 it keeps in record byte 31h), E0h, E3h
; (waveforms), 40h (modulator level, through VoiceModulatorLevel, base byte kept at +2Eh), 43h (carrier level, base byte at +2Fh), 60h, 63h,
; 80h, 83h (attack, decay, sustain, release). cl = the operator offset of the voice (gOperatorOffsets), di = its record. What each
; register does is the OPL2's [H]; the oracle shows 40h to 55h taking 3Fh as full attenuation (WHITEPAPER 2.7, E177D.C:32 to 33)
        PUBLIC  _LoadVoicePatch              ; @ 0000:51A0
_LoadVoicePatch PROC NEAR
        mov       ah, 20h
        add       ah, cl
        lodsb
        call    _AdlibWrite
        mov       ah, 23h
        add       ah, cl
        lodsb
        call    _AdlibWrite
        mov       ah, 0c0h
        add       ah, byte ptr ds:[gVoiceIndex]
        lodsb
        call    _AdlibWrite
        and       al, 1
        mov       byte ptr [di + 31h], al
        mov       ah, 0e0h
        add       ah, cl
        lodsb
        call    _AdlibWrite
        mov       ah, 0e3h
        add       ah, cl
        lodsb
        call    _AdlibWrite
        mov       ah, 40h
        add       ah, cl
        lodsb
        mov       byte ptr [di + 2eh], al
        call    _VoiceModulatorLevel
        call    _AdlibWrite
        mov       ah, 43h
        add       ah, cl
        lodsb
        mov       byte ptr [di + 2fh], al
        call    _VoiceCarrierLevel
        call    _AdlibWrite
        mov       ah, 60h
        add       ah, cl
        lodsb
        call    _AdlibWrite
        mov       ah, 63h
        add       ah, cl
        lodsb
        call    _AdlibWrite
        mov       ah, 80h
        add       ah, cl
        lodsb
        call    _AdlibWrite
        mov       ah, 83h
        add       ah, cl
        lodsb
        call    _AdlibWrite
        ret
_LoadVoicePatch ENDP

; 0000:520C: silences a voice: writes ffh to the attack/decay and sustain/release registers (60h, 63h, 80h, 83h), writes B0 = 0 and then B0 = 20h (key-on set); with rhythm mode on, voices 6 and up return at once.
        PUBLIC  _VoiceSilence              ; @ 0000:520C
_VoiceSilence PROC NEAR
        cmp       byte ptr ds:[gRhythmKeyBits], 0
        je    L_5219
        cmp       cl, 6
        jl    L_5219
        ret
L_5219:
        mov       bx, offset gOperatorOffsets
        mov       ch, cl
        add       bl, cl
        mov       cl, byte ptr [bx]
        mov       ah, 60h
        add       ah, cl
        mov       al, 0ffh
        call    _AdlibWrite
        mov       ah, 63h
        add       ah, cl
        mov       al, 0ffh
        call    _AdlibWrite
        mov       ah, 80h
        add       ah, cl
        mov       al, 0ffh
        call    _AdlibWrite
        mov       ah, 83h
        add       ah, cl
        mov       al, 0ffh
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, ch
        mov       al, 0
        call    _AdlibWrite
        mov       al, 20h
        call    _AdlibWrite
        ret
L_5255:
        ret
L_5256:
        cmp       byte ptr [di + 1fh], 0
        je    L_5260
        dec       byte ptr [di + 1fh]
        ret
L_5260:
        mov       cl, byte ptr [di + 1eh]
        add       cl, byte ptr [di + 1dh]
        mov       byte ptr [di + 1eh], cl
        jb    L_526C
        ret
L_526C:
        mov       bx, word ptr [di + 17h]
        dec       byte ptr [di + 1ah]
        jne    L_5281
        xor       bx, 0ffffh
        inc       bx
        mov       word ptr [di + 17h], bx
        mov       al, byte ptr [di + 1bh]
        mov       byte ptr [di + 1ah], al
L_5281:
        mov       ax, word ptr [di + 27h]
        and       ax, 3ffh
        add       ax, bx
        mov       byte ptr [di + 27h], al
        mov       al, byte ptr [di + 28h]
        and       al, 0fch
        or        al, ah
        mov       byte ptr [di + 28h], al
        mov       ah, 0a0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, byte ptr [di + 27h]
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, byte ptr [di + 28h]
        call    _AdlibWrite
        ret
L_52AF:
        mov       al, byte ptr [di + 3ah]
        add       al, byte ptr [di + 39h]
        mov       byte ptr [di + 3ah], al
        jb    L_52BB
        ret
L_52BB:
        dec       byte ptr [di + 3ch]
        jns    L_52C6
        mov       al, byte ptr [di + 3bh]
        mov       byte ptr [di + 3ch], al
L_52C6:
        mov       ah, byte ptr [di + 3dh]
        add       ah, byte ptr ds:[gOperatorOffset]
        mov       al, byte ptr [di + 3ch]
        mov       bx, word ptr [di + 3eh]
        xlatb
        call    _AdlibWrite
        ret
L_52D8:
        mov       cl, byte ptr [di + 14h]
        add       cl, byte ptr [di + 13h]
        mov       byte ptr [di + 14h], cl
        jb    L_52E4
        ret
L_52E4:
        mov       bl, byte ptr [di + 27h]
        mov       bh, byte ptr [di + 28h]
        and       bh, 3
        mov       dl, byte ptr [di + 28h]
        mov       dh, dl
        and       dl, 1ch
        and       dh, 20h
        mov       cx, word ptr [di + 15h]
        or        cx, cx
        jl    L_531B
        add       bx, cx
        cmp       bx, 2deh
        jl    L_5335
        shr       bx, 1
        push      bx
        and       bx, 3ffh
        jne    L_5312
        pop       bx
        inc       bx
L_5312:
        pop       bx
        add       dl, 4
        and       dl, 1ch
        jmp short    L_5335
L_531B:
        add       bx, cx
        cmp       bx, 184h
        jg    L_5335
        shl       bx, 1
        push      bx
        and       bx, 3ffh
        jne    L_532E
        pop       bx
        dec       bx
L_532E:
        pop       bx
        sub       dl, 4
        and       dl, 1ch
L_5335:
        and       bx, 3ffh
        mov       cx, dx
        mov       ah, 0a0h
        add       ah, byte ptr ds:[gVoiceIndex]
        mov       al, bl
        call    _AdlibWrite
        mov       byte ptr [di + 27h], al
        mov       ah, 0b0h
        add       ah, byte ptr ds:[gVoiceIndex]
        or        bh, cl
        or        bh, ch
        mov       al, bh
        call    _AdlibWrite
        mov       byte ptr [di + 28h], al
        ret
_VoiceSilence ENDP

; 0000:535C: the driver random byte: adds 9248h to gRandomSeed (3F2E), rotates it right three times, stores it and returns the new seed in AX (caller line 1231 uses AX); the jump table at 536C follows it
        PUBLIC  _SoundRandom              ; @ 0000:535C
_SoundRandom PROC NEAR
        mov       ax, word ptr ds:[gRandomSeed]
        add       ax, 9248h
        ror       ax, 1
        ror       ax, 1
        ror       ax, 1
        mov       word ptr ds:[gRandomSeed], ax
        ret
_SoundRandom ENDP

; 0000:536C: the script command jump table (data, 148 bytes, 74 entries). UpdateVoices (4625 to 4638: and al,7Fh; cmp al,70h; jle;
; mov bx,[bx+536Ch]; jmp bx) runs entry n for a script byte with the high bit set, 80h + n; entries that name L_46E9 are commands
; with no handler of their own, they end the voice's script. QUIRK 4627: the bound is 70h but the table has 4Ah entries, so the
; script bytes CAh to F0h index past it, into the text that follows (the runtime library's "Runtime overlay error"), and jump to
; whatever address those characters make; only F1h to FFh reach L_46E9 by the range check.
        PUBLIC  _CommandTable              ; @ 0000:536C
_CommandTable LABEL WORD
        dw      L_464A, L_464F, L_4657, L_46A6, L_46AC, L_46B8
        dw      L_46D0, L_46E3, L_46E9, L_46FF, L_471C, L_4723
        dw      L_473E, L_4744, L_4761, L_46E9, L_4785, L_479B
        dw      L_47B1, L_47BF, L_46E9, L_47C5, L_46E9, L_46E9
        dw      L_46E9, L_46E9, L_47E4, L_46E9, L_47EA, L_4806
        dw      L_4836, L_46E9, L_483F, L_4852, L_46E9, L_46E9
        dw      L_4868, L_46E9, L_4871, L_4878, L_46E9, L_4881
        dw      L_46E9, L_4887, L_488D, L_48AE, L_48D1, L_48EF
        dw      L_490D, L_46E9, L_46E9, L_4919, L_46E9, L_4985
        dw      L_49B8, L_46E9, L_46E9, L_49C1, L_4A7B, L_4A85
        dw      L_4A89, L_4A8F, L_46E9, L_4AAC, L_4AF8, L_4B01
        dw      L_4BE0, L_4C10, L_4C23, L_4CC7, L_4D6B, L_4DFB
        dw      L_4E02, L_4E08

        END
