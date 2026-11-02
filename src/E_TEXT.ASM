; Unit at EOB.EXE 0000:020C (19996 bytes), the _TEXT segment of the EOB.EXE load module between the start-up
; (e_c0) and the Borland runtime library (e_lib): Westwood's AdLib sound driver, code and data in one segment,
; running with DS = CS. LoadSoundDriver (E177D.C) reads the 32C8h-byte block of adlib.dat or pcgamesn.dat into
; cs:0416h; the AdLib driver (sound card 0, E177D.C:147-149) plays the sounds in it, the PC speaker and Tandy
; driver is E1948.ASM. In address order:
;   020C  the pitch curve tables the script commands read (data), then GetScriptFlag (03A1)
;   03AA  the five far routines LoadSoundDriver, PlaySound and PlaySoundAtCell call to reach the block (E177D.C)
;   0416  the driver block (data: the EXE's image of it), then the driver's tables (3944), variables (3B36),
;         ten voice records (3B93) and voice tables (3E31)
;   3E66  the two INT 08h handlers (the AdLib timer, the sample player)
;   3F77  the driver: start, stop, tick, the sound queue, the per-voice script interpreter UpdateVoices (418F) and
;         its helpers, and the interpreter's command jump table (4F94, data)
; The source uses the names src/E177D.C and src/E07C8.C give the shared routines and names the rest from the
; code. The data is the EXE's own bytes: tables are written out, run-time state is zero or the initial value the
; image holds.

        .MODEL  LARGE
        .CODE

; 0000:020C: the pitch curve tables (data, 405 bytes). gCurveTablePtrs holds six near pointers, indexed by 2 * a curve number
; (mov bx,[bx+20Ch] in the script command that starts a pitch effect; GetDriverTable returns 020Ch for its argument 4);
; gCurvePairs steps down by 1 every second byte from 50h to 10h, gCurveRamp counts 00h to 7Fh and gCurveTriples steps down
; every third byte from 40h to 15h. QUIRK 02F9: the ramp has 6Fh where its pattern wants 5Fh (the neighbours are 5Eh and 60h);
; the EXE's byte is kept. gDriverTablePtrs (039C) holds the near pointers SelectDriverTable (40E5) picks the sound table from,
; both 0416h. gScriptFlag (03A0) is the byte the script command C7h sets from its operand (4A23: mov [3A0h],ah), read back by
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
        PUBLIC  _f0_03AA              ; @ 0000:03AA
_f0_03AA PROC NEAR
        mov       dx, cs
        mov       ax, 416h
        retf
_f0_03AA ENDP

; 0000:03B0: writes one byte of the driver block: idx selects the 300-word table at 0416, reg offsets into it (E177D.C)
        PUBLIC  _f0_03B0              ; @ 0000:03B0
_f0_03B0 PROC NEAR
        push      bp
        mov       bp, sp
        push      bx
        mov       bx, word ptr [bp + 6]
        shl       bx, 1
        mov       bx, word ptr cs:[bx + 416h]
        add       bx, word ptr [bp + 8]
        mov       ax, word ptr [bp + 0ah]
        mov       byte ptr cs:[bx], al
        pop       bx
        pop       bp
        retf
_f0_03B0 ENDP

; 0000:03CA: reads one byte of the driver block, zero-extended (E177D.C)
        PUBLIC  _f0_03CA              ; @ 0000:03CA
_f0_03CA PROC NEAR
        push      bp
        mov       bp, sp
        push      bx
        mov       bx, word ptr [bp + 6]
        shl       bx, 1
        mov       bx, word ptr cs:[bx + 416h]
        add       bx, word ptr [bp + 8]
        mov       al, byte ptr cs:[bx]
        xor       ah, ah
        pop       bx
        pop       bp
        retf
_f0_03CA ENDP

; 0000:03E3: returns the far driver routine pointer cs:[0416 + 2*idx] (E177D.C)
        PUBLIC  _f0_03E3              ; @ 0000:03E3
_f0_03E3 PROC NEAR
        push      bp
        mov       bp, sp
        push      bx
        mov       ax, word ptr [bp + 6]
        mov       bx, ax
        shl       bx, 1
        add       bx, 416h
        mov       ax, word ptr cs:[bx]
        mov       dx, cs
        pop       bx
        pop       bp
        retf
_f0_03E3 ENDP

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
        mov       ax, 416h
L_0409:
        add       word ptr [di + 416h], ax
        add       di, 2
        loop    L_0409
        pop       ds
        pop       di
        pop       cx
        retf
_RebaseDriverBlock ENDP

; 0000:0416: the driver block (data, 13614 bytes to 3943). LoadSoundDriver reads the 32C8h bytes of adlib.dat or pcgamesn.dat
; over 0416 to 36DD at run time (E177D.C:152) and RebaseDriverBlock turns its first 300 words into near pointers; the bytes
; here are the EXE's image of it and are overwritten before they are read: zeros but for 12 bytes in three places, none of
; them the file's (adlib.dat's first word is 0258h, this block's is 066Eh): Purpose unknown. gBlockSpare (36DE, 614 bytes)
; follows the block, zero and unreferenced in this file.
        PUBLIC  _DriverBlock              ; @ 0000:0416
_DriverBlock LABEL BYTE
        dw      066Eh
        db      298 dup (0)
        dw      0671h
        db      300 dup (0)
        db      088h,001h,001h,008h,001h,000h,018h,000h,054h,0F2h,060h,046h
        db      12386 dup (0)
gBlockSpare LABEL BYTE
        db      614 dup (0)

; 0000:3944: the driver's tables (data, 498 bytes). gEntryTable (3944) lists the eleven near entry points of the driver in the
; order the original links them: no reader in this file (Purpose unknown; the block's code may use it [H]). gLevelCurvePtrs
; (395A) points to the fourteen 32-byte level curves from 3976: monotone maps of a level 0 to 1Fh to a larger one (UpdateVoices
; reads them with mov si,[bx+395Ah], 4709 and others). QUIRK 3A86: curve 8 has 1Ah, 19h where its neighbours rise (17h,1Ah,
; 19h,1Ah,1Ch); QUIRK 3B14: curve 12 has 34h between 3Bh and 41h (the pattern wants about 3Eh); the EXE's bytes are kept.
        PUBLIC  _DriverTables              ; @ 0000:3944
_DriverTables LABEL BYTE
gEntryTable LABEL WORD
        dw      _StartAdlibDriver, _StopAdlibDriver, _SoundDriverTick, _f0_400C, _StartSample, _GetVoiceState
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

; 0000:3B36: the driver's variables (data, 93 bytes), DS = CS. The values are the EXE image's initial ones. Names from the code
; that uses them (the UpdateVoices commands, the INT 08h handlers, E177D.C); the ones with no use in this file are spare.
        PUBLIC  _DriverVariables              ; @ 0000:3B36
_DriverVariables LABEL BYTE
gBusyCountdown  db      0               ; 3B36: set to 0Bh by PlaySoundAtCell (E177D.C:268) and counted down by _AdlibTimerIsr
        db      4 dup (0)               ; 3B37 to 3B3A: spare
gNoteDuration   db      0               ; 3B3B: StartVoiceNote stores the note's duration byte; zero lets the voice's script go on
        db      2 dup (0)               ; 3B3C to 3B3D: spare
gVoiceIndex     db      0               ; 3B3E: the voice UpdateVoices is serving, 9 down to 0 (the register offset of voice n is n)
gUpdateEnable   db      0               ; 3B3F: UpdateVoices runs only while nonzero (set when a sound starts, cleared by reset)
gRhythmKeyBits  db      0               ; 3B40: the key bits last written to register BDh (bit 5 is the rhythm mode)
gTempoRate      db      0               ; 3B41: the script's speed byte: added to gTempoAccum each tick, the followers' voice speed
gEffectMode     db      0               ; 3B42: the pitch effect in force, 0 none (_AdlibTimerIsr acts while it is nonzero)
gEffectPeriod   db      1               ; 3B43: the reload of gEffectCounter
gEffectCounter  db      1               ; 3B44: counted down by _AdlibTimerIsr
gEffectIndex    dw      0FFFFh          ; 3B45: the position in the effect's curve
gRandomSeed     dw      1234h           ; 3B47: SoundRandom's state, 1234h from ResetVoices on
gToggleBit      db      0               ; 3B49: _AdlibTimerIsr inverts it; its bit 5 becomes the key-on bit of voice 0
gRegBDDepth     db      0               ; 3B4A: bits 7 and 6 of register BDh (amplitude and vibrato depth)
gOperatorOffset db      0               ; 3B4B: the operator register offset of the voice being served (gOperatorOffsets)
gRhythmLevelA   db      5 dup (0)       ; 3B4C: three five-byte sets of rhythm operator level terms; the commands at 486Eh to 48EAh
gRhythmLevelB   db      5 dup (0)       ; 3B51:   add one byte of each to the operand and write the sum to registers 51h to 55h
gRhythmLevelC   db      5 dup (0)       ; 3B56
gTempoReload    db      0               ; 3B5B: half the tempo operand (the command at 4416h)
gTempoCounter   db      0               ; 3B5C: counted down from gTempoReload each time gTempoAccum carries
gTempoAccum     db      0FFh            ; 3B5D: TickTempo adds gTempoRate; a carry steps the counter
gTempoBeat      db      0               ; 3B5E: the beat count: TickTempo adds one each time gTempoCounter reaches zero
gTempoWait      db      0               ; 3B5F: the wait-for-beat command's phase flag (4430h)
gEffectCurveA   dw      0               ; 3B60: the near pointers the effect command picks from gCurveTablePtrs
gEffectCurveB   dw      0               ; 3B62
gSoundTable     dw      0               ; 3B64: the sound-number table ProcessSoundQueue indexes (SelectDriverTable, StartAdlibDriver)
gSavedTimerOff  dw      0               ; 3B66: the BIOS INT 08h vector StartAdlibDriver saved, offset
gSavedTimerSeg  dw      0               ; 3B68: and segment; StopAdlibDriver restores it, _AdlibTimerIsr chains to it
gSampleSavedOff dw      0               ; 3B6A: the INT 08h vector StartSample saved (3E66's), offset
gSampleSavedSeg dw      0               ; 3B6C: and segment; _SampleTimerIsr puts it back
gTimerPasses12  db      0Ch             ; 3B6E: _AdlibTimerIsr calls the tick when it counts down to zero
gTimerPasses48  db      30h             ; 3B6F: and chains to the BIOS when this one does
        db      4 dup (0)               ; 3B70 to 3B73: spare
gSampleNibbles  db      0               ; 3B74: the sample's pass counter (bit 0 selects the nibble)
gSampleLevel    db      0               ; 3B75: the sample's running level, set to 8 by StartSample
gSampleLength   dw      0               ; 3B76: its length in bytes
gSamplePtrOff   dw      0               ; 3B78: the sample's read pointer, offset (zero when none plays: GetSampleOffset)
gSamplePtrSeg   dw      0               ; 3B7A: and segment
gSampleStart    dw      0               ; 3B7C: the sample's first offset
gSampleEnable   db      0               ; 3B7E: nonzero while a sample plays (SetSamplePlaying, StartSample)
gQueueWrite     dw      0               ; 3B7F: the sound queue's write index, 0 to 0Fh (_f0_400C)
gQueueRead      dw      0               ; 3B81: its read index (ProcessSoundQueue)
gQueueRing      db      16 dup (0)      ; 3B83: the queued driver sound numbers, one byte each (a word is stored at each index)

; 0000:3B93: the ten voice records (data, 670 bytes, 43h each): the state of the nine AdLib voices (0 to 8) and the rhythm voice 9
; as UpdateVoices and its helpers read it through di. Zero in the image.
        PUBLIC  _VoiceRecords              ; @ 0000:3B93
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

; 0000:3E31: the voice tables (data, 53 bytes). gVoiceRecordPtrs: the near pointer of each voice record, indexed by 2 * voice.
; gOperatorOffsets: the AdLib modulator operator offset of each of voices 0 to 8 (the carrier is 3 above it; register 20h + offset
; and so on). gFNumbers: the twelve F-numbers (register A0h and the low bits of B0h) of the notes of an octave.
        PUBLIC  _VoiceTables              ; @ 0000:3E31
_VoiceTables LABEL BYTE
gVoiceRecordPtrs LABEL WORD
        dw      gVoiceRecord0, gVoiceRecord1, gVoiceRecord2, gVoiceRecord3, gVoiceRecord4, gVoiceRecord5, gVoiceRecord6, gVoiceRecord7, gVoiceRecord8, gVoiceRecord9
gOperatorOffsets LABEL BYTE
        db      000h,001h,002h,008h,009h,00Ah,010h,011h,012h
gFNumbers LABEL WORD
        dw      0134h,0147h,015Ah,016Fh,0184h,019Ch,01B4h,01CEh,01E9h,0207h,0225h,0246h

; 0000:3E66: the AdLib driver's INT 08h handler, installed by StartAdlibDriver (3F77: mov dx,3E66h; int 21h 2508h, 3F97 to 3F9D).
; The PIT runs at divisor 0555h there (3FA2 to 3FB0), 874 Hz if its clock is the usual 1193182 Hz [H], and this handler divides it down:
;   - while byte 3B42 is nonzero, counts 3B44 down; at zero it toggles the key-on bit (20h) of voice 0's register B0h image (the
;     byte at +28h of the voice record 3B93, VoiceKeyOn sets the same bit), writes the register to the card, and reloads 3B44
;     from 3B43;
;   - counts the byte 3B36 down to zero (nothing else in this file reads or writes it: Purpose unknown);
;   - every 0Ch passes (72.8 Hz) calls SoundDriverTick (3FF4);
;   - every 30h passes (18.2 Hz) chains to the saved BIOS handler, the vector StartAdlibDriver stored at 3B66 (offset) and 3B68
;     (segment); that handler sends the end of interrupt, so this path does not; on every other pass it sends EOI itself.
; Saves and restores ax, bx, dx and ds; DS is set to CS, where the driver keeps its variables.
        PUBLIC  _AdlibTimerIsr              ; @ 0000:3E66
_AdlibTimerIsr PROC NEAR
        push      ax
        push      bx
        push      dx
        push      ds
        cld
        mov       ax, cs
        mov       ds, ax
        cmp       byte ptr ds:[3b42h], 0
        je    L_3EAF
        dec       byte ptr ds:[3b44h]
        jne    L_3EAF
        mov       bx, 3b93h
        mov       al, byte ptr [bx + 28h]
        mov       ah, al
        and       al, 1fh
        xor       byte ptr ds:[3b49h], 0ffh
        mov       ah, byte ptr ds:[3b49h]
        and       ah, 20h
        or        al, ah
        mov       byte ptr [bx + 28h], al
        mov       bl, al
        mov       dx, 388h
        mov       al, 0b0h
        out       dx, al
        call    _AdlibDelay4
        mov       al, bl
        inc       dx
        out       dx, al
        call    _AdlibDelay17
        mov       al, byte ptr ds:[3b43h]
        mov       byte ptr ds:[3b44h], al
L_3EAF:
        cmp       byte ptr ds:[3b36h], 0
        je    L_3EBA
        dec       byte ptr ds:[3b36h]
L_3EBA:
        dec       byte ptr ds:[3b6eh]
        jne    L_3EC8
        mov       byte ptr ds:[3b6eh], 0ch
        call    _SoundDriverTick
L_3EC8:
        dec       byte ptr ds:[3b6fh]
        jne    L_3EDC
        mov       byte ptr ds:[3b6fh], 30h
        pop       ds
        pop       dx
        pop       bx
        pop       ax
        jmp       dword ptr cs:[3b66h]
L_3EDC:
        mov       al, 20h
        out       20h, al
        pop       ds
        pop       dx
        pop       bx
        pop       ax
        iret
_AdlibTimerIsr ENDP

; 0000:3EE5: the INT 08h handler that plays a sample, installed by StartSample (4B7F: mov dx,3EE5h; int 21h 2508h, 4C49 to 4C4F)
; in place of 3E66, with the PIT at divisor 004Bh (4C51 to 4C61), about 15.9 kHz on the same clock [H]. Each pass reads one 4-bit delta from the sample at the far
; pointer cs:3B78/3B7A (low nibble of a byte first, then the high one, 3B74 counts the passes), adds it to the level byte at
; 3B75 when bit 3 is clear and subtracts (value and 7) when it is set, and writes the level to the AdLib data port 389h (the
; register StartSample selected last, 40h, at 4C2B to 4C37). The sample ends after 3B76 bytes from the start 3B7C, or at once when 3B7E (set by
; StartSample, SetSamplePlaying) is zero: then it puts the PIT back to 0555h, clears 3B78, restores the INT 08h vector StartSample
; saved at 3B6A/3B6C (the vector 3E66 was installed as) and clears the voice enable 3B3F (UpdateVoices runs only while it is nonzero).
; QUIRK 3EE5: it always sends EOI itself and never chains to the BIOS handler, so the BIOS clock loses the ticks of a sample.
        PUBLIC  _SampleTimerIsr              ; @ 0000:3EE5
_SampleTimerIsr PROC NEAR
        push      ax
        push      dx
        push      ds
        push      si
        cmp       byte ptr cs:[3b7eh], 0
        je    L_3F40
        lds       si, dword ptr cs:[3b78h]
        cld
        lodsb
        mov       ah, byte ptr cs:[3b75h]
        test      byte ptr cs:[3b74h], 1
        je    L_3F0E
        shr       al, 1
        shr       al, 1
        shr       al, 1
        shr       al, 1
        inc       si
L_3F0E:
        dec       si
        and       al, 0fh
        test      al, 8
        jne    L_3F19
        add       ah, al
        jmp short    L_3F1D
L_3F19:
        and       al, 7
        sub       ah, al
L_3F1D:
        mov       al, ah
        mov       byte ptr cs:[3b75h], ah
        mov       dx, 389h
        out       dx, al
        dec       byte ptr cs:[3b74h]
        mov       word ptr cs:[3b78h], si
        mov       ax, si
        sub       ax, word ptr cs:[3b7ch]
        cmp       ax, word ptr cs:[3b76h]
        jb    L_3F6E
L_3F40:
        mov       al, 36h
        out       43h, al
        mov       al, 55h
        out       40h, al
        mov       al, 5
        out       40h, al
        mov       word ptr cs:[3b78h], 0
        mov       si, 20h
        xor       ax, ax
        mov       ds, ax
        mov       dx, word ptr cs:[3b6ah]
        mov       ax, word ptr cs:[3b6ch]
        mov       word ptr [si], dx
        mov       word ptr [si + 2], ax
        mov       byte ptr cs:[3b3fh], 0
L_3F6E:
        mov       al, 20h
        out       20h, al
        pop       si
        pop       ds
        pop       dx
        pop       ax
        iret
_SampleTimerIsr ENDP

; 0000:3F77: the AdLib driver on: installs the INT 08h handler at 3E66, resets the AdLib voices (413B), programs the PIT and clears the ring
        PUBLIC  _StartAdlibDriver              ; @ 0000:3F77
_StartAdlibDriver PROC NEAR
        push      ds
        push      es
        push      si
        push      di
        mov       ax, cs
        mov       ds, ax
        xor       bx, bx
        mov       bx, word ptr [bx + 39ch]
        mov       word ptr ds:[3b64h], bx
        cli
        mov       ax, 3508h
        int       21h
        mov       word ptr ds:[3b68h], es
        mov       word ptr ds:[3b66h], bx
        mov       dx, 3e66h
        mov       ax, 2508h
        int       21h
        call    _ResetVoices
        mov       al, 36h
        out       43h, al
        jmp    $+2
L_3FA8:
        mov       al, 55h
        out       40h, al
        jmp    $+2
L_3FAE:
        mov       al, 5
        out       40h, al
        in        al, 61h
        and       al, 0feh
        out       61h, al
        mov       word ptr ds:[3b81h], 0
        mov       word ptr ds:[3b7fh], 0
        sti
        pop       di
        pop       si
        pop       es
        pop       ds
        retf
_StartAdlibDriver ENDP

; 0000:3FCA: the AdLib driver off: restores the saved INT 08h vector, resets the AdLib voices and stops the PIT (E07C8.C:65)
        PUBLIC  _StopAdlibDriver              ; @ 0000:3FCA
_StopAdlibDriver PROC NEAR
        push      ds
        mov       ax, cs
        mov       ds, ax
        push      ds
        cli
        mov       dx, word ptr ds:[3b66h]
        mov       ax, word ptr ds:[3b68h]
        mov       ds, ax
        mov       ax, 2508h
        int       21h
        sti
        pop       ds
        call    _ResetVoices
        mov       al, 36h
        out       43h, al
        jmp    $+2
L_3FEA:
        mov       al, 0
        out       40h, al
        jmp    $+2
L_3FF0:
        out       40h, al
        pop       ds
        retf
_StopAdlibDriver ENDP

; 0000:3FF4: the driver tick, called from the INT 08h handler at 3E66 every 0ch passes: processes the ring, the voices and the tempo
        PUBLIC  _SoundDriverTick              ; @ 0000:3FF4
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

; 0000:400C: queues a driver sound in the 16-entry ring (head 3B7F, masked 0fh); E177D.C calls it with the AdLib number
        PUBLIC  _f0_400C              ; @ 0000:400C
_f0_400C PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        mov       bx, cs
        mov       ds, bx
        mov       bx, word ptr ds:[3b7fh]
        mov       ax, word ptr [bp + 6]
        mov       word ptr [bx + 3b83h], ax
        inc       bx
        and       bx, 0fh
        mov       word ptr ds:[3b7fh], bx
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_f0_400C ENDP

; 0000:402C: drains the 16-entry ring (head 3B81, tail 3B7F): loads each queued sound into a voice and starts it
        PUBLIC  _ProcessSoundQueue              ; @ 0000:402C
_ProcessSoundQueue PROC NEAR
        push      ds
        push      si
        push      di
        mov       bx, cs
        mov       ds, bx
L_4033:
        mov       bx, word ptr ds:[3b81h]
        cmp       bx, word ptr ds:[3b7fh]
        jne    L_403F
        jmp short    L_409A
L_403F:
        mov       bx, word ptr [bx + 3b83h]
        sub       bh, bh
        shl       bx, 1
        add       bx, word ptr ds:[3b64h]
        mov       si, word ptr [bx]
        mov       bx, word ptr cs:[si]
        cmp       bl, 0
        jne    L_405A
        mov       byte ptr ds:[3b42h], 0
L_405A:
        mov       al, bh
        mov       cl, bl
        sub       bh, bh
        shl       bx, 1
        mov       di, word ptr [bx + 3e31h]
        cmp       al, byte ptr [di + 8]
        jl    L_408F
        call    _ClearVoice
        mov       byte ptr [di + 8], al
        inc       si
        inc       si
        mov       word ptr [di + 3], si
        mov       byte ptr [di + 25h], 0ffh
        mov       byte ptr [di + 26h], 0ffh
        mov       byte ptr [di + 5], 1
        mov       byte ptr ds:[3b3fh], 1
        cmp       bl, 12h
        je    L_408F
        call    _VoiceSilence
L_408F:
        inc       word ptr ds:[3b81h]
        and       word ptr ds:[3b81h], 0fh
        jmp short    L_4033
L_409A:
        pop       di
        pop       si
        pop       ds
        ret
_ProcessSoundQueue ENDP

; 0000:409E: stores the note argument and reads the two block pointers now at 3B60/3B62, then writes register A0
        PUBLIC  _SetNote              ; @ 0000:409E
_SetNote PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        push      si
        push      di
        mov       ax, word ptr [bp + 6]
        mov       bx, cs
        mov       ds, bx
        mov       word ptr ds:[3b45h], ax
        and       ax, 7fffh
        mov       bx, ax
        mov       si, word ptr ds:[3b60h]
        mov       al, byte ptr [bx + si]
        mov       byte ptr ds:[3b43h], al
        mov       si, word ptr ds:[3b62h]
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

; 0000:40CE: sets the byte at 3B7E, the AdLib sample ISR enable read by 3EE5
        PUBLIC  _SetSamplePlaying              ; @ 0000:40CE
_SetSamplePlaying PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        push      si
        push      di
        mov       ax, word ptr [bp + 6]
        mov       bx, cs
        mov       ds, bx
        mov       byte ptr ds:[3b7eh], al
        pop       di
        pop       si
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_SetSamplePlaying ENDP

; 0000:40E5: stores the block pointer table[bx+39ch] at 3B64 and returns it
        PUBLIC  _SelectDriverTable              ; @ 0000:40E5
_SelectDriverTable PROC NEAR
        push      bp
        mov       bp, sp
        push      ds
        mov       bx, cs
        mov       ds, bx
        mov       bx, word ptr [bp + 6]
        shl       bx, 1
        mov       bx, word ptr [bx + 39ch]
        mov       word ptr ds:[3b64h], bx
        mov       ax, word ptr ds:[3b64h]
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_SelectDriverTable ENDP

; 0000:4102: returns a near driver table pointer for arg 0 to 4: cs, block table, 542h, 20ch
        PUBLIC  _GetDriverTable              ; @ 0000:4102
_GetDriverTable PROC NEAR
        push      bp
        mov       bp, sp
        cmp       word ptr [bp + 6], 0
        jne    L_410F
        mov       ax, cs
        jmp short    L_4137
L_410F:
        cmp       word ptr [bp + 6], 1
        jne    L_411B
        mov       ax, word ptr cs:[3b64h]
        jmp short    L_4137
L_411B:
        cmp       word ptr [bp + 6], 2
        jne    L_4125
        mov       ax, word ptr cs:[3b64h]
L_4125:
        cmp       word ptr [bp + 6], 3
        jne    L_412E
        mov       ax, 542h
L_412E:
        cmp       word ptr [bp + 6], 4
        jne    L_4137
        mov       ax, 20ch
L_4137:
        mov       sp, bp
        pop       bp
        retf
_GetDriverTable ENDP

; 0000:413B: writes the driver reset registers and clears all ten voice records (calls 4D95)
        PUBLIC  _ResetVoices              ; @ 0000:413B
_ResetVoices PROC NEAR
        mov       byte ptr ds:[3b3fh], 0
        mov       word ptr ds:[3b47h], 1234h
        mov       ax, 120h
        call    _AdlibWrite
        mov       ax, 800h
        call    _AdlibWrite
        mov       ax, 0bd00h
        call    _AdlibWrite
        mov       byte ptr ds:[3b42h], 0
        mov       cx, 0ah
        mov       bh, 0
L_4162:
        mov       bl, cl
        dec       bl
        cmp       bl, 9
        je    L_4183
        mov       ah, byte ptr [bx + 3e45h]
        add       ah, 40h
        mov       al, 3fh
        call    _AdlibWrite
        mov       ah, byte ptr [bx + 3e45h]
        add       ah, 43h
        mov       al, 3fh
        call    _AdlibWrite
L_4183:
        shl       bl, 1
        mov       di, word ptr [bx + 3e31h]
        call    _ClearVoice
        loop    L_4162
        ret
_ResetVoices ENDP

; 0000:418F: the per-voice script interpreter: loops voice 9 down to 0 and runs its command through the 4F94 jump table
        PUBLIC  _UpdateVoices              ; @ 0000:418F
_UpdateVoices PROC NEAR
        cmp       byte ptr ds:[3b3fh], 0
        jne    L_4197
        ret
L_4197:
        mov       byte ptr ds:[3b3eh], 9
L_419C:
        mov       bl, byte ptr ds:[3b3eh]
        mov       bh, 0
        shl       bl, 1
        mov       di, word ptr [bx + 3e31h]
        mov       al, byte ptr [di + 5]
        or        al, al
        jne    L_41B2
        jmp    L_4267
L_41B2:
        mov       bx, 3e45h
        add       bl, byte ptr ds:[3b3eh]
        mov       al, byte ptr [bx]
        mov       byte ptr ds:[3b4bh], al
        cmp       byte ptr [di + 40h], 0
        je    L_41CA
        mov       al, byte ptr ds:[3b41h]
        mov       byte ptr [di + 25h], al
L_41CA:
        mov       al, byte ptr [di + 26h]
        add       al, byte ptr [di + 25h]
        mov       byte ptr [di + 26h], al
        jb    L_41D8
        jmp    L_425F
L_41D8:
        dec       byte ptr [di + 5]
        je    L_41F9
        mov       al, byte ptr [di + 5]
        cmp       al, byte ptr [di + 21h]
        jne    L_41E8
        call    _VoiceKeyOff
L_41E8:
        cmp       al, byte ptr [di + 35h]
        jne    L_41F7
        cmp       byte ptr ds:[3b3eh], 9
        je    L_41F7
        call    _VoiceKeyOff
L_41F7:
        jmp short    L_425F
L_41F9:
        mov       si, word ptr [di + 3]
L_41FC:
        lodsw
        or        al, al
        jns    L_4203
        jmp short    L_424A
L_4203:
        call    _SetVoicePitch
        call    _VoiceKeyOn
        call    _StartVoiceNote
        cmp       word ptr [di + 32h], 0
        je    L_4214
        jne    L_421A
L_4214:
        cmp       word ptr [di + 33h], 0
        je    L_423E
L_421A:
        lodsb
        mov       byte ptr [di + 34h], al
        call    _VoiceLevelA0
        mov       dx, 388h
        mov       ah, byte ptr ds:[3b4bh]
        add       ah, 43h
        call    _AdlibWrite
        call    _VoiceLevelB0
        mov       dx, 388h
        mov       ah, byte ptr ds:[3b4bh]
        add       ah, 40h
        call    _AdlibWrite
L_423E:
        mov       al, byte ptr ds:[3b3bh]
        or        al, al
        je    L_41FC
        mov       word ptr [di + 3], si
        jmp short    L_425F
L_424A:
        and       al, 7fh
        cmp       al, 70h
        jle    L_4253
        jmp    L_4311
L_4253:
        mov       bl, al
        sub       bh, bh
        shl       bl, 1
        mov       bx, word ptr [bx + 4f94h]
        jmp       bx
L_425F:
        push      di
        call      word ptr [di + 29h]
        pop       di
        call      word ptr [di + 2bh]
L_4267:
        dec       byte ptr ds:[3b3eh]
        js    L_4270
        jmp    L_419C
L_4270:
        ret
L_4271:
        mov       byte ptr [di + 6], ah
        jmp short    L_41FC
L_4276:
        dec       byte ptr [di + 6]
        jne    L_42D4
        inc       si
        jmp    L_41FC
L_427F:
        push      si
        push      di
        push      bx
        mov       bl, ah
        cmp       bl, 0ffh
        jb    L_428B
        jmp short    L_42C8
L_428B:
        sub       bh, bh
        shl       bx, 1
        add       bx, word ptr ds:[3b64h]
        mov       si, word ptr [bx]
        mov       bx, word ptr cs:[si]
        mov       al, bh
        mov       cl, bl
        sub       bh, bh
        shl       bx, 1
        mov       di, word ptr [bx + 3e31h]
        cmp       al, byte ptr [di + 8]
        jl    L_42C8
        call    _ClearVoice
        mov       byte ptr [di + 8], al
        inc       si
        inc       si
        mov       word ptr [di + 3], si
        mov       byte ptr [di + 25h], 0ffh
        mov       byte ptr [di + 26h], 0ffh
        mov       byte ptr [di + 5], 1
        mov       byte ptr ds:[3b3fh], 1
        call    _VoiceSilence
L_42C8:
        pop       bx
        pop       di
        pop       si
        jmp    L_41FC
L_42CE:
        mov       byte ptr [di + 35h], ah
        jmp    L_41FC
L_42D4:
        mov       cl, ah
        lodsb
        xchg      ah, al
        mov       al, cl
        mov       si, ax
        jmp    L_41FC
L_42E0:
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
        jmp    L_41FC
L_42F8:
        dec       si
        dec       byte ptr [di + 9]
        mov       bl, byte ptr [di + 9]
        mov       bh, 0
        shl       bl, 1
        mov       ax, word ptr [bx + di + 0ah]
        mov       si, ax
        jmp    L_41FC
L_430B:
        mov       byte ptr [di + 7], ah
        jmp    L_41FC
L_4311:
        mov       byte ptr [di + 8], 0
        mov       word ptr [di + 3], 0
        cmp       byte ptr ds:[3b3eh], 9
        je    L_4324
        call    _VoiceKeyOff
L_4324:
        jmp    L_4267
L_4327:
        call    _StartVoiceNote
        mov       word ptr [di + 3], si
        cmp       byte ptr ds:[3b3eh], 9
        je    L_4337
        call    _VoiceKeyOff
L_4337:
        mov       al, byte ptr ds:[3b3bh]
        or        al, al
        je    L_4341
        jmp    L_425F
L_4341:
        jmp    L_41FC
L_4344:
        lodsb
        call    _AdlibWrite
        jmp    L_41FC
L_434B:
        xchg      ah, al
        call    _SetVoicePitch
        lodsb
        xchg      ah, al
        call    _StartVoiceNote
        mov       word ptr [di + 3], si
        mov       al, byte ptr ds:[3b3bh]
        or        al, al
        je    L_4363
        jmp    L_425F
L_4363:
        jmp    L_41FC
L_4366:
        mov       byte ptr [di + 12h], ah
        jmp    L_41FC
L_436C:
        mov       byte ptr [di + 3ah], ah
        mov       byte ptr [di + 39h], ah
        lodsb
        mov       byte ptr [di + 3bh], al
        mov       byte ptr [di + 3ch], al
        lodsb
        mov       byte ptr [di + 3dh], al
        lodsw
        mov       word ptr [di + 3eh], ax
        mov       word ptr [di + 2bh], 4ed7h
        jmp    L_41FC
L_4389:
        mov       al, byte ptr ds:[3b3eh]
        push      ax
        push      di
        mov       bl, ah
        mov       bh, 0
        shl       bl, 1
        mov       di, word ptr [bx + 3e31h]
        mov       byte ptr [di + 5], 0
        mov       byte ptr [di + 8], 0
        mov       word ptr [di + 3], 0
        pop       di
        pop       ax
        mov       byte ptr ds:[3b3eh], al
        jmp    L_41FC
L_43AD:
        push      si
        mov       cl, byte ptr ds:[3b4bh]
        sub       bx, bx
        mov       bl, ah
        shl       bx, 1
        mov       si, word ptr [bx + 542h]
        call    _LoadVoicePatch
        pop       si
        jmp    L_41FC
L_43C3:
        mov       byte ptr [di + 13h], ah
        lodsb
        xchg      ah, al
        lodsb
        mov       word ptr [di + 15h], ax
        mov       word ptr [di + 29h], 4f00h
        mov       byte ptr [di + 14h], 0ffh
        jmp    L_41FC
L_43D9:
        dec       si
        mov       word ptr [di + 29h], 4e7dh
        mov       word ptr [di + 15h], 0
        jmp    L_41FC
L_43E7:
        mov       byte ptr [di + 24h], ah
        jmp    L_41FC
L_43ED:
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
        mov       word ptr [di + 29h], 4e7eh
        jmp    L_41FC
L_440C:
        mov       byte ptr [di + 8], ah
        jmp    L_41FC
L_4412:
        shr       ah, 1
        mov       byte ptr ds:[3b5bh], ah
        mov       byte ptr ds:[3b5ch], ah
        mov       byte ptr ds:[3b5dh], 0ffh
        mov       ah, 0
        mov       byte ptr ds:[3b5eh], ah
        mov       byte ptr ds:[3b5fh], ah
        jmp    L_41FC
L_442E:
        mov       al, byte ptr ds:[3b5fh]
        or        al, al
        jne    L_4441
        and       ah, byte ptr ds:[3b5eh]
        jne    L_444F
        inc       byte ptr ds:[3b5fh]
        jmp short    L_444F
L_4441:
        and       ah, byte ptr ds:[3b5eh]
        je    L_444F
        mov       byte ptr ds:[3b5fh], 0
        jmp    L_41FC
L_444F:
        dec       si
        dec       si
        mov       byte ptr [di + 5], 1
        mov       word ptr [di + 3], si
        jmp    L_4267
        jmp    L_41FC
L_445E:
        mov       byte ptr [di + 20h], ah
        call    _WriteVoiceReg
        jmp    L_41FC
L_4467:
        call    _StartVoiceNote
        mov       word ptr [di + 3], si
        mov       al, byte ptr ds:[3b3bh]
        or        al, al
        je    L_4477
        jmp    L_425F
L_4477:
        jmp    L_41FC
L_447A:
        call    _StartVoiceNote
        call    _VoiceKeyOn
        mov       word ptr [di + 3], si
        mov       al, byte ptr ds:[3b3bh]
        or        al, al
        je    L_448D
        jmp    L_425F
L_448D:
        jmp    L_41FC
L_4490:
        and       ah, 7
        mov       byte ptr [di + 2dh], ah
        jmp    L_41FC
L_4499:
        mov       byte ptr ds:[3b41h], ah
        jmp    L_41FC
L_44A0:
        dec       si
        mov       word ptr [di + 2bh], 4e7dh
        jmp    L_41FC
L_44A9:
        mov       byte ptr [di + 25h], ah
        jmp    L_41FC
L_44AF:
        mov       byte ptr [di + 30h], ah
        jmp    L_41FC
L_44B5:
        mov       al, byte ptr ds:[3b3eh]
        push      ax
        push      di
        lodsb
        mov       byte ptr ds:[3b3eh], ah
        mov       bl, ah
        mov       bh, 0
        shl       bl, 1
        mov       di, word ptr [bx + 3e31h]
        mov       byte ptr [di], al
        call    _WriteVoiceReg
        pop       di
        pop       ax
        mov       byte ptr ds:[3b3eh], al
        jmp    L_41FC
L_44D6:
        mov       al, byte ptr ds:[3b3eh]
        push      ax
        push      di
        mov       byte ptr ds:[3b3eh], ah
        sub       bx, bx
        mov       bl, ah
        lodsb
        shl       bl, 1
        mov       di, word ptr [bx + 3e31h]
        add       al, byte ptr [di]
        mov       byte ptr [di], al
        call    _WriteVoiceReg
        pop       di
        pop       ax
        mov       byte ptr ds:[3b3eh], al
        jmp    L_41FC
L_44F9:
        and       ah, 1
        mov       cl, 7
        shl       ah, cl
        mov       bl, byte ptr ds:[3b4ah]
        and       bl, 7fh
        or        bl, ah
        mov       byte ptr ds:[3b4ah], bl
        mov       ah, 0bdh
        mov       al, bl
        call    _AdlibWrite
        jmp    L_41FC
L_4517:
        and       ah, 1
        mov       cl, 6
        shl       ah, cl
        mov       bl, byte ptr ds:[3b4ah]
        and       bl, 0bfh
        or        bl, ah
        mov       byte ptr ds:[3b4ah], bl
        mov       ah, 0bdh
        mov       al, bl
        call    _AdlibWrite
        jmp    L_41FC
L_4535:
        add       ah, byte ptr [di + 20h]
        mov       byte ptr [di + 20h], ah
        call    _WriteVoiceReg
        jmp    L_41FC
L_4541:
        mov       al, byte ptr ds:[3b3eh]
        push      ax
        push      di
        cmp       ah, 0
        jne    L_4550
        mov       byte ptr ds:[3b42h], 0
L_4550:
        mov       byte ptr ds:[3b3eh], ah
        mov       bl, ah
        mov       bh, 0
        shl       bl, 1
        mov       di, word ptr [bx + 3e31h]
        mov       byte ptr [di + 5], 0
        mov       byte ptr [di + 8], 0
        mov       word ptr [di + 3], 0
        mov       byte ptr [di], 0
        cmp       byte ptr ds:[3b3eh], 9
        je    L_45A5
        mov       bl, ah
        xor       bh, bh
        mov       cl, byte ptr [bx + 3e45h]
        mov       ah, 0c0h
        add       ah, byte ptr ds:[3b3eh]
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
        add       ah, byte ptr ds:[3b3eh]
        mov       al, 0
        call    _AdlibWrite
L_45A5:
        pop       di
        pop       ax
        mov       byte ptr ds:[3b3eh], al
        jmp    L_41FC
L_45AD:
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
        add       ah, byte ptr ds:[3b3eh]
        mov       al, bl
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, bh
        call    _AdlibWrite
        jmp    L_41FC
L_45E0:
        dec       si
        mov       word ptr [di + 29h], 4e7dh
        jmp    L_41FC
L_45E9:
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
        jge    L_4610
        cmp       al, 0
        jge    L_4615
        add       al, 0ch
        sub       ch, 10h
        jmp short    L_4615
L_4610:
        sub       al, 0ch
        add       ch, 10h
L_4615:
        mov       bl, al
        mov       bh, 0
        shl       bl, 1
        mov       ax, word ptr [bx + 3e4eh]
        sub       dx, dx
        mov       dl, byte ptr [di + 24h]
        add       ax, dx
        shr       ch, 1
        shr       ch, 1
        and       ch, 1ch
        or        ch, ah
        mov       cl, al
        cmp       byte ptr [di + 42h], 0
        jl    L_4658
        xor       bh, bh
        mov       bl, byte ptr [di + 41h]
        and       bl, 0fh
        inc       bl
        inc       bl
        shl       bx, 1
        push      si
        mov       si, word ptr [bx + 395ah]
        xor       bh, bh
        mov       bl, byte ptr [di + 42h]
        mov       al, byte ptr [bx + si]
        pop       si
        xor       ah, ah
        add       cx, ax
        jmp short    L_467A
L_4658:
        xor       bh, bh
        mov       bl, byte ptr [di + 41h]
        and       bl, 0fh
        shl       bx, 1
        push      si
        mov       si, word ptr [bx + 395ah]
        xor       bh, bh
        mov       bl, byte ptr [di + 42h]
        xor       bl, 0ffh
        inc       bl
        mov       al, byte ptr [bx + si]
        pop       si
        xor       ah, ah
        sub       cx, ax
        jmp    $+2
L_467A:
        mov       ah, 0a0h
        mov       al, byte ptr [di + 28h]
        and       al, 20h
        or        ch, al
        mov       byte ptr [di + 28h], ch
        mov       byte ptr [di + 27h], cl
        mov       ah, 0a0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, cl
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, ch
        call    _AdlibWrite
        pop       ax
        jmp    L_41FC
L_46A3:
        dec       si
        mov       al, byte ptr ds:[3b41h]
        mov       byte ptr [di + 25h], al
        jmp    L_41FC
L_46AD:
        dec       si
        jmp    L_41FC
L_46B1:
        mov       byte ptr [di + 36h], ah
        jmp    L_41FC
L_46B7:
        or        ah, ah
        js    L_46C4
        add       ah, byte ptr [di + 25h]
        jae    L_46CE
        mov       ah, 0ffh
        jmp short    L_46CE
L_46C4:
        add       ah, byte ptr [di + 25h]
        cmp       ah, byte ptr [di + 25h]
        jb    L_46CE
        mov       ah, 1
L_46CE:
        mov       byte ptr [di + 25h], ah
        jmp    L_41FC
L_46D4:
        mov       byte ptr ds:[3b42h], ah
        lodsb
        push      si
        sub       bh, bh
        mov       bl, al
        shl       bx, 1
        mov       bx, word ptr [bx + 20ch]
        mov       word ptr ds:[3b60h], bx
        inc       al
        sub       bh, bh
        mov       bl, al
        shl       bx, 1
        mov       bx, word ptr [bx + 20ch]
        mov       word ptr ds:[3b62h], bx
        cmp       ah, 2
        jne    L_471C
        mov       bx, 0
        mov       word ptr ds:[3b45h], bx
        mov       si, word ptr ds:[3b60h]
        mov       al, byte ptr [bx + si]
        mov       byte ptr ds:[3b43h], al
        mov       si, word ptr ds:[3b62h]
        mov       al, byte ptr [bx + si]
        mov       ah, 0a0h
        call    _AdlibWrite
        pop       si
        jmp    L_41FC
L_471C:
        pop       si
        jmp    L_41FC
L_4720:
        dec       si
        mov       byte ptr ds:[3b42h], 0
        jmp    L_41FC
L_4729:
        mov       al, byte ptr ds:[3b3eh]
        push      ax
        mov       al, byte ptr ds:[3b4bh]
        push      ax
        push      si
        mov       byte ptr ds:[3b3eh], 6
        mov       bx, 3e45h
        add       bl, 6
        mov       al, byte ptr [bx]
        mov       byte ptr ds:[3b4bh], al
        mov       dx, 388h
        mov       cl, byte ptr ds:[3b4bh]
        sub       bx, bx
        mov       bl, ah
        shl       bx, 1
        mov       si, word ptr [bx + 542h]
        mov       al, byte ptr [si + 6]
        mov       byte ptr ds:[3b4ch], al
        call    _LoadVoicePatch
        pop       si
        lodsb
        push      si
        mov       byte ptr ds:[3b3eh], 7
        mov       bx, 3e45h
        add       bl, 7
        mov       ah, byte ptr [bx]
        mov       byte ptr ds:[3b4bh], ah
        mov       dx, 388h
        mov       cl, byte ptr ds:[3b4bh]
        sub       bx, bx
        mov       bl, al
        shl       bx, 1
        mov       si, word ptr [bx + 542h]
        mov       al, byte ptr [si + 5]
        mov       byte ptr ds:[3b4eh], al
        mov       al, byte ptr [si + 6]
        mov       byte ptr ds:[3b4dh], al
        call    _LoadVoicePatch
        pop       si
        lodsb
        push      si
        mov       byte ptr ds:[3b3eh], 8
        mov       bx, 3e45h
        add       bl, 8
        mov       ah, byte ptr [bx]
        mov       byte ptr ds:[3b4bh], ah
        mov       dx, 388h
        mov       cl, byte ptr ds:[3b4bh]
        sub       bx, bx
        mov       bl, al
        shl       bx, 1
        mov       si, word ptr [bx + 542h]
        mov       al, byte ptr [si + 5]
        mov       byte ptr ds:[3b4fh], al
        mov       al, byte ptr [si + 6]
        mov       byte ptr ds:[3b50h], al
        call    _LoadVoicePatch
        pop       si
        lodsb
        and       al, 2fh
        mov       byte ptr ds:[3d4dh], al
        mov       ah, 0b6h
        call    _AdlibWrite
        lodsb
        mov       ah, 0a6h
        call    _AdlibWrite
        lodsb
        and       al, 2fh
        mov       byte ptr ds:[3d90h], al
        mov       ah, 0b7h
        call    _AdlibWrite
        lodsb
        mov       ah, 0a7h
        call    _AdlibWrite
        lodsb
        and       al, 2fh
        mov       byte ptr ds:[3dd3h], al
        mov       ah, 0b8h
        call    _AdlibWrite
        lodsb
        mov       ah, 0a8h
        call    _AdlibWrite
        mov       byte ptr ds:[3b40h], 20h
        pop       ax
        mov       byte ptr ds:[3b4bh], al
        pop       ax
        mov       byte ptr ds:[3b3eh], al
        jmp    L_41FC
L_4808:
        mov       dx, 388h
        mov       al, 0bdh
        out       dx, al
        call    _AdlibDelay4
        inc       dx
        xchg      ah, al
        push      ax
        and       al, 1fh
        xor       al, 0ffh
        and       al, byte ptr ds:[3b40h]
        or        al, 20h
        out       dx, al
        call    _AdlibDelay17
        pop       ax
        or        al, byte ptr ds:[3b40h]
        mov       byte ptr ds:[3b40h], al
        or        al, byte ptr ds:[3b4ah]
        or        al, 20h
        out       dx, al
        call    _AdlibDelay17
        jmp    L_41FC
L_4838:
        dec       si
        mov       al, byte ptr ds:[3b4ah]
        mov       byte ptr ds:[3b40h], 0
        and       al, 0c0h
        mov       ah, 0bdh
        call    _AdlibWrite
        jmp    L_41FC
L_484B:
        lodsb
        mov       bx, ax
        test      ah, 1
        je    L_486C
        mov       byte ptr ds:[3b58h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b4eh]
        add       al, byte ptr ds:[3b53h]
        add       al, byte ptr ds:[3b58h]
        call    _ClampLevel
        mov       ah, 51h
        call    _AdlibWrite
L_486C:
        mov       ax, bx
        test      ah, 2
        je    L_488C
        mov       byte ptr ds:[3b5ah], al
        xor       ah, ah
        add       al, byte ptr ds:[3b50h]
        add       al, byte ptr ds:[3b55h]
        add       al, byte ptr ds:[3b5ah]
        call    _ClampLevel
        mov       ah, 55h
        call    _AdlibWrite
L_488C:
        mov       ax, bx
        test      ah, 4
        je    L_48AC
        mov       byte ptr ds:[3b59h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b4fh]
        add       al, byte ptr ds:[3b54h]
        add       al, byte ptr ds:[3b59h]
        call    _ClampLevel
        mov       ah, 52h
        call    _AdlibWrite
L_48AC:
        mov       ax, bx
        test      ah, 8
        je    L_48CC
        mov       byte ptr ds:[3b57h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b4dh]
        add       al, byte ptr ds:[3b52h]
        add       al, byte ptr ds:[3b57h]
        call    _ClampLevel
        mov       ah, 54h
        call    _AdlibWrite
L_48CC:
        mov       ax, bx
        test      ah, 10h
        je    L_48EC
        mov       byte ptr ds:[3b56h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b4ch]
        add       al, byte ptr ds:[3b51h]
        add       al, byte ptr ds:[3b56h]
        call    _ClampLevel
        mov       ah, 53h
        call    _AdlibWrite
L_48EC:
        jmp    L_41FC
L_48EF:
        lodsb
        mov       bx, ax
        test      ah, 1
        je    L_4910
        xor       ah, ah
        add       al, byte ptr ds:[3b4eh]
        add       al, byte ptr ds:[3b53h]
        add       al, byte ptr ds:[3b58h]
        call    _ClampLevel
        mov       byte ptr ds:[3b53h], al
        mov       ah, 51h
        call    _AdlibWrite
L_4910:
        mov       ax, bx
        test      ah, 2
        je    L_4930
        xor       ah, ah
        add       al, byte ptr ds:[3b50h]
        add       al, byte ptr ds:[3b55h]
        add       al, byte ptr ds:[3b5ah]
        call    _ClampLevel
        mov       byte ptr ds:[3b55h], al
        mov       ah, 55h
        call    _AdlibWrite
L_4930:
        mov       ax, bx
        test      ah, 4
        je    L_4950
        xor       ah, ah
        add       al, byte ptr ds:[3b4fh]
        add       al, byte ptr ds:[3b54h]
        add       al, byte ptr ds:[3b59h]
        call    _ClampLevel
        mov       byte ptr ds:[3b54h], al
        mov       ah, 52h
        call    _AdlibWrite
L_4950:
        mov       ax, bx
        test      ah, 8
        je    L_4970
        xor       ah, ah
        add       al, byte ptr ds:[3b4dh]
        add       al, byte ptr ds:[3b52h]
        add       al, byte ptr ds:[3b57h]
        call    _ClampLevel
        mov       byte ptr ds:[3b52h], al
        mov       ah, 54h
        call    _AdlibWrite
L_4970:
        mov       ax, bx
        test      ah, 10h
        je    L_4990
        xor       ah, ah
        add       al, byte ptr ds:[3b4ch]
        add       al, byte ptr ds:[3b51h]
        add       al, byte ptr ds:[3b56h]
        call    _ClampLevel
        mov       byte ptr ds:[3b51h], al
        mov       ah, 53h
        call    _AdlibWrite
L_4990:
        jmp    L_41FC
L_4993:
        lodsb
        mov       bx, ax
        test      ah, 1
        je    L_49B0
        mov       byte ptr ds:[3b53h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b4eh]
        add       al, byte ptr ds:[3b58h]
        call    _ClampLevel
        mov       ah, 51h
        call    _AdlibWrite
L_49B0:
        mov       ax, bx
        test      ah, 2
        je    L_49CC
        mov       byte ptr ds:[3b55h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b50h]
        add       al, byte ptr ds:[3b5ah]
        call    _ClampLevel
        mov       ah, 55h
        call    _AdlibWrite
L_49CC:
        mov       ax, bx
        test      ah, 4
        je    L_49E8
        mov       byte ptr ds:[3b54h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b4fh]
        add       al, byte ptr ds:[3b59h]
        call    _ClampLevel
        mov       ah, 52h
        call    _AdlibWrite
L_49E8:
        mov       ax, bx
        test      ah, 8
        je    L_4A04
        mov       byte ptr ds:[3b52h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b4dh]
        add       al, byte ptr ds:[3b57h]
        call    _ClampLevel
        mov       ah, 54h
        call    _AdlibWrite
L_4A04:
        mov       ax, bx
        test      ah, 10h
        je    L_4A20
        mov       byte ptr ds:[3b51h], al
        xor       ah, ah
        add       al, byte ptr ds:[3b4ch]
        add       al, byte ptr ds:[3b56h]
        call    _ClampLevel
        mov       ah, 53h
        call    _AdlibWrite
L_4A20:
        jmp    L_41FC
L_4A23:
        mov       byte ptr ds:[3a0h], ah
        jmp    L_41FC
L_4A2A:
        mov       byte ptr [di + 40h], ah
        jmp    L_41FC
L_4A30:
        mov       byte ptr [di + 32h], ah
        lodsb
        mov       byte ptr [di + 33h], al
        jmp    L_41FC
_UpdateVoices ENDP

; 0000:4A3A: builds the voice frequency and octave from its record and writes registers A0 and B0
        PUBLIC  _SetVoicePitch              ; @ 0000:4A3A
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
        jge    L_4A5E
        cmp       al, 0
        jge    L_4A63
        add       al, 0ch
        sub       ch, 10h
        jmp short    L_4A63
L_4A5E:
        sub       al, 0ch
        add       ch, 10h
L_4A63:
        mov       bl, al
        mov       bh, 0
        shl       bl, 1
        mov       ax, word ptr [bx + 3e4eh]
        sub       dx, dx
        mov       dl, byte ptr [di + 24h]
        add       ax, dx
        shr       ch, 1
        shr       ch, 1
        and       ch, 1ch
        or        ch, ah
        mov       cl, al
        cmp       byte ptr [di + 42h], 0
        je    L_4AC8
        jl    L_4AA8
        xor       bh, bh
        mov       bl, byte ptr [di + 41h]
        and       bl, 0fh
        inc       bl
        inc       bl
        shl       bx, 1
        push      si
        mov       si, word ptr [bx + 395ah]
        xor       bh, bh
        mov       bl, byte ptr [di + 42h]
        mov       al, byte ptr [bx + si]
        pop       si
        xor       ah, ah
        add       cx, ax
        jmp short    L_4AC8
L_4AA8:
        xor       bh, bh
        mov       bl, byte ptr [di + 41h]
        and       bl, 0fh
        shl       bx, 1
        push      si
        mov       si, word ptr [bx + 395ah]
        xor       bh, bh
        mov       bl, byte ptr [di + 42h]
        xor       bl, 0ffh
        inc       bl
        mov       al, byte ptr [bx + si]
        pop       si
        xor       ah, ah
        sub       cx, ax
L_4AC8:
        mov       ah, 0a0h
        mov       al, byte ptr [di + 28h]
        and       al, 20h
        or        ch, al
        mov       byte ptr [di + 28h], ch
        mov       byte ptr [di + 27h], cl
        mov       ah, 0a0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, cl
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, ch
        call    _AdlibWrite
        pop       ax
        ret
_SetVoicePitch ENDP

; 0000:4AEF: writes register B0 for the voice key-on gate and computes its period
        PUBLIC  _VoiceKeyOn              ; @ 0000:4AEF
_VoiceKeyOn PROC NEAR
        push      ax
        mov       ah, 0b0h
        add       ah, byte ptr ds:[3b3eh]
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

; 0000:4B1F: clears the key-on bit of the voice and writes register B0 (skips the first six voices while a sample plays)
        PUBLIC  _VoiceKeyOff              ; @ 0000:4B1F
_VoiceKeyOff PROC NEAR
        cmp       byte ptr ds:[3b40h], 0
        je    L_4B2E
        cmp       byte ptr ds:[3b3eh], 6
        jl    L_4B2E
        ret
L_4B2E:
        cmp       byte ptr ds:[3b3eh], 9
        je    L_4B46
        mov       ah, 0b0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, byte ptr [di + 28h]
        and       al, 0dfh
        mov       byte ptr [di + 28h], al
        call    _AdlibWrite
L_4B46:
        ret
_VoiceKeyOff ENDP

; 0000:4B47: starts the note of a voice: records the level byte, uses the random byte and the 2dh shift, sets the delay
        PUBLIC  _StartVoiceNote              ; @ 0000:4B47
_StartVoiceNote PROC NEAR
        mov       byte ptr ds:[3b3bh], ah
        cmp       byte ptr [di + 36h], 0
        mov       bl, ah
        je    L_4B5F
        call    _SoundRandom
        and       al, byte ptr [di + 36h]
        add       bl, al
        mov       byte ptr [di + 5], bl
        ret
L_4B5F:
        cmp       byte ptr [di + 2dh], 0
        je    L_4B7B
        mov       bl, ah
        shr       ah, 1
        shr       ah, 1
        shr       ah, 1
        mov       cl, byte ptr [di + 2dh]
        xor       ch, ch
        xor       al, al
L_4B74:
        add       al, ah
        loop    L_4B74
        mov       byte ptr [di + 21h], al
L_4B7B:
        mov       byte ptr [di + 5], bl
        ret
_StartVoiceNote ENDP

; 0000:4B7F: the AdLib sample driver on: programs the PIT and the speakers and installs the INT 08h handler at 3EE5 for a sample far pointer
        PUBLIC  _StartSample              ; @ 0000:4B7F
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
L_4BD8:
        mov       al, 0
        out       43h, al
        in        al, 40h
        mov       cl, al
        in        al, 40h
        mov       ch, al
        neg       cx
        add       cx, bx
        cmp       cx, 952h
        jb    L_4BD8
        mov       ax, 0b020h
        call    _AdlibWrite
        mov       ax, 0a000h
        call    _AdlibWrite
        sti
        lds       si, [bp + 6]
        mov       ax, si
        mov       word ptr cs:[3b78h], ax
        mov       word ptr cs:[3b7ch], ax
        mov       ax, ds
        mov       word ptr cs:[3b7ah], ax
        mov       cx, word ptr [bp + 0ah]
        mov       word ptr cs:[3b76h], cx
        mov       byte ptr cs:[3b75h], 8
        mov       byte ptr cs:[3b74h], 0
        mov       ax, cs
        mov       ds, ax
        mov       byte ptr ds:[3b7eh], 1
        mov       dx, 388h
        mov       al, 40h
        out       dx, al
        call    _AdlibDelay4
        mov       al, 0
        inc       dx
        out       dx, al
        call    _AdlibDelay17
        cli
        mov       ax, 3508h
        int       21h
        mov       word ptr ds:[3b6ch], es
        mov       word ptr ds:[3b6ah], bx
        mov       dx, 3ee5h
        mov       ax, 2508h
        int       21h
        mov       al, 36h
        out       43h, al
        call    _AdlibDelay4
        mov       al, 4bh
        out       40h, al
        call    _AdlibDelay4
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

; 0000:4C6B: writes the AdLib register AL and then AH (ports 388h and 389h), with the short delay
        PUBLIC  _AdlibWrite              ; @ 0000:4C6B
_AdlibWrite PROC NEAR
        mov       dx, 388h
        xchg      al, ah
        out       dx, al
        call    _AdlibDelay4
        inc       dx
        xchg      al, ah
        out       dx, al
        call    _AdlibDelay17
        ret
_AdlibWrite ENDP

; 0000:4C7C: clamps the level in AL to 0 to 3fh, returning 0 or 3fh at the ends
        PUBLIC  _ClampLevel              ; @ 0000:4C7C
_ClampLevel PROC NEAR
        cmp       ax, 0
        jl    L_4C87
        cmp       ax, 3fh
        jg    L_4C8A
        ret
L_4C87:
        mov       al, 0
        ret
L_4C8A:
        mov       al, 3fh
        ret
_ClampLevel ENDP

; 0000:4C8D: advances the tempo counters at 3B5D/3B5C and the step byte 3B5E
        PUBLIC  _TickTempo              ; @ 0000:4C8D
_TickTempo PROC NEAR
        mov       al, byte ptr ds:[3b5dh]
        add       al, byte ptr ds:[3b41h]
        mov       byte ptr ds:[3b5dh], al
        jb    L_4C9A
        ret
L_4C9A:
        dec       byte ptr ds:[3b5ch]
        je    L_4CA1
        ret
L_4CA1:
        mov       al, byte ptr ds:[3b5bh]
        mov       byte ptr ds:[3b5ch], al
        inc       byte ptr ds:[3b5eh]
        ret
_TickTempo ENDP

; 0000:4CAC: returns ax = the sample read pointer's offset (the word at 3B78), zero when no sample plays. Far routine, no arguments;
; the seventh entry of the driver's entry table (3944).
        PUBLIC  _GetSampleOffset              ; @ 0000:4CAC
_GetSampleOffset PROC NEAR
        mov       ax, word ptr cs:[3b78h]
        retf
_GetSampleOffset ENDP

; 0000:4CB1: returns the byte at [di+5] of the voice record selected by the argument
        PUBLIC  _GetVoiceState              ; @ 0000:4CB1
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
        mov       di, word ptr [bx + 3e31h]
        mov       al, byte ptr [di + 5]
        xor       ah, ah
        pop       di
        pop       ds
        mov       sp, bp
        pop       bp
        retf
_GetVoiceState ENDP

; 0000:4CD0: reads the AdLib status port 388h 17 times
        PUBLIC  _AdlibDelay17              ; @ 0000:4CD0
_AdlibDelay17 PROC NEAR
        push      ax
        push      cx
        push      dx
        mov       dx, 388h
        mov       cx, 17h
L_4CD9:
        in        al, dx
        loop    L_4CD9
        pop       dx
        pop       cx
        pop       ax
        ret
_AdlibDelay17 ENDP

; 0000:4CE0: reads the AdLib status port 388h 4 times
        PUBLIC  _AdlibDelay4              ; @ 0000:4CE0
_AdlibDelay4 PROC NEAR
        push      ax
        push      cx
        push      dx
        mov       dx, 388h
        mov       cx, 4
L_4CE9:
        in        al, dx
        loop    L_4CE9
        pop       dx
        pop       cx
        pop       ax
        ret
_AdlibDelay4 ENDP

; 0000:4CF0: writes the voice 3E45-table register 43h and, when the voice is two-operator, register 40h
        PUBLIC  _WriteVoiceReg              ; @ 0000:4CF0
_WriteVoiceReg PROC NEAR
        call    _VoiceLevelA0
        mov       bx, 3e45h
        add       bl, byte ptr ds:[3b3eh]
        mov       cl, byte ptr [bx]
        mov       ah, cl
        add       ah, 43h
        call    _AdlibWrite
        cmp       byte ptr [di + 31h], 0
        je    L_4D1E
        call    _VoiceLevelB0
        mov       bx, 3e45h
        add       bl, byte ptr ds:[3b3eh]
        mov       cl, byte ptr [bx]
        mov       ah, cl
        add       ah, 40h
        call    _AdlibWrite
L_4D1E:
        ret
_WriteVoiceReg ENDP

; 0000:4D1F: computes the voice register A0 value: level, added offsets and the shift at 33h/34h
        PUBLIC  _VoiceLevelA0              ; @ 0000:4D1F
_VoiceLevelA0 PROC NEAR
        mov       al, byte ptr [di + 2fh]
        and       al, 3fh
        add       al, byte ptr [di + 20h]
        add       al, byte ptr [di]
        add       al, byte ptr [di + 30h]
        cmp       byte ptr [di + 33h], 0
        je    L_4D40
        mov       cl, byte ptr [di + 33h]
        inc       cl
        xor       bh, bh
        mov       bl, byte ptr [di + 34h]
        shl       bx, cl
        sub       al, bh
L_4D40:
        cmp       al, 3fh
        jbe    L_4D4E
        cmp       al, 0
        jl    L_4D4C
        mov       al, 3fh
        jmp short    L_4D4E
L_4D4C:
        mov       al, 0
L_4D4E:
        mov       bl, byte ptr [di + 2fh]
        and       bl, 0c0h
        or        al, bl
        ret
_VoiceLevelA0 ENDP

; 0000:4D57: computes the voice register B0 value: level, added offsets and the shift at 32h/34h
        PUBLIC  _VoiceLevelB0              ; @ 0000:4D57
_VoiceLevelB0 PROC NEAR
        mov       al, byte ptr [di + 2eh]
        and       al, 3fh
        cmp       byte ptr [di + 31h], 0
        je    L_4D6A
        add       al, byte ptr [di + 20h]
        add       al, byte ptr [di]
        add       al, byte ptr [di + 30h]
L_4D6A:
        cmp       byte ptr [di + 32h], 0
        je    L_4D7E
        mov       cl, byte ptr [di + 32h]
        inc       cl
        xor       bh, bh
        mov       bl, byte ptr [di + 34h]
        shl       bx, cl
        sub       al, bh
L_4D7E:
        cmp       al, 3fh
        jbe    L_4D8C
        cmp       al, 0
        jl    L_4D8A
        mov       al, 3fh
        jmp short    L_4D8C
L_4D8A:
        mov       al, 0
L_4D8C:
        mov       bl, byte ptr [di + 2eh]
        and       bl, 0c0h
        or        al, bl
        ret
_VoiceLevelB0 ENDP

; 0000:4D95: zeroes the 40h-byte voice record from offset 3 and resets its delay and script pointers
        PUBLIC  _ClearVoice              ; @ 0000:4D95
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
        mov       word ptr [di + 29h], 4e7dh
        mov       word ptr [di + 2bh], 4e7dh
        mov       byte ptr [di + 35h], 1
        pop       cx
        pop       ax
        pop       es
        ret
_ClearVoice ENDP

; 0000:4DC8: writes a voice patch: the ten AdLib registers built from the 3e45 table offset CL
        PUBLIC  _LoadVoicePatch              ; @ 0000:4DC8
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
        add       ah, byte ptr ds:[3b3eh]
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
        call    _VoiceLevelB0
        call    _AdlibWrite
        mov       ah, 43h
        add       ah, cl
        lodsb
        mov       byte ptr [di + 2fh], al
        call    _VoiceLevelA0
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

; 0000:4E34: silences a voice: writes ffh to its level registers and B0, then zeroes its gate and key-on
        PUBLIC  _VoiceSilence              ; @ 0000:4E34
_VoiceSilence PROC NEAR
        cmp       byte ptr ds:[3b40h], 0
        je    L_4E41
        cmp       cl, 6
        jl    L_4E41
        ret
L_4E41:
        mov       bx, 3e45h
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
        ret
        cmp       byte ptr [di + 1fh], 0
        je    L_4E88
        dec       byte ptr [di + 1fh]
        ret
L_4E88:
        mov       cl, byte ptr [di + 1eh]
        add       cl, byte ptr [di + 1dh]
        mov       byte ptr [di + 1eh], cl
        jb    L_4E94
        ret
L_4E94:
        mov       bx, word ptr [di + 17h]
        dec       byte ptr [di + 1ah]
        jne    L_4EA9
        xor       bx, 0ffffh
        inc       bx
        mov       word ptr [di + 17h], bx
        mov       al, byte ptr [di + 1bh]
        mov       byte ptr [di + 1ah], al
L_4EA9:
        mov       ax, word ptr [di + 27h]
        and       ax, 3ffh
        add       ax, bx
        mov       byte ptr [di + 27h], al
        mov       al, byte ptr [di + 28h]
        and       al, 0fch
        or        al, ah
        mov       byte ptr [di + 28h], al
        mov       ah, 0a0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, byte ptr [di + 27h]
        call    _AdlibWrite
        mov       ah, 0b0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, byte ptr [di + 28h]
        call    _AdlibWrite
        ret
        mov       al, byte ptr [di + 3ah]
        add       al, byte ptr [di + 39h]
        mov       byte ptr [di + 3ah], al
        jb    L_4EE3
        ret
L_4EE3:
        dec       byte ptr [di + 3ch]
        jns    L_4EEE
        mov       al, byte ptr [di + 3bh]
        mov       byte ptr [di + 3ch], al
L_4EEE:
        mov       ah, byte ptr [di + 3dh]
        add       ah, byte ptr ds:[3b4bh]
        mov       al, byte ptr [di + 3ch]
        mov       bx, word ptr [di + 3eh]
        xlatb
        call    _AdlibWrite
        ret
        mov       cl, byte ptr [di + 14h]
        add       cl, byte ptr [di + 13h]
        mov       byte ptr [di + 14h], cl
        jb    L_4F0C
        ret
L_4F0C:
        mov       bl, byte ptr [di + 27h]
        mov       bh, byte ptr [di + 28h]
        and       bh, 3
        mov       dl, byte ptr [di + 28h]
        mov       dh, dl
        and       dl, 1ch
        and       dh, 20h
        mov       cx, word ptr [di + 15h]
        or        cx, cx
        jl    L_4F43
        add       bx, cx
        cmp       bx, 2deh
        jl    L_4F5D
        shr       bx, 1
        push      bx
        and       bx, 3ffh
        jne    L_4F3A
        pop       bx
        inc       bx
L_4F3A:
        pop       bx
        add       dl, 4
        and       dl, 1ch
        jmp short    L_4F5D
L_4F43:
        add       bx, cx
        cmp       bx, 184h
        jg    L_4F5D
        shl       bx, 1
        push      bx
        and       bx, 3ffh
        jne    L_4F56
        pop       bx
        dec       bx
L_4F56:
        pop       bx
        sub       dl, 4
        and       dl, 1ch
L_4F5D:
        and       bx, 3ffh
        mov       cx, dx
        mov       ah, 0a0h
        add       ah, byte ptr ds:[3b3eh]
        mov       al, bl
        call    _AdlibWrite
        mov       byte ptr [di + 27h], al
        mov       ah, 0b0h
        add       ah, byte ptr ds:[3b3eh]
        or        bh, cl
        or        bh, ch
        mov       al, bh
        call    _AdlibWrite
        mov       byte ptr [di + 28h], al
        ret
_VoiceSilence ENDP

; 0000:4F84: the driver random byte: rotates 3B47 after adding 9248h; the jump table at 4F94 follows it
        PUBLIC  _SoundRandom              ; @ 0000:4F84
_SoundRandom PROC NEAR
        mov       ax, word ptr ds:[3b47h]
        add       ax, 9248h
        ror       ax, 1
        ror       ax, 1
        ror       ax, 1
        mov       word ptr ds:[3b47h], ax
        ret
_SoundRandom ENDP

; 0000:4F94: the script command jump table (data, 148 bytes, 74 entries). UpdateVoices (4250 to 425E: and al,7Fh; cmp al,70h;
; mov bx,[bx+4F94h]; jmp bx) runs entry n for a script byte with the high bit set, 80h + n; entries that name L_4311 are
; commands with no handler of their own, they end the voice's script. QUIRK 4250: the bound is 70h but the table has 4Ah
; entries, so the opcodes F4h to FFh index past it, into the text that follows (the runtime library's "Runtime overlay error").
        PUBLIC  _CommandTable              ; @ 0000:4F94
_CommandTable LABEL WORD
        dw      L_4271, L_4276, L_427F, L_42CE, L_42D4, L_42E0
        dw      L_42F8, L_430B, L_4311, L_4327, L_4344, L_434B
        dw      L_4366, L_436C, L_4389, L_4311, L_43AD, L_43C3
        dw      L_43D9, L_43E7, L_4311, L_43ED, L_4311, L_4311
        dw      L_4311, L_4311, L_440C, L_4311, L_4412, L_442E
        dw      L_445E, L_4311, L_4467, L_447A, L_4311, L_4311
        dw      L_4490, L_4311, L_4499, L_44A0, L_4311, L_44A9
        dw      L_4311, L_44AF, L_44B5, L_44D6, L_44F9, L_4517
        dw      L_4535, L_4311, L_4311, L_4541, L_4311, L_45AD
        dw      L_45E0, L_4311, L_4311, L_45E9, L_46A3, L_46AD
        dw      L_46B1, L_46B7, L_4311, L_46D4, L_4720, L_4729
        dw      L_4808, L_4838, L_484B, L_48EF, L_4993, L_4A23
        dw      L_4A2A, L_4A30

        END
