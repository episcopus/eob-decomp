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
        db      80h,00h,00h,01h
kChainFarPtr    dd 00000000h    ; 1AAD:0048: far pointer called when AL is 6Eh (1AAD:099B)
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
; AH & 2 masks the result to the low 5 bits when the key is in the bit table at 1AAD:01DF. Key
; codes 4Bh..6Dh (the numeric keypad) take the table at 1AAD:0357 unless kShiftFlagsCopy bit 2 is
; set and kModifierState bit 200h is clear, when they take 1AAD:037Ah with a different index base.
; Key codes 70h..79h subtract a base (0C5h, or 98h/A2h/ACh by the control bits) and return the
; difference. Other codes come back with bit 80h set, or as ESC (1Bh) for 6Eh. Purpose unknown:
; the meaning of each table entry is not established.
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

; 1AAD:05F1: the key-state query. It takes a key code at [bp+6], clears the extra-bit byte
; 1AAD:066E, and copies ds:80EA into kModifierState. Unless kModifierState bit 2 is set it
; searches the key's low byte in the 14-entry list 1AAD:0677; a match takes the mode byte from
; 1AAD:0685 and reads that mode's bits out of the held-key bitmap 1AAD:01FF into 1AAD:066E. Then
; it recomputes the bit for the key's own low byte (bit 1AAD:066F + index>>3) and returns
; AL = held-key bitmap bits OR 1AAD:066E. Called far from 16A5:00EA/0106 with the key pushed. The
; block at 1AAD:0693 (inside this region) wraps it: it keeps the key in BX, calls 1AAD:05F1 and
; returns AL = 0FFh when the folded modifier byte AH equals the key's high byte BH, and 0 otherwise,
; after folding in the Ctrl/Alt/Shift bits from kKeyFlag204/kKeyFlag206/kKeyFlag207 and kShiftFlags
; through the bit tables at 1AAD:01DF/01EF. Purpose unknown: how the 1AAD:0693 wrapper is entered
; (the map takes it for part of 1AAD:05F1), and why its near call reads the key from [bp+6].
        PUBLIC  _GetKeyState            ; @ 1AAD:05F1
_GetKeyState PROC FAR
        push    bp
        mov     bp,sp
        mov     ax,[bp+6]
        pop     bp
        push    di
        push    bx
        mov     byte ptr cs:[kKeyBits66E],0
        push    ds
        push    ax
        mov     ax,212Eh
        mov     ds,ax
        mov     ax,[g80EA]
        mov     cs:[kModifierState],ax
        pop     ax
        pop     ds
        test    word ptr cs:[kModifierState],2
        jne     f05F1_064B
        mov     di,0Eh
f05F1_061C:
        dec     di
        jl      f05F1_064B
        cmp     al,byte ptr cs:[di+kScanList677]
        jne     f05F1_061C
        push    ax
        mov     al,byte ptr cs:[di+kModeList685]
        xor     ah,ah
        mov     di,ax
        shr     di,1
        shr     di,1
        shr     di,1
        mov     bx,ax
        and     bx,7
        mov     al,byte ptr cs:[bx+kKeyMasks66F]
        cli
        and     al,byte ptr cs:[di+kKeyDownBits]
        mov     byte ptr cs:[kKeyBits66E],al
        pop     ax
f05F1_064B:
        xor     ah,ah
        mov     di,ax
        shr     di,1
        shr     di,1
        shr     di,1
        mov     bx,ax
        and     bx,7
        mov     al,byte ptr cs:[bx+kKeyMasks66F]
        cli
        and     al,byte ptr cs:[di+kKeyDownBits]
        or      al,byte ptr cs:[kKeyBits66E]
        sti
        pop     bx
        pop     di
        retf
_GetKeyState ENDP

