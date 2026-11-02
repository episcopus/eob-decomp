; Unit at overlay 1 of EOB.EXE (segment 2102, file offset 0, 9864 bytes, 12 functions), kind asm
; [H] by the map. It is the party display and inventory overlay: it reads and rewrites the party's
; records at ds:F580 (gChars, 6 records of 0xF3 bytes, E08E6.C), draws rows and items through
; DrawRowList/DrawShadowText and the icon table ds:DD28 (gIconDD28, EOV2.C), and searches class
; slots with FindClassSlot and carried slots with FindCarriedSlot (E0D1C.C). ovl1:1ACD is the
; overlay entry point (its caller is the overlay dispatch, no direct call found) and ovl1:0000 is
; reached from it. Most functions are still stubs or drafts; names marked unknown are inferred from
; the code only. Whitepaper 4 (data) and 2.7.

        .MODEL  LARGE

        EXTRN   _farfree:FAR            ; @ 0000:71EE: free a far block (library)
        EXTRN   _RotateBlocks:FAR       ; @ 082C:07C5 (E082C.C)
        EXTRN   _f0D1C_0670:FAR         ; @ 0D1C:0670 (E0D1C.C)
        EXTRN   _f0D1C_035A:FAR         ; @ 0D1C:035A (E0D1C.C)
        EXTRN   _RedrawMember:FAR       ; @ 1547:0991 (E08E6.C)
        EXTRN   _LockHidePointer:FAR    ; @ 1BBE:0307 (E0ACD.C)
        EXTRN   _LockShowPointer:FAR    ; @ 1BBE:0332 (E0ACD.C)
        EXTRN   _SelectCharacter:FAR    ; @ 0D1C:003F (E0ACD.C)
        EXTRN   _BuildList1:FAR         ; @ 0CF7:0028 (E0CF7.C)
        EXTRN   _RearmCharEffects:FAR   ; @ 1414:0B2E (E1414.C)
        EXTRN   _f1A47_0169:FAR         ; @ 1A47:0169: select a font (E07A9.C)
        EXTRN   _SetTextWindowAndClear:FAR ; @ 1DE7:00B6 (E1DE7.C)
        EXTRN   _SetTextWindow:FAR      ; @ 1DE7:0009 (E07C8.C)
        EXTRN   _f0D1C_0C21:FAR         ; @ 0D1C:0C21 (E0D1C.C)
        EXTRN   _SkipStrings:FAR        ; @ 0D1C:07A7 (E0BD2.C)
        EXTRN   _DrawShadowText:FAR     ; @ 082C:0898 (E082C.C)
        EXTRN   _ClipRect:FAR           ; @ 19F3:000E (E082C.C)
        EXTRN   _ReadKey:FAR            ; @ 1AAD:038E (E082C.C)
        EXTRN   _FlushKeyBuffer:FAR     ; @ 1AAD:0480 (E082C.C)
        EXTRN   _WaitFrames:FAR         ; @ 1A16:000B (E14EF.C)
        EXTRN   _sprintf:FAR            ; @ 0000:57AE (library)
        EXTRN   _memset:FAR             ; @ 0000:6384 (library)
        EXTRN   _strcpy:FAR             ; @ 0000:67CD (library)

        EXTRN   gChars:BYTE             ; @ ds:F580: 6 character records of 0xF3 bytes (E08E6.C)
        EXTRN   gBufDCFC:DWORD          ; @ ds:DCFC: 4096-byte scratch allocation (E07A9.C)
        EXTRN   gFillRect:DWORD         ; @ ds:806A: far fill-rect routine (E082C.C)
        EXTRN   gMemberBoxes:WORD       ; @ ds:62E2: two 8-byte (x1,y1,x2,y2) member-list boxes
        EXTRN   g8115:WORD              ; @ ds:8115: nonzero while a status line is up (E082C.C)
        EXTRN   g811F:WORD              ; @ ds:811F (E07A9.C)
        EXTRN   g159:WORD               ; @ ds:0159 (EOV6.C)
        EXTRN   gF57E:BYTE              ; @ ds:F57E (E0BD2.C)
        EXTRN   gFont1A47:DWORD         ; @ ds:FE4E: the current far font pointer (E1547.C)
        EXTRN   gFE5A:DWORD             ; @ ds:FE5A: the 8x8 font far pointer (E07C8.C)
        EXTRN   gTextHeight:WORD        ; @ ds:FE60 (E1C0E.C)
        EXTRN   gTextWidth:WORD         ; @ ds:FE62 (E1C0E.C)
        EXTRN   gTextLeft:WORD          ; @ ds:FE64 (E14EF.C)
        EXTRN   gTextTop:WORD           ; @ ds:FE66 (E14EF.C)
        EXTRN   gStrings2003:BYTE       ; @ 2003:0000: the message string table (EOV6.C)
        EXTRN   g8102:WORD              ; @ ds:8102: colour-row mode (E07A9.C)
        EXTRN   g1278:WORD              ; @ ds:1278: far item-name pointer table (E0BD2.C)
        EXTRN   g12E0:WORD              ; @ ds:12E0: far item-name pointer table (E0BD2.C)
        EXTRN   gFmt645B:BYTE           ; @ ds:645B: the "%-18s %1d" format string
        EXTRN   gFDEA:BYTE              ; @ ds:FDEA: per-item count, indexed by item*2
        EXTRN   gFileNameBuf:BYTE       ; @ ds:FE6E: text scratch buffer (E082C.C)
        EXTRN   gDrawText:DWORD         ; @ ds:80CA: far text routine (E082C.C)
        EXTRN   gFDEC:BYTE              ; @ ds:FDEC: 32 two-byte spell counters, built by ovl1:00DD
        EXTRN   gFD9A:BYTE              ; @ ds:FD9A: 32-byte spell index table, built by ovl1:00DD
        EXTRN   gFE2C:WORD              ; @ ds:FE2C: set when a known slot is retired (ovl1:025D)

        .CODE

