; Unit at EOB.EXE 1AAD:000C (3405 bytes, 19 functions), kind asm [H] by the map. The game's own
; keyboard driver: it installs an INT 9 handler (1AAD:0718, which also keeps the handler body at
; 1AAD:07C0), translates scancodes to key codes, buffers key and mouse-button events in a 256-byte
; ring at 1AAD:007C and hands them to the game through the far entry points 1AAD:038E/0409/053A/0588
; and 1AAD:05D3. 1AAD:0B0E folds INT 33h mouse buttons into the same event buffer. A second path
; (1AAD:0C17, 1AAD:0D26) records and replays the input stream through the file handle at ds:80ED.
; The code segment also holds the unit's variables and tables (1AAD:000C..0222, 1AAD:02DB..038D).
; Whitepaper 2.7 (Keyboard). Placeholder names mark state whose exact role is inferred from this
; code only.

        .MODEL  LARGE

        EXTRN   _FileRead:FAR           ; @ 1C0E:0400 (reads handle, far buffer, unsigned long length)
        EXTRN   _f1C0E_0578:FAR         ; @ 1C0E:0578 (the write twin of FileRead: same arguments)

        .DATA
        EXTRN   gSoundOn:BYTE           ; @ ds:80E6: sound on unless argv[2] was '4' (E07C8.C)
        EXTRN   g80E8:WORD              ; @ ds:80E8: toggled by Ctrl-key handling, role unknown
        EXTRN   g80EA:WORD              ; @ ds:80EA: start-up flag copied into the unit's 1AAD:0180
        EXTRN   g80EC:BYTE              ; @ ds:80EC: game state, 0/1/2, read all over this unit
        EXTRN   g80ED:WORD              ; @ ds:80ED: file handle for the recorded input stream
        EXTRN   g80EF:WORD              ; @ ds:80EF: key of the recorded event last decoded
        EXTRN   g80F1:WORD              ; @ ds:80F1: count that goes with that recorded event
        EXTRN   g80F3:WORD              ; @ ds:80F3: x of the recorded mouse event
        EXTRN   g80F5:WORD              ; @ ds:80F5: y of the recorded mouse event
        EXTRN   g80F7:WORD              ; @ ds:80F7: 1 when reading the recorded stream failed
        EXTRN   g8136:WORD              ; @ ds:8136: 1 while a mouse button is held (E1A72.ASM)
        EXTRN   _gCursorX:WORD          ; @ ds:8138: mouse x in pixels (E1A72.ASM)
        EXTRN   _gCursorY:WORD          ; @ ds:813A: mouse y in pixels (E1A72.ASM)
        EXTRN   g813C:WORD              ; @ ds:813C: x of the event just handled (E1A72.ASM)
        EXTRN   g813E:WORD              ; @ ds:813E: y of the event just handled (E1A72.ASM)
        EXTRN   g8140:WORD              ; @ ds:8140: x of the last mouse click (E14EF.C)
        EXTRN   g8142:WORD              ; @ ds:8142: y of the last mouse click (E14EF.C)
        EXTRN   g8488:WORD              ; @ ds:8488: counter that paces the recorded input replay
        .CODE

; The unit's data and hand-written tables, in the code segment (the map took 1AAD:000C..0222 for
; one function, but it is data). The names below are the ones the code below uses.
kKeyBitMasks    db 01h,02h,04h,08h,10h,20h,40h,80h   ; 1AAD:000C: 8 bytes, bit for a key index low 3 bits
        db      20h,02h,20h,03h,0Ch,06h,0Dh,07h      ; 1AAD:0014
        db      6Ah,06h,69h,06h,30h,02h,30h,03h
        db      7Dh,00h,7Dh,01h,5Ah,02h,5Ah,03h
        db      00h,02h,08h,00h,08h,00h,10h,00h
        db      10h,00h,10h,00h,10h,00h,20h,00h
        db      20h,00h,40h,00h,40h,00h,80h,00h
        db      80h,00h,00h,01h,00h,00h,00h,00h