kKeyBits66E     db 00h          ; 1AAD:066E: extra held bits accumulated by 1AAD:05F1
kKeyMasks66F    db 01h,02h,04h,08h,10h,20h,40h,80h       ; 1AAD:066F: bit for a key low 3 bits
kScanList677    db 3Ch,3Ah,63h,68h,5Ch,5Bh,5Dh,60h,62h,65h,67h,66h,37h,2Bh  ; 1AAD:0677
kModeList685    db 3Eh,40h,4Bh,4Ch,4Fh,50h,51h,53h,54h,55h,56h,59h,5Fh,6Ch  ; 1AAD:0685

; 1AAD:0693: the wrapper described in the 1AAD:05F1 comment. Purpose unknown: its entry.
f05F1_0693:
        push    bx
        push    cx
        push    di
        mov     bx,ax
        call    _GetKeyState
        je      f05F1_0712
        mov     ax,bx
        xor     ah,ah
        mov     di,ax
        shr     di,1
        shr     di,1
        shr     di,1
        mov     ch,1
        mov     cl,al
        and     cl,7
        shl     ch,cl
        test    byte ptr cs:[kKeyFlag206],4
        jne     f05F1_06C3
        test    byte ptr cs:[kKeyFlag207],1
        je      f05F1_06C6
f05F1_06C3:
        or      ah,2
f05F1_06C6:
        test    byte ptr cs:[kKeyFlag206],50h
        je      f05F1_06D1
        or      ah,4
f05F1_06D1:
        test    byte ptr cs:[kKeyFlag204],10h
        jne     f05F1_06E1
        test    byte ptr cs:[kKeyFlag206],2
        je      f05F1_06E4
f05F1_06E1:
        or      ah,1
f05F1_06E4:
        test    word ptr cs:[kShiftFlags],1
        je      f05F1_06F7
        test    byte ptr cs:[di+kKeyBitTable1],ch
        je      f05F1_06F7
        xor     ah,1
f05F1_06F7:
        test    word ptr cs:[kShiftFlags],2
        je      f05F1_070A
        test    byte ptr cs:[di+kKeyBitTable2],ch
        je      f05F1_070A
        xor     ah,1
f05F1_070A:
        mov     al,0FFh
        cmp     ah,bh
        je      f05F1_0712
        xor     ax,ax
f05F1_0712:
        pop     di
        pop     cx
        pop     bx
        or      ax,ax
        retf

; 1AAD:0718: install the keyboard driver. It records the BIOS shift state (40:17/18/96) into
; kShiftFlags/kKeyFlag204/kKeyFlag206/kKeyFlag207 and seeds kKeyFlag20F/kKeyFlag210, then chains
; INT 9 to the handler at 1AAD:07C0 (through INT 21h 35h/25h, saving the old vector at
; kInt9SavedOff/kInt9SavedSeg) and INT 23h (Ctrl-C) to the IRET at 1AAD:09F5 (saving it at
; kInt23SavedOff/kInt23SavedSeg). The handler body at 1AAD:07C0 (inside this region) reads the
; scancode from port 60h, tracks E0/E1 prefixes, translates the scancode through 1AAD:0186, updates
; the held-key bitmap 1AAD:01FF and the modifier byte kModifierOut, toggles gSoundOn/g80E8 on the
; Ctrl/Alt sound keys, and appends the event to the ring (or chains to the old INT 9 vector when it
; does not consume the key). The iret and the two data bytes at 1AAD:09F2 finish the region. Called
; from main at 07C8:008F. Purpose unknown: several of the port/handler paths.
        PUBLIC  _InstallKeyboard          ; @ 1AAD:0718
_InstallKeyboard PROC FAR
        push    ax
        push    bx
        push    ds
        push    dx
        push    es
        mov     ax,0040h
        mov     es,ax
        mov     al,byte ptr es:[17h]
        test    al,40h
        je      f0718_0730
        or      word ptr cs:[kShiftFlags],1
f0718_0730:
        test    al,20h
        je      f0718_073A
        or      word ptr cs:[kShiftFlags],2
f0718_073A:
        test    al,2
        je      f0718_0744
        or      byte ptr cs:[kKeyFlag204],10h