; ovl1:0000: rotate the character whose index is `who` (0..5) out of the party display. It returns
; at once when `who` is negative or fails f0D1C_0670(who, 1) (the member must be alive); otherwise
; f0D1C_035A drops the member's inventory, and, when `who` is below 4, the source position is 5 or
; 4 (whichever of those two is alive, 4 when neither), the three character records source, `who`
; and gBufDCFC are rotated through RotateBlocks, the member is redrawn, the pointer is bracketed and
; one of the two gMemberBoxes rectangles (index who-4) is filled through gFillRect in colour 0xC.
; Then the first character is deselected, the list rebuilt and the effects rearmed. Called from
; ovl1:1ACD with the result of ovl1:0416(0x35). Purpose unknown: the original command and why
; slots 4/5 are the source for indices below 4.
        PUBLIC  _RotateOutMember         ; @ ovl1:0000
_RotateOutMember PROC FAR
        push    bp
        mov     bp,sp
        push    si
        push    di
        mov     di,[bp+6]
        or      di,di
        jge     f0000_000F
        jmp     f0000_00D9
f0000_000F:
        mov     ax,1
        push    ax
        push    di
        call    far ptr _f0D1C_0670
        pop     cx
        pop     cx
        or      ax,ax
        jne     f0000_0022
        jmp     f0000_00D9
f0000_0022:
        push    di
        call    far ptr _f0D1C_035A
        pop     cx
        mov     si,di
        cmp     di,4
        jge     f0000_007E
        mov     ax,1
        push    ax
        mov     ax,5
        push    ax
        call    far ptr _f0D1C_0670
        pop     cx
        pop     cx
        or      ax,ax
        je      f0000_0048
        mov     si,5
        jmp     short f0000_004B
f0000_0048:
        mov     si,4
f0000_004B:
        mov     ax,0F3h
        push    ax
        push    word ptr [gBufDCFC+2]
        push    word ptr [gBufDCFC]
        push    ds
        mov     ax,di
        mov     dx,0F3h
        imul    dx
        add     ax,OFFSET gChars
        push    ax
        push    ds
        mov     ax,si
        mov     dx,0F3h
        imul    dx
        add     ax,OFFSET gChars
        push    ax
        call    far ptr _RotateBlocks
        add     sp,0Eh
        push    di
        call    far ptr _RedrawMember
        pop     cx
