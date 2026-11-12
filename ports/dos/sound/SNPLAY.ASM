; SNPLAY.ASM: the Tandy sound player that SN.DAT and SNSOUND.DAT carry (sound/README.md, "The game's sound").
;
; The loaders (tandy-sound.patch: LoadSoundDriver, src/E177D.C, for EOB.EXE; StartupGame, src/I0892.C, for INTRO.EXE)
; read the file's block to cs:0416 of the AdLib driver's segment, which a Tandy never runs; this code is the block's
; bytes from cs:0516, after the 128-word table of driver sounds (sndat.py puts it there). The Tandy driver's tick
; (_SoundTick, src/E1948.ASM, 291 Hz) finds the mark 'SN' at cs:051A of the segment of the song it was given, writes
; that segment at cs:0518 and calls far through cs:0516: DS is the driver's data and AX the offset of its gSongPtr
; there (the driver's variables are 4 bytes further on in INTRO.EXE than in EOB.EXE), ES this segment, which
; Tick makes DS too once it has read the driver's requests.
;
; Four slots play at once: the music and three effects. A sound whose table word has bit 15 set is music: notes and
; instruments (sndat.py music, arrange.py), the three tones each a part with an envelope and, for a lead, a vibrato.
; Every other sound is an effect: a stream of the chip's register changes (sndat.py effect), its first byte the
; channels it uses. A new effect takes a free slot, or the one with the weakest claim if that is no stronger.
;
; The chip's four channels are shared by claim: an effect's is 20h less its distance, as the AdLib's priority
; (177D:01DA), the music's 0, and the newer of two equal claims wins. A slot that writes a channel held by a weaker
; claim takes it and writes all of it (period or noise, and volume); a weaker one keeps what it would write and writes
; nothing. When a slot ends, each channel it held goes to the slot with the strongest claim that sounds there, which
; writes all of it, or is silenced. An effect's tones go, when it starts, to the chip's tones: free ones first, then
; the music's, then those of the effect with the weakest claim (a noise clocked by tone 2 keeps its tone 2).
;
; Every fourth call is a tick (72.8 Hz); the other three return at once. PlaySound and PlaySoundAtCell write the
; distance of the sound they start at cs:051C (tandy-sound.patch); the player reads it when it takes the start up
; and sets it back to 7FFFh, none, so that of the starts it has not taken up yet the game keeps the nearest. From 11
; cells an effect is not played; nearer, each of its volumes is (distance - 1) * 3 / 2 steps quieter, the AdLib's
; 2.95 dB a cell. A sound whose first byte has bit 5 set (driver sound 0, the silence) stops every sound first. At
; the end of a sound its flag, when 1, sets the AdLib driver's script flag at cs:03A0 (the character generation
; music's loop). gPlaying and gRunning are 1 while any slot plays; a stop request (StopSound) ends them all.
;
; sndat.py's class Player is this routine in Python, call for call. Assembled by TASM 2.0 and linked by TLINK 3.0 to
; an EXE whose image, from offset 0516h, is these bytes; sndat.py writes PERIODS.INC (arrange.py's tables) beside it.

; the driver's variables (src/E1948.ASM), from its gSongPtr (EOB.EXE's ds:0016, INTRO.EXE's ds:001A), which DS:BP
; points at in Tick
gPlaying        equ     19Dh - 16h
gStartRequest   equ     19Eh - 16h
gStopRequest    equ     19Fh - 16h
gRunning        equ     1C6h - 16h
gScriptFlag     equ     3A0h            ; in this segment (src/E_TEXT.ASM, src/I_TEXT.ASM: the AdLib driver's)

NO_DISTANCE     equ     7FFFh
H_CLOCK         equ     10h             ; an effect's first byte: bits 0 to 3 its channels, 4 its noise clocked by
H_STOP          equ     20h             ; tone 2, 5 it stops every sound

; a slot
S_POS           equ     0               ; word: the stream's byte (the music's: its next event)
S_HALF          equ     2               ; 1 when the high nibble is next
S_HOLD          equ     3               ; ticks to the next frame (the music's: to its next events)
S_DIV           equ     4               ; (unused)
S_LIVE          equ     5               ; 1 while it plays
S_STEPS         equ     6               ; volume steps the distance takes off
S_PRIO          equ     7               ; its claim
S_SEQ           equ     8               ; word: when it started
S_USES          equ     10              ; its first byte
S_MAP           equ     11              ; four bytes: the chip's channel of each of its own (0FFh none)
S_PER           equ     15              ; three words: the tone periods it has set (the music's: has written)
S_VOL           equ     21              ; four bytes: the volumes as the stream has them (the music's: as written)
S_NOISE         equ     25              ; the noise register
S_SIZE          equ     26

; a music channel's note
N_LIVE          equ     0
N_PER           equ     1               ; word: its period
N_INS           equ     3               ; word: its instrument
N_GAIN          equ     5
N_DUR           equ     6               ; word: ticks held
N_AGE           equ     8               ; word
N_PHASE         equ     10              ; 0 attack, 1 held, 2 released, 3 done
N_COUNT         equ     11
N_VOL           equ     12
N_VIB           equ     13              ; the vibrato's tick, from VIB_DELAY on
N_SIZE          equ     14

VIB_DELAY       equ     8

        JUMPS                           ; TASM lengthens a conditional jump that does not reach
SNPLAY  SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:SNPLAY, DS:SNPLAY, ES:SNPLAY
        ORG     516h

Entry           dw      offset Tick     ; 0516: the far pointer _SoundTick calls through
EntrySeg        dw      0               ; 0518: its segment, written by _SoundTick
Mark            db      'SN'            ; 051A: what _SoundTick looks for
Distance        dw      NO_DISTANCE     ; 051C: written by PlaySound (0) and PlaySoundAtCell (gCurAttenuation)
Seq             dw      0               ; starts so far
TickDiv         db      1               ; calls to the next tick (72.8 Hz): a start or a stop waits for it
Own             dw      4 dup (0)       ; the slot that holds each channel of the chip, or 0
InsBase         dw      0               ; the music's instruments
Notes           db      3 * N_SIZE dup (0)
Music           LABEL   BYTE            ; the slots: the music, then the three effects, their volumes off
        REPT    4
                db      S_VOL dup (0)
                db      0Fh, 0Fh, 0Fh, 0Fh
                db      0
        ENDM
SLOTS_END       LABEL   BYTE
        INCLUDE PERIODS.INC             ; Periods (64 words), Vib (12 bytes): arrange.py's

Tick    PROC    FAR
        dec     byte ptr es:[TickDiv]
        jz      tWork
        ret                             ; three calls in four: nothing to do
tWork:  mov     byte ptr es:[TickDiv], 4
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        mov     bp, ax                  ; DS:BP the driver's gSongPtr
        xor     bx, bx                  ; BX: the song of a start, or 0
        mov     cl, ds:[bp+gStopRequest]
        cmp     byte ptr ds:[bp+gStartRequest], 0
        je      tOurs
        mov     byte ptr ds:[bp+gStartRequest], 0
        mov     byte ptr ds:[bp+gStopRequest], 0
        mov     bx, ds:[bp]
tOurs:  push    ds                      ; from here DS is this segment
        push    es
        pop     ds
        push    bp
        or      bx, bx
        jz      tStop
        mov     dx, NO_DISTANCE
        xchg    dx, [Distance]
        js      tMusic
        call    StartEffect
        jmp     tRun
tMusic: and     bh, 7Fh
        call    StartMusic
        jmp     tRun
tStop:  or      cl, cl
        jz      tRun
        call    StopAll
tRun:   call    RunMusic
        mov     di, offset Music + S_SIZE
tRunE:  call    RunEffect
        add     di, S_SIZE
        cmp     di, offset SLOTS_END
        jb      tRunE
        xor     al, al
        mov     di, offset Music
tLive:  or      al, [di+S_LIVE]
        add     di, S_SIZE
        cmp     di, offset SLOTS_END
        jb      tLive
        pop     bp
        pop     ds
        mov     ds:[bp+gPlaying], al
        mov     ds:[bp+gRunning], al
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
Tick    ENDP

; ---- starts and ends

; every slot ends, every note, and the chip is silenced
StopAll PROC    NEAR
        push    di
        mov     di, offset Music
saSlot: mov     byte ptr [di+S_LIVE], 0
        add     di, S_SIZE
        cmp     di, offset SLOTS_END
        jb      saSlot
        mov     byte ptr [Notes+N_LIVE], 0
        mov     byte ptr [Notes+N_SIZE+N_LIVE], 0
        mov     byte ptr [Notes+2*N_SIZE+N_LIVE], 0
        mov     word ptr [Music+S_VOL], 0F0Fh
        mov     word ptr [Music+S_VOL+2], 0F0Fh
        xor     ax, ax
        mov     [Own], ax
        mov     [Own+2], ax
        mov     [Own+4], ax
        mov     [Own+6], ax
        mov     al, 9Fh
saOut:  out     0C0h, al
        add     al, 20h
        jnc     saOut
        pop     di
        ret
StopAll ENDP

; the effect at BX (its first byte), from DX away
StartEffect PROC NEAR
        xor     cx, cx                  ; CL: the steps, CH: the distance for the claim
        cmp     dx, NO_DISTANCE
        je      seNear
        mov     ch, dl
        cmp     dx, 1
        jle     seNear
        mov     ax, dx
        dec     ax
        mov     cl, al
        shl     ax, 1
        add     al, cl
        shr     al, 1                   ; (distance - 1) * 3 / 2
        cmp     ax, 15
        jae     seOut                   ; silent: not played
        mov     cl, al
seNear: mov     al, [bx]
        test    al, H_STOP
        jz      seSlot
        call    StopAll
seSlot: mov     ah, 20h
        sub     ah, ch                  ; AH: the claim
        mov     di, offset Music + S_SIZE
seFree: cmp     byte ptr [di+S_LIVE], 0
        je      seTake
        add     di, S_SIZE
        cmp     di, offset SLOTS_END
        jb      seFree
        mov     di, offset Music + S_SIZE   ; all three play: the one with the weakest claim, if no stronger than this
        mov     si, di
seLow:  add     si, S_SIZE
        cmp     si, offset SLOTS_END
        jae     seLowed
        mov     dl, [si+S_PRIO]
        cmp     dl, [di+S_PRIO]
        ja      seLow
        jb      seLower
        mov     dx, [si+S_SEQ]
        cmp     dx, [di+S_SEQ]
        jae     seLow
seLower:
        mov     di, si
        jmp     seLow
seLowed:
        cmp     [di+S_PRIO], ah
        ja      seOut
        mov     byte ptr [di+S_LIVE], 0
        push    ax
        push    bx
        push    cx
        call    Release
        pop     cx
        pop     bx
        pop     ax
seTake: inc     word ptr [Seq]
        mov     dx, [Seq]
        mov     [di+S_SEQ], dx
        mov     byte ptr [di+S_LIVE], 1
        mov     [di+S_PRIO], ah
        mov     [di+S_STEPS], cl
        mov     dl, [bx]             ; DL: its first byte
        mov     [di+S_USES], dl
        inc     bx
        mov     [di+S_POS], bx
        mov     byte ptr [di+S_HALF], 0
        mov     byte ptr [di+S_HOLD], 1
        xor     ax, ax
        mov     [di+S_PER], ax
        mov     [di+S_PER+2], ax
        mov     [di+S_PER+4], ax
        mov     word ptr [di+S_VOL], 0F0Fh
        mov     word ptr [di+S_VOL+2], 0F0Fh
        mov     [di+S_NOISE], al
        mov     word ptr [di+S_MAP], 0FFFFh
        mov     word ptr [di+S_MAP+2], 03FFh
        xor     bl, bl                  ; BL: the chip's tones it has
        mov     al, dl
        and     al, H_CLOCK + 4
        cmp     al, H_CLOCK + 4
        jne     seTones
        mov     byte ptr [di+S_MAP+2], 2
        mov     bl, 4
seTones:
        mov     bp, di                  ; BP: the slot, for es:[bp+si]
        xor     si, si                  ; SI: its tone
seTone: mov     cx, si
        mov     al, 1
        shl     al, cl
        test    dl, al
        jz      seNext
        cmp     byte ptr es:[bp+si+S_MAP], 0FFh
        jne     seNext
        call    BestTone                ; AL: the chip's tone
        mov     es:[bp+si+S_MAP], al
        mov     cl, al
        mov     al, 1
        shl     al, cl
        or      bl, al
seNext: inc     si
        cmp     si, 3
        jb      seTone
seOut:  ret
StartEffect ENDP

; AL: the chip's tone, of those not in BL, that an effect's tone goes to: a free one, then the music's, then that of
; the effect with the weakest claim; the lowest of equals
BestTone PROC NEAR
        push    cx
        push    dx
        push    si
        mov     dx, 0FFFFh              ; DX: the best cost so far
        xor     cl, cl
        mov     ch, 0FFh                ; CH: its tone
btTone: mov     al, 1
        shl     al, cl
        test    bl, al
        jnz     btNext
        mov     si, cx
        and     si, 3
        shl     si, 1
        mov     si, [si+Own]
        xor     ax, ax                  ; free: 0
        or      si, si
        jz      btCost
        inc     ax                      ; the music's: 1
        cmp     si, offset Music
        je      btCost
        mov     al, [si+S_PRIO]
        add     ax, 2                   ; an effect's: 2 and its claim
btCost: cmp     ax, dx
        jae     btNext
        mov     dx, ax
        mov     ch, cl
btNext: inc     cl
        cmp     cl, 3
        jb      btTone
        mov     al, ch
        pop     si
        pop     dx
        pop     cx
        ret
BestTone ENDP

; the music at BX (its instruments' count)
StartMusic PROC NEAR
        mov     di, offset Music
        cmp     byte ptr [di+S_LIVE], 0
        je      smNew
        mov     byte ptr [di+S_LIVE], 0
        push    bx
        call    Release
        pop     bx
smNew:  mov     byte ptr [Notes+N_LIVE], 0
        mov     byte ptr [Notes+N_SIZE+N_LIVE], 0
        mov     byte ptr [Notes+2*N_SIZE+N_LIVE], 0
        mov     cl, [bx]
        inc     bx
        mov     [InsBase], bx
smIns:  or      cl, cl
        jz      smGo
        mov     al, [bx]
        xor     ah, ah
        add     bx, ax
        add     bx, 5
        dec     cl
        jmp     smIns
smGo:   inc     word ptr [Seq]
        mov     ax, [Seq]
        mov     [di+S_SEQ], ax
        mov     byte ptr [di+S_LIVE], 1
        xor     ax, ax
        mov     [di+S_PRIO], al
        mov     [di+S_STEPS], al
        mov     [di+S_POS], bx
        mov     [di+S_HOLD], al
        mov     [di+S_PER], ax
        mov     [di+S_PER+2], ax
        mov     [di+S_PER+4], ax
        mov     word ptr [di+S_VOL], 0F0Fh
        mov     word ptr [di+S_VOL+2], 0F0Fh
        ret
StartMusic ENDP

; the channels the slot at DI held go to the slot with the strongest claim that sounds there, or are silenced. DI is
; kept
Release PROC    NEAR
        push    di
        xor     cx, cx                  ; CL: the chip's channel
rlChan: mov     si, cx
        shl     si, 1
        cmp     [si+Own], di
        jne     rlNext
        push    si
        xor     dx, dx                  ; DX: the best slot
        mov     bx, offset Music
rlSlot: cmp     bx, di
        je      rlSkip
        cmp     byte ptr [bx+S_LIVE], 0
        je      rlSkip
        call    Logical                 ; SI: its channel there, or carry
        jc      rlSkip
        mov     al, [bx+si+S_VOL]
        add     al, [bx+S_STEPS]
        cmp     al, 15
        jae     rlSkip
        or      dx, dx
        jz      rlBest
        mov     si, dx
        mov     al, [bx+S_PRIO]
        cmp     al, [si+S_PRIO]
        jb      rlSkip
        ja      rlBest
        mov     ax, [bx+S_SEQ]
        cmp     ax, [si+S_SEQ]
        jbe     rlSkip
rlBest: mov     dx, bx
rlSkip: add     bx, S_SIZE
        cmp     bx, offset SLOTS_END
        jb      rlSlot
        pop     si
        mov     [si+Own], dx
        or      dx, dx
        jnz     rlGive
        call    Latch
        mov     al, ch
        or      al, 9Fh
        out     0C0h, al
        jmp     rlNext
rlGive: mov     bx, dx
        call    Restore
rlNext: xor     ch, ch                  ; (Latch set it)
        inc     cl
        cmp     cl, 4
        jb      rlChan
        pop     di
        ret
Release ENDP

; CH: the latch bits of the chip's channel CL (CL << 5). AL is lost
Latch   PROC    NEAR
        mov     al, cl
        push    cx
        mov     cl, 5
        shl     al, cl
        pop     cx
        mov     ch, al
        ret
Latch   ENDP

; SI: the slot at BX's own channel on the chip's channel CL, or carry when it has none there. AX is lost
Logical PROC    NEAR
        xor     si, si
        cmp     bx, offset Music
        jne     lgNext
        mov     al, cl
        xor     ah, ah
        mov     si, ax
        clc
        ret
lgNext: cmp     [bx+si+S_MAP], cl
        jne     lgNo
        push    cx
        mov     cx, si
        mov     al, 1
        shl     al, cl
        pop     cx
        test    [bx+S_USES], al
        jz      lgNo
        clc
        ret
lgNo:   inc     si
        cmp     si, 4
        jb      lgNext
        stc
        ret
Logical ENDP

; the slot at BX writes all of the chip's channel CL: its period (or noise), then its volume. CX and SI are kept
Restore PROC    NEAR
        push    cx
        push    si
        call    Logical
        call    Latch
        cmp     cl, 3
        je      rsNoise
        push    si
        shl     si, 1
        mov     ax, [bx+si+S_PER]
        pop     si
        call    OutTone
        jmp     rsVol
rsNoise:
        mov     al, [bx+S_NOISE]
        or      al, 0E0h
        out     0C0h, al
rsVol:  call    OutVol
        pop     si
        pop     cx
        ret
Restore ENDP

; the period AX to the chip's tone whose latch bits are CH: the latch and low four bits, then the high six
OutTone PROC    NEAR
        push    ax
        and     al, 0Fh
        or      al, ch
        or      al, 80h
        out     0C0h, al
        pop     ax
        push    cx
        mov     cl, 4
        shr     ax, cl
        pop     cx
        and     al, 3Fh
        out     0C0h, al
        ret
OutTone ENDP

; the volume of the slot at BX's channel SI, with its steps, to the chip's channel whose latch bits are CH
OutVol  PROC    NEAR
        mov     al, [bx+si+S_VOL]
        add     al, [bx+S_STEPS]
        cmp     al, 15
        jbe     ovOut
        mov     al, 15
ovOut:  or      al, ch
        or      al, 90h
        out     0C0h, al
        ret
OutVol  ENDP

; may the slot at BX write the chip's channel CL now? Carry clear: yes, and CH is the latch bits. Taking it from a
; weaker claim writes all of it, and carry is set: nothing more to write. CL and SI are kept
Owns    PROC    NEAR
        push    si
        mov     si, cx
        and     si, 3
        shl     si, 1
        mov     ax, [si+Own]
        cmp     ax, bx
        je      owYes
        or      ax, ax
        jz      owTake
        push    si
        mov     si, ax
        mov     al, [bx+S_PRIO]
        cmp     al, [si+S_PRIO]
        jb      owNo
        ja      owTake1
        mov     ax, [bx+S_SEQ]
        cmp     ax, [si+S_SEQ]
        jb      owNo
owTake1:
        pop     si
owTake: mov     [si+Own], bx
        pop     si
        call    Restore
        stc
        ret
owNo:   pop     si
        pop     si
        stc
        ret
owYes:  pop     si
        call    Latch
        clc
        ret
Owns    ENDP

; the slot at BX's tone SI: its period to the chip, if it may. CX is kept
Tone    PROC    NEAR
        push    cx
        mov     cl, [bx+si+S_MAP]
        cmp     bx, offset Music
        jne     toMap
        mov     cx, si
toMap:  call    Owns
        jc      toOut
        push    si
        shl     si, 1
        mov     ax, [bx+si+S_PER]
        pop     si
        call    OutTone
toOut:  pop     cx
        ret
Tone    ENDP

; the slot at BX's volume SI: to the chip, if it may. CX is kept
Volume  PROC    NEAR
        push    cx
        mov     cl, [bx+si+S_MAP]
        cmp     bx, offset Music
        jne     voMap
        mov     cx, si
voMap:  call    Owns
        jc      voOut
        call    OutVol
voOut:  pop     cx
        ret
Volume  ENDP

; ---- the effects' streams

; one call of the effect slot at DI
RunEffect PROC  NEAR
        cmp     byte ptr [di+S_LIVE], 0
        je      reOut
        dec     byte ptr [di+S_HOLD]
        jnz     reOut
        mov     bx, di
        mov     bp, [di+S_POS]       ; BP: the stream's byte
        call    Nib
        mov     dl, al                  ; DL: the tones and the noise that change
        call    Nib
        mov     dh, al                  ; DH: the volumes that change
        or      dx, dx
        jnz     reFrame
        call    Nib                     ; a pause
        or      al, al
        jnz     rePause
        call    Nib
        mov     ah, al
        call    Nib
        mov     cl, 4
        shl     al, cl
        or      al, ah
        jnz     rePause
        call    Nib                     ; the end
        or      al, al
        jz      reEnd
        mov     byte ptr ds:[gScriptFlag], 1
reEnd:  mov     byte ptr [di+S_LIVE], 0
        call    Release
reOut:  ret
rePause:
        mov     [di+S_HOLD], al
        jmp     reSave
reFrame:
        mov     byte ptr [di+S_HOLD], 1
        xor     si, si                  ; SI: its tone
reTone: shr     dl, 1
        jnc     reToneNext
        call    Nib
        or      al, al
        jnz     reDelta
        call    Nib                     ; the period itself, three nibbles
        mov     ah, al
        call    Nib
        mov     cl, 4
        shl     al, cl
        or      ah, al
        call    Nib
        xchg    al, ah
        jmp     reSet
reDelta:
        cmp     al, 8
        jne     reSmall
        call    Nib                     ; a byte to add
        mov     ah, al
        call    Nib
        mov     cl, 4
        shl     al, cl
        or      al, ah
        jmp     reAdd
reSmall:
        cmp     al, 8
        jb      reAdd
        or      al, 0F0h
reAdd:  cbw
        push    si
        shl     si, 1
        add     ax, [bx+si+S_PER]
        pop     si
        and     ax, 3FFh
reSet:  push    si
        shl     si, 1
        mov     [bx+si+S_PER], ax
        pop     si
        call    Tone
reToneNext:
        inc     si
        cmp     si, 3
        jb      reTone
        shr     dl, 1                   ; the noise
        jnc     reVol
        call    Nib
        mov     [bx+S_NOISE], al
        mov     cl, 3
        call    Owns
        jc      reVol
        mov     al, [bx+S_NOISE]
        or      al, 0E0h
        out     0C0h, al
reVol:  xor     si, si
reVolL: shr     dh, 1
        jnc     reVolN
        call    Nib
        mov     [bx+si+S_VOL], al
        call    Volume
reVolN: inc     si
        cmp     si, 4
        jb      reVolL
reSave: mov     [di+S_POS], bp
        ret
RunEffect ENDP

; the next nibble of the stream at ES:BP of the slot at DI into AL, the low one of a byte first; CL is lost
Nib     PROC    NEAR
        mov     al, es:[bp]
        xor     byte ptr [di+S_HALF], 1
        jz      nHigh
        and     al, 0Fh
        ret
nHigh:  inc     bp
        mov     cl, 4
        shr     al, cl
        ret
Nib     ENDP

; ---- the music's notes

; one call of the music
RunMusic PROC   NEAR
        mov     bx, offset Music
        cmp     byte ptr [bx+S_LIVE], 0
        je      rmOut
        mov     bp, [bx+S_POS]
rmEvent:
        cmp     byte ptr [bx+S_HOLD], 0
        jne     rmTick
        mov     al, es:[bp]
        or      al, al
        jnz     rmWait
        cmp     byte ptr es:[bp+1], 0   ; the end
        je      rmEnd
        mov     byte ptr ds:[gScriptFlag], 1
rmEnd:  mov     byte ptr [bx+S_LIVE], 0
        mov     byte ptr [Notes+N_LIVE], 0
        mov     byte ptr [Notes+N_SIZE+N_LIVE], 0
        mov     byte ptr [Notes+2*N_SIZE+N_LIVE], 0
        mov     word ptr [bx+S_VOL], 0F0Fh
        mov     word ptr [bx+S_VOL+2], 0F0Fh
        mov     di, bx
        call    Release
rmOut:  ret
rmWait: test    al, 80h
        jnz     rmNote
        mov     [bx+S_HOLD], al
        inc     bp
        jmp     rmEvent
rmNote: and     ax, 3
        mov     si, ax                  ; SI: the channel
        mov     cl, N_SIZE
        mul     cl
        add     ax, offset Notes
        mov     di, ax                  ; DI: its note
        mov     byte ptr [di+N_LIVE], 1
        mov     al, es:[bp+1]
        xor     ah, ah
        push    si
        mov     si, ax
        shl     si, 1
        mov     ax, [si+Periods]
        pop     si
        mov     [di+N_PER], ax
        mov     al, es:[bp+2]
        mov     ah, al
        and     ah, 0Fh
        mov     [di+N_GAIN], ah
        mov     cl, 4
        shr     al, cl
        mov     cl, al                  ; its instrument's record
        push    bx
        mov     bx, [InsBase]
rmIns:  or      cl, cl
        jz      rmInsd
        mov     al, [bx]
        xor     ah, ah
        add     bx, ax
        add     bx, 5
        dec     cl
        jmp     rmIns
rmInsd: mov     [di+N_INS], bx
        pop     bx
        mov     al, es:[bp+3]
        xor     ah, ah
        add     bp, 4
        test    al, 80h
        jz      rmDur
        and     al, 7Fh
        mov     ah, al
        mov     al, es:[bp]
        inc     bp
rmDur:  mov     [di+N_DUR], ax
        xor     ax, ax
        mov     [di+N_AGE], ax
        mov     [di+N_PHASE], al
        mov     [di+N_COUNT], al
        mov     [di+N_VIB], al
        mov     byte ptr [di+N_VOL], 15
        mov     byte ptr [bx+si+S_VOL], 0FFh   ; written on its first tick
        push    si
        shl     si, 1
        mov     word ptr [bx+si+S_PER], 0FFFFh
        pop     si
        jmp     rmEvent
rmTick: dec     byte ptr [bx+S_HOLD]
        mov     [bx+S_POS], bp
        xor     si, si                  ; SI: the channel
        mov     di, offset Notes
rmChan: cmp     byte ptr [di+N_LIVE], 0
        je      rmNext
        call    NoteTick                ; AX the period, DL the volume
        push    si
        shl     si, 1
        cmp     ax, [bx+si+S_PER]
        je      rmSameP
        mov     [bx+si+S_PER], ax
        pop     si
        push    dx
        call    Tone
        pop     dx
        jmp     rmVol
rmSameP:
        pop     si
rmVol:  cmp     dl, [bx+si+S_VOL]
        je      rmDone
        mov     [bx+si+S_VOL], dl
        call    Volume
rmDone: cmp     byte ptr [di+N_PHASE], 3
        jne     rmNext
        mov     byte ptr [di+N_LIVE], 0
rmNext: add     di, N_SIZE
        inc     si
        cmp     si, 3
        jb      rmChan
        ret
RunMusic ENDP

; the note at DI, one tick: AX its period, DL its volume. BX, SI and DI are kept
NoteTick PROC   NEAR
        push    bx
        push    si
        mov     bx, [di+N_INS]       ; BX: the instrument: n, n volumes, fall, floor, release, vib
        mov     al, [bx]
        xor     ah, ah
        mov     si, ax                  ; SI: n
        cmp     byte ptr [di+N_PHASE], 2
        jae     ntPhase
        mov     ax, [di+N_AGE]
        cmp     ax, [di+N_DUR]
        jb      ntPhase
        mov     byte ptr [di+N_PHASE], 2
        mov     byte ptr [di+N_COUNT], 0
ntPhase:
        mov     al, [di+N_PHASE]
        or      al, al
        jnz     ntHeld
        mov     ax, [di+N_AGE]       ; the attack
        push    bx
        add     bx, ax
        mov     al, [bx+1]
        pop     bx
        mov     [di+N_VOL], al
        mov     ax, [di+N_AGE]
        inc     ax
        cmp     ax, si
        jne     ntVib
        mov     byte ptr [di+N_PHASE], 1
        mov     byte ptr [di+N_COUNT], 0
        jmp     ntVib
ntHeld: cmp     al, 1
        jne     ntRel
        mov     al, [bx+si+1]        ; fall
        or      al, al
        jz      ntVib
        inc     byte ptr [di+N_COUNT]
        cmp     [di+N_COUNT], al
        jne     ntVib
        mov     byte ptr [di+N_COUNT], 0
        mov     al, [di+N_VOL]
        cmp     al, [bx+si+2]        ; floor
        jae     ntVib
        inc     byte ptr [di+N_VOL]
        jmp     ntVib
ntRel:  inc     byte ptr [di+N_COUNT]
        mov     al, [bx+si+3]        ; release
        cmp     [di+N_COUNT], al
        jne     ntVib
        mov     byte ptr [di+N_COUNT], 0
        inc     byte ptr [di+N_VOL]
        cmp     byte ptr [di+N_VOL], 15
        jb      ntVib
        mov     byte ptr [di+N_VOL], 15
        mov     byte ptr [di+N_PHASE], 3
ntVib:  mov     cx, [di+N_PER]          ; CX: the period
        cmp     byte ptr [bx+si+4], 0   ; vib
        je      ntDone
        cmp     word ptr [di+N_AGE], VIB_DELAY
        jb      ntDone
        mov     al, [di+N_VIB]
        inc     ax
        cmp     al, 12
        jb      ntVibN
        xor     al, al
ntVibN: xchg    al, [di+N_VIB]
        xor     ah, ah
        mov     si, ax
        mov     dx, cx
        push    cx
        mov     cl, 6
        shr     dx, cl                  ; DX: the depth, period >> 6
        pop     cx
        mov     al, [si+Vib]
        or      al, al
        jz      ntDone
        js      ntDown
        add     cx, dx
        cmp     cx, 3FFh
        jbe     ntDone
        mov     cx, 3FFh
        jmp     ntDone
ntDown: sub     cx, dx
ntDone: inc     word ptr [di+N_AGE]
        mov     ax, cx                  ; the period
        mov     dl, [di+N_VOL]
        add     dl, [di+N_GAIN]
        cmp     dl, 15
        jbe     ntOut
        mov     dl, 15
ntOut:  pop     si
        pop     bx
        ret
NoteTick ENDP

SNPLAY  ENDS
        END
