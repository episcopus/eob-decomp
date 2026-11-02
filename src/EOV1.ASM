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

        EXTRN   gChars:BYTE             ; @ ds:F580: 6 character records of 0xF3 bytes (E08E6.C)
        EXTRN   gBufDCFC:DWORD          ; @ ds:DCFC: 4096-byte scratch allocation (E07A9.C)
        EXTRN   gFillRect:DWORD         ; @ ds:806A: far fill-rect routine (E082C.C)
        EXTRN   gMemberBoxes:WORD       ; @ ds:62E2: two 8-byte (x1,y1,x2,y2) member-list boxes

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

        PUBLIC  _Ovl1_025D               ; @ ovl1:025D stub
_Ovl1_025D PROC FAR
        ret
_Ovl1_025D ENDP

        PUBLIC  _Ovl1_0416               ; @ ovl1:0416 stub
_Ovl1_0416 PROC FAR
        ret
_Ovl1_0416 ENDP

        PUBLIC  _Ovl1_0881               ; @ ovl1:0881 stub
_Ovl1_0881 PROC FAR
        ret
_Ovl1_0881 ENDP

        PUBLIC  _Ovl1_097A               ; @ ovl1:097A stub
_Ovl1_097A PROC FAR
        ret
_Ovl1_097A ENDP

        PUBLIC  _Ovl1_1153               ; @ ovl1:1153 stub
_Ovl1_1153 PROC FAR
        ret
_Ovl1_1153 ENDP

        PUBLIC  _Ovl1_13F7               ; @ ovl1:13F7 stub
_Ovl1_13F7 PROC FAR
        ret
_Ovl1_13F7 ENDP

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