f0000_007E:
        sub     si,4
        call    far ptr _LockHidePointer
        mov     ax,0Ch
        push    ax
        mov     bx,si
        shl     bx,1
        shl     bx,1
        shl     bx,1
        push    word ptr [bx+gMemberBoxes+6]
        mov     bx,si
        shl     bx,1
        shl     bx,1
        shl     bx,1
        push    word ptr [bx+gMemberBoxes+4]
        mov     bx,si
        shl     bx,1
        shl     bx,1
        shl     bx,1
        push    word ptr [bx+gMemberBoxes+2]
        mov     bx,si
        shl     bx,1
        shl     bx,1
        shl     bx,1
        push    word ptr [bx+gMemberBoxes]
        call    dword ptr [gFillRect]
        add     sp,0Ah
        call    far ptr _LockShowPointer
        xor     ax,ax
        push    ax
        call    far ptr _SelectCharacter
        pop     cx
        call    far ptr _BuildList1
        call    far ptr _RearmCharEffects
f0000_00D9:
        pop     di
        pop     si
        pop     bp
        ret
_RotateOutMember ENDP

        PUBLIC  _Ovl1_00DD               ; @ ovl1:00DD stub
_Ovl1_00DD PROC FAR
        ret
_Ovl1_00DD ENDP

