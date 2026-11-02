; Unit at overlay 1 of EOB.EXE (segment 2102, 9864 bytes, 12 functions), kind asm [H] by the map.
; It is the party display and inventory overlay: it reads and rewrites the party's records at
; ds:F580 (gChars, 6 records of 0xF3 bytes, E08E6.C), draws rows and items through
; DrawRowList/DrawShadowText and the icon table ds:DD28 (gIconDD28, EOV2.C), and searches class
; slots with FindClassSlot and carried slots with FindCarriedSlot (E0D1C.C). ovl1:1ACD has no
; direct caller in the map (it is reached through the overlay dispatch) and calls most of the
; others, including ovl1:0000. The 30 counters at ds:FDEC, the byte table ds:FD9A and the 9-byte
; lists at ds:FDBA are the spell tables ovl1:00DD builds and ovl1:025D steps. Status: ovl1:0000,
; 00DD, 025D, 0416, 0881, 13F7, 1516, 1551 (DrawListNode) and 243A are matched (L2); ovl1:1153 is a
; draft (its far-array reads need TCC's internal assembler, see its registry note: this unit is C);
; ovl1:097A and 1ACD are stubs. Names not proven by an address or a caller come from the code alone
; and are marked in each function's comment. See whitepaper 2.7 and 4.

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
        EXTRN   _F_LXLSH@:FAR           ; @ 0000:77FF: long left shift helper (library)
        EXTRN   _FindClassSlot:FAR      ; @ 0A72:007B (E0A72.C)
        EXTRN   _FindCarriedSlot:FAR    ; @ 0D1C:00F1 (E0D1C.C)
        EXTRN   _DrawRowList:FAR        ; @ 0D1C:0454 (E0D1C.C)
        EXTRN   _DispatchKeyWait:FAR    ; @ 16A5:005A (E0ACD.C)
        EXTRN   _f1A29_000F:FAR         ; @ 1A29:000F (E082C.C)
        EXTRN   _FindNode:FAR           ; @ 1658:01A2 (E1658.C)
        EXTRN   _PointInRect:FAR        ; @ 14EF:00E1 (E14EF.C)
        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007 (E19BC.C)
        EXTRN   _ListAdd:FAR            ; @ 1658:001E (E0CF7.C)
        EXTRN   _movmem:FAR             ; @ 0000:63A3: move n bytes (library)
        EXTRN   _strlen:FAR             ; @ 0000:67F6: string length (library)

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
        EXTRN   gFDBA:BYTE              ; @ ds:FDBA: 9-byte spell-slot list heads, built by ovl1:00DD
        EXTRN   gFDE7:BYTE              ; @ ds:FDE7: 10-byte table cleared by ovl1:00DD
        EXTRN   gFDEB:BYTE              ; @ ds:FDEB: 32 two-byte spell counters, built by ovl1:00DD
        EXTRN   gFDE6:BYTE              ; @ ds:FDE6: 32-byte spell counter, built by ovl1:00DD
        EXTRN   gMap62AF:BYTE           ; @ ds:62AF: spell-level to slot map for class A
        EXTRN   gMap62C9:BYTE           ; @ ds:62C9: spell-level to slot map for class B
        EXTRN   gIconDD28:DWORD         ; @ ds:DD28: far pointer to the icon table (EOV2.C)
        EXTRN   gMouseDisabled:WORD     ; @ ds:814A: nonzero while the mouse is off (E07A9.C)
        EXTRN   gCursorX:WORD           ; @ ds:8138: mouse x in pixels (E14EF.C)
        EXTRN   gCursorY:WORD           ; @ ds:813A: mouse y in pixels (E14EF.C)
        EXTRN   gA9C0:DWORD             ; @ ds:A9C0: far pointer to the 8x8 font (E07A9.C)
        EXTRN   g1231:BYTE              ; @ ds:1231: class bit mask table (E0BD2.C)
        EXTRN   g629B:BYTE              ; @ ds:629B: last command byte (ovl1:0416)
        EXTRN   g62F2:BYTE              ; @ ds:62F2: per-member icon row table (ovl1:0416)
        EXTRN   g62F8:BYTE              ; @ ds:62F8: per-member icon column table (ovl1:0416)
        EXTRN   gFD98:WORD              ; @ ds:FD98: cleared while the member icons are drawn (ovl1:0416)
        EXTRN   g5E20:BYTE              ; @ ds:5E20: nonzero when the mouse moved (E16A5.C)
        EXTRN   gNodeTemplate:BYTE      ; @ ds:3A62: the 0x3c-byte node template (EOV0.C)
        EXTRN   gMemberMenu:BYTE        ; @ ds:628C: six five-byte per-member menu records (this unit)
        EXTRN   gColourTable:BYTE       ; @ ds:00A9: sixteen 16-byte colour records, indexed by attribute << 4
        EXTRN   gSpellInfo9:BYTE        ; @ 208E:0000: nine-byte per-entry text/box records (this unit)

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

; ovl1:00DD: build the spell tables for character `who` from its spell book. It returns when
; `who` is negative; otherwise it clears gFDBA (0x2D), gFDE7 (0x0A), gFDEC (0x40) and gFD9A
; (0x20), copies the class spell-level map (gMap62C9 when `flag`, gMap62AF otherwise) into gFD9A
; with strcpy, and sets the spell-list pointer p to rec+0x55 (memorized) when `flag`, rec+0x37
; (known) otherwise. First it walks up to 0x1E signed bytes of p (positive ones counted in
; gFDEB + 2*c, negatives in gFDEC + 2*(-c) after decrementing) and counts the slot gFD9A[c] in
; gFDE6 + slot.
; Then, for each bit i of `mask` (a long, 1L << i through F_LXLSH@), it takes slot di = gFD9A[i]-1
; and, when the source far array `spells` has a nonzero entry there (or `flag` is clear), appends
; i+1 to the gFDBA record of di (its first byte is the count, then the indices). Called by
; ovl1:097A. Purpose unknown: the counter semantics and why the two maps differ.
        PUBLIC  _BuildSpellSlots        ; @ ovl1:00DD
_BuildSpellSlots PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,0Ah
        push    si
        push    di
        cmp     word ptr [bp+6],0
        jge     short f00DD_00EE
        jmp     f00DD_0257
f00DD_00EE:
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        add     ax,OFFSET gChars
        mov     [bp-6],ax
        mov     [bp-4],ds
        mov     ax,2Dh
        push    ax
        xor     ax,ax
        push    ax
        push    ds
        mov     ax,OFFSET gFDBA
        push    ax
        call    far ptr _memset
        add     sp,8
        mov     ax,0Ah
        push    ax
        xor     ax,ax
        push    ax
        push    ds
        mov     ax,OFFSET gFDE7
        push    ax
        call    far ptr _memset
        add     sp,8
        mov     ax,40h
        push    ax
        xor     ax,ax
        push    ax
        push    ds
        mov     ax,OFFSET gFDEC
        push    ax
        call    far ptr _memset
        add     sp,8
        mov     ax,20h
        push    ax
        xor     ax,ax
        push    ax
        push    ds
        mov     ax,OFFSET gFD9A
        push    ax
        call    far ptr _memset
        add     sp,8
        cmp     word ptr [bp+10h],0
        je      short f00DD_015B
        push    ds
        mov     ax,OFFSET gMap62C9
        jmp     short f00DD_015F
f00DD_015B:
        push    ds
        mov     ax,OFFSET gMap62AF
f00DD_015F:
        push    ax
        push    ds
        mov     ax,OFFSET gFD9A
        push    ax
        call    far ptr _strcpy
        add     sp,8
        mov     ax,[bp-4]
        mov     dx,[bp-6]
        add     dx,37h
        mov     [bp-0Ah],dx
        mov     [bp-8],ax
        cmp     word ptr [bp+10h],0
        je      short f00DD_0191
        mov     ax,[bp-4]
        mov     dx,[bp-6]
        add     dx,55h
        mov     [bp-0Ah],dx
        mov     [bp-8],ax
f00DD_0191:
        mov     word ptr [bp-2],0
        jmp     short f00DD_01D3
f00DD_0198:
        les     bx,[bp-0Ah]
        mov     al,es:[bx]
        cbw
        mov     si,ax
        inc     word ptr [bp-0Ah]
        or      ax,ax
        je      short f00DD_01D0
        or      ax,ax
        jle     short f00DD_01B6
        mov     bx,si
        shl     bx,1
        inc     byte ptr [bx+gFDEB]
        jmp     short f00DD_01BC
f00DD_01B6:
        mov     ax,si
        neg     ax
        mov     si,ax
f00DD_01BC:
        dec     si
        mov     bx,si
        shl     bx,1
        inc     byte ptr [bx+gFDEC]
        mov     al,[si+gFD9A]
        cbw
        mov     bx,ax
        inc     byte ptr [bx+gFDE6]
f00DD_01D0:
        inc     word ptr [bp-2]
f00DD_01D3:
        cmp     word ptr [bp-2],1Eh
        jl      short f00DD_0198
        mov     word ptr [bp-2],0
        jmp     short f00DD_0251
f00DD_01E0:
        xor     dx,dx
        mov     ax,1
        mov     cl,[bp-2]
        call    far ptr _F_LXLSH@
        mov     bx,[bp+0Eh]
        mov     cx,[bp+0Ch]
        and     cx,ax
        and     bx,dx
        or      cx,bx
        je      short f00DD_024E
        mov     bx,[bp-2]
        mov     al,[bx+gFD9A]
        cbw
        dec     ax
        mov     di,ax
        cmp     word ptr [bp+10h],0
        je      short f00DD_0215
        les     bx,[bp+8]
        cmp     byte ptr es:[bx+di],0
        jne     short f00DD_021B
f00DD_0215:
        cmp     word ptr [bp+10h],0
        jne     short f00DD_024E
f00DD_021B:
        or      di,di
        jl      short f00DD_024E
        mov     ax,di
        mov     dx,9
        imul    dx
        mov     bx,ax
        inc     byte ptr [bx+gFDBA]
        mov     ax,di
        mov     dx,9
        imul    dx
        push    ax
        mov     ax,di
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+gFDBA]
        cbw
        pop     bx
        add     bx,ax
        mov     al,[bp-2]
        inc     al
        mov     [bx+gFDBA],al