kExtendKeys     db 38h,1Dh,52h,53h,4Bh,47h,4Fh,48h,50h,49h,51h,4Dh   ; 1AAD:004C: 17 bytes, extended-key codes for 1AAD:0718
        db      35h,1Ch,37h,46h,3Eh
        db      40h,4Bh,4Ch,4Fh,50h,51h,53h,54h      ; 1AAD:005D
        db      55h,56h,59h,5Fh,6Ch,7Ch,00h,3Ch
        db      3Ah,63h,68h,5Ch,5Bh,5Dh,60h,62h
        db      65h,67h,66h,37h,2Bh,7Ch,00h
kKeyBuf         db 256 dup(0)   ; 1AAD:007C: 256-byte event ring (word events, or 3 words
kKeyBufRead    dw      0000h           ; 1AAD:017C: ring read index, 0 .. 0FFh
kKeyBufWrite   dw      0000h           ; 1AAD:017E: ring write index, 0 .. 0FFh
kModifierState dw      0110h           ; 1AAD:0180: copied from ds:80EA at handler install
kShiftFlags    dw      0000h           ; 1AAD:0182: shift/control/alt state, built by 1AAD:0718
kShiftFlagsCopy dw      0000h          ; 1AAD:0184: kShiftFlags as of the last consumed event
kScanToKey      db 7Fh,6Eh,02h,03h,04h,05h,06h,07h,08h,09h,0Ah,0Bh   ; 1AAD:0186: scancode -> key code (1AAD:0718)
        db      0Ch,0Dh,0Fh,10h,11h,12h,13h,14h,15h,16h,17h,18h
        db      19h,1Ah,1Bh,1Ch,2Bh,3Ah,1Fh,20h,21h,22h,23h,24h
        db      25h,26h,27h,28h,29h,01h,2Ch,1Dh,2Eh,2Fh,30h,31h
        db      32h,33h,34h,35h,36h,37h,39h,64h,3Ch,3Dh,1Eh,70h
        db      71h,72h,73h,74h,75h,76h,77h,78h,79h,5Ah,7Dh,5Bh
        db      60h,65h,69h,5Ch,61h,66h,6Ah,5Dh,62h,67h,63h,68h
        db      7Fh,7Fh,7Fh,7Ah,7Bh
kKeyBitTable1   db 00h,00h,0FEh,87h,0FFh,0C0h,1Fh,00h   ; 1AAD:01DF: 8 bytes of key bits tested with bit 0
        db      00h,00h,00h,00h,00h,00h,00h,00h      ; 1AAD:01E7
kKeyBitTable2   db 00h,00h,00h,00h,00h,00h,00h,00h   ; 1AAD:01EF: 8 bytes of key bits tested with bit 1
        db      00h,00h,00h,38h,0EFh,01h,00h,00h      ; 1AAD:01F7
kKeyDownBits    db 00h,00h,00h,00h,00h   ; 1AAD:01FF: bitmap of keys held down
kKeyFlag204     db 00h,00h   ; 1AAD:0204: copy of 40:96 bit 4 (a shift modifier)
kKeyFlag206     db 00h   ; 1AAD:0206: packed modifiers from 40:17/18/96
kKeyFlag207     db 00h,00h,00h,00h,00h,00h,00h,00h   ; 1AAD:0207: copy of 40:96 bit 2
kKeyFlag20F     db 00h   ; 1AAD:020F: 1 while an E0-prefixed scancode is expected
kKeyFlag210     db 00h   ; 1AAD:0210: countdown of E1-prefixed bytes to swallow
kInt9SavedOff  dw      0000h           ; 1AAD:0211: saved INT 9 vector offset
kInt9SavedSeg  dw      0000h           ; 1AAD:0213: saved INT 9 vector segment
kInt23SavedOff dw      0000h           ; 1AAD:0215: saved INT 23h (Ctrl-C) vector offset
kInt23SavedSeg dw      0000h           ; 1AAD:0217: saved INT 23h vector segment
kSpecialKeys    db 1Eh,2Ch,39h,3Ah,3Ch,3Eh,40h,5Ah,80h   ; 1AAD:0219: 9 key codes the far readers treat specially
kModifierOut    db 00h   ; 1AAD:0222: modifier bits published to 1AAD:0B0E

; 1AAD:0223: map a key code in AL with modifier bits in AH to an ASCII code in AL. Key codes below
; 3Eh index one of two 3Eh-entry tables by AL & 3Fh and by the shift bit AH & 1; the control bit
; AH & 2 masks the result to the low 5 bits when the key is in the bit table at 1AAD:01DF. Keys
; 4Bh..6Dh are the numeric keypad and use the tables at 1AAD:0357/037Ah, gated by kModifierState
; bit 200h and kShiftFlagsCopy bit 2; keys 70h..79h (Ctrl-F1..F10) use the table at 1AAD:037Ah.
; Other keys are returned with bit 80h set, or as ESC (1Bh) for 6Eh. Purpose unknown: the meaning
; of each table entry is not established.
        PUBLIC  _TranslateKey            ; @ 1AAD:0223
_TranslateKey PROC NEAR
        push    bx
        push    cx
        push    di
        push    ds
        mov     bx,cs
        mov     ds,bx
        cmp     al,6Eh
        jne     f0223_0232
        jmp     f0223_02D4
f0223_0232:
        cmp     al,3Eh
        jae     f0223_026C
        mov     bx,ax
        and     bx,3Fh
        test    ah,1
        jne     f0223_0247
        mov     al,cs:[bx+kUnshiftedCodes]
        jmp     short f0223_024C
f0223_0247:
        mov     al,cs:[bx+kShiftedCodes]
f0223_024C:
        test    ah,2
        je      f0223_026A
        mov     di,bx
        and     di,7
        mov     cl,cs:[di+kKeyBitMasks]
        shr     bx,1
        shr     bx,1
        shr     bx,1
        test    byte ptr cs:[bx+kKeyBitTable1],cl
        je      f0223_026A
        and     al,1Fh
f0223_026A:
        jmp     short f0223_02D6
f0223_026C:
        cmp     al,4Bh
        jb      f0223_02C8
        cmp     al,6Eh
        jae     f0223_029E
        xor     bh,bh
        mov     bl,al
        sub     bl,4Bh
        test    word ptr cs:[kModifierState],200h
        jne     f0223_028D
        test    word ptr cs:[kShiftFlagsCopy],2
        jne     f0223_0294
f0223_028D:
        mov     al,cs:[bx+kKeypadCodes]
        jmp     short f0223_02D6
f0223_0294:
        sub     bl,0Fh
        mov     al,cs:[bx+kCtrlFnCodes]
        jmp     short f0223_02D6
f0223_029E:
        cmp     al,70h
        jb      f0223_02D0
        cmp     al,79h
        ja      f0223_02D0
        mov     bl,al
        sub     bl,70h
        mov     bh,0C5h
        test    ah,7
        je      f0223_02C2
        mov     bh,98h
        test    ah,4
        jne     f0223_02C2
        mov     bh,0A2h
        test    ah,2
        jne     f0223_02C2
        mov     bh,0ACh
f0223_02C2:
        sub     bh,bl
        mov     al,bh
        jmp     short f0223_02D6
f0223_02C8:
        cmp     al,41h
        jb      f0223_02D0
        add     al,85h
        jmp     short f0223_02D6
f0223_02D0:
        or      al,80h
        jmp     short f0223_02D6
f0223_02D4:
        mov     al,1Bh
f0223_02D6:
        pop     ds
        pop     di
        pop     cx
        pop     bx
        ret
_TranslateKey ENDP

; The four tables _TranslateKey indexes, inside its own 363-byte region (after the ret). The first
; two are 3Eh entries each, the keypad one 23h, the Ctrl-function one 14h.
kUnshiftedCodes db 00h,60h,31h,32h,33h,34h,35h,36h,37h,38h,39h,30h   ; 1AAD:02DB: byte codes, no shift
        db      2Dh,3Dh,00h,08h,09h,71h,77h,65h,72h,74h,79h,75h
        db      69h,6Fh,70h,5Bh,5Dh,5Ch,00h,61h,73h,64h,66h,67h
        db      68h,6Ah,6Bh,6Ch,3Bh,27h,00h,0Dh,00h,00h,7Ah,78h
        db      63h,76h,62h,6Eh,6Dh,2Ch,2Eh,2Fh,00h,00h,00h,00h
        db      00h,20h
kShiftedCodes   db 00h,7Eh,21h,40h,23h,24h,25h,5Eh,26h,2Ah,28h,29h   ; 1AAD:0319: byte codes, shift or caps
        db      5Fh,2Bh,00h,08h,09h,51h,57h,45h,52h,54h,59h,55h
        db      49h,4Fh,50h,7Bh,7Dh,7Ch,00h,41h,53h,44h,46h,47h
        db      48h,4Ah,4Bh,4Ch,3Ah,22h,00h,0Dh,00h,00h,5Ah,58h
        db      43h,56h,42h,4Eh,4Dh,3Ch,3Eh,3Fh,00h,00h,00h,00h
        db      00h,20h
kKeypadCodes    db 0AEh,0ADh,00h,00h,0B5h,0B9h,0B1h,00h,0B8h,0B0h,0B7h,0AFh   ; 1AAD:0357: numeric-keypad codes
        db      00h,00h,0B3h,00h,0B9h,0B5h,0B1h,00h,2Fh,0B8h,0B4h,0B0h
        db      0AEh,2Ah,0B7h,0B3h,0AFh,0ADh,2Dh,2Bh,00h,0Dh,00h
kCtrlFnCodes    db 00h,37h,34h,31h,00h,2Fh,38h,35h,32h,30h,2Ah,39h   ; 1AAD:037A: Ctrl-function-key codes
        db      36h,33h,2Eh,2Dh,2Bh,00h,0Dh,00h

; 1AAD:038E: non-blocking read of one game key from the event ring. It calls 1AAD:0D26 first,
; then, unless the game is in state 2 (ds:80EC = 2) and the ring is empty, reads the word at the
; read index; it discards an event that is in the special list 1AAD:0219 or whose AH has bit 8 or
; whose AL is 7Ah or more, advancing the read index (by 4 when AL is 41h..44h, a mouse event), and
; loops until it can return a key. It copies kShiftFlags to kShiftFlagsCopy, passes a nonzero key
; through 1AAD:0223 and clears AH. Callers: the camp loop (EOV6.C) and several overlays. Purpose
; unknown: why this filter differs from 1AAD:0409's.
        PUBLIC  _ReadKey                 ; @ 1AAD:038E
_ReadKey PROC FAR
        call    _InjectRecordedKey
        push    cx
        push    di
        push    si
        push    es
        mov     ax,cs
        mov     es,ax
f038E_0399:
        cli
        mov     ax,cs:[kKeyBufRead]
        cmp     byte ptr [g80EC],2
        je      f038E_03AC
        cmp     ax,cs:[kKeyBufWrite]
        je      f038E_03ED
f038E_03AC:
        mov     si,ax
        mov     ax,word ptr cs:[si+kKeyBuf]
        cmp     byte ptr [g80EC],2
        jne     f038E_03BE
        or      ax,ax
        je      f038E_03EF
f038E_03BE:
        mov     di,OFFSET kSpecialKeys
        mov     cx,9
        repne   scasb
        or      cx,cx
        jne     f038E_03D3
        test    ah,8
        jne     f038E_03D3
        cmp     al,7Ah
        jb      f038E_03EF
f038E_03D3:
        cmp     al,41h
        jb      f038E_03DE
        cmp     al,44h
        ja      f038E_03DE
        add     si,4
f038E_03DE:
        add     si,2
        and     si,0FFh
        mov     cs:[kKeyBufRead],si
        sti
        jmp     short f038E_0399
f038E_03ED:
        xor     ax,ax
f038E_03EF:
        mov     cx,cs:[kShiftFlags]
        mov     cs:[kShiftFlagsCopy],cx
        sti
        or      ax,ax
        je      f038E_0403
        call    _TranslateKey
        xor     ah,ah
f038E_0403:
        sti
        pop     es
        pop     si
        pop     di
        pop     cx
        retf
_ReadKey ENDP

; 1AAD:0409: non-blocking read of one key from the event ring that is in the special list
; 1AAD:0219 and below 7Ah; events that are not are consumed and the read index advances, so it
; returns 0 when the ring holds no such key. Like 1AAD:038E it calls 1AAD:0D26 first, copies
; kShiftFlags to kShiftFlagsCopy and translates the key through 1AAD:0223. Called by the text
; input helper 16A5:0009. Purpose unknown: the exact filter and its difference from 1AAD:038E.
        PUBLIC  _ReadKeySpecial          ; @ 1AAD:0409
_ReadKeySpecial PROC FAR
        call    _InjectRecordedKey
        push    cx
        push    di
        push    si
        push    es
        mov     ax,cs
        mov     es,ax
f0409_0414:
        cli
        mov     ax,cs:[kKeyBufRead]
        cmp     byte ptr [g80EC],2
        je      f0409_0427
        cmp     ax,cs:[kKeyBufWrite]
        je      f0409_0458
f0409_0427:
        mov     si,ax
        mov     ax,word ptr cs:[si+kKeyBuf]
        cmp     byte ptr [g80EC],2
        jne     f0409_0439
        or      ax,ax
        je      f0409_045A
f0409_0439:
        mov     di,OFFSET kSpecialKeys
        mov     cx,9
        repne   scasb
        or      cx,cx
        jne     f0409_0449
        cmp     al,7Ah
        jb      f0409_045A
f0409_0449:
        add     si,2
        and     si,0FFh
        mov     cs:[kKeyBufRead],si
        sti
        jmp     short f0409_0414
f0409_0458:
        xor     ax,ax
f0409_045A:
        mov     cx,cs:[kShiftFlags]
        mov     cs:[kShiftFlagsCopy],cx
        sti
        or      ax,ax
        je      f0409_046C
        call    _TranslateKey
f0409_046C:
        pop     es
        pop     si
        pop     di
        pop     cx
        retf
_ReadKeySpecial ENDP

; 1AAD:0471: 1 (nonzero, actually the xor of the ring read and write indices) when at least one
; event waits in the ring, 0 when it is empty. It calls 1AAD:0D26 for its side effect. Called by
; 16A5:005A and UpdateOptionList (14EF:0274) to test before reading.
        PUBLIC  _IsKeyPending           ; @ 1AAD:0471
_IsKeyPending PROC FAR
        cli
        mov     ax,cs:[kKeyBufRead]
        xor     ax,cs:[kKeyBufWrite]
        sti
        call    _InjectRecordedKey
        retf
_IsKeyPending ENDP

; 1AAD:0480: flush the ring by setting the write index equal to the read index, discarding every
; buffered event. AX is preserved across it. Called from many overlay and UI functions.
        PUBLIC  _FlushKeyBuffer         ; @ 1AAD:0480
_FlushKeyBuffer PROC FAR
        push    ax
        cli
        mov     ax,cs:[kKeyBufRead]
        mov     cs:[kKeyBufWrite],ax
        sti
        pop     ax
        retf
_FlushKeyBuffer ENDP

; 1AAD:048D: take the event at ring index AX (1AAD:007C + AX) and advance the ring read index by
; 2, or by 6 for a mouse event (AL 41h..44h, whose extra words become ds:80F3/ds:80F5, and for
; AL 41h/42h also ds:8140/ds:8142). Unless the game is in state 2 (ds:80EC = 2, which keeps the
; index) the read index is written back. Events are copied to ds:80EF and, in state 2, read from
; ds:80EF instead of the ring. Returns the key in AX. Called by 1AAD:053A/0588/05D3.
        PUBLIC  _DequeueKeyEvent        ; @ 1AAD:048D
_DequeueKeyEvent PROC NEAR
        push    ds
        push    di
        mov     di,ax
        mov     ax,212Eh
        mov     ds,ax
        mov     ax,word ptr cs:[di+kKeyBuf]
        cmp     byte ptr [g80EC],2
        jne     f048D_04A5
        mov     ax,[g80EF]
f048D_04A5:
        mov     [g80EF],ax
        add     di,2
        and     di,0FFh
        cmp     al,41h
        jb      f048D_052B
        cmp     al,42h
        ja      f048D_04F3
        push    ax
        mov     ax,word ptr cs:[di+kKeyBuf]
        cmp     byte ptr [g80EC],2
        jne     f048D_04C7
        mov     ax,[g80F3]
f048D_04C7:
        mov     [g80F3],ax
        add     di,2
        and     di,0FFh
        mov     [g8140],ax
        mov     ax,word ptr cs:[di+kKeyBuf]
        cmp     byte ptr [g80EC],2
        jne     f048D_04E3
        mov     ax,[g80F5]
f048D_04E3:
        mov     [g80F5],ax
        add     di,2
        and     di,0FFh
        mov     [g8142],ax
        pop     ax
        jmp     short f048D_052B
f048D_04F3:
        cmp     al,44h
        ja      f048D_052B
        push    ax
        mov     ax,word ptr cs:[di+kKeyBuf]
        cmp     byte ptr [g80EC],2
        jne     f048D_0507
        mov     ax,[g80F3]
f048D_0507:
        mov     [g80F3],ax
        add     di,2
        and     di,0FFh
        mov     ax,word ptr cs:[di+kKeyBuf]
        cmp     byte ptr [g80EC],2
        jne     f048D_0520
        mov     ax,[g80F5]
f048D_0520:
        mov     [g80F5],ax
        add     di,2
        and     di,0FFh
        pop     ax
f048D_052B:
        cmp     byte ptr [g80EC],2
        je      f048D_0537
        mov     cs:[kKeyBufRead],di
f048D_0537:
        pop     di
        pop     ds
        ret
_DequeueKeyEvent ENDP

; 1AAD:053A: block until the ring holds a game key that is not in the special list 1AAD:0219,
; whose AH bit 8 is clear and whose AL is below 7Ah, take it with 1AAD:048D, translate it with
; 1AAD:0223 and process it with 1AAD:0C17. Called by StartupGame, LoadGamePanel and the camp path
; (EOV6.C). Purpose unknown: the exact meaning of the filter.
        PUBLIC  _WaitGameKey            ; @ 1AAD:053A
_WaitGameKey PROC FAR
        push    bx
        push    cx
        push    di
        push    ds
        push    es
        mov     ax,cs
        mov     es,ax
f053A_0543:
        cli
        cmp     byte ptr [g80EC],2
        je      f053A_0559
        mov     ax,cs:[kKeyBufRead]
        cmp     ax,cs:[kKeyBufWrite]
        jne     f053A_0559
        sti
        jmp     short f053A_0543
f053A_0559:
        call    _DequeueKeyEvent
        mov     cx,cs:[kShiftFlags]
        mov     cs:[kShiftFlagsCopy],cx
        sti
        mov     di,OFFSET kSpecialKeys
        mov     cx,9
        repne   scasb
        or      cx,cx
        jne     f053A_0543
        test    ah,8
        jne     f053A_0543
        cmp     al,7Ah
        jae     f053A_0543
        call    _TranslateKey
        pop     es
        pop     ds
        pop     di
        pop     cx
        pop     bx
        call    _ProcessKeyEvent
        retf
_WaitGameKey ENDP

; 1AAD:0588: 1AAD:053A without the AH bit-8 test; also points DS at the unit's segment. Blocks
; until the ring holds a key that is not in the special list and below 7Ah, takes and translates
; it and processes it with 1AAD:0C17. Called by the text input helper 16A5:0009.
        PUBLIC  _WaitGameKey2           ; @ 1AAD:0588
_WaitGameKey2 PROC FAR
        push    bx
        push    cx
        push    di
        push    ds
        push    es
        mov     ax,cs
        mov     es,ax
        mov     ds,ax
f0588_0593:
        cli
        cmp     byte ptr [g80EC],2
        je      f0588_05A9
        mov     ax,cs:[kKeyBufRead]
        cmp     ax,cs:[kKeyBufWrite]
        jne     f0588_05A9
        sti
        jmp     short f0588_0593
f0588_05A9:
        call    _DequeueKeyEvent
        mov     cx,cs:[kShiftFlags]
        mov     cs:[kShiftFlagsCopy],cx
        sti
        mov     di,OFFSET kSpecialKeys
        mov     cx,9
        repne   scasb
        or      cx,cx
        jne     f0588_0593
        cmp     al,7Ah
        jae     f0588_0593
        call    _TranslateKey
        pop     es
        pop     ds
        pop     di
        pop     cx
        pop     bx
        call    _ProcessKeyEvent
        retf
_WaitGameKey2 ENDP

; 1AAD:05D3: block until the ring is non-empty, take one event with 1AAD:048D and process it with
; 1AAD:0C17, with no key filter. Called by several overlays and by 16A5:005A.
        PUBLIC  _WaitKeyEvent           ; @ 1AAD:05D3
_WaitKeyEvent PROC FAR
f05D3_05D3:
        cli
        cmp     byte ptr [g80EC],2
        je      f05D3_05E9
        mov     ax,cs:[kKeyBufRead]
        cmp     ax,cs:[kKeyBufWrite]
        jne     f05D3_05E9
        sti
        jmp     short f05D3_05D3
f05D3_05E9:
        call    _DequeueKeyEvent
        sti
        call    _ProcessKeyEvent
        retf
_WaitKeyEvent ENDP

        PUBLIC  _f1AAD_05F1              ; @ 1AAD:05F1 stub
_f1AAD_05F1 PROC FAR
        ret
_f1AAD_05F1 ENDP

        PUBLIC  _f1AAD_0718              ; @ 1AAD:0718 stub
_f1AAD_0718 PROC FAR
        ret
_f1AAD_0718 ENDP

        PUBLIC  _f1AAD_09F6              ; @ 1AAD:09F6 stub
_f1AAD_09F6 PROC FAR
        ret
_f1AAD_09F6 ENDP

        PUBLIC  _f1AAD_0A1F              ; @ 1AAD:0A1F stub
_f1AAD_0A1F PROC FAR
        ret
_f1AAD_0A1F ENDP

        PUBLIC  _f1AAD_0B0E              ; @ 1AAD:0B0E stub
_f1AAD_0B0E PROC FAR
        ret
_f1AAD_0B0E ENDP

        PUBLIC  _f1AAD_0B9C              ; @ 1AAD:0B9C stub
_f1AAD_0B9C PROC NEAR
        ret
_f1AAD_0B9C ENDP

        PUBLIC  _f1AAD_0BF1              ; @ 1AAD:0BF1 stub
_f1AAD_0BF1 PROC NEAR
        ret
_f1AAD_0BF1 ENDP

        PUBLIC  _ProcessKeyEvent         ; @ 1AAD:0C17 stub
_ProcessKeyEvent PROC NEAR
        ret
_ProcessKeyEvent ENDP

        PUBLIC  _InjectRecordedKey       ; @ 1AAD:0D26 stub
_InjectRecordedKey PROC NEAR
        ret
_InjectRecordedKey ENDP

        END
