; Unit at EOB.EXE 1948:0006 (1147 bytes, 13 functions): the PC speaker and Tandy (SN76489) sound
; driver. It plays a stream of note and command bytes for up to three voices on an INT 8 handler
; that the game installs (1948:0377) and removes (1948:03DA). The handler chases the BIOS INT 8
; every sixteenth tick; _SoundTick (1948:0050) advances each voice in the song, picks the note's
; period from a table (PC speaker or Tandy), programs the pitch (1948:0316) and turns the voice on
; or off (1948:02C7 / 1948:02F1). _StartSound (1948:0418) is the entry 177D:0070 uses to start a
; sound; the driver is selected by gSpeakerOn (ds:8A67): 1 is Tandy, 0 is the PC speaker
; (whitepaper 2.7, the sound choice in 2.6, E177D.C).
;
; The driver keeps its state in its own segment (the loaded DS is 29B8h; DGROUP is 212Eh, whitepaper
; 2.x, so DS = DGROUP + 88Ah and the driver's offset 1C7h is gSpeakerOn at DGROUP:8A67). The
; offsets below are the driver-segment ones; the code and the note tables (at 1Ah for the PC
; speaker, DAh for Tandy) live in the loaded driver data, not in this code segment.

        .MODEL  LARGE

; The driver's data segment. It is a separate segment, not DGROUP: the code below sets DS to
; 29B8h itself, and TASM then encodes every `[reg+name]` as a word displacement (16-bit), as the
; original shows (the offsets in the original are 16-bit even where they fit in a byte).
SND     SEGMENT PARA PUBLIC 'SNDDATA'
        db      6 dup (?)                ; 06h chain reload and count live here
gChainReload    dw      ?               ; 06h: 0Fh, reload for the BIOS chain countdown
gChainCount     dw      ?               ; 08h: counts down; at 0 the handler chains to the BIOS
gSongPos        dw      ?               ; 0Ah: current song position, offset
gSongPosSeg     dw      ?               ; 0Ch: current song position, segment
gVoicePos       dw      4 dup (?)       ; 0Eh: word per voice, offset into the song data
gSongPtr        dw      ?               ; 16h: song pointer offset (_StartSound's argument)
gSongPtrSeg     dw      ?               ; 18h: song pointer segment
gNoteTablePc    dw      96 dup (?)       ; 1Ah: PC speaker note -> PIT divisor (0 = rest)
gNoteTableTandy dw      96 dup (?)       ; DAh: Tandy note -> SN76489 tone (0 = rest)
gPort61Save     db      ?               ; 19Ah: port 61h value saved at install
gVoiceCount     dw      ?               ; 19Bh: active voices, 3 Tandy or 1 PC speaker
gPlaying        db      ?               ; 19Dh: 1 while a song is playing
gStartRequest   db      ?               ; 19Eh: 1 = (re)load the song at gSongPtr
gStopRequest    db      ?               ; 19Fh: 1 = mute every voice
gVoiceRestart   db      4 dup (?)       ; 1A0h: byte per voice, 1 = re-issue the note
gVoiceActive    db      4 dup (?)       ; 1A4h: byte per voice, 1 while a note is sounding
gVoiceSilent    db      4 dup (?)       ; 1A8h: byte per voice, 1 while the voice is muted
gVoiceDuration  dw      4 dup (?)       ; 1ACh: word per voice, ticks left of the note
gVoiceTandyVol  db      4 dup (?)       ; 1B4h: byte per voice, Tandy attenuation nibble
gVoiceAmp       db      4 dup (?)       ; 1B8h: byte per voice, length multiplier
gVoiceRepeat1   db      4 dup (?)       ; 1BCh: byte per voice, counter for command CEh
gVoiceRepeat2   db      4 dup (?)       ; 1C0h: byte per voice, counter for command D2h
gVoiceDelay1    db      ?               ; 1C4h: countdown before voice 1 plays
gVoiceDelay2    db      ?               ; 1C5h: countdown before voice 2 plays
gRunning        db      ?               ; 1C6h: 1 while the driver has an active song
gSpeakerOn      db      ?               ; 1C7h: 1 Tandy, 0 PC speaker (ds:8A67)
gTickCount      dw      ?               ; 1C8h: handler tick counter, reloads gTickPhase
gTickPhase      db      ?               ; 1CAh: handler phase counter, wraps at 3Bh
SND     ENDS

        .CODE
        ASSUME  DS:SND

; 1948:0006: the INT 8 timer handler installed at 1948:000A. Its first four bytes are the saved
; BIOS INT 8 vector (offset, segment) that the install routine writes and the handler chains to;
; the ivector is read through CS, the counters through DS. Every tick it calls _SoundTick; once
; every sixteenth it does not acknowledge the PIC but jumps to the old BIOS handler instead, so
; the BIOS clock still ticks at its own rate.
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
        mov     al, [gTickPhase]
        inc     al
        cmp     al, 3Bh
        jle     storePhase
        xor     al, al
storePhase:
        mov     [gTickPhase], al
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
; pointer, restarts the voices (delay counters 23h and 46h) and clears the stop request; then,
; for each active voice up to gVoiceCount, it reads note and command bytes from the song data at
; gSongPos+gVoicePos[voice] and issues the tone and the voice on/off. Commands 80h and up set the
; repeat counters (CDh, D1h), loop back (CEh, D2h), rest (E6h), set or add to the note length (E2h,
; E8h, E9h) or end the voice. When all voices end silent, the song stops. Called by the handler.
        PUBLIC  _SoundTick              ; @ 1948:0050
_SoundTick PROC NEAR
        cmp     byte ptr [gStartRequest], 0
        je      checkPlaying
        mov     byte ptr [gRunning], 1
        mov     byte ptr [gPlaying], 1
        mov     byte ptr [gStartRequest], 0
        mov     byte ptr [gStopRequest], 0
        mov     ax, [gSongPtr]
        mov     [gSongPos], ax
        mov     ax, [gSongPtrSeg]
        mov     [gSongPosSeg], ax
        push    di
        xor     di, di
        mov     al, 3
startVoice:
        mov     byte ptr [di+gVoiceActive], 1
        mov     byte ptr [di+gVoiceRestart], 0
        mov     byte ptr [di+gVoiceSilent], 0
        mov     byte ptr [di+gVoiceAmp], 1
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
        cmp     byte ptr [si+gVoiceActive], 0
        jne     readFirst
        jmp     checkRestart
readFirst:
        mov     es, [gSongPosSeg]
        mov     bx, [gSongPos]
        add     bx, bp
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
        cmp     byte ptr [gSpeakerOn], 1
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
        inc     al
        mul     byte ptr [si+gVoiceAmp]
        dec     ax
        mov     [di+gVoiceDuration], ax
        inc     bx
        add     bp, 2
        mov     byte ptr [si+gVoiceActive], 0
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
        jno     cmdCEOk
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
        jno     cmdD2Ok
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
        cmp     byte ptr [gSpeakerOn], 1
        je      cmdE6Tandy
        mov     dx, 12h
        mov     ax, 34DCh
        div     word ptr es:[bx]
        jmp     cmdE6After
cmdE6Tandy:
        mov     dx, 1
        mov     ax, 0B4F4h
        div     word ptr es:[bx]
        cmp     ax, 400h
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
        mov     [si+gVoiceAmp], al
        inc     bx
        add     bp, 2
        jmp     readLoop
cmdE8:
        cmp     al, 0E8h
        jne     cmdE9
        inc     bx
        mov     al, es:[bx]
        sub     byte ptr [si+gVoiceAmp], al
        jno     cmdE8Ok
        jne     cmdE8Ok
        mov     byte ptr [si+gVoiceAmp], 1
cmdE8Ok:
        inc     bx
        add     bp, 2
        jmp     readLoop
cmdE9:
        cmp     al, 0E9h
        jne     endCommand
        inc     bx
        mov     al, es:[bx]
        add     byte ptr [si+gVoiceAmp], al
        jno     cmdE9Ok
        mov     byte ptr [si+gVoiceAmp], 0FFh
cmdE9Ok:
        inc     bx
        add     bp, 2
        jmp     readLoop
endCommand:
        mov     byte ptr [si+gVoiceActive], 0
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
        mov     byte ptr [si+gVoiceActive], 1
restartDone:
        cmp     byte ptr [si+gVoiceRestart], 0
        je      nextVoice
muteVoice:
        mov     byte ptr [si+gVoiceRestart], 0
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
; attenuation (gVoiceTandyVol, inverted). SI is the voice.
        PUBLIC  _VoiceOn                ; @ 1948:02C7
_VoiceOn PROC NEAR
        cmp     byte ptr [gSpeakerOn], 1
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
; bits; on Tandy it writes the voice's volume register with full attenuation (silence). SI is the
; voice.
        PUBLIC  _VoiceOff               ; @ 1948:02F1
_VoiceOff PROC NEAR
        cmp     byte ptr [gSpeakerOn], 1
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
        cmp     byte ptr [gSpeakerOn], 1
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

; 1948:034A: three-voice output. Sets gVoiceCount = 3, silences all four voices through _VoiceOff
; (so the Tandy chip is quiet) and sets the port 61h bits 5 and 6. Called by the install routine
; on Tandy (1948:0377) and by _SoundDriverResume (1948:0458).
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
; speaker (1948:0377) and by the uninstall routine. QUIRK 1948:036D: the port 61h read is thrown
; away, the stored value is written instead.
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
; handler passes one tick in sixteen to the BIOS, points INT 8 at cs:000Ah and programs PIT channel
; 2 (mode B6h) with divisor 1000h, then remembers port 61h in gPort61Save and sets the output up
; for Tandy (_SetThreeVoices) or the PC speaker (_SetOneVoice). Called by main (07C8:0557, 055E)
; after the sound card choice.
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
        out     61h, al
        cmp     byte ptr [gSpeakerOn], 1
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

; 1948:03DA: remove the driver. Restores the saved BIOS INT 8 vector (cs:oldInt8Off/oldInt8Seg),
; programs PIT channel 2 with divisor 0 and, on Tandy, silences the three voices before
; _SetOneVoice restores port 61h. Called by the game's exit path (07C8:05B4).
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
        cmp     byte ptr [gSpeakerOn], 1
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
; sets the run, play and start-request flags so the next handler tick loads it. The pointer is
; the routine 0000:03E3 returns for the chosen driver sound (E177D.C's play path). Argument: a
; far pointer at [bp+6] offset, [bp+8] segment.
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

; 1948:0443: ask the driver to stop. Sets gStopRequest, which makes the next tick mute every voice
; and, once all are silent, clear the play flags.
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

; 1948:0458: restart the three-voice output (_SetThreeVoices). E177D.C calls it when the PC
; speaker gate bit gSpeakerOn changes, and main calls it once after the Tandy install.
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
; (_SetOneVoice). Called on the Tandy exit path (07C8:05AA) before the driver is uninstalled.
        PUBLIC  _SoundDriverPause       ; @ 1948:046B
_SoundDriverPause PROC FAR
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
_SoundDriverPause ENDP

        END