f00DD_024E:
        inc     word ptr [bp-2]
f00DD_0251:
        cmp     word ptr [bp-2],20h
        jl      short f00DD_01E0
f00DD_0257:
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
        ret
_BuildSpellSlots ENDP

; ovl1:025D: spend one step of the spell counters for character `who`. It returns when `who` is
; negative; otherwise it clears the 30 bytes at rec+0x37 (known slots) or rec+0x55 (memorized
; slots) when `flag` is set, then walks the 32 two-byte counters gFDEC + i*2. It first clamps the
; pair's second byte down to the first when the first is smaller; when the second byte is nonzero
; it decrements both and, for the gFD9A[i]-th slot record of the known or memorized group, writes
; the spell index i+1 into the first free sub-slot si of its six. When the second byte is 0 it
; instead decrements the first byte, sets gFE2C and writes the negation of i+1 the same way.
; gFD9A and gFDEC are the tables
; ovl1:00DD fills from the character's spell book. Called by ovl1:097A. Purpose unknown: the exact
; counter semantics and why one path negates the slot index.
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

; ovl1:0416: act on the character-display command `key` (0x1A, 0x31, 0x35, and the extended keys
; 0x8012..0x8018 among others). It first builds `avail`, the six selectable member slots: a slot is
; selectable when the member is alive (gChars[i] byte +1 bit 0), its byte +0x1B is positive, its
; byte +0x23 is nonzero, and, unless `key` is 0x35, its byte +0x20 selects the `mode` bit (4 for
; key 0x1A, else 2) in the ds:1231 class-mask table. `count` is how many are selectable and `sel`
; is the last one (or -1). With none it shows a message keyed by `key` (0x17 -> 0x1C, 0x1A ->
; 0x1B, 0x31 -> 0x34) and returns -1. With one it draws that member's icons, then checks its class
; slot (FindClassSlot, kind 4): when the slot byte +0x24 is at most 8 it shows message 0x18 and
; returns -1. Otherwise it stores `key` at ds:629B, builds a far key list with _Ovl1_1153(3), sets
; `sel` = -2 and enters the wait loop: DispatchKeyWait on the list, DrawRowList, then a switch on
; the key (key table at ovl1:0839, 18 keys and their case offsets) that moves `cur` to the previous
; or next non-empty slot (0x8013..0x8018 pick a slot directly, 0x43/0x97 accept `cur`, 0x110/0x8012
; call _DrawListNode twice), continuing while `sel` == -2. On exit it frees the list and, for a chosen
; member and a command other than 0x35, checks the class slot (kind 4) when `mode` is 4 and the
; carried slot (FindCarriedSlot, kind 0x1D) when `mode` is 2, showing message 0x18 or 0x19 and
; returning -1 when the check fails. Called from ovl1:097A and ovl1:1551 with a command byte.
; Purpose unknown: what gFD98 marks, what the four f1A29_000F icon draws per member are, and the
; exact command meanings (the 0x80xx keys look like mouse pseudo-keys, the others scan codes).
; Stack frame: [bp+6] `key`; [bp-0x12] `mode`; [bp-0x10] `count`; [bp-8] `sel`; [bp-0xe..-9]
; `avail[6]`; [bp-4]/[bp-2] the far `list`; [bp-6] the key read in the loop; [bp-0x16] `slot`;
; [bp-0x14] `hit`; si `i`; di `cur`.
        PUBLIC  _MemberCommand           ; @ ovl1:0416
