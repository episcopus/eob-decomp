; Unit at EOB.EXE 1948:0006 (1147 bytes, 13 functions): the PC speaker and Tandy (SN76489) sound
; driver. It plays a stream of note and command bytes for up to three voices on an INT 8 handler
; that the game installs (1948:0377) and removes (1948:03DA). The handler chains to the BIOS INT 8
; handler every sixteenth tick; _SoundTick (1948:0050) advances each voice in the song, picks the
; note's period from a table (PC speaker or Tandy), programs the pitch (1948:0316) and turns the
; voice on or off (1948:02C7 / 1948:02F1). _StartSound (1948:0418) starts a sound for PlaySound
; (177D:0070) and PlaySoundAtCell (177D:0106); the driver is selected by gTandyOn (ds:8A67): 1 is
; Tandy, 0 is the PC speaker (StartupGame installs and selects it, src/E07C8.C:345-351; PlaySound
; starts sounds through StartSound at src/E177D.C:194 and PlaySoundAtCell at src/E177D.C:285).
;
; The driver keeps its state in a segment that is an alias of DGROUP at 88A0h: the code sets DS to
; 29B8h itself (the value the install and every entry set), and DGROUP is 212Eh (whitepaper 2.2), so
; 29B8h is DGROUP + 88Ah and the driver's offset 1C7h is gTandyOn at DGROUP:8A67. The offsets below
; are the driver-segment ones; the segment holds the driver's variables and the note tables the code
; reads at 1Ah (PC speaker) and DAh (Tandy). Nothing in src/ fills those tables (88B to 897A appear
; only in this unit); src/E_TEXT.ASM:6 describes 0000:020C to 03A1 in _TEXT as "the pitch curve
; tables the script commands read (data), then GetScriptFlag (03A1)", which does not describe these
; note tables, so no src/ file names their filler. The song bytes are not in this segment: they are
; read from the block gSongPtrSeg names (cs:0416, the far pointer GetSoundData returns,
; src/E177D.C:121 to 123), read as a song [H] (the whitepaper gives the block only as u16 offsets,
; docs/design/WHITEPAPER.md:177). The code of this unit lives in this code segment.

        .MODEL  LARGE

; The driver's data segment, an alias of DGROUP at 88A0h (DS is set to 29B8h, the value the install
; and every entry set), so its offset 1C7h is DGROUP:8A67. TASM encodes every `[reg+name]` as a
; word displacement (16-bit), as the original shows (the offsets in the original are 16-bit even
; where they fit in a byte).
SND     SEGMENT PARA PUBLIC 'SNDDATA'
        db      6 dup (?)                ; 0 to 5: this unit does not read them. Offset 2 is
                                         ; DGROUP:88A2, read by the code just before the 07A9 unit
                                         ; (mov ax,[88A2] at 07A9:0003, src/E07A9.C:2-3); the saved
                                         ; INT 8 vector lives in the code segment at 1948:0006
gChainReload     dw      ?               ; @ ds:88A6: offset 06h, 0Fh, reload for the BIOS chain countdown
gChainCount      dw      ?               ; @ ds:88A8: offset 08h, counts down; at 0 the handler chains to the BIOS
gSongBase        dw      ?               ; @ ds:88AA: offset 0Ah, song base offset, copied once from gSongPtr
                                         ; at the start of a song (1948:006E); every voice reads from the base + gVoicePos