f0718_0744:
        and     al,1
        shl     al,1
        mov     ah,al
        mov     al,byte ptr es:[18h]
        shl     al,1
        shl     al,1
        and     al,0Ch
        or      ah,al
        mov     al,byte ptr es:[96h]
        test    al,8
        je      f0718_0761
        or      ah,40h
f0718_0761:
        mov     byte ptr cs:[kKeyFlag206],ah
        test    al,4
        je      f0718_0770
        or      byte ptr cs:[kKeyFlag207],1
f0718_0770:
        test    al,2
        je      f0718_077A
        mov     byte ptr cs:[kKeyFlag20F],1
f0718_077A:
        test    al,1
        je      f0718_0784
        mov     byte ptr cs:[kKeyFlag210],2
f0718_0784:
        mov     ax,3509h
        int     21h
        mov     word ptr cs:[kInt9SavedOff],bx
        mov     word ptr cs:[kInt9SavedSeg],es
        mov     bx,cs
        mov     ds,bx
        mov     dx,07C0h
        mov     ax,2509h
        int     21h
        mov     ax,3523h
        int     21h
        mov     word ptr cs:[kInt23SavedOff],bx
        mov     word ptr cs:[kInt23SavedSeg],es
        mov     bx,cs
        mov     ds,bx
        mov     dx,09F5h
        mov     ax,2523h
        int     21h
        pop     es
        pop     dx
        pop     ds
        pop     bx
        pop     ax
        retf
_InstallKeyboard ENDP

; 1AAD:07C0: the INT 9 handler installed above (inside the install function's region, not a
; separate map function). It clears kShiftFlags bits 0/1, ORs in the live BIOS shift state, copies
; ds:80EA into kModifierState, reads a scancode from port 60h and either swallows it (E1/E0
; prefix, break code) or translates and buffers it. kKeyMode9F2/kScanCode9F3 hold the state of the
; prefix path. It ends by chaining to the saved INT 9 vector (1AAD:09BD) or acknowledging the PIC
; itself (1AAD:09CA). Purpose unknown: the exact scan-code paths.
f0718_07C0:
        push    ax
        push    bx
        push    cx
        push    di
        push    ds
        push    dx
        push    es
        push    si
        and     word ptr cs:[kShiftFlags],0FFFCh
        mov     ax,0040h
        mov     es,ax
        test    byte ptr es:[17h],40h
        je      f0718_07E1
        or      word ptr cs:[kShiftFlags],1
f0718_07E1:
        test    byte ptr es:[17h],20h
        je      f0718_07EF
        or      word ptr cs:[kShiftFlags],2
f0718_07EF:
        mov     ax,212Eh
        mov     ds,ax
        mov     ax,[g80EA]
        mov     cs:[kModifierState],ax
        mov     dx,word ptr cs:[kModifierState]
        mov     ax,cs
        mov     ds,ax
        mov     es,ax
        cld
        xor     ah,ah
        mov     bx,0101h
        in      al,60h
        cmp     al,0E1h
        jne     f0718_0818
        mov     byte ptr cs:[kKeyFlag210],3
f0718_0818:
        cmp     byte ptr cs:[kKeyFlag210],0
        je      f0718_082E
        dec     byte ptr cs:[kKeyFlag210]
        test    dx,80h
        jne     f0718_0838
        jmp     f0718_09CA
f0718_082E:
        cmp     al,0E0h
        jne     f0718_083B
        mov     byte ptr cs:[kKeyFlag20F],1
f0718_0838:
        jmp     f0718_09BD
f0718_083B:
        test    al,80h
        je      f0718_0843
        xor     bl,bl
        and     al,7Fh
f0718_0843:
        cmp     byte ptr cs:[kKeyFlag20F],0
        je      f0718_086F
        mov     byte ptr cs:[kKeyFlag20F],0
        mov     byte ptr cs:[kKeyMode9F2],1
        mov     di,4Ch
        mov     cx,11h
        repne   scasb
        mov     al,byte ptr [di+0Fh]
        or      cx,cx
        jne     f0718_0876
        mov     byte ptr cs:[kKeyMode9F2],0
        jmp     f0718_09CA