_MemberCommand PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,16h
        push    si
        push    di
        cmp     word ptr [bp+6],1Ah
        jne     short f0416_0429
        mov     ax,4
        jmp     short f0416_042C
f0416_0429:
        mov     ax,2
f0416_042C:
        mov     [bp-12h],ax
        call    far ptr _FlushKeyBuffer
        mov     word ptr [gFD98],0
        mov     word ptr [bp-10h],0
        mov     word ptr [bp-8],0FFFFh
        xor     si,si
        jmp     short f0416_04B7
f0416_0448:
        mov     byte ptr [bp+si-0Eh],0FFh
        mov     ax,si
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        test    byte ptr [bx+gChars+1],1
        je      short f0416_0496
        mov     ax,si
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        cmp     byte ptr [bx+gChars+1Bh],0
        jle     short f0416_0496
        mov     ax,si
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        cmp     byte ptr [bx+gChars+23h],0
        je      short f0416_0496
        mov     ax,si
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gChars+20h]
        cbw
        mov     bx,ax
        mov     al,byte ptr [bx+g1231]
        cbw
        test    word ptr [bp-12h],ax
        jne     short f0416_04AC
f0416_0496:
        cmp     word ptr [bp+6],35h
        jne     short f0416_04B6
        mov     ax,si
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        test    byte ptr [bx+gChars+1],1
        je      short f0416_04B6
f0416_04AC:
        mov     byte ptr [bp+si-0Eh],0
        inc     word ptr [bp-10h]
        mov     [bp-8],si
f0416_04B6:
        inc     si
f0416_04B7:
        cmp     si,6
        jl      short f0416_0448
        cmp     word ptr [bp-10h],0
        jne     short f0416_04FA
        mov     ax,[bp+6]
        cmp     ax,17h
        je      short f0416_04DD
        cmp     ax,1Ah
        je      short f0416_04D6
        cmp     ax,31h
        je      short f0416_04E4
        jmp     short f0416_04E9
f0416_04D6:
        mov     word ptr [bp-10h],1Bh
        jmp     short f0416_04E9
f0416_04DD:
        mov     word ptr [bp-10h],1Ch
        jmp     short f0416_04E9
f0416_04E4:
        mov     word ptr [bp-10h],34h
f0416_04E9:
        push    word ptr [bp-10h]
        nop
        push    cs
        call    near ptr _ShowMessage
        pop     cx
        mov     word ptr [bp-8],0FFFFh
        jmp     f0416_0830
f0416_04FA:
        xor     si,si
        jmp     f0416_05D3
f0416_04FF:
        cmp     byte ptr [bp+si-0Eh],0FFh
        je      short f0416_0508
        jmp     f0416_05D2
f0416_0508:
        mov     ax,1
        push    ax
        push    si
        call    far ptr _f0D1C_0670
        pop     cx
        pop     cx
        or      ax,ax
        jne     short f0416_051B
        jmp     f0416_05D2