gSongBaseSeg     dw      ?               ; @ ds:88AC: offset 0Ch, song base segment, copied once at song start (1948:0074)
gVoicePos        dw      4 dup (?)       ; @ ds:88AE: offset 0Eh, word per voice, offset into the song data from the base
gSongPtr         dw      ?               ; @ ds:88B6: offset 16h, song pointer offset (_StartSound's argument)
gSongPtrSeg      dw      ?               ; @ ds:88B8: offset 18h, song pointer segment
gNoteTablePc     dw      96 dup (?)      ; @ ds:88BA: offset 1Ah, PC speaker note -> PIT divisor (0 = rest)
gNoteTableTandy  dw      96 dup (?)      ; @ ds:897A: offset DAh, Tandy note -> SN76489 tone (0 = rest)
gPort61Save      db      ?               ; @ ds:8A3A: offset 19Ah, port 61h value saved at install
gVoiceCount      dw      ?               ; @ ds:8A3B: offset 19Bh, active voices, 3 Tandy or 1 PC speaker
gPlaying         db      ?               ; @ ds:8A3D: offset 19Dh, 1 while a song is playing
gStartRequest    db      ?               ; @ ds:8A3E: offset 19Eh, 1 = (re)load the song at gSongPtr
gStopRequest     db      ?               ; @ ds:8A3F: offset 19Fh, 1 = stop requested. checkRestart
                                         ; (1948:0258) is its only reader, and it runs only for a voice
                                         ; whose gVoiceReadNext is 0 (1948:00F0 to 00F7), so a sounding
                                         ; voice is muted at its next check; a voice that reads its next
                                         ; note or command on the stop tick plays it first (readFirst,
                                         ; 1948:00FA) and is muted on the following tick
gVoiceMute       db      4 dup (?)       ; @ ds:8A40: offset 1A0h, byte per voice, nonzero silences the voice
                                         ; this tick (1948:0278 falls through to muteVoice); only 0 is written
                                         ; here (1948:0081 and 027F), the writer of nonzero is unknown
gVoiceReadNext   db      4 dup (?)       ; @ ds:8A44: offset 1A4h, byte per voice, 1 = read the next note or command
                                         ; this tick (set at 1948:007C and 0273); 0 = a note is sounding and
                                         ; its duration counts down (set at 1948:0151; endCommand leaves 0 too).
                                         ; When the countdown ends, checkRestart sets this back to 1
                                         ; (1948:0273) and calls no _VoiceOff, so the tone keeps sounding
                                         ; until the next note (its pitch is set again by _SetVoicePitch,
                                         ; 1948:0316), a rest (1948:0139) or a mute (muteVoice, 1948:027F)
gVoiceSilent     db      4 dup (?)       ; @ ds:8A48: offset 1A8h, byte per voice, 1 when the driver has
                                         ; turned this voice off. Three writers: startVoice clears all
                                         ; four at each song start (1948:0086), muteVoice (1948:027F) sets
                                         ; one voice, and _SetThreeVoices (1948:034A) sets all four, so
                                         ; after _SoundDriverResume, _SoundDriverInstall or
                                         ; _SoundDriverUninstall the flags are set with no mute by the
                                         ; song. Two readers: checkRestart (1948:0258) skips the re-arm
                                         ; for a flagged voice (1948:025F to 0264), and the end-of-song
                                         ; count clears gPlaying once every voice is flagged (1948:029D
                                         ; to 02BB). A voice muted by a CEh or D2h overflow keeps this
                                         ; flag while it sounds, so checkRestart never re-arms it
gVoiceDuration   dw      4 dup (?)       ; @ ds:8A4C: offset 1ACh, word per voice, note countdown, not
                                         ; "ticks left": readLoop stores (duration byte + 1) times
                                         ; gVoiceLengthMul, less 1 (1948:0142 to 0148). checkRestart
                                         ; decrements it (1948:0266) and keeps the note sounding while the
                                         ; result is 0 or -1 (jns at 1948:026A, the -1 test at 1948:026C);
                                         ; only a result below -1 re-arms the voice (1948:0273), so a
                                         ; stored 0 still sounds one more tick
gVoiceTandyVol   db      4 dup (?)       ; @ ds:8A54: offset 1B4h, byte per voice, Tandy volume 0 silent to 15
                                         ; loudest [H]; no writer is known in src/ (the name and the offset
                                         ; 8A54 appear only in this unit), its only read is _VoiceOn
                                         ; (1948:02D7), which sends the inverse to the chip
gVoiceLengthMul  db      4 dup (?)       ; @ ds:8A58: offset 1B8h, byte per voice, multiplier of the note length,
                                         ; not a volume: it scales the duration (1948:0144)
gVoiceRepeat1    db      4 dup (?)       ; @ ds:8A5C: offset 1BCh, byte per voice, loop counter for CEh.
                                         ; CDh sets it to the following byte, or 1 when that byte is 0
                                         ; (1948:0189); CEh counts it down and loops while nonzero (1948:015D).
                                         ; Nothing clears it at song start (startVoice, 1948:007C to 00A3)
                                         ; or in the install, so a CEh before its CDh counts down whatever
                                         ; the previous song or the install left: the counters start unset
gVoiceRepeat2    db      4 dup (?)       ; @ ds:8A60: offset 1C0h, byte per voice, loop counter for D2h.
                                         ; D1h sets it the same way (1948:01C6); D2h counts it down (1948:0198).
                                         ; Like gVoiceRepeat1 it starts unset, so a D2h before its D1h
                                         ; counts down whatever the previous song or the install left
gVoiceDelay1     db      ?               ; @ ds:8A64: offset 1C4h, countdown before the voice at index 1 plays
gVoiceDelay2     db      ?               ; @ ds:8A65: offset 1C5h, countdown before the voice at index 2 plays
gRunning         db      ?               ; @ ds:8A66: offset 1C6h, 1 while the driver has an active song;
                                         ; duplicates gPlaying (both are written at the same three points,
                                         ; 1948:0057, 02BB and 0439), read nowhere in src/ (grep)
gTandyOn         db      ?               ; @ ds:8A67: offset 1C7h, 1 Tandy, 0 PC speaker
gTickCount       dw      ?               ; @ ds:8A68: offset 1C8h, handler tick counter, 0 to 123h (292 ticks, about
                                         ; one second at 291 Hz [H]); when it passes 123h it wraps to 0 and
                                         ; gTickSeconds is incremented
gTickSeconds     db      ?               ; @ ds:8A6A: offset 1CAh, [H] seconds counter, incremented once per
                                         ; gTickCount period and wrapped at 3Bh (0 to 59); nothing outside
                                         ; this unit reads it
SND     ENDS

        .CODE
        ASSUME  DS:SND

; 1948:0006: the INT 8 timer handler installed at 1948:000A. Its first four bytes are the saved
; BIOS INT 8 vector (offset, segment) that the install routine writes and the handler chains to;
; the vector is read through CS, the counters through DS. Every tick it calls _SoundTick; once
; every sixteenth it does not acknowledge the PIC but jumps to the old BIOS handler instead, so
; the BIOS clock still ticks at its own rate: the PIT clock (1193180 Hz [H]) divided by the install
; divisor 1000h gives about 291.3 Hz, and chaining on every sixteenth tick divides that by sixteen
; for 1193180 / 65536, about 18.2 Hz, the BIOS INT 8 rate.
        PUBLIC  _SoundTimerIsr          ; @ 1948:0006
_SoundTimerIsr PROC FAR
oldInt8Off      dw      0               ; 1948:0006: saved BIOS INT 8 handler offset
oldInt8Seg      dw      0               ; 1948:0008: saved BIOS INT 8 handler segment
        push    ds
        push    ax
        mov     ax, 29B8h
        mov     ds, ax
        mov     ax, [gTickCount]
        inc     ax
        cmp     ax, 123h
        jle     storeTick
        mov     al, [gTickSeconds]
        inc     al
        cmp     al, 3Bh
        jle     storePhase
        xor     al, al
storePhase:
        mov     [gTickSeconds], al
        xor     ax, ax
storeTick:
        mov     [gTickCount], ax
        call    _SoundTick
        mov     ax, [gChainCount]
        or      al, al
        jne     ackPIC
        mov     ax, [gChainReload]
        mov     [gChainCount], ax
        pop     ax
        pop     ds
        jmp     dword ptr cs:[oldInt8Off]
ackPIC:
        dec     al
        mov     [gChainCount], ax
        mov     al, 20h
        out     20h, al
        pop     ax
        pop     ds
        iret
_SoundTimerIsr ENDP

; 1948:0050: advance the song by one tick. A start request (gStartRequest) reloads the song
; pointer, restarts the voices (the delay counters 23h and 46h hold back only the voices at index 1
; and 2, so voice 0 starts at once) and clears the stop request; then,
; for each active voice up to gVoiceCount, it reads note and command bytes from the song data at
; gSongBase+gVoicePos[voice] and issues the tone and the voice on/off. Note bytes 00h to 5Fh index
; the 96-entry note table; 60h to 7Fh are neither notes nor commands and leave the voice silent for
; good: it jumps to muteVoice (1948:027F) without consuming the byte, so the position and
; gVoiceReadNext stay and it reads the same byte again on every later tick (1948:010B). The command
; bytes are CDh and D1h (set the repeat counters), CEh and D2h (loop back: the
; command's position plus 1 minus the following offset word; a signed overflow of that position mutes
; the voice for that tick only, 1948:0176 and 01B2, unlike 60h to 7Fh which stay silent for good),
; E6h (play a note at an explicit frequency), E2h (set the length multiplier), E8h and E9h (subtract
; from or add to it); any other byte from 80h up goes to endCommand (1948:0250), which ends
; the voice. An overflow mute does not set gVoiceReadNext back to 0, so the next
; tick reads again from the wrapped position saved at 1948:028C and a note there can sound; that voice
; keeps gVoiceSilent at 1, and checkRestart never re-arms a flagged voice (1948:025F to 0264), so the
; note it reads never ends. When all voices end silent, the song stops. Called by the handler.
        PUBLIC  _SoundTick              ; @ 1948:0050
_SoundTick PROC NEAR
        cmp     byte ptr [gStartRequest], 0
        je      checkPlaying
        mov     byte ptr [gRunning], 1
        mov     byte ptr [gPlaying], 1
        mov     byte ptr [gStartRequest], 0
        mov     byte ptr [gStopRequest], 0
        mov     ax, [gSongPtr]
        mov     [gSongBase], ax
        mov     ax, [gSongPtrSeg]
        mov     [gSongBaseSeg], ax
        push    di
        xor     di, di
        mov     al, 3
startVoice:
        mov     byte ptr [di+gVoiceReadNext], 1
        mov     byte ptr [di+gVoiceMute], 0
        mov     byte ptr [di+gVoiceSilent], 0
        mov     byte ptr [di+gVoiceLengthMul], 1
        shl     di, 1
        mov     word ptr [di+gVoicePos], 0
        mov     word ptr [di+gVoiceDuration], 0
        shr     di, 1
        inc     di
        dec     al
        jne     startVoice
        pop     di
        mov     byte ptr [gVoiceDelay1], 23h
        mov     byte ptr [gVoiceDelay2], 46h
checkPlaying:
        cmp     byte ptr [gPlaying], 0
        jne     beginVoices
        jmp     tickDone
beginVoices:
        push    es
        push    si
        push    di
        push    bp
        push    bx
        push    dx
        xor     si, si
voiceLoop:
        mov     di, si
        shl     di, 1
        mov     bp, [di+gVoicePos]
        cmp     si, 1
        jne     notVoice1
        cmp     byte ptr [gVoiceDelay1], 0
        je      voiceActive
        dec     byte ptr [gVoiceDelay1]
        jmp     nextVoice
notVoice1:
        cmp     si, 2
        jne     voiceActive
        cmp     byte ptr [gVoiceDelay2], 0
        je      voiceActive
        dec     byte ptr [gVoiceDelay2]
        jmp     nextVoice
voiceActive:
        cmp     byte ptr [si+gVoiceReadNext], 0
        jne     readFirst
        jmp     checkRestart
readFirst:
        mov     es, [gSongBaseSeg]
        mov     bx, [gSongBase]
        add     bx, bp
; 00h to 5Fh: a note table index; 60h to 7Fh: neither a note nor a command, it jumps to muteVoice
; without consuming the byte, so the voice stays silent for the rest of the song
readLoop:
        mov     al, es:[bx]
        or      al, al
        js      command
        cmp     al, 5Fh
        jle     playNote
        jmp     muteVoice
playNote:
        push    di
        xor     ah, ah
        shl     ax, 1
        mov     di, ax
        cmp     byte ptr [gTandyOn], 1
        je      useTandyTable
        mov     ax, [di+gNoteTablePc]
        jmp     afterTable
useTandyTable:
        mov     ax, [di+gNoteTableTandy]
afterTable:
        pop     di
afterTandy:
        or      ax, ax
        je      silence
        call    _SetVoicePitch
        call    _VoiceOn
        jmp     noteDone
silence:
        call    _VoiceOff
noteDone:
        inc     bx
        xor     ah, ah
        mov     al, es:[bx]
        inc     al              ; QUIRK 1948:0142: a duration byte of 0FFh makes AL 0 after the inc;
                                ; the mul (1948:0144) then gives 0 times the length multiplier, which
                                ; is 0 for every multiplier, and dec ax (1948:0148) leaves FFFFh. So
                                ; the stored count is -1 whatever the multiplier, and checkRestart
                                ; re-arms the voice one tick sooner than for a count of 0
        mul     byte ptr [si+gVoiceLengthMul]
        dec     ax
        mov     [di+gVoiceDuration], ax
        inc     bx
        add     bp, 2
        mov     byte ptr [si+gVoiceReadNext], 0
        jmp     nextVoice
command:
        cmp     al, 0CEh
        jne     cmdCD
        dec     byte ptr [si+gVoiceRepeat1]
        jne     cmdCEJump
        add     bx, 3
        add     bp, 3
        jmp     readLoop
cmdCEJump:
        inc     bx
        mov     ax, es:[bx]
        inc     bp
        sub     bx, ax
        sub     bp, ax
        jno     cmdCEOk         ; QUIRK 1948:0174: a signed overflow of the loop target mutes the voice
                                ; for this tick only; the next tick reads from the wrapped position
        jmp     muteVoice
cmdCEOk:
        jmp     readLoop
cmdCD:
        cmp     al, 0CDh
        jne     cmdD2
        inc     bx
        mov     al, es:[bx]
        or      al, al
        jne     cmdCDStore
        mov     al, 1
cmdCDStore:
        mov     [si+gVoiceRepeat1], al
        inc     bx
        add     bp, 2
        jmp     readLoop
cmdD2:
        cmp     al, 0D2h
        jne     cmdD1
        dec     byte ptr [si+gVoiceRepeat2]
        jne     cmdD2Jump
        add     bx, 3
        add     bp, 3
        jmp     readLoop
cmdD2Jump:
        inc     bx
        mov     ax, es:[bx]
        inc     bp
        sub     bx, ax
        sub     bp, ax
        jno     cmdD2Ok         ; QUIRK 1948:01B0: a signed overflow of the loop target mutes the voice
                                ; for this tick only; the next tick reads from the wrapped position
        jmp     muteVoice
cmdD2Ok:
        jmp     readLoop
cmdD1:
        cmp     al, 0D1h
        jne     cmdE6
        inc     bx
        mov     al, es:[bx]
        or      al, al
        jne     cmdD1Store
        mov     al, 1
cmdD1Store:
        mov     [si+gVoiceRepeat2], al
        inc     bx
        add     bp, 2
        jmp     readLoop
cmdE6:
        cmp     al, 0E6h
        jne     cmdE2
        inc     bx
        cmp     byte ptr [gTandyOn], 1
        je      cmdE6Tandy
        mov     dx, 12h
        mov     ax, 34DCh
        div     word ptr es:[bx]  ; QUIRK 1948:01E3: INT 0 when the quotient exceeds 0FFFFh,
                                  ; a frequency word of 18 or less [H]
        jmp     cmdE6After
cmdE6Tandy:
        mov     dx, 1
        mov     ax, 0B4F4h
        div     word ptr es:[bx]  ; QUIRK 1948:01EF: INT 0 for a frequency word of 0 or 1 [H]
        cmp     ax, 400h          ; clamp to 3FFh, the widest 10-bit SN76489 tone; a port must keep it
        jb      cmdE6After
        mov     ax, 3FFh
cmdE6After:
        inc     bx
        add     bp, 2
        jmp     afterTandy
cmdE2:
        cmp     al, 0E2h
        jne     cmdE8
        inc     bx
        mov     al, es:[bx]
        or      al, al
        jne     cmdE2Store
        mov     al, 1
cmdE2Store:
        mov     [si+gVoiceLengthMul], al
        inc     bx
        add     bp, 2
        jmp     readLoop
cmdE8:
        cmp     al, 0E8h
        jne     cmdE9
        inc     bx
        mov     al, es:[bx]
        sub     byte ptr [si+gVoiceLengthMul], al
        jno     cmdE8Ok
        jne     cmdE8Ok         ; QUIRK 1948:0228: E8h never clamps to 1: sub sets OF only on signed
                                ; overflow, so an underflow that stays in the signed range wraps (5 minus
                                ; 10 gives 251) and the jno at 1948:0226 takes the normal path. The clamp
                                ; at 1948:022A is reached only on signed overflow, and there this jne
                                ; always jumps (a zero result cannot overflow), so even it is dead; a
                                ; zero multiplier is one instance of the missing clamp
        mov     byte ptr [si+gVoiceLengthMul], 1
cmdE8Ok:
        inc     bx
        add     bp, 2
        jmp     readLoop
cmdE9:
        cmp     al, 0E9h
        jne     endCommand
        inc     bx
        mov     al, es:[bx]
        add     byte ptr [si+gVoiceLengthMul], al
        jno     cmdE9Ok         ; QUIRK 1948:0242: saturates to 0FFh only on signed overflow, so 100
                                ; plus 100 gives 255 but 200 plus 100 wraps to 44
        mov     byte ptr [si+gVoiceLengthMul], 0FFh
cmdE9Ok:
        inc     bx
        add     bp, 2
        jmp     readLoop
endCommand:
        mov     byte ptr [si+gVoiceReadNext], 0
        jmp     muteVoice
checkRestart:
        cmp     byte ptr [gStopRequest], 1
        je      muteVoice
        cmp     byte ptr [si+gVoiceSilent], 1
        je      restartDone
        dec     word ptr [di+gVoiceDuration]
        jns     restartDone
        cmp     word ptr [di+gVoiceDuration], -1
        je      restartDone
        mov     byte ptr [si+gVoiceReadNext], 1
restartDone:
        cmp     byte ptr [si+gVoiceMute], 0
        je      nextVoice
muteVoice:
        mov     byte ptr [si+gVoiceMute], 0
        mov     byte ptr [si+gVoiceSilent], 1
        call    _VoiceOff
nextVoice:
        mov     [di+gVoicePos], bp
        inc     si
        cmp     word ptr [gVoiceCount], si
        jle     loopDone
        jmp     voiceLoop
loopDone:
        xor     ax, ax
        dec     si
countSilent:
        cmp     byte ptr [si+gVoiceSilent], 1
        jne     countNext
        inc     ax
countNext:
        dec     si
        cmp     si, 0
        jge     countSilent
        cmp     word ptr [gVoiceCount], ax
        jne     voicesDone
        mov     byte ptr [gStopRequest], 0
        mov     byte ptr [gPlaying], 0
        mov     byte ptr [gRunning], 0
voicesDone:
        pop     dx
        pop     bx
        pop     bp
        pop     di
        pop     si
        pop     es
tickDone:
        ret
_SoundTick ENDP

; 1948:02C7: turn the current voice on. On the PC speaker it sets the port 61h gate and data
; bits; on Tandy it writes the voice's volume register of the SN76489 with the voice's stored
; volume (gVoiceTandyVol) inverted, because the chip takes 0 as loudest and 15 as off [H]. SI is
; the voice.
        PUBLIC  _VoiceOn                ; @ 1948:02C7
_VoiceOn PROC NEAR
        cmp     byte ptr [gTandyOn], 1
        je      tandyOn
        in      al, 61h
        or      al, 3
        out     61h, al
        ret
tandyOn:
        mov     ax, si
        mov     ah, [si+gVoiceTandyVol]
        xor     ah, 0Fh
        shl     al, 1
        inc     al
        shl     al, 1
        shl     al, 1
        shl     al, 1
        shl     al, 1
        or      al, ah
        or      al, 80h
        out     0C0h, al
        ret
_VoiceOn ENDP

; 1948:02F1: turn the current voice off. On the PC speaker it clears the port 61h gate and data
; bits; on Tandy it writes the voice's volume register with 0Fh, the chip's full attenuation
; (silence) [H]. SI is the voice.
        PUBLIC  _VoiceOff               ; @ 1948:02F1
_VoiceOff PROC NEAR
        cmp     byte ptr [gTandyOn], 1
        je      tandyOff
        in      al, 61h
        and     al, 0FCh
        out     61h, al
        ret
tandyOff:
        mov     ax, si
        mov     ah, 0Fh
        shl     al, 1
        inc     al
        shl     al, 1
        shl     al, 1
        shl     al, 1
        shl     al, 1
        or      al, ah
        or      al, 80h
        out     0C0h, al
        ret
_VoiceOff ENDP

; 1948:0316: program the current voice's pitch, AX = the table value. On the PC speaker it writes
; the 16-bit divisor to PIT channel 2; on Tandy it writes the SN76489's tone register (the low
; nibble, then the high bits). SI is the voice.
        PUBLIC  _SetVoicePitch          ; @ 1948:0316