f0718_086F:
        mov     di,ax
        mov     al,byte ptr cs:[di+kScanToKey]
f0718_0876:
        mov     word ptr cs:[kScanCode9F3],di
        mov     di,ax
        shr     di,1
        shr     di,1
        shr     di,1
        mov     cl,al
        and     cl,7
        shl     bl,cl
        shl     bh,cl
        not     bh
        mov     cl,byte ptr cs:[di+kKeyDownBits]
        and     cl,bl
        and     byte ptr cs:[di+kKeyDownBits],bh
        or      byte ptr cs:[di+kKeyDownBits],bl
        test    byte ptr cs:[kKeyFlag206],4
        jne     f0718_08AF
        test    byte ptr cs:[kKeyFlag207],1
        je      f0718_08B2
f0718_08AF:
        or      ah,2
f0718_08B2:
        test    byte ptr cs:[kKeyFlag206],50h
        je      f0718_08BD
        or      ah,4
f0718_08BD:
        cmp     byte ptr cs:[kKeyMode9F2],0
        je      f0718_08DE
        mov     byte ptr cs:[kKeyMode9F2],0
        test    dx,2
        jne     f0718_08DE
        mov     si,word ptr cs:[kScanCode9F3]
        mov     al,byte ptr [si+1Fh]
        cmp     si,21h
        jge     f0718_0917
f0718_08DE:
        test    byte ptr cs:[kKeyFlag204],10h
        jne     f0718_08EE
        test    byte ptr cs:[kKeyFlag206],2
        je      f0718_08F1
f0718_08EE:
        or      ah,1
f0718_08F1:
        test    word ptr cs:[kShiftFlags],1
        je      f0718_0904
        test    byte ptr cs:[di+kKeyBitTable1],ch
        je      f0718_0904
        xor     ah,1
f0718_0904:
        test    word ptr cs:[kShiftFlags],2
        je      f0718_0917
        test    byte ptr cs:[di+kKeyBitTable2],ch
        je      f0718_0917
        xor     ah,1
f0718_0917:
        mov     byte ptr cs:[kModifierOut],ah
        test    dx,1
        jne     f0718_0929
        or      cl,cl
        je      f0718_0929
        jmp     f0718_09CA
f0718_0929:
        cmp     ax,0668h
        je      f0718_0937
        cmp     ax,064Ch
        je      f0718_0937
        or      bl,bl
        jne     f0718_093A
f0718_0937:
        jmp     f0718_09BD
f0718_093A:
        mov     di,word ptr cs:[kKeyBufWrite]
        mov     si,di
        add     di,2
        and     di,0FFh
        cmp     di,word ptr cs:[kKeyBufRead]
        je      f0718_0959
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     word ptr cs:[kKeyBufWrite],di
f0718_0959:
        mov     di,OFFSET kSpecialKeys
        mov     cx,9
        repne   scasb
        or      cx,cx
        jne     f0718_09BD
        cmp     ax,0420h
        je      f0718_0971
        cmp     ax,0434h
        je      f0718_0981
        jne     f0718_098F
f0718_0971:
        push    ds
        push    ax
        mov     ax,212Eh
        mov     ds,ax
        xor     word ptr [gSoundOn],1
        pop     ax
        pop     ds
        jmp     short f0718_098F
f0718_0981:
        push    ds
        push    ax
        mov     ax,212Eh
        mov     ds,ax
        xor     word ptr [g80E8],1
        pop     ax
        pop     ds
f0718_098F:
        cmp     al,6Eh
        jne     f0718_09A0
        cmp     word ptr cs:[kChainFarPtr+2],0
        je      f0718_09A0
        call    dword ptr cs:[kChainFarPtr]
f0718_09A0:
        mov     di,14h
        mov     cx,1Ch
        shr     cx,1
        repne   scasw
        or      cx,cx
        je      f0718_09B7
        mov     bx,word ptr [di+18h]
        and     bx,dx
        jne     f0718_09BD
        jmp     short f0718_09CA