f0416_051B:
        xor     ax,ax
        push    ax
        mov     al,byte ptr [si+g62F8]
        mov     ah,0
        push    ax
        mov     al,byte ptr [si+g62F2]
        mov     ah,0
        shl     ax,1
        shl     ax,1
        shl     ax,1
        push    ax
        push    word ptr [gIconDD28+2]
        push    word ptr [gIconDD28]
        xor     ax,ax
        push    ax
        call    far ptr _f1A29_000F
        add     sp,0Ch
        xor     ax,ax
        push    ax
        mov     al,byte ptr [si+g62F8]
        mov     ah,0
        push    ax
        mov     al,byte ptr [si+g62F2]
        mov     ah,0
        shl     ax,1
        shl     ax,1
        shl     ax,1
        add     ax,10h
        push    ax
        push    word ptr [gIconDD28+2]
        push    word ptr [gIconDD28]
        xor     ax,ax
        push    ax
        call    far ptr _f1A29_000F
        add     sp,0Ch
        xor     ax,ax
        push    ax
        mov     al,byte ptr [si+g62F8]
        mov     ah,0
        push    ax
        mov     al,byte ptr [si+g62F2]
        mov     ah,0
        shl     ax,1
        shl     ax,1
        shl     ax,1
        add     ax,20h
        push    ax
        push    word ptr [gIconDD28+2]
        push    word ptr [gIconDD28]
        xor     ax,ax
        push    ax
        call    far ptr _f1A29_000F
        add     sp,0Ch
        xor     ax,ax
        push    ax
        mov     al,byte ptr [si+g62F8]
        mov     ah,0
        push    ax
        mov     al,byte ptr [si+g62F2]
        mov     ah,0
        shl     ax,1
        shl     ax,1
        shl     ax,1
        add     ax,30h
        push    ax
        push    word ptr [gIconDD28+2]
        push    word ptr [gIconDD28]
        xor     ax,ax
        push    ax
        call    far ptr _f1A29_000F
        add     sp,0Ch
        mov     word ptr [gFD98],1
f0416_05D2:
        inc     si
f0416_05D3:
        cmp     si,6
        jge     short f0416_05DB
        jmp     f0416_04FF
f0416_05DB:
        cmp     word ptr [bp-10h],1
        jne     short f0416_062D
        mov     ax,[bp-8]
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gChars+20h]
        cbw
        push    ax
        mov     ax,4
        push    ax
        call    far ptr _FindClassSlot
        pop     cx
        pop     cx
        mov     [bp-16h],ax
        cmp     ax,0FFFFh
        je      short f0416_062A
        mov     ax,[bp-8]
        mov     dx,0F3h
        imul    dx
        add     ax,[bp-16h]
        mov     bx,ax
        cmp     byte ptr [bx+gChars+24h],8
        jg      short f0416_062A
        mov     ax,18h
        push    ax
        nop
        push    cs
        call    near ptr _ShowMessage
        pop     cx
        mov     word ptr [bp-8],0FFFFh
        jmp     f0416_0830
f0416_062A:
        jmp     f0416_0830
f0416_062D:
        mov     al,byte ptr [bp+6]
        mov     byte ptr [g629B],al
        mov     ax,3
        push    ax
        nop
        push    cs
        call    near ptr _Ovl1_1153
        pop     cx
        mov     [bp-4],ax
        mov     [bp-2],dx
        mov     word ptr [bp-8],0FFFEh
        xor     di,di
        jmp     short f0416_0655
f0416_064C:
        mov     al,byte ptr [bp+di-0Eh]
        cbw
        or      ax,ax
        je      short f0416_065A
        inc     di
f0416_0655:
        cmp     di,6
        jl      short f0416_064C
f0416_065A:
        cmp     word ptr [gMouseDisabled],0
        jne     short f0416_0664
        mov     di,0FFFFh
f0416_0664:
        push    word ptr [gA9C0+2]
        push    word ptr [gA9C0]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        jmp     f0416_076E
f0416_0676:
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        call    far ptr _DispatchKeyWait
        pop     cx
        pop     cx
        mov     [bp-6],ax
        push    di
        call    far ptr _DrawRowList
        pop     cx
        mov     ax,[bp-6]
        mov     [bp-14h],ax
        mov     cx,12h
        mov     bx,839h
f0416_0699:
        mov     ax,word ptr cs:[bx]
        cmp     ax,[bp-14h]
        je      short f0416_06A8
        inc     bx
        inc     bx
        loop    f0416_0699
        jmp     f0416_076E
f0416_06A8:
        jmp     word ptr cs:[bx+24h]
f0416_06AC:
        mov     ax,0FFFFh
        push    ax
        call    far ptr _DrawRowList
        pop     cx
        push    di
        call    far ptr _RedrawMember
        pop     cx
        dec     di
        or      di,di
        jge     short f0416_06CC
        jmp     short f0416_06C9
f0416_06C4:
        dec     di
        or      di,di
        jge     short f0416_06CC
f0416_06C9:
        mov     di,5
f0416_06CC:
        cmp     byte ptr [bp+di-0Eh],0
        jne     short f0416_06C4
        jmp     f0416_076E
f0416_06D5:
        mov     ax,0FFFFh
        push    ax
        call    far ptr _DrawRowList
        pop     cx
        push    di
        call    far ptr _RedrawMember
        pop     cx
        inc     di
        cmp     di,6
        jl      short f0416_06F6
        jmp     short f0416_06F4
f0416_06EE:
        inc     di
        cmp     di,6
        jl      short f0416_06F6
f0416_06F4:
        xor     di,di
f0416_06F6:
        cmp     byte ptr [bp+di-0Eh],0
        jne     short f0416_06EE
        jmp     short f0416_076E
f0416_06FE:
        push    word ptr [gFE5A+2]
        push    word ptr [gFE5A]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        mov     ax,1
        push    ax
        push    ax
        push    ax
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        nop
        push    cs
        call    near ptr _DrawListNode
        add     sp,0Ah
        mov     ax,5
        push    ax
        call    far ptr _WaitFrames
        pop     cx
        mov     ax,1
        push    ax
        xor     ax,ax
        push    ax
        push    ax
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        nop
        push    cs
        call    near ptr _DrawListNode
        add     sp,0Ah
        mov     word ptr [bp-8],0FFFFh
        jmp     f0416_0664
f0416_0749:
        mov     ax,[bp-6]
        sub     ax,8013h
        mov     [bp-8],ax
        lea     ax,[bp-0Eh]
        mov     bx,[bp-8]
        add     bx,ax
        cmp     byte ptr ss:[bx],0
        je      short f0416_076E
        mov     word ptr [bp-8],0FFFEh
        jmp     short f0416_076E
f0416_0767:
        or      di,di
        jl      short f0416_076E
        mov     [bp-8],di