_SetVoicePitch PROC NEAR
        cmp     byte ptr [gTandyOn], 1
        je      tandyPitch
        out     42h, al
        mov     al, ah
        out     42h, al
        ret
tandyPitch:
        mov     dx, ax
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        mov     dl, al
        and     dl, 0Fh
        mov     ax, si
        shl     al, 1
        shl     al, 1
        shl     al, 1
        shl     al, 1
        shl     al, 1
        or      al, dl
        or      al, 80h
        out     0C0h, al
        mov     al, dh
        out     0C0h, al
        ret
_SetVoicePitch ENDP

; 1948:034A: three-voice output. Sets gVoiceCount = 3, silences the four voices through _VoiceOff
; (so the Tandy chip is quiet) and sets the port 61h bits 5 and 6 (their meaning is unknown; the
; code only sets them, and on the PC these are read-only status bits [H], so the writes do nothing
; there; whether they switch on the Tandy chip is open [H] and a question for the oracle). The fourth
; silence (voice index 3) writes the SN76489 noise channel, because
; the latch's channel bits 6-5 are 3 [H]; no path sets gVoiceCount to 4, so voice 3 is never played.
; It also sets gVoiceSilent for all four voices in the loop (1948:0352), so a call during a song
; flags every voice: the resume callers start a sound straight after (src/E177D.C:185 and 189;
; 274 and 278) and the next tick's start path clears the flags (1948:0050 to 0086, the startVoice
; loop at 1948:007C clears gVoiceSilent at 0086), but the shutdown caller of
; _SoundDriverSilenceToOneVoice (src/E07C8.C:374) does not, so its flags end the song at the next
; tick (see gVoiceSilent).
; Called by the install routine on Tandy (1948:0377), _SoundDriverResume (1948:0458),
; _SoundDriverUninstall (1948:03DA) and _SoundDriverSilenceToOneVoice (1948:046B).
        PUBLIC  _SetThreeVoices         ; @ 1948:034A
