; SNPLAY.ASM: the Tandy sound player that SN.DAT and SNSOUND.DAT carry (sound/README.md, "The game's sound").
;
; The loaders (tandy-sound.patch: LoadSoundDriver, src/E177D.C, for EOB.EXE; StartupGame, src/I0892.C, for INTRO.EXE)
; read the file's block to cs:0416 of the AdLib driver's segment, which a Tandy never runs; this code is the block's
; bytes from cs:0516, after the 128-word table of driver sounds (sndat.py puts it there). The Tandy driver's tick
; (_SoundTick, src/E1948.ASM, 291 Hz) finds the mark 'SN' at cs:051A of the segment of the song it was given, writes
; that segment at cs:0518 and calls far through cs:0516: DS is the driver's data and AX the offset of its gSongPtr
; there (the driver's variables are 4 bytes further on in INTRO.EXE than in EOB.EXE), ES this segment. PlaySound and
; PlaySoundAtCell (EOB.EXE) write the distance of the sound they start at cs:051C before they call _StartSound;
; INTRO.EXE leaves it at 0.
;
; Two slots play at once: the music and an effect. A sound whose table word has bit 15 set is music (the intro's
; three pieces); every other sound is an effect (all of EOB.EXE's). A new sound replaces the one in its slot. The
; effect takes each channel it writes, silencing it first, and holds it to its end; the music's writes to a channel
; the effect holds are kept, not made, and when the effect ends each channel it held gets the music's period (or
; noise) and volume back. With no music playing that is silence: EOB.EXE plays one sound at a time, as the old
; driver did.
;
; Every fourth call is a driver tick (72.8 Hz) of a slot, when its stream moves one frame: the format is in
; sndat.py, whose class Player is this routine in Python. An effect's start reads the distance once: from 16 the
; sound is not played (as on the AdLib, 177D:012B) and what is playing goes on; under it every volume of the sound
; is (distance - 1) * 7 / 8 steps quieter (the AdLib's level offset, src/E177D.C, at 4 OPL2 steps of 0.75 dB to one
; 2 dB step). At the end of a sound its slot gives its channels up (the music silences those the effect does not
; hold), and the end's flag nibble, when 1, sets the AdLib driver's script flag at cs:03A0 (the character generation
; music's loop). gPlaying and gRunning are 1 while either slot plays, and a stop request (StopSound) ends both.
;
; Assembled by TASM 2.0 and linked by TLINK 3.0 to an EXE whose image, from offset 0516h, is these bytes.

; the driver's variables (src/E1948.ASM), from its gSongPtr (EOB.EXE's ds:0016, INTRO.EXE's ds:001A), which DS:SI
; points at
gPlaying        equ     19Dh - 16h
gStartRequest   equ     19Eh - 16h
gStopRequest    equ     19Fh - 16h
gRunning        equ     1C6h - 16h
gScriptFlag     equ     3A0h            ; in this segment (src/E_TEXT.ASM, src/I_TEXT.ASM: the AdLib driver's)

; a slot: where its stream is and what it last set
S_POS           equ     0               ; word: the stream's byte
S_HALF          equ     2               ; 1 when its high nibble is next
S_HOLD          equ     3               ; ticks to the next frame
S_DIV           equ     4               ; calls to the next tick
S_LIVE          equ     5               ; 1 while it plays
S_STEPS         equ     6               ; volume steps the distance takes off (the music's stays 0)
S_PER           equ     7               ; three words: the tone periods
S_VOL           equ     13              ; four bytes: the volumes, as the stream has them (the music's: what is restored)
S_NOISE         equ     17              ; the noise register
S_SIZE          equ     18

        JUMPS                           ; TASM lengthens a conditional jump that does not reach
SNPLAY  SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:SNPLAY, DS:NOTHING, ES:SNPLAY
        ORG     516h

Entry           dw      offset Tick     ; 0516: the far pointer _SoundTick calls through
EntrySeg        dw      0               ; 0518: its segment, written by _SoundTick
Mark            db      'SN'            ; 051A: what _SoundTick looks for
Distance        dw      0               ; 051C: written by PlaySound (0) and PlaySoundAtCell (gCurAttenuation)
Own             db      0               ; the channels the effect holds: bit n for channel n (3 the noise)
Music           LABEL   BYTE            ; the music's slot: its volumes to restore start off
                dw      0
                db      5 dup (0)
                dw      3 dup (0)
                db      4 dup (0Fh)
                db      0
Effect          db      S_SIZE dup (0)

Tick    PROC    FAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     si, ax                  ; DS:SI the driver's gSongPtr
        cmp     byte ptr ds:[si+gStartRequest], 0
        je      tStop
        mov     byte ptr ds:[si+gStartRequest], 0
        mov     byte ptr ds:[si+gStopRequest], 0
        mov     bx, ds:[si]
        mov     di, offset Music
        or      bx, bx
        js      tMusic
        mov     ax, es:[Distance]
        cmp     ax, 16
        jge     tRun                    ; too far: not played, and what plays goes on
        dec     ax
        jns     tSteps
        xor     ax, ax
tSteps: mov     cx, ax
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        sub     ax, cx
        shr     ax, 1
        shr     ax, 1
        shr     ax, 1
        mov     di, offset Effect
        cmp     byte ptr es:[di+S_LIVE], 0
        je      tEffect
        push    ax
        call    GiveBack                ; the effect it replaces gives its channels back
        pop     ax
tEffect:
        mov     es:[di+S_STEPS], al
        jmp     tStart
tMusic: and     bh, 7Fh
        call    Hush                    ; the music it replaces is silenced
tStart: mov     es:[di+S_POS], bx
        mov     byte ptr es:[di+S_HALF], 0
        mov     byte ptr es:[di+S_HOLD], 1
        mov     byte ptr es:[di+S_DIV], 1
        mov     byte ptr es:[di+S_LIVE], 1
        jmp     tRun
tStop:  cmp     byte ptr ds:[si+gStopRequest], 0
        je      tRun
        xor     al, al
        mov     es:[Music+S_LIVE], al
        mov     es:[Effect+S_LIVE], al
        mov     es:[Own], al
        call    Hush
tRun:   push    si
        mov     di, offset Music
        call    Run
        mov     di, offset Effect
        call    Run
        pop     si
        mov     al, es:[Music+S_LIVE]
        or      al, es:[Effect+S_LIVE]
        mov     ds:[si+gPlaying], al
        mov     ds:[si+gRunning], al
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
Tick    ENDP

; one call of the slot at ES:DI: on its tick, the next frame of its stream, a pause, or its end
Run     PROC    NEAR
        cmp     byte ptr es:[di+S_LIVE], 0
        je      rOut
        dec     byte ptr es:[di+S_DIV]
        jnz     rOut
        mov     byte ptr es:[di+S_DIV], 4
        dec     byte ptr es:[di+S_HOLD]
        jnz     rOut
        mov     bx, es:[di+S_POS]
        call    Nib
        mov     dl, al                  ; DL: the tones and the noise that change
        call    Nib
        mov     dh, al                  ; DH: the volumes that change
        or      dx, dx
        jnz     rFrame
        call    Nib                     ; a pause
        or      al, al
        jnz     rPause
        call    Nib
        mov     ah, al
        call    Nib
        mov     cl, 4
        shl     al, cl
        or      al, ah
        jnz     rPause
        call    Nib                     ; the end
        or      al, al
        jz      rEnd
        mov     byte ptr es:[gScriptFlag], 1
rEnd:   mov     byte ptr es:[di+S_LIVE], 0
        cmp     di, offset Music
        jne     GiveBack
        jmp     Hush
rPause: mov     es:[di+S_HOLD], al
        jmp     rSave
rFrame: mov     byte ptr es:[di+S_HOLD], 1
        lea     si, [di+S_PER]
        xor     ch, ch                  ; CH: the channel
rTone:  shr     dl, 1
        jnc     rToneNext
        call    Nib
        or      al, al
        jnz     rDelta
        call    Nib                     ; the period itself, three nibbles
        mov     ah, al
        call    Nib
        mov     cl, 4
        shl     al, cl
        or      ah, al
        call    Nib
        xchg    al, ah
        jmp     rSet
rDelta: cmp     al, 8
        jne     rSmall
        call    Nib                     ; a byte to add
        mov     ah, al
        call    Nib
        mov     cl, 4
        shl     al, cl
        or      al, ah
        jmp     rAdd
rSmall: cmp     al, 8
        jb      rAdd
        or      al, 0F0h
rAdd:   cbw
        add     ax, es:[si]
        and     ax, 3FFh
rSet:   mov     es:[si], ax
        push    ax
        and     al, 0Fh
        mov     ah, ch
        mov     cl, 5
        shl     ah, cl
        or      al, ah
        or      al, 80h                 ; the latch with the low four bits
        call    Put
        pop     ax
        mov     cl, 4
        shr     ax, cl                  ; then the high six
        call    Put
rToneNext:
        inc     si
        inc     si
        inc     ch
        cmp     ch, 3
        jne     rTone
        shr     dl, 1                   ; the noise
        jnc     rVol
        call    Nib
        mov     es:[di+S_NOISE], al
        or      al, 0E0h
        call    Put
rVol:   xor     ch, ch
rVolL:  shr     dh, 1
        jnc     rVolN
        call    Nib
        push    bx
        mov     bl, ch
        xor     bh, bh
        mov     es:[bx+di+S_VOL], al    ; as the stream has it
        pop     bx
        add     al, es:[di+S_STEPS]
        cmp     al, 0Fh
        jbe     rVolW
        mov     al, 0Fh
rVolW:  mov     ah, ch
        mov     cl, 5
        shl     ah, cl
        or      al, ah
        or      al, 90h
        call    Put
rVolN:  inc     ch
        cmp     ch, 4
        jne     rVolL
rSave:  mov     es:[di+S_POS], bx
rOut:   ret
Run     ENDP

; AL to the chip for channel CH of the slot at ES:DI. The effect takes a channel it does not hold yet, silencing it
; first; the music's write to a channel the effect holds is not made. CL is lost
Put     PROC    NEAR
        push    ax
        mov     cl, ch
        mov     ah, 1
        shl     ah, cl                  ; AH: the channel's bit
        cmp     di, offset Music
        je      pMusic
        test    es:[Own], ah
        jnz     pOut
        or      es:[Own], ah
        mov     al, ch
        mov     cl, 5
        shl     al, cl
        or      al, 9Fh
        out     0C0h, al
pOut:   pop     ax
        out     0C0h, al
        ret
pMusic: test    es:[Own], ah
        jz      pOut
        pop     ax
        ret
Put     ENDP

; the channels the effect holds get the music's period (or noise) and volume back, and the effect holds none
GiveBack PROC   NEAR
        push    bx
        push    dx
        push    si
        push    di
        mov     si, offset Music + S_PER
        mov     di, offset Music + S_VOL
        mov     dx, 0180h               ; DL: the channel's latch, DH: its bit
gLoop:  test    es:[Own], dh
        jz      gNext
        cmp     dl, 0E0h
        je      gNoise
        mov     bx, es:[si]
        mov     al, bl
        and     al, 0Fh
        or      al, dl
        out     0C0h, al
        mov     cl, 4
        shr     bx, cl
        mov     al, bl
        out     0C0h, al
        jmp     gVol
gNoise: mov     al, es:[Music+S_NOISE]
        or      al, 0E0h
        out     0C0h, al
gVol:   mov     al, es:[di]
        or      al, dl
        or      al, 10h
        out     0C0h, al
gNext:  inc     si
        inc     si
        inc     di
        shl     dh, 1
        add     dl, 20h
        jnc     gLoop
        mov     byte ptr es:[Own], 0
        pop     di
        pop     si
        pop     dx
        pop     bx
        ret
GiveBack ENDP

; the music silent: its volumes to restore are off, and the channels the effect does not hold are silenced
Hush    PROC    NEAR
        mov     word ptr es:[Music+S_VOL], 0F0Fh
        mov     word ptr es:[Music+S_VOL+2], 0F0Fh
        mov     ax, 019Fh               ; AL: the channel's silence, AH: its bit
hLoop:  test    es:[Own], ah
        jnz     hNext
        out     0C0h, al
hNext:  shl     ah, 1
        add     al, 20h
        jnc     hLoop
        ret
Hush    ENDP

; the next nibble of the stream at ES:BX of the slot at ES:DI into AL, the low one of a byte first; CL is lost
Nib     PROC    NEAR
        mov     al, es:[bx]
        xor     byte ptr es:[di+S_HALF], 1
        jz      nHigh
        and     al, 0Fh
        ret
nHigh:  inc     bx
        mov     cl, 4
        shr     al, cl
        ret
Nib     ENDP

SNPLAY  ENDS
        END