f0416_076E:
        cmp     word ptr [bp-8],-2
        jne     short f0416_0777
        jmp     f0416_0676
f0416_0777:
        mov     ax,0FFFFh
        push    ax
        call    far ptr _DrawRowList
        pop     cx
        or      di,di
        je      short f0416_078C
        push    di
        call    far ptr _RedrawMember
        pop     cx
f0416_078C:
        push    word ptr [gFE5A+2]
        push    word ptr [gFE5A]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        cmp     word ptr [bp-8],-1
        jne     short f0416_07A4
        jmp     f0416_0823
f0416_07A4:
        cmp     word ptr [bp+6],35h
        je      short f0416_0823
        cmp     word ptr [bp-12h],4
        jne     short f0416_07F6
        mov     ax,[bp-8]
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gChars+20h]
        cbw
        push    ax
        mov     ax,4
        push    ax
        call    far ptr _FindClassSlot
        pop     cx
        pop     cx
        mov     [bp-16h],ax
        cmp     ax,0FFFFh
        je      short f0416_07F6
        mov     ax,[bp-8]
        mov     dx,0F3h
        imul    dx
        add     ax,[bp-16h]
        mov     bx,ax
        cmp     byte ptr [bx+gChars+24h],8
        jg      short f0416_07F6
        mov     ax,18h
        push    ax
        nop
        push    cs
        call    near ptr _ShowMessage
        pop     cx
        mov     word ptr [bp-8],0FFFFh
f0416_07F6:
        cmp     word ptr [bp-12h],2
        jne     short f0416_0823
        mov     ax,0FFFFh
        push    ax
        mov     ax,1Dh
        push    ax
        push    word ptr [bp-8]
        call    far ptr _FindCarriedSlot
        add     sp,6
        cmp     ax,0FFFFh
        jne     short f0416_0823
        mov     ax,19h
        push    ax
        nop
        push    cs
        call    near ptr _ShowMessage
        pop     cx
        mov     word ptr [bp-8],0FFFFh
f0416_0823:
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        nop
        push    cs
        call    near ptr _FreeFarList
        pop     cx
        pop     cx
f0416_0830:
        mov     ax,[bp-8]
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
        ret
f0416_0839:
        dw      12h, 1Fh, 20h, 2Bh, 2Eh, 5Ch, 60h, 61h, 62h, 66h, 6Eh
        dw      8012h, 8013h, 8014h, 8015h, 8016h, 8017h, 8018h
        dw      f0416_06AC, f0416_06AC, f0416_06D5, f0416_0767, f0416_06D5, f0416_06AC
        dw      f0416_06AC, f0416_0767, f0416_06D5, f0416_06D5, f0416_06FE, f0416_06FE
        dw      f0416_0749, f0416_0749, f0416_0749, f0416_0749, f0416_0749, f0416_0749
_MemberCommand ENDP

; ovl1:0881: draw the name of item `item` on row `row` in the character display. It picks a base x
; or colour (1, or 0xF when g8102 is nonzero), returns when `row` is negative, computes y =
; row * 9 + 0x32, hides the pointer and, for a nonzero `item`, formats
; sprintf(gFileNameBuf, "%-18s %1d", nameTable[item], count[item]) where nameTable is g12E0 when
; `mode` is set and g1278 otherwise, and count is the byte at gFDEA + item*2. `style` 0 draws it
; through DrawShadowText in colour `base` (or 6 when `hi` is set) with extra gF57E, nonzero draws
; it through gDrawText in the same colour with extra 0; a zero `item` with `style` 0 instead fills
; the row's background through gFillRect in gF57E. The pointer is shown again. Called from
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

; ovl1:1153: build and return the far linked list of the menu rows for member `who`. It reads the
; five-byte gMemberMenu[who] record (message index, attribute, first table index, row count, colour),
; tints gColourTable[attribute] with gF57E, clears text window `attribute` and fills it through
; f0D1C_0C21, then draws the record's message (gStrings2003[message index] through SkipStrings and
; DrawShadowText). For i in 0..count-1 it allocates a 0x3c-byte node, copies gNodeTemplate over it
; and fills it from gSpellInfo9[first + i]: key index+1, key06 = rec+5, key08 = rec+5+0x100, x =
; rec+1<<3, y = rec+2, width = rec+3<<3, height = rec+4, flags 0x1100, the far record pointer at
; +0x34/+0x36 and rec+7 at +0x38; the node is drawn once through _DrawListNode and chained with
; ListAdd. At the end it clips the text window, sets g5E20, flushes the key buffer and clears g8115.
; Called by ovl1:0416, 097A, 1551, 1ACD and 243A with a member index and returns the list head.
; Purpose unknown: the exact meaning of the menu fields and of the 9-byte record bytes.
        PUBLIC  _Ovl1_1153              ; @ ovl1:1153 draft
_Ovl1_1153 PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,0Ah
        push    si
        push    di
        mov     di,[bp+6]
        mov     word ptr [g8115],2
        mov     ax,di
        mov     dx,5
        imul    dx
        mov     bx,ax
        cmp     byte ptr [bx+gMemberMenu+1],0
        je      short f1153_11CD
        mov     ax,di
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gMemberMenu+1]
        cbw
        mov     cl,4
        shl     ax,cl
        push    ax
        mov     al,[gF57E]
        cbw
        pop     bx
        mov     word ptr [bx+gColourTable],ax
        mov     ax,di
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gMemberMenu+1]
        cbw
        push    ax
        call    far ptr _SetTextWindowAndClear
        pop     cx
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
f1153_11CD:
        xor     ax,ax
        push    ax
        mov     ax,di
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gMemberMenu+4]
        cbw
        push    ax
        mov     ax,5
        push    ax
        push    ax
        mov     ax,di
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gMemberMenu]
        cbw
        push    ax
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
        mov     word ptr [bp-6],0
        mov     word ptr [bp-4],0
        mov     word ptr [bp-2],0
        jmp     f1153_1394
