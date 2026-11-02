; Unit at EOB.EXE 0000:020C (19996 bytes, 34 functions), the _TEXT segment of the EOB.EXE
; load module: Westwood's AdLib / PC speaker sound driver, linked between the start-up
; (e_c0) and the Borland runtime library (e_lib). 020ch to 03a1h hold the driver's
; frequency and note tables. 0416h holds the 32c8h driver block that LoadSoundDriver reads
; from adlib.dat or pcgamesn.dat, so those bytes are the block's run-time image, not source
; data; the code runs from 03aah and again from 3f77h, and the 4f94h command jump table is
; the byte-code dispatch of the voice interpreter (_UpdateVoices). The source uses the names
; src/E177D.C and src/E07C8.C already give the shared functions (f0_03AA, f0_03B0, f0_03CA,
; f0_03E3, f0_03FA, f0_400C, f0_3F77, f0_3FCA) and names the rest from the code. The
; 0416h driver block, the 3e66h/3ee5h INT 08h handlers and the 4f94h table sit inside
; f0_03FA's range, which is a stub; see its comment and re/symbols.toml.

        .MODEL  LARGE
        .CODE

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

; 0000:03FA: adds the 0416 base to the driver block 300-word table; keeps the loaded 32C8h block and the two ISRs after it
        PUBLIC  _f0_03FA              ; @ 0000:03FA stub
_f0_03FA PROC NEAR
        ret
_f0_03FA ENDP

; 0000:3F77: the AdLib driver on: installs the INT 08h handler at 3E66, resets the AdLib voices (413B), programs the PIT and clears the ring
        PUBLIC  _f0_3F77              ; @ 0000:3F77
_f0_3F77 PROC NEAR
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
_f0_3F77 ENDP

; 0000:3FCA: the AdLib driver off: restores the saved INT 08h vector, resets the AdLib voices and stops the PIT (E07C8.C:65)
        PUBLIC  _f0_3FCA              ; @ 0000:3FCA
_f0_3FCA PROC NEAR
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
_f0_3FCA ENDP

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
        mov       byte ptr [di + 6], ah
        jmp short    L_41FC
        dec       byte ptr [di + 6]
        jne    L_42D4
        inc       si
        jmp    L_41FC
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
        mov       byte ptr [di + 35h], ah
        jmp    L_41FC
L_42D4:
        mov       cl, ah
        lodsb
        xchg      ah, al
        mov       al, cl
        mov       si, ax
        jmp    L_41FC
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
        dec       si
        dec       byte ptr [di + 9]
        mov       bl, byte ptr [di + 9]
        mov       bh, 0
        shl       bl, 1
        mov       ax, word ptr [bx + di + 0ah]
        mov       si, ax
        jmp    L_41FC
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
        lodsb
        call    _AdlibWrite
        jmp    L_41FC
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
        mov       byte ptr [di + 12h], ah
        jmp    L_41FC
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
        push      si
        mov       cl, byte ptr ds:[3b4bh]
        sub       bx, bx
        mov       bl, ah
        shl       bx, 1
        mov       si, word ptr [bx + 542h]
        call    _LoadVoicePatch
        pop       si
        jmp    L_41FC
        mov       byte ptr [di + 13h], ah
        lodsb
        xchg      ah, al
        lodsb
        mov       word ptr [di + 15h], ax
        mov       word ptr [di + 29h], 4f00h
        mov       byte ptr [di + 14h], 0ffh
        jmp    L_41FC
        dec       si
        mov       word ptr [di + 29h], 4e7dh
        mov       word ptr [di + 15h], 0
        jmp    L_41FC
        mov       byte ptr [di + 24h], ah
        jmp    L_41FC
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
        mov       byte ptr [di + 8], ah
        jmp    L_41FC
        shr       ah, 1
        mov       byte ptr ds:[3b5bh], ah
        mov       byte ptr ds:[3b5ch], ah
        mov       byte ptr ds:[3b5dh], 0ffh
        mov       ah, 0
        mov       byte ptr ds:[3b5eh], ah
        mov       byte ptr ds:[3b5fh], ah
        jmp    L_41FC
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
        mov       byte ptr [di + 20h], ah
        call    _WriteVoiceReg
        jmp    L_41FC
        call    _StartVoiceNote
        mov       word ptr [di + 3], si
        mov       al, byte ptr ds:[3b3bh]
        or        al, al
        je    L_4477
        jmp    L_425F
L_4477:
        jmp    L_41FC
        call    _StartVoiceNote
        call    _VoiceKeyOn
        mov       word ptr [di + 3], si
        mov       al, byte ptr ds:[3b3bh]
        or        al, al
        je    L_448D
        jmp    L_425F
L_448D:
        jmp    L_41FC
        and       ah, 7
        mov       byte ptr [di + 2dh], ah
        jmp    L_41FC
        mov       byte ptr ds:[3b41h], ah
        jmp    L_41FC
        dec       si
        mov       word ptr [di + 2bh], 4e7dh
        jmp    L_41FC
        mov       byte ptr [di + 25h], ah
        jmp    L_41FC
        mov       byte ptr [di + 30h], ah
        jmp    L_41FC
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
        add       ah, byte ptr [di + 20h]
        mov       byte ptr [di + 20h], ah
        call    _WriteVoiceReg
        jmp    L_41FC
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
        dec       si
        mov       word ptr [di + 29h], 4e7dh
        jmp    L_41FC
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
        dec       si
        mov       al, byte ptr ds:[3b41h]
        mov       byte ptr [di + 25h], al
        jmp    L_41FC
        dec       si
        jmp    L_41FC
        mov       byte ptr [di + 36h], ah
        jmp    L_41FC
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
        dec       si
        mov       byte ptr ds:[3b42h], 0
        jmp    L_41FC
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
        dec       si
        mov       al, byte ptr ds:[3b4ah]
        mov       byte ptr ds:[3b40h], 0
        and       al, 0c0h
        mov       ah, 0bdh
        call    _AdlibWrite
        jmp    L_41FC
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
        mov       byte ptr ds:[3a0h], ah
        jmp    L_41FC
        mov       byte ptr [di + 40h], ah
        jmp    L_41FC
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

; 0000:4C8D: advances the tempo counters at 3B5D/3B5C and the step byte 3B5E; its tail at 4CAC returns cs:[3B78]
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
        mov       ax, word ptr cs:[3b78h]
        retf
_TickTempo ENDP

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

        END