; ovl1:025D: spend one step of the spell counters for character `who`. It returns when `who` is
; negative; otherwise it clears the 30 bytes at rec+0x37 (known slots) or rec+0x55 (memorized
; slots) when `flag` is set, then walks the 32 counters gFDEC + i*2: it keeps the pair's low byte
; as the maximum of the two, and when the paired byte reaches 0 it retires a spell (decrementing
; the counter) and, for every free slot si of the six, writes gFD9A[i] into rec[si] of the known
; or memorized group (i+1, or its negation when the counter's other byte ran out), setting gFE2C
; in the latter case. gFD9A and gFDEC are the tables ovl1:00DD fills from the character's spell
; book. Called by ovl1:097A. Purpose unknown: the exact counter semantics.
        PUBLIC  _SpendSpellSlot         ; @ ovl1:025D
_SpendSpellSlot PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,6
        push    si
        push    di
        cmp     word ptr [bp+6],0
        jge     short f025D_026E
        jmp     f025D_0410
f025D_026E:
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        add     ax,OFFSET gChars
        mov     [bp-6],ax
        mov     [bp-4],ds
        cmp     word ptr [bp+8],0
        je      short f025D_0294
        mov     ax,1Eh
        push    ax
        xor     ax,ax
        push    ax
        mov     ax,[bp-6]
        add     ax,55h
        jmp     short f025D_02A1
f025D_0294:
        mov     ax,1Eh
        push    ax
        xor     ax,ax
        push    ax
        mov     ax,[bp-6]
        add     ax,37h
f025D_02A1:
        push    word ptr [bp-4]
        push    ax
        call    far ptr _memset
        add     sp,8
        mov     word ptr [bp-2],0
        jmp     f025D_0407
f025D_02B5:
        mov     bx,[bp-2]
        shl     bx,1
        mov     al,[bx+gFDEC]
        mov     bx,[bp-2]
        shl     bx,1
        cmp     al,[bx+gFDEC+1]
        jge     short f025D_02DB
        mov     bx,[bp-2]
        shl     bx,1
        mov     al,[bx+gFDEC]
        mov     bx,[bp-2]
        shl     bx,1
        mov     [bx+gFDEC+1],al
f025D_02DB:
        mov     bx,[bp-2]
        shl     bx,1
        cmp     byte ptr [bx+gFDEC+1],0
        jne     short f025D_02EA
        jmp     f025D_0370
f025D_02EA:
        mov     bx,[bp-2]
        shl     bx,1
        dec     byte ptr [bx+gFDEC+1]
        mov     bx,[bp-2]
        shl     bx,1
        dec     byte ptr [bx+gFDEC]
        mov     bx,[bp-2]
        mov     al,[bx+gFD9A]
        cbw
        dec     ax
        mov     di,ax
        xor     si,si
        jmp     short f025D_0368
f025D_030B:
        cmp     word ptr [bp+8],0
        je      short f025D_033C
        mov     ax,di
        mov     dx,6
        imul    dx
        les     bx,[bp-6]
        add     bx,ax
        cmp     byte ptr es:[bx+si+55h],0
        jne     short f025D_0367
        mov     ax,di
        mov     dx,6
        imul    dx
        mov     bx,[bp-6]
        add     bx,ax
        mov     al,[bp-2]
        inc     al
        mov     es:[bx+si+55h],al
        jmp     f025D_0401
f025D_033C:
        mov     ax,di
        mov     dx,6
        imul    dx
        les     bx,[bp-6]
        add     bx,ax
        cmp     byte ptr es:[bx+si+37h],0
        jne     short f025D_0367
        mov     ax,di
        mov     dx,6
        imul    dx
        mov     bx,[bp-6]
        add     bx,ax
        mov     al,[bp-2]
        inc     al
        mov     es:[bx+si+37h],al
        jmp     f025D_0401
f025D_0367:
        inc     si
f025D_0368:
        cmp     si,6
        jl      short f025D_030B
        jmp     f025D_0401
f025D_0370:
        mov     bx,[bp-2]
        shl     bx,1
        cmp     byte ptr [bx+gFDEC],0
        jne     short f025D_037F
        jmp     f025D_0404
f025D_037F:
        mov     bx,[bp-2]
        shl     bx,1
        dec     byte ptr [bx+gFDEC]
        mov     bx,[bp-2]
        mov     al,[bx+gFD9A]
        cbw
        dec     ax
        mov     di,ax
        mov     word ptr [gFE2C],1
        xor     si,si
        jmp     short f025D_03FC
f025D_039D:
        cmp     word ptr [bp+8],0
        je      short f025D_03CF
        mov     ax,di
        mov     dx,6
        imul    dx
        les     bx,[bp-6]
        add     bx,ax
        cmp     byte ptr es:[bx+si+55h],0
        jne     short f025D_03FB
        mov     ax,di
        mov     dx,6
        imul    dx
        mov     bx,[bp-6]
        add     bx,ax
        mov     al,[bp-2]
        inc     al
        neg     al
        mov     es:[bx+si+55h],al
        jmp     short f025D_0401
f025D_03CF:
        mov     ax,di
        mov     dx,6
        imul    dx
        les     bx,[bp-6]
        add     bx,ax
        cmp     byte ptr es:[bx+si+37h],0
        jne     short f025D_03FB
        mov     ax,di
        mov     dx,6
        imul    dx
        mov     bx,[bp-6]
        add     bx,ax
        mov     al,[bp-2]
        inc     al
        neg     al
        mov     es:[bx+si+37h],al
        jmp     short f025D_0401
f025D_03FB:
        inc     si
f025D_03FC:
        cmp     si,6
        jl      short f025D_039D
f025D_0401:
        dec     word ptr [bp-2]
f025D_0404:
        inc     word ptr [bp-2]
f025D_0407:
        cmp     word ptr [bp-2],20h
        jge     short f025D_0410
        jmp     f025D_02B5
f025D_0410:
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
        ret
_SpendSpellSlot ENDP

        PUBLIC  _Ovl1_0416               ; @ ovl1:0416 stub
_Ovl1_0416 PROC FAR
        ret
_Ovl1_0416 ENDP

; ovl1:0881: draw the name of item `item` on row `row` in the character display. It picks a base x
; or colour (1, or 0xF when g8102 is nonzero), returns when `row` is negative, computes y =
; row * 9 + 0x32, hides the pointer and, for a nonzero `item`, formats
; sprintf(gFileNameBuf, "%-18s %1d", nameTable[item], count[item]) where nameTable is g12E0 when
; `mode` is set and g1278 otherwise, and count is the byte at gFDEA + item*2. `style` 0 draws it
; through DrawShadowText with the colour from gF57E (or 6 when `hi` is set), nonzero draws it
; through gDrawText in colour `base` (or 6 when `hi` is set); a zero `item` with `style` 0 instead
; fills the row's background through gFillRect in gF57E. The pointer is shown again. Called from
; ovl1:097A. Purpose unknown: the exact meaning of `mode`, `style` and `hi`.
        PUBLIC  _ShowItemName          ; @ ovl1:0881
_ShowItemName PROC FAR
        push    bp
        mov     bp,sp
        push    si
        push    di
        cmp     word ptr [g8102],0
        jne     short f0881_0892
        mov     ax,1
        jmp     short f0881_0895
f0881_0892:
        mov     ax,0Fh
f0881_0895:
        mov     di,ax
        cmp     word ptr [bp+8],0
        jge     short f0881_08A0
        jmp     f0881_0976
f0881_08A0:
        mov     ax,[bp+8]
        mov     dx,9
        imul    dx
        add     ax,32h
        mov     si,ax
        call    far ptr _LockHidePointer
        cmp     word ptr [bp+6],0
        jne     short f0881_08BB
        jmp     f0881_0950
f0881_08BB:
        mov     bx,[bp+6]
        shl     bx,1
        mov     al,[bx+gFDEA]
        cbw
        push    ax
        cmp     word ptr [bp+0Ah],0
        je      short f0881_08DD
        mov     bx,[bp+6]
        shl     bx,1
        shl     bx,1
        mov     dx,word ptr [bx+g12E0+2]
        mov     ax,word ptr [bx+g12E0]
        jmp     short f0881_08EC
f0881_08DD:
        mov     bx,[bp+6]
        shl     bx,1
        shl     bx,1
        mov     dx,word ptr [bx+g1278+2]
        mov     ax,word ptr [bx+g1278]
f0881_08EC:
        push    dx
        push    ax
        push    ds
        mov     ax,OFFSET gFmt645B
        push    ax
        push    ds
        mov     ax,OFFSET gFileNameBuf
        push    ax
        call    far ptr _sprintf
        add     sp,0Eh
        cmp     word ptr [bp+0Ch],0
        je      short f0881_0929
        xor     ax,ax
        push    ax
        cmp     word ptr [bp+0Eh],ax
        je      short f0881_0913
        mov     ax,6
        jmp     short f0881_0915
f0881_0913:
        mov     ax,di
f0881_0915:
        push    ax
        push    si
        mov     ax,8
        push    ax
        push    ds
        mov     ax,OFFSET gFileNameBuf
        push    ax
        call    dword ptr [gDrawText]
        add     sp,0Ch
        jmp     short f0881_0971
f0881_0929:
        mov     al,[gF57E]
        cbw
        push    ax
        cmp     word ptr [bp+0Eh],0
        je      short f0881_0939
        mov     ax,6
        jmp     short f0881_093B
f0881_0939:
        mov     ax,di
f0881_093B:
        push    ax
        push    si
        mov     ax,8
        push    ax
        push    ds
        mov     ax,OFFSET gFileNameBuf
        push    ax
        call    far ptr _DrawShadowText
        add     sp,0Ch
        jmp     short f0881_0971
f0881_0950:
        cmp     word ptr [bp+0Ch],0
        jne     short f0881_0971
        mov     al,[gF57E]
        cbw
        push    ax
        mov     ax,si
        add     ax,8
        push    ax
        mov     ax,0A8h
        push    ax
        push    si
        mov     ax,6
        push    ax
        call    dword ptr [gFillRect]
        add     sp,0Ah
f0881_0971:
        call    far ptr _LockShowPointer
f0881_0976:
        pop     di
        pop     si
        pop     bp
        ret
_ShowItemName ENDP

        PUBLIC  _Ovl1_097A               ; @ ovl1:097A stub
_Ovl1_097A PROC FAR
        ret
_Ovl1_097A ENDP

        PUBLIC  _Ovl1_1153               ; @ ovl1:1153 stub
_Ovl1_1153 PROC FAR
        ret
_Ovl1_1153 ENDP

; ovl1:13F7: show the message whose key is `message` for up to 0x78 frames. It saves g8115, the
; current font and g811F, loads the 8x8 font (gFE5A), hides the pointer, copies gF57E into g159,
; clears text window 0xB, sets g8115 = 2, fills the region through f0D1C_0C21, draws the
; `message`-th string of the far table gStrings2003 (seg 2003:0000) at the text origin +5 through
; DrawShadowText, clips the text rectangle through ClipRect, shows the pointer and then polls
; ReadKey for up to 0x78 frames, flushing the key buffer and stopping on any key (WaitFrames(1) per
; frame otherwise). It restores the font, text window and g8115 and flushes the buffer. Called from
; ovl1:0416, ovl1:1551 and ovl1:1ACD with a message key.
        PUBLIC  _ShowMessage            ; @ ovl1:13F7
_ShowMessage PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,6
        push    si
        push    di
        mov     di,[g8115]
        mov     ax,word ptr [gFont1A47+2]
        mov     dx,word ptr [gFont1A47]
        mov     [bp-6],dx
        mov     [bp-4],ax
        mov     ax,[g811F]
        mov     [bp-2],ax
        push    word ptr [gFE5A+2]
        push    word ptr [gFE5A]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        call    far ptr _LockHidePointer
        mov     al,[gF57E]
        cbw
        mov     [g159],ax
        mov     ax,0Bh
        push    ax
        call    far ptr _SetTextWindowAndClear
        pop     cx
        mov     [g8115],2
        xor     ax,ax
        push    ax
        push    ax
        push    word ptr [gTextHeight]
        mov     ax,[gTextWidth]
        shl     ax,1
        shl     ax,1
        shl     ax,1
        push    ax
        push    word ptr [gTextTop]
        mov     ax,[gTextLeft]
        shl     ax,1
        shl     ax,1
        shl     ax,1
        push    ax
        call    far ptr _f0D1C_0C21
        add     sp,0Ch
        xor     ax,ax
        push    ax
        mov     ax,0Fh
        push    ax
        mov     ax,[gTextTop]
        add     ax,5
        push    ax
        mov     ax,[gTextLeft]
        shl     ax,1
        shl     ax,1
        shl     ax,1
        add     ax,5
        push    ax
        push    word ptr [bp+6]
        mov     ax,SEG gStrings2003
        push    ax
        mov     ax,OFFSET gStrings2003
        push    ax
        call    far ptr _SkipStrings
        add     sp,6
        push    dx
        push    ax
        call    far ptr _DrawShadowText
        add     sp,0Ch
        xor     ax,ax
        push    ax
        mov     ax,2
        push    ax
        push    word ptr [gTextHeight]
        push    word ptr [gTextWidth]
        push    word ptr [gTextTop]
        push    word ptr [gTextLeft]
        push    word ptr [gTextTop]
        push    word ptr [gTextLeft]
        call    far ptr _ClipRect
        add     sp,10h
        call    far ptr _LockShowPointer
        xor     si,si
        jmp     short f13F7_14EC
f13F7_14D1:
        call    far ptr _ReadKey
        or      ax,ax
        je      short f13F7_14E1
        call    far ptr _FlushKeyBuffer
        jmp     short f13F7_14F1
f13F7_14E1:
        mov     ax,1
        push    ax
        call    far ptr _WaitFrames
        pop     cx
        inc     si
f13F7_14EC:
        cmp     si,78h
        jl      short f13F7_14D1
f13F7_14F1:
        mov     [g8115],di
        push    word ptr [bp-4]
        push    word ptr [bp-6]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        push    word ptr [bp-2]
        call    far ptr _SetTextWindow
        pop     cx
        call    far ptr _FlushKeyBuffer
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
        ret
_ShowMessage ENDP

; ovl1:1516: free every block of a NULL-terminated far linked list. `first` is a far pointer
; whose next link is the far pointer held in its first 4 bytes (offset at +0, segment at +2); each
; block is released with farfree and the walk continues with the block's link until the pointer is
; NULL:NULL. Callers pass the malloc'd list head by value (ovl1:0416, ovl1:097A, ovl1:1551,
; ovl1:1ACD, ovl1:243A). Name from what it does; the original name is unknown.
        PUBLIC  _FreeFarList             ; @ ovl1:1516
_FreeFarList PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,4
        jmp     short f1516_1545
f1516_151E:
        les     bx,[bp+6]
        mov     ax,es:[bx+2]
        mov     dx,es:[bx]
        mov     [bp-4],dx
        mov     [bp-2],ax
        push    word ptr [bp+8]
        push    bx
        call    far ptr _farfree
        pop     cx
        pop     cx
        mov     ax,[bp-2]
        mov     dx,[bp-4]
        mov     [bp+6],dx
        mov     [bp+8],ax
f1516_1545:
        mov     ax,[bp+6]
        or      ax,[bp+8]
        jne     short f1516_151E
        mov     sp,bp
        pop     bp
        ret
_FreeFarList ENDP

        PUBLIC  _Ovl1_1551               ; @ ovl1:1551 stub
_Ovl1_1551 PROC FAR
        ret
_Ovl1_1551 ENDP

        PUBLIC  _Ovl1_1ACD               ; @ ovl1:1ACD stub
_Ovl1_1ACD PROC FAR
        ret
_Ovl1_1ACD ENDP

        PUBLIC  _Ovl1_243A               ; @ ovl1:243A stub
_Ovl1_243A PROC FAR
        ret
_Ovl1_243A ENDP

        END