f0718_09B7:
        test    dx,4
        je      f0718_09CA
f0718_09BD:
        pop     si
        pop     es
        pop     dx
        pop     ds
        pop     di
        pop     cx
        pop     bx
        pop     ax
        jmp     dword ptr cs:[kInt9SavedOff]
f0718_09CA:
        in      al,61h
        mov     ah,al
        or      al,80h
        out     61h,al
        xchg    ah,al
        out     61h,al
        mov     ax,0040h
        mov     es,ax
        mov     al,byte ptr es:[96h]
        and     al,0FDh
        mov     byte ptr es:[96h],al
        mov     al,20h
        out     20h,al
        pop     si
        pop     es
        pop     dx
        pop     ds
        pop     di
        pop     cx
        pop     bx
        pop     ax
        iret
kKeyMode9F2     db 00h          ; 1AAD:09F2: 1 while the previous event was an extended key
kScanCode9F3    dw 0000h        ; 1AAD:09F3: scancode of the event being handled
        iret                    ; 1AAD:09F5: the INT 23h handler installed at 1AAD:07B2

; 1AAD:09F6: remove the keyboard driver. It restores INT 23h from kInt23SavedOff/kInt23SavedSeg
; and INT 9 from kInt9SavedOff/kInt9SavedSeg through INT 21h 25h. Called from the exit path at
; 07C8:058A.
        PUBLIC  _RemoveKeyboard         ; @ 1AAD:09F6
_RemoveKeyboard PROC FAR
        push    ax
        push    ds
        push    dx
        mov     dx,word ptr cs:[kInt23SavedSeg]
        mov     ds,dx
        mov     dx,word ptr cs:[kInt23SavedOff]
        mov     ax,2523h
        int     21h
        mov     dx,word ptr cs:[kInt9SavedSeg]
        mov     ds,dx
        mov     dx,word ptr cs:[kInt9SavedOff]
        mov     ax,2509h
        int     21h
        pop     dx
        pop     ds
        pop     ax
        retf
_RemoveKeyboard ENDP

; 1AAD:0A1F: queue one input event (the code at [bp+6], low 12 bits) into the ring. Code 41h/42h
; queues three words: the code and the mouse position es:8138/es:813A; code 43h/44h queues the code
; three times; anything else queues one word. It tests the ring for room first (write index walking
; to the read index) and returns the code in AX, or 0 when the ring is full. ES is DGROUP (212Eh)
; and DS is the unit's segment for the whole call. Purpose unknown: its entry (no direct caller in
; the map; probably a far pointer call).
        PUBLIC  _QueueInputEvent        ; @ 1AAD:0A1F
_QueueInputEvent PROC FAR
        push    bp
        mov     bp,sp
        push    di
        push    si
        push    ds
        push    es
        mov     ax,212Eh
        mov     es,ax
        mov     ax,cs
        mov     ds,ax
        mov     ax,[bp+6]
        and     ax,0FFFh
        cli
        or      al,al
        jne     f0A1F_0A3D
f0A1F_0A3A:
        jmp     f0A1F_0B05
f0A1F_0A3D:
        cmp     al,41h
        jae     f0A1F_0A44
        jmp     f0A1F_0AE4
f0A1F_0A44:
        cmp     al,42h
        ja      f0A1F_0A98
        mov     di,word ptr cs:[kKeyBufWrite]
        mov     si,di
        add     di,6
        and     di,0FFh
        cmp     si,di
        ja      f0A1F_0A61
        cmp     si,word ptr cs:[kKeyBufRead]
        jae     f0A1F_0A68
f0A1F_0A61:
        cmp     di,word ptr cs:[kKeyBufRead]
        jae     f0A1F_0A3A
f0A1F_0A68:
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     bp,ax
        mov     ax,word ptr es:[_gCursorX]
        add     si,2
        and     si,0FFh
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     ax,word ptr es:[_gCursorY]
        add     si,2
        and     si,0FFh
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     word ptr cs:[kKeyBufWrite],di
        mov     ax,bp
        jmp     short f0A1F_0B07