f1153_121F:
        mov     ax,di
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gMemberMenu+2]
        cbw
        mov     dx,[bp-2]
        add     dx,ax
        mov     si,dx
        xor     ax,ax
        push    ax
        mov     dx,3Ch
        push    ax
        push    dx
        call    far ptr _MemAlloc
        add     sp,6
        mov     [bp-0Ah],ax
        mov     [bp-8],dx
        mov     ax,3Ch
        push    ax
        push    dx
        push    word ptr [bp-0Ah]
        push    ds
        mov     ax,OFFSET gNodeTemplate
        push    ax
        call    far ptr _movmem
        add     sp,0Ah
        les     bx,[bp-0Ah]
        mov     ax,si
        inc     ax
        mov     word ptr es:[bx+4],ax
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     dx,SEG gSpellInfo9
        mov     bx,ax
        mov     es,dx
        NOSMART
        mov     ax,word ptr es:[bx+5]
        SMART
        les     bx,[bp-0Ah]
        mov     word ptr es:[bx+6],ax
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     dx,SEG gSpellInfo9
        mov     bx,ax
        mov     es,dx
        NOSMART
        mov     ax,word ptr es:[bx+5]
        SMART
        add     ax,100h
        les     bx,[bp-0Ah]
        mov     word ptr es:[bx+8],ax
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     dx,SEG gSpellInfo9
        mov     bx,ax
        mov     es,dx
        NOSMART
        mov     al,byte ptr es:[bx+1]
        SMART
        mov     ah,0
        shl     ax,1
        shl     ax,1
        shl     ax,1
        les     bx,[bp-0Ah]
        mov     word ptr es:[bx+1Eh],ax
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     dx,SEG gSpellInfo9
        mov     bx,ax
        mov     es,dx
        NOSMART
        mov     al,byte ptr es:[bx+2]
        SMART
        mov     ah,0
        les     bx,[bp-0Ah]
        mov     word ptr es:[bx+20h],ax
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     dx,SEG gSpellInfo9
        mov     bx,ax
        mov     es,dx
        NOSMART
        mov     al,byte ptr es:[bx+3]
        SMART
        mov     ah,0
        shl     ax,1
        shl     ax,1
        shl     ax,1
        les     bx,[bp-0Ah]
        mov     word ptr es:[bx+22h],ax
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     dx,SEG gSpellInfo9
        mov     bx,ax
        mov     es,dx
        NOSMART
        mov     al,byte ptr es:[bx+4]
        SMART
        mov     ah,0
        les     bx,[bp-0Ah]
        mov     word ptr es:[bx+24h],ax
        mov     ax,si
        mov     dx,9
        imul     dx
        NOSMART
        add     ax,0
        SMART
        mov     word ptr es:[bx+34h],ax
        mov     word ptr es:[bx+36h],SEG gSpellInfo9
        mov     word ptr es:[bx+0Eh],1100h
        mov     ax,si
        mov     dx,9
        imul     dx
        mov     dx,SEG gSpellInfo9
        mov     bx,ax
        mov     es,dx
        NOSMART
        mov     ax,word ptr es:[bx+7]
        SMART
        les     bx,[bp-0Ah]
        mov     word ptr es:[bx+38h],ax
        xor     ax,ax
        push    ax
        push    ax
        push    ax
        push    word ptr [bp-8]
        push    bx
        nop
        push    cs
        call    near ptr _DrawListNode
        add     sp,0Ah
        mov     ax,[bp-6]
        or      ax,[bp-4]
        je      short f1153_1385
        push    word ptr [bp-8]
        push    word ptr [bp-0Ah]
        push    word ptr [bp-4]
        push    word ptr [bp-6]
        call    far ptr _ListAdd
        add     sp,8
        mov     [bp-6],ax
        mov     [bp-4],dx
        jmp     short f1153_1391
f1153_1385:
        mov     ax,[bp-8]
        mov     dx,[bp-6]
        mov     [bp-6],dx
        mov     [bp-4],ax
f1153_1391:
        inc     word ptr [bp-2]
f1153_1394:
        mov     ax,di
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,byte ptr [bx+gMemberMenu+3]
        cbw
        cmp     ax,[bp-2]
        jle     short f1153_13AA
        jmp     f1153_121F
f1153_13AA:
        call    far ptr _LockHidePointer
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
        mov     byte ptr [g5E20],1
        call    far ptr _FlushKeyBuffer
        mov     word ptr [g8115],0
        mov     dx,[bp-4]
        mov     ax,[bp-6]
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
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

; ovl1:1551: draw the list node `node` (a far pointer to a 60-byte panel record, src/EOV0.C's
; struct Node) in the text window. It returns at once when `node` is null. `colour` is 1, or 0xF
; when g8102 is nonzero. It reads the node's far record pointer (node+0x34/+0x36), hides the
; pointer and, when the record's word +7 has bit 0, fills the node's box (node+0x1E..+0x24) through
; f0D1C_0C21 with the caller's arguments 2 and 3. It then draws the record's string:
; SkipStrings(gStrings2003, rec[0]) returns it and, when the record's word +7 has bit 2, it is
; centred in the box (di = (node+0x22 - 8*strlen)/2 + 1, si = (node+0x24 - 7)/2), otherwise at
; (4, 3). The draw colour is 6 when argument 4 is nonzero, else `colour`; the text goes through the
; text routine gDrawText when arguments 4 and 5 are both nonzero, else through DrawShadowText. The
; pointer is shown again. Called from ovl1:0416, 097A, 1153, 1ACD and 243A. Purpose unknown: the
; meaning of the three mode arguments and of the record flag bits.
        PUBLIC  _DrawListNode           ; @ ovl1:1551
_DrawListNode PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,0Ah
        push    si
        push    di
        mov     ax,[bp+6]
        or      ax,[bp+8]
        jne     short f1551_1564
        jmp     f1551_168B