_SetThreeVoices PROC NEAR
        mov     word ptr [gVoiceCount], 3
        xor     si, si
threeLoop:
        mov     byte ptr [si+gVoiceSilent], 1
        call    _VoiceOff
        inc     si
        cmp     si, 4
        jne     threeLoop
        in      al, 61h
        or      al, 60h
        out     61h, al
        ret
_SetThreeVoices ENDP

; 1948:0367: one-voice output. Sets gVoiceCount = 1 and restores port 61h from the value saved at
; install (gPort61Save), clearing the gate and data bits. Called by the install routine on the PC
; speaker (1948:0377), the uninstall routine (1948:03DA) and _SoundDriverSilenceToOneVoice (1948:046B).
; QUIRK 1948:036D: the port 61h read is thrown away, the stored value is written instead.
        PUBLIC  _SetOneVoice            ; @ 1948:0367
_SetOneVoice PROC NEAR
        mov     word ptr [gVoiceCount], 1
        in      al, 61h
        mov     al, [gPort61Save]
        and     al, 0FCh
        out     61h, al
        ret
_SetOneVoice ENDP

; 1948:0377: install the driver. Saves the BIOS INT 8 vector in the handler's own words
; (cs:oldInt8Off/oldInt8Seg), sets the chain reload (gChainReload = 0Fh) and count (0) so the
; handler passes one tick in sixteen to the BIOS and points INT 8 at cs:000Ah. It programs PIT
; channel 0 to mode 3 with divisor 1000h (about 291 Hz [H]) and then puts channel 2 in mode 3
; without loading its count (_SetVoicePitch loads it, 1948:0316), remembers port 61h in
; gPort61Save and sets the output up for Tandy (_SetThreeVoices, 1948:03D2) or the PC speaker
; (_SetOneVoice, 1948:03CC) by gTandyOn (1948:03C5). Called by StartupGame after the sound card
; choice (src/E07C8.C:345 and 348). At both call sites gTandyOn is still 0 while the install runs
; (the Tandy case sets it to 1 only afterwards, src/E07C8.C:349), so from StartupGame the install
; takes its one-voice branch and the three voices come from _SoundDriverResume (src/E07C8.C:351); the
; Tandy branch is reached only if a PlaySound call set gTandyOn to 1 first (src/E177D.C:188).
        PUBLIC  _SoundDriverInstall     ; @ 1948:0377