f0A1F_0A98:
        cmp     al,44h
        ja      f0A1F_0AE4
        mov     di,word ptr cs:[kKeyBufWrite]
        mov     si,di
        add     di,2
        and     di,0FFh
        cmp     si,di
        ja      f0A1F_0AB5
        cmp     si,word ptr cs:[kKeyBufRead]
        jae     f0A1F_0ABC
f0A1F_0AB5:
        cmp     di,word ptr cs:[kKeyBufRead]
        jae     f0A1F_0B05
f0A1F_0ABC:
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     bp,ax
        add     si,2
        and     si,0FFh
        mov     word ptr cs:[si+kKeyBuf],ax
        add     si,2
        and     si,0FFh
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     word ptr cs:[kKeyBufWrite],di
        mov     ax,bp
        jmp     short f0A1F_0B07
f0A1F_0AE4:
        mov     di,word ptr cs:[kKeyBufWrite]
        mov     si,di
        add     di,2
        and     di,0FFh
        cmp     di,word ptr cs:[kKeyBufRead]
        je      f0A1F_0B05
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     word ptr cs:[kKeyBufWrite],di
        jmp     short f0A1F_0B07
f0A1F_0B05:
        xor     ax,ax
f0A1F_0B07:
        sti
        pop     es
        pop     ds
        pop     si
        pop     di
        pop     bp
        retf
_QueueInputEvent ENDP

; 1AAD:0B0E: the mouse-button glue called from the INT 33h handler (1A72:017A) with the new button
; state at [bp+6]. It xors that with the last state (kMouseButtons) and, for each changed button,
; builds the key code 41h/42h, folds in kModifierOut and the shift-bit-0 test through
; kShiftFlags, sets the held bit with 1AAD:0BF1 and queues the event with 1AAD:0B9C. It stores the
; new state back to kMouseButtons. Purpose unknown: the exact key-code convention.
        PUBLIC  _HandleMouseButtons     ; @ 1AAD:0B0E
_HandleMouseButtons PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    dx
        push    di
        push    si
        push    ds
        push    es
        mov     dl,[bp+6]
        mov     ch,byte ptr cs:[kMouseButtons]
        xor     ch,dl
        mov     ax,ds
        mov     es,ax
        mov     ax,cs
        mov     ds,ax
        xor     ax,ax
        test    ch,1
        je      f0B0E_0B5E
        mov     bx,0101h
        mov     ax,41h
        test    dl,1
        jne     f0B0E_0B3E
        xor     bl,bl
f0B0E_0B3E:
        call    _SetKeyDownBit
        mov     ah,byte ptr cs:[kModifierOut]
        test    word ptr cs:[kShiftFlags],1
        je      f0B0E_0B52
        or      ah,1
f0B0E_0B52:
        or      bl,bl
        jne     f0B0E_0B59
        or      ah,8
f0B0E_0B59:
        call    _QueueMouseEvent
        je      f0B0E_0B8E
f0B0E_0B5E:
        test    ch,2
        je      f0B0E_0B8E
        mov     bx,0101h
        mov     ax,42h
        test    dl,2
        jne     f0B0E_0B70
        xor     bl,bl
f0B0E_0B70:
        call    _SetKeyDownBit
        mov     ah,byte ptr cs:[kModifierOut]
        test    word ptr cs:[kShiftFlags],1
        je      f0B0E_0B84
        or      ah,1
f0B0E_0B84:
        or      bl,bl
        jne     f0B0E_0B8B
        or      ah,8
f0B0E_0B8B:
        call    _QueueMouseEvent
f0B0E_0B8E:
        mov     byte ptr cs:[kMouseButtons],dl
        pop     es
        pop     ds
        pop     si
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     bp
        retf
_HandleMouseButtons ENDP

; 1AAD:0B9C: queue a three-word mouse event (the code in AX and the position es:813C/es:813E) into
; the ring, returning AX (0 when the ring is full). A near helper of 1AAD:0B0E.
        PUBLIC  _QueueMouseEvent        ; @ 1AAD:0B9C
