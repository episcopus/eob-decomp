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

        .CODE

        PUBLIC  _Ovl1_0000               ; @ ovl1:0000 stub
_Ovl1_0000 PROC FAR
        ret
_Ovl1_0000 ENDP

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