_SoundDriverInstall PROC FAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
        mov     ax, 29B8h
        mov     ds, ax
        push    ds
        mov     al, 8
        mov     ah, 35h
        int     21h
        mov     word ptr cs:[oldInt8Off], bx
        mov     word ptr cs:[oldInt8Seg], es
        mov     word ptr [gChainReload], 0Fh
        mov     word ptr [gChainCount], 0
        mov     ax, cs
        mov     ds, ax
        mov     dx, 0Ah
        mov     al, 8
        mov     ah, 25h
        int     21h
        mov     al, 36h
        out     43h, al
        mov     ax, 1000h
        out     40h, al
        mov     al, ah
        out     40h, al
        mov     al, 0B6h
        out     43h, al
        pop     ds
        in      al, 61h
        mov     [gPort61Save], al
        out     61h, al              ; the value just read is written back unchanged, not a gate change:
                                     ; it keeps the port as the BIOS left it until _SetOneVoice or
                                     ; _SetThreeVoices adjusts the bits
        cmp     byte ptr [gTandyOn], 1
        je      installTandy
        call    _SetOneVoice
        jmp     installDone
installTandy:
        call    _SetThreeVoices
installDone:
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
_SoundDriverInstall ENDP

; 1948:03DA: remove the driver. Restores the saved BIOS INT 8 vector (cs:oldInt8Off/oldInt8Seg)
; and programs PIT channel 0 to mode 3 with divisor 0 (65536, about 18.2 Hz, the BIOS divisor);
; on Tandy it silences the three voices before _SetOneVoice restores port 61h. Called by
; ShutdownGame (src/E07C8.C:368 and 376, the gSoundCard 1 and 2 cases).
        PUBLIC  _SoundDriverUninstall   ; @ 1948:03DA