_QueueMouseEvent PROC NEAR
        mov     di,word ptr cs:[kKeyBufWrite]
        mov     si,di
        add     di,6
        and     di,0FFh
        cmp     si,di
        ja      f0B9C_0BB5
        cmp     si,word ptr cs:[kKeyBufRead]
        jae     f0B9C_0BBC
f0B9C_0BB5:
        cmp     di,word ptr cs:[kKeyBufRead]
        jae     f0B9C_0BEC
f0B9C_0BBC:
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     bp,ax
        mov     ax,word ptr es:[g813C]
        add     si,2
        and     si,0FFh
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     ax,word ptr es:[g813E]
        add     si,2
        and     si,0FFh
        mov     word ptr cs:[si+kKeyBuf],ax
        mov     word ptr cs:[kKeyBufWrite],di
        mov     ax,bp
        jmp     short f0B9C_0BEE
f0B9C_0BEC:
        xor     ax,ax
f0B9C_0BEE:
        or      ax,ax
        ret
_QueueMouseEvent ENDP

; 1AAD:0BF1: set the held-down bit for the key code in AX to the mask BL (0 for a key-up), folding
; it into the bitmap kKeyDownBits at kKeyDownBits+(key>>3), and return the bit's old state in CL.
; A near helper of 1AAD:0B0E.
        PUBLIC  _SetKeyDownBit          ; @ 1AAD:0BF1
_SetKeyDownBit PROC NEAR
        mov     di,ax
        shr     di,1
        shr     di,1
        shr     di,1
        mov     cl,al
        and     cl,7
        shl     bl,cl
        shl     bh,cl
        not     bh
        mov     cl,byte ptr cs:[di+kKeyDownBits]
        and     cl,bl
        and     byte ptr cs:[di+kKeyDownBits],bh
        or      byte ptr cs:[di+kKeyDownBits],bl
        ret
_SetKeyDownBit ENDP
kMouseButtons   db 00h          ; 1AAD:0C16: mouse button state last handled by 1AAD:0B0E

; 1AAD:0C17: the recorded-input event handler, called after an event is taken from the ring
; (1AAD:053A/0588/05D3). In state 0 (ds:80EC = 0) it does nothing. In state 1 it writes the last
; decoded key (ds:80EF) and the pacing count (ds:8488) to the file handle g80ED through
; 1C0E:0578 (4 bytes), and for a mouse key (41h..44h) writes ds:80F3/ds:80F5 instead, then clears
; the count. In state 2 it reads 4 bytes through FileRead into kRecKeyD1E/kRecCountD20; on success
; it copies them to ds:80EF/ds:80F1 and, for a mouse key, reads 4 more bytes into kRecXD22/kRecYD24
; and copies them to ds:80F3/ds:80F5 and gCursorX/gCursorY (setting ds:8136). A failed read sets
; ds:80F7. Purpose unknown: the exact file format and why state 2 reads rather than writes.
        PUBLIC  _ProcessKeyEvent        ; @ 1AAD:0C17
_ProcessKeyEvent PROC NEAR
        push    ax
        cmp     byte ptr [g80EC],0
        jne     f0C17_0C22
        jmp     f0C17_0D18
f0C17_0C22:
        mov     ax,[g80EF]
        cmp     byte ptr [g80EC],1
        jne     f0C17_0C91
        mov     word ptr cs:[kRecKeyD1E],ax
        mov     ax,[g8488]
        mov     word ptr cs:[kRecCountD20],ax
        push    word ptr cs:[kRecLenD1A+2]
        push    word ptr cs:[kRecLenD1A]
        push    cs
        mov     ax,OFFSET kRecKeyD1E
        push    ax
        push    word ptr [g80ED]
        call    far ptr _f1C0E_0578
        add     sp,0Ah
        mov     ax,word ptr cs:[kRecKeyD1E]
        cmp     ax,41h
        jb      f0C17_0C88
        cmp     al,44h
        ja      f0C17_0C88
        mov     ax,[g80F3]
        mov     word ptr cs:[kRecKeyD1E],ax
        mov     ax,[g80F5]
        mov     word ptr cs:[kRecCountD20],ax
        push    word ptr cs:[kRecLenD1A+2]
        push    word ptr cs:[kRecLenD1A]
        push    cs
        mov     ax,OFFSET kRecKeyD1E
        push    ax
        push    word ptr [g80ED]
        call    far ptr _f1C0E_0578
        add     sp,0Ah