f1551_1564:
        cmp     word ptr [g8102],0
        jne     short f1551_1570
        mov     ax,1
        jmp     short f1551_1573
f1551_1570:
        mov     ax,0Fh
f1551_1573:
        mov     [bp-0Ah],ax
        les     bx,[bp+6]
        mov     ax,word ptr es:[bx+36h]
        mov     dx,word ptr es:[bx+34h]
        mov     [bp-4],dx
        mov     [bp-2],ax
        call    far ptr _LockHidePointer
        les     bx,[bp-4]
        test    word ptr es:[bx+7],1
        je      short f1551_15B8
        push    word ptr [bp+0Eh]
        push    word ptr [bp+0Ah]
        les     bx,[bp+6]
        push    word ptr es:[bx+24h]
        push    word ptr es:[bx+22h]
        push    word ptr es:[bx+20h]
        push    word ptr es:[bx+1Eh]
        call    far ptr _f0D1C_0C21
        add     sp,0Ch
f1551_15B8:
        les     bx,[bp-4]
        cmp     byte ptr es:[bx],0
        jne     short f1551_15C4
        jmp     f1551_1686
f1551_15C4:
        mov     al,byte ptr es:[bx]
        cbw
        push    ax
        mov     ax,SEG gStrings2003
        push    ax
        mov     ax,OFFSET gStrings2003
        push    ax
        call    far ptr _SkipStrings
        add     sp,6
        mov     [bp-8],ax
        mov     [bp-6],dx
        les     bx,[bp-4]
        test    word ptr es:[bx+7],4
        je      short f1551_1616
        push    dx
        push    word ptr [bp-8]
        call    far ptr _strlen
        pop     cx
        pop     cx
        shl     ax,1
        shl     ax,1
        shl     ax,1
        les     bx,[bp+6]
        mov     dx,word ptr es:[bx+22h]
        sub     dx,ax
        shr     dx,1
        inc     dx
        mov     di,dx
        mov     ax,word ptr es:[bx+24h]
        NOSMART
        sub     ax,7
        SMART
        shr     ax,1
        mov     si,ax
        jmp     short f1551_161C
f1551_1616:
        mov     di,4
        mov     si,3
f1551_161C:
        cmp     word ptr [bp+0Eh],0
        jne     short f1551_1628
        cmp     word ptr [bp+0Ah],0
        je      short f1551_1656
f1551_1628:
        xor     ax,ax
        push    ax
        cmp     word ptr [bp+0Ch],ax
        je      short f1551_1635
        mov     ax,6
        jmp     short f1551_1638
f1551_1635:
        mov     ax,[bp-0Ah]
f1551_1638:
        push    ax
        les     bx,[bp+6]
        mov     ax,word ptr es:[bx+20h]
        add     ax,si
        push    ax
        mov     ax,word ptr es:[bx+1Eh]
        add     ax,di
        push    ax
        push    word ptr [bp-6]
        push    word ptr [bp-8]
        call    dword ptr [gDrawText]
        jmp     short f1551_1683
f1551_1656:
        xor     ax,ax
        push    ax
        cmp     word ptr [bp+0Ch],ax
        je      short f1551_1663
        mov     ax,6
        jmp     short f1551_1666
f1551_1663:
        mov     ax,[bp-0Ah]
f1551_1666:
        push    ax
        les     bx,[bp+6]
        mov     ax,word ptr es:[bx+20h]
        add     ax,si
        push    ax
        mov     ax,word ptr es:[bx+1Eh]
        add     ax,di
        push    ax
        push    word ptr [bp-6]
        push    word ptr [bp-8]
        call    far ptr _DrawShadowText
f1551_1683:
        add     sp,0Ch
f1551_1686:
        call    far ptr _LockShowPointer
f1551_168B:
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
        ret
_DrawListNode ENDP

        PUBLIC  _Ovl1_1691               ; @ ovl1:1691 stub
_Ovl1_1691 PROC FAR
        ret
_Ovl1_1691 ENDP

        PUBLIC  _Ovl1_1ACD               ; @ ovl1:1ACD stub
_Ovl1_1ACD PROC FAR
        ret
_Ovl1_1ACD ENDP

; ovl1:243A: show the gStrings2003 message `message` as a header and a scrollable list of the
; member inventory slots, letting the player pick one with the keyboard or the mouse. It saves
; g811F and the current font, clears g8115, selects the 8x8 font gFE5A, builds a far list of the
; nodes whose key is `cur+0x21` with _Ovl1_1153(5), hides the pointer, draws the message through
; SkipStrings(gStrings2003, message) and DrawShadowText, then loops: it highlights the current node
; with _DrawListNode (after _FindNode(index+0x21)), waits with DispatchKeyWait, and switches on the
; key (table at ovl1:266C, 7 keys): 0x5C/0x66 toggle `di`, 0x2B/0x3D/0x61 remove the node and walk
; the list to the node under the mouse (gCursorX/gCursorY through PointInRect), 0x8021/0x8022 end
; the loop with a result flag. On exit it frees the list, restores the font and g811F through
; _f1A47_0169 and SetTextWindow and returns the flag. Called from ovl1:1ACD and the camp loop
; (ovl6:0000). Purpose unknown: what the returned flag means and which menu `message` names.
; Stack frame: [bp+6] `message`; [bp-0xa] saved g811F; [bp-0xe]/[bp-0xc] the saved font far pointer
; (offset/segment); [bp-4]/[bp-2] the far `list` head (offset/segment); [bp-8]/[bp-6] the far list
; cursor; [bp-0x12] `looping`; [bp-0x10]/[bp-0x16] the key; [bp-0x14] the result; si `selected`;
; di `cur`.
        PUBLIC  _ChooseListEntry        ; @ ovl1:243A