_SoundDriverUninstall PROC FAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
        mov     ax, 29B8h
        mov     ds, ax
        push    ds
        mov     dx, word ptr cs:[oldInt8Off]
        mov     bx, word ptr cs:[oldInt8Seg]
        mov     ds, bx
        mov     al, 8
        mov     ah, 25h
        int     21h
        mov     al, 36h
        out     43h, al
        mov     ax, 0
        out     40h, al
        mov     al, ah
        out     40h, al
        pop     ds
        cmp     byte ptr [gTandyOn], 1
        jne     uninstallSpeaker
        call    _SetThreeVoices
uninstallSpeaker:
        call    _SetOneVoice
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
_SoundDriverUninstall ENDP

; 1948:0418: start a sound. Stores the far pointer to its song data in gSongPtr/gSongPtrSeg and
; sets the run, play and start-request flags so the next handler tick loads it. The pointer is the
; one GetSoundData (0000:03E3, src/E177D.C:121-123) returns for the chosen driver sound. That the
; block it points at is song data is [H]: _SoundTick reads its bytes as notes and commands, but
; whitepaper 4.4 (docs/design/WHITEPAPER.md:177) gives the block only as u16 offsets, and
; src/E177D.C:121-122 words it as a driver routine (StartSound's parameter is named `routine` at
; src/E177D.C:130); the two units disagree, to decide at merge. Called by PlaySound (177D:0070,
; src/E177D.C:194) and PlaySoundAtCell (177D:0106, src/E177D.C:285). Argument: a far pointer at
; [bp+6] offset, [bp+8] segment.
        PUBLIC  _StartSound             ; @ 1948:0418