f0C17_0C88:
        mov     word ptr [g8488],0
f0C17_0C8E:
        jmp     f0C17_0D18
f0C17_0C91:
        cmp     byte ptr [g80EC],2
        jne     f0C17_0C8E
        push    word ptr cs:[kRecLenD1A+2]
        push    word ptr cs:[kRecLenD1A]
        push    cs
        mov     ax,OFFSET kRecKeyD1E
        push    ax
        push    word ptr [g80ED]
        call    far ptr _FileRead
        add     sp,0Ah
        or      ax,ax
        je      f0C17_0D12
        mov     ax,word ptr cs:[kRecCountD20]
        mov     [g80F1],ax
        mov     ax,word ptr cs:[kRecKeyD1E]
        mov     [g80EF],ax
        cmp     ax,41h
        jb      f0C17_0D0A
        cmp     al,44h
        ja      f0C17_0D0A
        push    word ptr cs:[kRecLenD1A+2]
        push    word ptr cs:[kRecLenD1A]
        push    cs
        mov     ax,OFFSET kRecKeyD1E
        add     ax,4
        push    ax
        push    word ptr [g80ED]
        call    far ptr _FileRead
        add     sp,0Ah
        or      ax,ax
        je      f0C17_0D12
        mov     ax,word ptr cs:[kRecXD22]
        mov     [g80F3],ax
        mov     [_gCursorX],ax
        mov     ax,word ptr cs:[kRecYD24]
        mov     [g80F5],ax
        mov     [_gCursorY],ax
        mov     word ptr [g8136],1
f0C17_0D0A:
        mov     word ptr [g8488],0
        jmp     short f0C17_0D18
f0C17_0D12:
        mov     word ptr [g80F7],1
f0C17_0D18:
        pop     ax
        ret
_ProcessKeyEvent ENDP
kRecLenD1A      dd 4            ; 1AAD:0D1A: length for the recorded-input reads/writes (low word)
kRecKeyD1E      dw 0000h        ; 1AAD:0D1E: key read from or written to the recording
kRecCountD20    dw 0000h        ; 1AAD:0D20: count that goes with it
kRecXD22        dw 0000h        ; 1AAD:0D22: mouse x read from the recording
kRecYD24        dw 0000h        ; 1AAD:0D24: mouse y read from the recording

; 1AAD:0D26: inject the recorded key at the ring read index, called first by every reader
; (1AAD:038E/0409/0471). In state 0 or 1 it returns at once; otherwise, unless ds:80F7 is set
; (the recording ended), it writes ds:80EF at kKeyBuf+kKeyBufRead when the pacing count ds:8488
; has reached ds:80F1, and 0 otherwise. Purpose unknown: how the injected value is consumed.
        PUBLIC  _InjectRecordedKey      ; @ 1AAD:0D26
_InjectRecordedKey PROC NEAR
        cmp     byte ptr [g80EC],0
        je      f0D26_0D58
        cmp     byte ptr [g80EC],1
        je      f0D26_0D58
        cmp     word ptr [g80F7],0
        jne     f0D26_0D49
        mov     ax,[g80F1]
        cmp     [g8488],ax
        jb      f0D26_0D49
        mov     ax,[g80EF]
        jmp     short f0D26_0D4C
f0D26_0D49:
        mov     ax,0
f0D26_0D4C:
        push    di
        mov     di,word ptr cs:[kKeyBufRead]
        mov     word ptr cs:[di+kKeyBuf],ax
        pop     di
f0D26_0D58:
        ret
_InjectRecordedKey ENDP

        END