_ChooseListEntry PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,16h
        push    si
        push    di
        mov     ax,[g811F]
        mov     [bp-0Ah],ax
        mov     ax,word ptr [gFont1A47+2]
        mov     dx,word ptr [gFont1A47]
        mov     [bp-0Eh],dx
        mov     [bp-0Ch],ax
        mov     word ptr [g8115],0
        push    word ptr [gFE5A+2]
        push    word ptr [gFE5A]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        mov     ax,5
        push    ax
        call    far ptr _Ovl1_1153
        pop     cx
        mov     [bp-4],ax
        mov     [bp-2],dx
        call    far ptr _LockHidePointer
        xor     ax,ax
        push    ax
        mov     ax,0Fh
        push    ax
        mov     ax,[gTextTop]
        add     ax,4
        push    ax
        mov     ax,[gTextLeft]
        inc     ax
        shl     ax,1
        shl     ax,1
        shl     ax,1
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
        call    far ptr _LockShowPointer
        mov     si,0FFFFh
        xor     di,di
        mov     word ptr [bp-12h],1
        jmp     f243A_2638
f243A_24C6:
        cmp     di,si
        je      short f243A_251B
        cmp     si,-1
        je      short f243A_24F4
        mov     ax,1
        push    ax
        xor     ax,ax
        push    ax
        push    ax
        mov     ax,si
        add     ax,21h
        push    ax
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        call    far ptr _FindNode
        add     sp,6
        push    dx
        push    ax
        call    far ptr _DrawListNode
        add     sp,0Ah
f243A_24F4:
        mov     ax,1
        push    ax
        push    ax
        xor     ax,ax
        push    ax
        mov     ax,di
        add     ax,21h
        push    ax
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        call    far ptr _FindNode
        add     sp,6
        push    dx
        push    ax
        call    far ptr _DrawListNode
        add     sp,0Ah
        mov     si,di
f243A_251B:
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        call    far ptr _DispatchKeyWait
        pop     cx
        pop     cx
        mov     [bp-10h],ax
        mov     [bp-16h],ax
        mov     cx,7
        mov     bx,266Ch
f243A_2534:
        mov     ax,word ptr cs:[bx]
        cmp     ax,[bp-16h]
        je      short f243A_2542
        inc     bx
        inc     bx
        loop    f243A_2534
        jmp     short f243A_2578
f243A_2542:
        jmp     word ptr cs:[bx+0Eh]
f243A_2546:
        NOSMART                         ; the compiler kept the 16-bit immediate form here
        xor     di,1
        SMART
        jmp     f243A_25E6
f243A_254D:
        or      si,si
        jne     short f243A_2556
        mov     ax,1
        jmp     short f243A_2558
f243A_2556:
        xor     ax,ax
f243A_2558:
        mov     [bp-14h],ax
        mov     ax,si
        add     ax,8021h
        mov     [bp-10h],ax
        jmp     short f243A_2571
f243A_2565:
        mov     word ptr [bp-14h],1
        jmp     short f243A_2571
f243A_256C:
        mov     word ptr [bp-14h],0
f243A_2571:
        mov     word ptr [bp-12h],0
        jmp     short f243A_25E6
f243A_2578:
        cmp     word ptr [gMouseDisabled],0
        jne     short f243A_25E6
        mov     ax,[bp-2]
        mov     dx,[bp-4]
        jmp     short f243A_25D8
f243A_2587:
        les     bx,[bp-8]
        test    word ptr es:[bx+38h],2
        je      short f243A_25CE
        mov     ax,es:[bx+20h]
        add     ax,es:[bx+24h]
        push    ax
        mov     ax,es:[bx+1Eh]
        add     ax,es:[bx+22h]
        push    ax
        push    word ptr es:[bx+20h]
        push    word ptr es:[bx+1Eh]
        push    word ptr [gCursorY]
        push    word ptr [gCursorX]
        call    far ptr _PointInRect
        add     sp,0Ch
        or      ax,ax
        je      short f243A_25CE
        les     bx,[bp-8]
        mov     ax,es:[bx+4]
        sub     ax,21h
        mov     di,ax
        jmp     short f243A_25E6
f243A_25CE:
        les     bx,[bp-8]
        mov     ax,es:[bx+2]
        mov     dx,es:[bx]
f243A_25D8:
        mov     [bp-8],dx
        mov     [bp-6],ax
        mov     ax,[bp-8]
        or      ax,[bp-6]
        jne     short f243A_2587
f243A_25E6:
        cmp     word ptr [bp-12h],0
        jne     short f243A_2638
        mov     ax,si
        add     ax,21h
        push    ax
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        call    far ptr _FindNode
        add     sp,6
        mov     [bp-8],ax
        mov     [bp-6],dx
        mov     ax,1
        push    ax
        push    ax
        push    ax
        push    dx
        push    word ptr [bp-8]
        call    far ptr _DrawListNode
        add     sp,0Ah
        mov     ax,5
        push    ax
        call    far ptr _WaitFrames
        pop     cx
        mov     ax,1
        push    ax
        push    ax
        xor     ax,ax
        push    ax
        push    word ptr [bp-6]
        push    word ptr [bp-8]
        call    far ptr _DrawListNode
        add     sp,0Ah
        mov     di,si
f243A_2638:
        cmp     word ptr [bp-12h],0
        je      short f243A_2641
        jmp     f243A_24C6
f243A_2641:
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        call    far ptr _FreeFarList
        pop     cx
        pop     cx
        push    word ptr [bp-0Ch]
        push    word ptr [bp-0Eh]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        push    word ptr [bp-0Ah]
        call    far ptr _SetTextWindow
        pop     cx
        mov     ax,[bp-14h]
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
        ret
f243A_266C:
        dw      2Bh, 3Dh, 5Ch, 61h, 66h, 8021h, 8022h
        dw      f243A_254D, f243A_254D, f243A_2546, f243A_254D, f243A_2546
        dw      f243A_2565, f243A_256C
_ChooseListEntry ENDP

        END