_StartSound PROC FAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
        mov     ax, 29B8h
        mov     ds, ax
        mov     ax, [bp+6]
        mov     [gSongPtr], ax
        mov     ax, [bp+8]
        mov     [gSongPtrSeg], ax
        mov     byte ptr [gStartRequest], 1
        mov     byte ptr [gPlaying], 1
        mov     byte ptr [gRunning], 1
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
_StartSound ENDP

; 1948:0443: ask the driver to stop. Sets gStopRequest; on the next tick checkRestart (1948:0258)
; mutes each sounding voice, and once all voices are silent the play flags are cleared (1948:02B1
; to 02BB). A voice that reads its next note or command on the stop tick plays it first and is muted
; on the following tick. Purpose unknown: the packet finds no direct caller, so who asks for a stop
; is unknown.
        PUBLIC  _StopSound              ; @ 1948:0443
_StopSound PROC FAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
        mov     ax, 29B8h
        mov     ds, ax
        mov     byte ptr [gStopRequest], 1
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
_StopSound ENDP

; 1948:0458: set up the three-voice output (_SetThreeVoices), which sets gVoiceCount to 3 and flags
; all four voices silent (1948:034A). A resume during a song flags every voice, but the game never
; stops a song that way: both callers that change the driver select call StartSound straight after
; the resume (src/E177D.C:185 and 189, then 194; 274 and 278, then 285), and the next tick's start
; path restarts every voice and clears the flags (1948:0050 to 0086, startVoice at 1948:007C, its
; flag clear at 0086). The stop needs a resume with no start request after it, and no caller in src/
; makes one during a song (StartupGame's resume, src/E07C8.C:351, also has none, but no song is
; playing then). E177D.C calls it when the driver select gTandyOn changes (src/E177D.C:185 and 189 in
; PlaySound, 274 and 278 in PlaySoundAtCell); StartupGame calls it once after the Tandy install
; (src/E07C8.C:348 to 351).
        PUBLIC  _SoundDriverResume      ; @ 1948:0458
_SoundDriverResume PROC FAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
        mov     ax, 29B8h
        mov     ds, ax
        call    _SetThreeVoices
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
_SoundDriverResume ENDP

; 1948:046B: silence the three voices (_SetThreeVoices) and then fall back to one-voice output
; (_SetOneVoice). The flags _SetThreeVoices sets (1948:0352) end the song at the next tick:
; _SetOneVoice leaves gVoiceCount at 1 (1948:0367), the end-of-song count (1948:029A to 02BB) counts
; that one voice as flagged and clears gPlaying (1948:02B6). Only used on the way out: ShutdownGame's
; Tandy case calls it just before gTandyOn = 0 and _SoundDriverUninstall (src/E07C8.C:374-376), and
; nothing resumes the output afterwards. The uninstall follows at once and removes the handler, but a
; tick can still run before it. Because gTandyOn is then 0, _SoundDriverUninstall skips its Tandy
; step (1948:0406).
        PUBLIC  _SoundDriverSilenceToOneVoice       ; @ 1948:046B
_SoundDriverSilenceToOneVoice PROC FAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
        mov     ax, 29B8h
        mov     ds, ax
        call    _SetThreeVoices
        call    _SetOneVoice
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
_SoundDriverSilenceToOneVoice ENDP

        END
