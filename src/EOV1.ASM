; Unit at overlay 1 of EOB.EXE (segment 2102, 9864 bytes, 16 functions). The unit is C (escalation
; eov1, 2026-11-02; src/EOV1.C): this file is the stopgap transcription from the disassembly, kept
; until src/EOV1.C has each function in C (playbook 30).
; It is the party display and inventory overlay: it reads and rewrites the party's records at
; ds:F580 (gChars, 6 records of 0xF3 bytes, E08E6.C), draws rows and items through
; DrawRowList/DrawShadowText and the icon table ds:DD28 (gIconDD28, EOV2.C), and searches class
; slots with FindClassSlot and carried slots with FindCarriedSlot (E0D1C.C). ovl1:1ACD has no
; direct caller in the map (it is reached through the overlay dispatch) and calls most of the
; others, including ovl1:0000. The 30 counters at ds:FDEC, the byte table ds:FD9A and the 9-byte
; lists at ds:FDBA are the spell tables ovl1:00DD builds and ovl1:025D steps. Status: ovl1:0000,
; 00DD, 025D, 0416, 0881, 097A (the spell-slot screen, with its three option-field helpers at
; 10DE/1107/112D), 13F7, 1516, 1551 (DrawListNode), 1691 and 243A are matched (L2); ovl1:1153
; and 1ACD are drafts (each has a far-array read that needs TCC's internal assembler; see their
; registry notes: this unit is C). Names not proven
; by an address or a caller come from the code alone and are marked in each function's comment.
; See whitepaper 2.7 and 4.

        .MODEL  LARGE

        EXTRN   _farfree:FAR            ; @ 0000:71EE: free a far block (library)
        EXTRN   _SwapBlocks:FAR       ; @ 082C:07C5 (E082C.C)
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
        EXTRN   _IsPointInRect:FAR        ; @ 14EF:00E1 (E14EF.C)
        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007 (E19BC.C)
        EXTRN   _ListAdd:FAR            ; @ 1658:001E (E0CF7.C)
        EXTRN   _movmem:FAR             ; @ 0000:63A3: move n bytes (library)
        EXTRN   _strlen:FAR             ; @ 0000:67F6: string length (library)
        EXTRN   _f0D1C_0163:FAR         ; @ 0D1C:0163 (E0D1C.C)
        EXTRN   _RedrawCharacters:FAR   ; @ 0D1C:0583 (E0ACD.C)

        EXTRN   gChars:BYTE             ; @ ds:F580: 6 character records of 0xF3 bytes (E08E6.C)
        EXTRN   gBufDCFC:DWORD          ; @ ds:DCFC: 4096-byte scratch allocation (E07A9.C)
        EXTRN   gFillRect:DWORD         ; @ ds:806A: far fill-rect routine (E082C.C)
        EXTRN   gMemberBoxes:WORD       ; @ ds:62E2: two 8-byte (x1,y1,x2,y2) member-list boxes
        EXTRN   gStatusLine:WORD              ; @ ds:8115: nonzero while a status line is up (E082C.C)
        EXTRN   gTextRegion:WORD              ; @ ds:811F (E07A9.C)
        EXTRN   g159:WORD               ; @ ds:0159 (EOV6.C)
        EXTRN   gF57E:BYTE              ; @ ds:F57E (E0BD2.C)
        EXTRN   gFont1A47:DWORD         ; @ ds:FE4E: the current far font pointer (E1547.C)
        EXTRN   gFont8x8:DWORD             ; @ ds:FE5A: the 8x8 font far pointer (E07C8.C)
        EXTRN   gTextHeight:WORD        ; @ ds:FE60 (E1C0E.C)
        EXTRN   gTextWidth:WORD         ; @ ds:FE62 (E1C0E.C)
        EXTRN   gTextLeft:WORD          ; @ ds:FE64 (E14EF.C)
        EXTRN   gTextTop:WORD           ; @ ds:FE66 (E14EF.C)
        EXTRN   gStrings2003:BYTE       ; @ 2003:0000: the message string table (EOV6.C)
        EXTRN   gGraphicsChoice:WORD              ; @ ds:8102: colour-row mode (E07A9.C)
        EXTRN   gMageSpellNames:WORD              ; @ ds:1278: far item-name pointer table (E0BD2.C)
        EXTRN   gClericSpellNames:WORD              ; @ ds:12E0: far item-name pointer table (E0BD2.C)
        EXTRN   gFmt645B:BYTE           ; @ ds:645B: the "%-18s %1d" format string
        EXTRN   gFDEA:BYTE              ; @ ds:FDEA: per-item count, indexed by item*2
        EXTRN   gFileNameBuf:BYTE       ; @ ds:FE6E: text scratch buffer (E082C.C)
        EXTRN   gDrawText:DWORD         ; @ ds:80CA: far text routine (E082C.C)
        EXTRN   gFDEC:BYTE              ; @ ds:FDEC: 32 two-byte spell counters, built by ovl1:00DD
        EXTRN   gFD9A:BYTE              ; @ ds:FD9A: 32-byte spell index table, built by ovl1:00DD
        EXTRN   gNeedsRest:WORD              ; @ ds:FE2C: set when a known slot is retired (ovl1:025D)
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
        EXTRN   gMenus:BYTE        ; @ ds:628C: six five-byte per-member menu records (this unit)
        EXTRN   gColourTable:BYTE       ; @ ds:00A9: sixteen 16-byte colour records, indexed by attribute << 4
        EXTRN   gButtonDefs:BYTE        ; @ 208E:0000: nine-byte per-entry text/box records (this unit)
        EXTRN   g62FE:BYTE              ; @ ds:62FE: spell-level to slot-count table, class/kind map A
        EXTRN   g6330:BYTE              ; @ ds:6330: spell-level to slot-count table, class/kind map B
        EXTRN   g6358:BYTE              ; @ ds:6358: spell-level to slot-count table, class/kind map C
        EXTRN   g6394:BYTE              ; @ ds:6394: spell-level to slot-count table, class/kind map D
        EXTRN   g6465:BYTE              ; @ ds:6465: the "%d of %d Remaining.  " format string
        EXTRN   gSaveRect:DWORD         ; @ ds:80AE: far save-rectangle routine (E0BD2.C)
        EXTRN   gFn804A:DWORD           ; @ ds:804A: far blit routine (E0ACD.C)
        EXTRN   gSegDF4E:DWORD          ; @ ds:DF4E: far pointer to the option-field buffer (E07A9.C)
        EXTRN   gF542:BYTE              ; @ ds:F542: message buffer passed to Action01E3 (E0ACD.C)
        EXTRN   gFB44:BYTE              ; @ ds:FB44: display-mode byte (E0ACD.C)
        EXTRN   gFlag:BYTE              ; @ ds:F3B9 (E0ACD.C)
        EXTRN   gBarGraphsOn:BYTE              ; @ ds:F43C: set when the startup allocations are done (E07A9.C)
        EXTRN   gTunesOn:WORD              ; @ ds:80E8: toggled by the Ctrl-key handling (E1AAD.ASM)
        EXTRN   gSoundOn:WORD           ; @ ds:80E6: nonzero unless sound was turned off (E07C8.C)
        EXTRN   gMouseHandlerInstalled:WORD ; @ ds:8148: 1 when the mouse handler is installed (E07C8.C)
        EXTRN   gFlags849E:WORD         ; @ ds:849E (E08E6.C)
        EXTRN   gOverlayBuf:BYTE        ; @ ds:113F: DGROUP buffer the overlay loader is passed (E082C.C)
        EXTRN   gFn807E:DWORD           ; @ ds:807E: far blit routine (E0ACD.C)
        EXTRN   g6457:BYTE              ; @ ds:6457: key byte of the ovl1:6457 key-remap pair
        EXTRN   g6459:BYTE              ; @ ds:6459: remapped key byte of the pair
        EXTRN   g647B:BYTE              ; @ ds:647B: "Far heap corrupt!"
        EXTRN   g648D:BYTE              ; @ ds:648D: "Tunes are %-3s"
        EXTRN   g649C:BYTE              ; @ ds:649C: "Sounds are %-3s"
        EXTRN   g64AC:BYTE              ; @ ds:64AC: "Bar Graphs are %-3s"
        EXTRN   g64C0:BYTE              ; @ ds:64C0: "Mouse is %-3s"
        EXTRN   g64CE:BYTE              ; @ ds:64CE: "PLAYFLD" screen-image name
        EXTRN   g64D6:BYTE              ; @ ds:64D6: "INVENT" screen-image name
        EXTRN   g64DD:BYTE              ; @ ds:64DD: "EOBDATA.SAV" save-file name
        EXTRN   g64E9:BYTE              ; @ ds:64E9: "PLAYFLD" screen-image name
        EXTRN   g64F1:BYTE              ; @ ds:64F1: "INVENT" screen-image name
        EXTRN   gItems:BYTE             ; @ ds:8DD0: 0xE-byte item records (E0BD2.C)
        EXTRN   gClickY:WORD              ; @ ds:8142: y of the last mouse click (E14EF.C)

        EXTRN   _F_SCOPY@:FAR           ; @ 0000:78C9: block copy for a C local-array initialiser (library)
        EXTRN   _heapcheck:FAR          ; @ 0000:74B0: far-heap check (E082C.C)
        EXTRN   _LoadScreenImage:FAR    ; @ 082C:051B (E082C.C)
        EXTRN   _Action01E3:FAR         ; @ 0ACD:01E3 (E0ACD.C)
        EXTRN   _RedrawPartyPanel:FAR         ; @ 0EF3:2541 (E0ACD.C)
        EXTRN   _SortSpellSlots:FAR     ; @ 0EF3:000E (E0EF3.C)
        EXTRN   _ShutdownGame:FAR       ; @ 07C8:058A (E07C8.C)
        EXTRN   _FileOpen:FAR           ; @ 1C0E:00DB (E082C.C)
        EXTRN   _FileClose:FAR          ; @ 1C0E:03BB (E082C.C)
        EXTRN   _WaitGameKey:FAR        ; @ 1AAD:053A (E1AAD.ASM)
        EXTRN   _LoadGame:FAR              ; @ 210C:0039: overlay 3 entry, ovl3:15B9 (LoadGame, EOV3.C)
        EXTRN   _fOvl3_1368:FAR         ; @ 210C:0089: overlay 3 entry, ovl3:1368 (SaveGame, EOV3.C)
        EXTRN   _DrawViewFrame:FAR         ; @ 2116:005C: overlay 4 entry, ovl4:1F51 (DrawViewFrame, EOV4.C)
        EXTRN   _fOvl6_0000:FAR         ; @ 212A:0039: overlay 6 entry, ovl6:0000 (CampLoop, EOV6.C)
        EXTRN   _IsMouseInRect:FAR      ; @ 1A72:02A6 (E1A72.ASM)

        .CODE

; ovl1:0000: DropCharacter, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _DropCharacter ; @ ovl1:0000 stub
_DropCharacter PROC FAR
        ret
_DropCharacter ENDP

; ovl1:00DD: BuildSpellSlots, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _BuildSpellSlots ; @ ovl1:00DD stub
_BuildSpellSlots PROC FAR
        ret
_BuildSpellSlots ENDP

; ovl1:025D: SpendSpellSlot, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _SpendSpellSlot ; @ ovl1:025D stub
_SpendSpellSlot PROC FAR
        ret
_SpendSpellSlot ENDP

; ovl1:0416: SelectPartyMember, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _SelectPartyMember ; @ ovl1:0416 stub
_SelectPartyMember PROC FAR
        ret
_SelectPartyMember ENDP

; ovl1:0881: ShowItemName, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _DrawSpellRow ; @ ovl1:0881 stub
_DrawSpellRow PROC FAR
        ret
_DrawSpellRow ENDP

; ovl1:097A: the spell-slot screen for member `who` (a negative index returns at once). It picks
; the member's spell table: FindClassSlot(rec+0x20, 2, else 4, else 1) selects the class slot and
; rec[slot]+0x24 (minus 1 for kinds 2 and 1) indexes one of the five-byte-per-level tables
; g62FE+g6330, g6358 or g6394 into a local count[5]; the long `mask` (parameters 3 and 4) is the
; slot mask passed to BuildSpellSlots and parameter 2 is its flag (0 copies the far pointer at
; rec+0x73). BuildSpellSlots(who, count, mask, flag) then fills the spell lists and _BuildMenuButtons(4)
; builds the menu. The loop draws rows and item names, reads DispatchKeyWait and switches on a
; 20-entry key table (0x0C..0x8020) to move the cursor, add or remove one point and redraw the
; "%d of %d Remaining.  " line (g6465). On exit it frees the list, restores the fonts, redraws the
; member and calls SpendSpellSlot(who, flag). Called from ovl1:1ACD with the member chosen by
; ovl1:0416. Purpose unknown: whether the screen casts or memorizes (why `flag` picks the table)
; and what the long mask means.
; Stack frame: [bp+6] `who`; [bp+8] `flag`; [bp+0Ch]/[bp+0Eh] the long `mask`; [bp-6..-2] `count[5]`;
; [bp-0Ch]/[bp-0Ah] the table bases; [bp-10h]/[bp-0Eh] `mask` copy; [bp-12h] `flag` copy;
; [bp-18h]/[bp-16h] the far `list`; [bp-1Ch]/[bp-1Ah] the far `node`; [bp-20h] `sel`; [bp-1Eh] `cur`;
; [bp-22h] `pending`; [bp-24h] `key`; [bp-26h] `looping`; [bp-28h]/[bp-2Ah] redraw flags; si `sel`,
; di `i`/`cur`.
        PUBLIC  _SpellSlotScreen         ; @ ovl1:097A
_SpellSlotScreen PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,2Ah
        push    si
        push    di
        cmp     word ptr [bp+6],-1
        jne     short f097A_098B
        jmp     f097A_1088
f097A_098B:
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        mov     al,[bx+gChars+13h]
        cbw
        add     ax,0FFF4h
        mov     [bp-0Ch],ax
        or      ax,ax
        jge     short f097A_09A8
        xor     ax,ax
        jmp     short f097A_09AB
f097A_09A8:
        mov     ax,[bp-0Ch]
f097A_09AB:
        mov     [bp-0Ch],ax
        cmp     word ptr [bp+8],0
        jne     short f097A_09B7
        jmp     f097A_0A9F
f097A_09B7:
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        mov     al,[bx+gChars+20h]
        cbw
        push    ax
        mov     ax,2
        push    ax
        call    far ptr _FindClassSlot
        pop     cx
        pop     cx
        mov     [bp-8],ax
        cmp     ax,0FFFFh
        je      short f097A_0A2C
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        add     ax,[bp-8]
        mov     bx,ax
        mov     al,[bx+gChars+24h]
        cbw
        dec     ax
        mov     [bp-0Ah],ax
        xor     di,di
        jmp     short f097A_0A1B
f097A_09F4:
        mov     ax,[bp-0Ah]
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,[bx+di+62FEh]
        mov     [bp+di-6],al
        or      al,al
        je      short f097A_0A1A
        mov     ax,[bp-0Ch]
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,[bx+di+6330h]
        add     [bp+di-6],al
f097A_0A1A:
        inc     di
f097A_0A1B:
        cmp     di,5
        jl      short f097A_09F4
        mov     word ptr [bp-10h],0FFFFh
        mov     word ptr [bp-0Eh],7Bh
        jmp     short f097A_0A97
f097A_0A2C:
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        mov     al,[bx+gChars+20h]
        cbw
        push    ax
        mov     ax,4
        push    ax
        call    far ptr _FindClassSlot
        pop     cx
        pop     cx
        mov     [bp-8],ax
        cmp     ax,0FFFFh
        je      short f097A_0A97
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        add     ax,[bp-8]
        mov     bx,ax
        mov     al,[bx+gChars+24h]
        cbw
        mov     [bp-0Ah],ax
        or      ax,ax
        jge     short f097A_0A6C
        xor     ax,ax
        jmp     short f097A_0A6F
f097A_0A6C:
        mov     ax,[bp-0Ah]
f097A_0A6F:
        mov     [bp-0Ah],ax
        xor     di,di
        jmp     short f097A_0A88
f097A_0A76:
        mov     ax,[bp-0Ah]
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,[bx+di+6358h]
        mov     [bp+di-6],al
        inc     di
f097A_0A88:
        cmp     di,5
        jl      short f097A_0A76
        mov     word ptr [bp-10h],0FF2h
        mov     word ptr [bp-0Eh],80h
f097A_0A97:
        mov     word ptr [bp-12h],1
        jmp     f097A_0B28
f097A_0A9F:
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        mov     al,[bx+gChars+20h]
        cbw
        push    ax
        mov     ax,1
        push    ax
        call    far ptr _FindClassSlot
        pop     cx
        pop     cx
        mov     [bp-8],ax
        cmp     ax,0FFFFh
        je      short f097A_0B0B
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        add     ax,[bp-8]
        mov     bx,ax
        mov     al,[bx+gChars+24h]
        cbw
        dec     ax
        mov     [bp-0Ah],ax
        xor     di,di
        jmp     short f097A_0AEE
f097A_0ADC:
        mov     ax,[bp-0Ah]
        mov     dx,5
        imul    dx
        mov     bx,ax
        mov     al,[bx+di+6394h]
        mov     [bp+di-6],al
        inc     di
f097A_0AEE:
        cmp     di,5
        jl      short f097A_0ADC
        mov     ax,[bp+6]
        mov     dx,0F3h
        imul    dx
        mov     bx,ax
        mov     ax,word ptr [bx+gChars+75h]
        mov     dx,word ptr [bx+gChars+73h]
        mov     [bp-10h],dx
        mov     [bp-0Eh],ax
f097A_0B0B:
        mov     ax,1
        push    ax
        push    word ptr [bp+6]
        call    far ptr _f0D1C_0163
        pop     cx
        pop     cx
        or      ax,ax
        je      short f097A_0B23
        shl     byte ptr [bp-3],1
        shl     byte ptr [bp-2],1
f097A_0B23:
        mov     word ptr [bp-12h],0
f097A_0B28:
        push    word ptr [bp-12h]
        push    word ptr [bp-0Eh]
        push    word ptr [bp-10h]
        push    ss
        lea     ax,[bp-6]
        push    ax
        push    word ptr [bp+6]
        push    cs
        call    near ptr _BuildSpellSlots
        add     sp,0Ch
        mov     word ptr [bp-18h],0
        mov     word ptr [bp-16h],0
        mov     word ptr [bp-28h],1
        mov     ax,0FFFFh
        mov     [bp-20h],ax
        mov     si,ax
        xor     ax,ax
        mov     [bp-22h],ax
        mov     [bp-1Eh],ax
        mov     ax,4
        push    ax
        nop
        push    cs
        call    near ptr _BuildMenuButtons
        pop     cx
        mov     [bp-18h],ax
        mov     [bp-16h],dx
        mov     word ptr [bp-26h],1
        jmp     f097A_1035
f097A_0B77:
        push    word ptr [bp+6]
        call    far ptr _DrawRowList
        pop     cx
        cmp     word ptr [bp-1Eh],0
        jge     short f097A_0B8B
        mov     word ptr [bp-1Eh],4
f097A_0B8B:
        cmp     word ptr [bp-1Eh],4
        jle     short f097A_0B96
        mov     word ptr [bp-1Eh],0
f097A_0B96:
        cmp     si,[bp-1Eh]
        je      short f097A_0C13
        or      si,si
        jl      short f097A_0BCD
        mov     ax,si
        add     ax,1Bh
        push    ax
        push    word ptr [bp-16h]
        push    word ptr [bp-18h]
        call    far ptr _FindNode
        add     sp,6
        mov     [bp-1Ch],ax
        mov     [bp-1Ah],dx
        mov     ax,1
        push    ax
        xor     ax,ax
        push    ax
        push    ax
        push    dx
        push    word ptr [bp-1Ch]
        nop
        push    cs
        call    near ptr _DrawListNode
        add     sp,0Ah
f097A_0BCD:
        mov     ax,[bp-1Eh]
        add     ax,1Bh
        push    ax
        push    word ptr [bp-16h]
        push    word ptr [bp-18h]
        call    far ptr _FindNode
        add     sp,6
        mov     [bp-1Ch],ax
        mov     [bp-1Ah],dx
        mov     ax,1
        push    ax
        push    ax
        xor     ax,ax
        push    ax
        push    dx
        push    word ptr [bp-1Ch]
        nop
        push    cs
        call    near ptr _DrawListNode
        add     sp,0Ah
        mov     word ptr [bp-22h],0
        mov     word ptr [bp-20h],0FFFFh
        mov     si,[bp-1Eh]
        mov     word ptr [bp-28h],1
        mov     word ptr [bp-2Ah],1
f097A_0C13:
        cmp     word ptr [bp-2Ah],0
        je      short f097A_0C8A
        mov     word ptr [bp-2Ah],0
        mov     word ptr [gStatusLine],2
        mov     di,1
        jmp     short f097A_0C4B
f097A_0C29:
        xor     ax,ax
        push    ax
        push    ax
        push    word ptr [bp-12h]
        mov     ax,di
        dec     ax
        push    ax
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+di+gFDBA]
        cbw
        push    ax
        push    cs
        call    near ptr _DrawSpellRow
        add     sp,0Ah
        inc     di
f097A_0C4B:
        cmp     di,9
        jl      short f097A_0C29
        mov     word ptr [gStatusLine],0
        call    far ptr _LockHidePointer
        xor     ax,ax
        push    ax
        mov     ax,2
        push    ax
        mov     ax,48h
        push    ax
        mov     ax,16h
        push    ax
        mov     ax,32h
        push    ax
        xor     ax,ax
        push    ax
        mov     ax,32h
        push    ax
        xor     ax,ax
        push    ax
        call    far ptr _ClipRect
        add     sp,10h
        call    far ptr _LockShowPointer
        mov     word ptr [bp-20h],0FFFFh
f097A_0C8A:
        cmp     word ptr [bp-28h],0
        je      short f097A_0CE2
        mov     word ptr [bp-28h],0
        mov     al,[bp+si-6]
        cbw
        push    ax
        mov     al,[bp+si-6]
        cbw
        push    ax
        mov     al,[si+gFDE7]
        cbw
        pop     dx
        sub     dx,ax
        push    dx
        push    ds
        mov     ax,6465h
        push    ax
        push    ds
        mov     ax,OFFSET gFileNameBuf
        push    ax
        call    far ptr _sprintf
        add     sp,0Ch
        call    far ptr _LockHidePointer
        mov     al,[gF57E]
        cbw
        push    ax
        mov     ax,9
        push    ax
        mov     ax,26h
        push    ax
        mov     ax,8
        push    ax
        push    ds
        mov     ax,OFFSET gFileNameBuf
        push    ax
        call    far ptr _DrawShadowText
        add     sp,0Ch
        call    far ptr _LockShowPointer
f097A_0CE2:
        cmp     word ptr [bp-22h],0
        jge     short f097A_0CFA
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+gFDBA]
        cbw
        dec     ax
        mov     [bp-22h],ax
f097A_0CFA:
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+gFDBA]
        cbw
        cmp     ax,[bp-22h]
        jg      short f097A_0D12
        mov     word ptr [bp-22h],0
f097A_0D12:
        mov     ax,[bp-22h]
        cmp     ax,[bp-20h]
        je      short f097A_0D6A
        xor     ax,ax
        push    ax
        mov     ax,1
        push    ax
        push    word ptr [bp-12h]
        push    word ptr [bp-20h]
        mov     ax,si
        mov     dx,9
        imul    dx
        add     ax,[bp-20h]
        mov     bx,ax
        mov     al,[bx+gFDBA+1]
        cbw
        push    ax
        push    cs
        call    near ptr _DrawSpellRow
        add     sp,0Ah
        mov     ax,1
        push    ax
        push    ax
        push    word ptr [bp-12h]
        push    word ptr [bp-22h]
        mov     ax,si
        mov     dx,9
        imul    dx
        add     ax,[bp-22h]
        mov     bx,ax
        mov     al,[bx+gFDBA+1]
        cbw
        push    ax
        push    cs
        call    near ptr _DrawSpellRow
        add     sp,0Ah
        mov     ax,[bp-22h]
        mov     [bp-20h],ax
f097A_0D6A:
        push    word ptr [bp-16h]
        push    word ptr [bp-18h]
        call    far ptr _DispatchKeyWait
        pop     cx
        pop     cx
        mov     [bp-24h],ax
        cmp     ax,5Ch
        je      short f097A_0DA5
        cmp     ax,66h
        je      short f097A_0D93
        cmp     ax,6Eh
        je      short f097A_0D8B
        jmp     short f097A_0DBA
f097A_0D8B:
        mov     word ptr [bp-24h],801Ah
        jmp     f097A_0E1A
f097A_0D93:
        mov     ax,si
        inc     ax
        mov     bx,5
        cwd
        idiv    bx
        add     dx,801Bh
        mov     [bp-24h],dx
        jmp     short f097A_0E1A
f097A_0DA5:
        or      si,si
        jne     short f097A_0DB0
        mov     word ptr [bp-24h],801Fh
        jmp     short f097A_0E1A
f097A_0DB0:
        mov     ax,si
        add     ax,801Ah
        mov     [bp-24h],ax
        jmp     short f097A_0E1A
f097A_0DBA:
        cmp     word ptr [gMouseDisabled],0
        jne     short f097A_0E1A
        mov     ax,7Ah
        push    ax
        mov     ax,0A8h
        push    ax
        mov     ax,32h
        push    ax
        mov     ax,8
        push    ax
        push    word ptr [gCursorY]
        push    word ptr [gCursorX]
        call    far ptr _IsPointInRect
        add     sp,0Ch
        or      ax,ax
        je      short f097A_0E1A
        mov     ax,[gCursorY]
        add     ax,0FFCEh
        mov     bx,9
        cwd
        idiv    bx
        mov     [bp-22h],ax
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+gFDBA]
        cbw
        dec     ax
        cmp     ax,[bp-22h]
        jge     short f097A_0E1A
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+gFDBA]
        cbw
        dec     ax
        mov     [bp-22h],ax
f097A_0E1A:
        test    word ptr [bp-24h],8000h
        je      short f097A_0E6E
        mov     ax,[bp-24h]
        and     ax,7FFFh
        push    ax
        push    word ptr [bp-16h]
        push    word ptr [bp-18h]
        call    far ptr _FindNode
        add     sp,6
        mov     [bp-1Ch],ax
        mov     [bp-1Ah],dx
        mov     ax,1
        push    ax
        push    ax
        push    ax
        push    dx
        push    word ptr [bp-1Ch]
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
        push    word ptr [bp-1Ah]
        push    word ptr [bp-1Ch]
        nop
        push    cs
        call    near ptr _DrawListNode
        add     sp,0Ah
f097A_0E6E:
        mov     ax,[bp-24h]
        mov     [bp-14h],ax
        mov     cx,14h
        mov     bx,108Eh
f097A_0E7A:
        mov     ax,word ptr cs:[bx]
        cmp     ax,[bp-14h]
        je      short f097A_0E89
        inc     bx
        inc     bx
        loop    f097A_0E7A
        jmp     f097A_1035
f097A_0E89:
        jmp     word ptr cs:[bx+28h]
f097A_0E8D:
        mov     ax,[bp-24h]
        sub     ax,801Bh
        mov     [bp-1Eh],ax
        cmp     ax,si
        je      short f097A_0E9D
        jmp     f097A_1035
f097A_0E9D:
        mov     ax,[bp-24h]
        and     ax,7FFFh
        push    ax
        push    word ptr [bp-16h]
        push    word ptr [bp-18h]
        call    far ptr _FindNode
        add     sp,6
        mov     [bp-1Ch],ax
        mov     [bp-1Ah],dx
        mov     ax,1
        push    ax
        push    ax
        xor     ax,ax
        push    ax
        push    dx
        push    word ptr [bp-1Ch]
        nop
        push    cs
        call    near ptr _DrawListNode
        add     sp,0Ah
        jmp     f097A_1035
f097A_0ECF:
        cmp     byte ptr [si+gFDE7],0
        jne     short f097A_0ED9
        jmp     f097A_1035
f097A_0ED9:
        mov     di,1
        jmp     short f097A_0F10
f097A_0EDE:
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+di+gFDBA]
        cbw
        shl     ax,1
        mov     bx,ax
        mov     al,[bx+gFDEA]
        sub     byte ptr [si+gFDE7],al
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+di+gFDBA]
        cbw
        shl     ax,1
        mov     bx,ax
        mov     byte ptr [bx+gFDEA],0
        inc     di
f097A_0F10:
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+gFDBA]
        cbw
        cmp     ax,di
        jge     short f097A_0EDE
        mov     word ptr [bp-28h],1
        mov     word ptr [bp-2Ah],1
        jmp     f097A_1035
f097A_0F2F:
        call    far ptr _RedrawCharacters
        mov     word ptr [bp-26h],0
        jmp     f097A_1035
f097A_0F3C:
        mov     ax,[bp-20h]
        inc     ax
        jmp     short f097A_0F5E
f097A_0F42:
        mov     word ptr [bp-22h],0
        jmp     f097A_1035
f097A_0F4A:
        mov     ax,[bp-20h]
        jmp     short f097A_0F5D
f097A_0F4F:
        mov     ax,si
        mov     dx,9
        imul    dx
        mov     bx,ax
        mov     al,[bx+gFDBA]
        cbw
f097A_0F5D:
        dec     ax
f097A_0F5E:
        mov     [bp-22h],ax
        jmp     f097A_1035
f097A_0F64:
        mov     al,[bp+si-6]
        cbw
        push    ax
        mov     al,[si+gFDE7]
        cbw
        pop     dx
        sub     dx,ax
        jg      short f097A_0F76
        jmp     f097A_1035
f097A_0F76:
        cmp     word ptr [bp-20h],-1
        jg      short f097A_0F7F
        jmp     f097A_1035
f097A_0F7F:
        mov     ax,si
        mov     dx,9
        imul    dx
        add     ax,[bp-20h]
        mov     bx,ax
        mov     al,[bx+gFDBA+1]
        cbw
        shl     ax,1
        mov     bx,ax
        inc     byte ptr [bx+gFDEA]
        inc     byte ptr [si+gFDE7]
        mov     ax,1
        push    ax
        xor     ax,ax
        push    ax
        push    word ptr [bp-12h]
        push    word ptr [bp-20h]
        mov     ax,[bp-1Eh]
        mov     dx,9
        imul    dx
        add     ax,[bp-20h]
        mov     bx,ax
        mov     al,[bx+gFDBA+1]
        cbw
        push    ax
        push    cs
        call    near ptr _DrawSpellRow
        add     sp,0Ah
        mov     word ptr [bp-28h],1
        jmp     short f097A_1035
f097A_0FCA:
        cmp     byte ptr [bp+si-6],0
        je      short f097A_1035
        mov     ax,si
        mov     dx,9
        imul    dx
        add     ax,[bp-20h]
        mov     bx,ax
        mov     al,[bx+gFDBA+1]
        cbw
        shl     ax,1
        mov     bx,ax
        cmp     byte ptr [bx+gFDEA],0
        je      short f097A_1035
        mov     ax,si
        mov     dx,9
        imul    dx
        add     ax,[bp-20h]
        mov     bx,ax
        mov     al,[bx+gFDBA+1]
        cbw
        shl     ax,1
        mov     bx,ax
        dec     byte ptr [bx+gFDEA]
        dec     byte ptr [si+gFDE7]
        mov     ax,1
        push    ax
        xor     ax,ax
        push    ax
        push    word ptr [bp-12h]
        push    word ptr [bp-20h]
        mov     ax,[bp-1Eh]
        mov     dx,9
        imul    dx
        add     ax,[bp-20h]
        mov     bx,ax
        mov     al,[bx+gFDBA+1]
        cbw
        push    ax
        push    cs
        call    near ptr _DrawSpellRow
        add     sp,0Ah
        mov     word ptr [bp-28h],1
f097A_1035:
        cmp     word ptr [bp-26h],0
        je      short f097A_103E
        jmp     f097A_0B77
f097A_103E:
        push    word ptr [bp-16h]
        push    word ptr [bp-18h]
        nop
        push    cs
        call    near ptr _FreeFarList
        pop     cx
        pop     cx
        mov     ax,0FFFFh
        push    ax
        call    far ptr _DrawRowList
        pop     cx
        push    word ptr [gA9C0+2]
        push    word ptr [gA9C0]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        push    word ptr [bp+6]
        call    far ptr _RedrawMember
        pop     cx
        push    word ptr [gFont8x8+2]
        push    word ptr [gFont8x8]
        call    far ptr _f1A47_0169
        pop     cx
        pop     cx
        push    word ptr [bp-12h]
        push    word ptr [bp+6]
        push    cs
        call    near ptr _SpendSpellSlot
        pop     cx
        pop     cx
f097A_1088:
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
        ret
f097A_108E:
        dw      0Ch, 0Dh, 2Bh, 3Dh, 42h, 5Bh, 5Dh, 60h, 61h, 62h, 69h, 6Ah
        dw      8019h, 801Ah, 801Bh, 801Ch, 801Dh, 801Eh, 801Fh, 8020h
        dw      f097A_0FCA, f097A_0F64, f097A_0F64, f097A_0F64, f097A_0FCA, f097A_0F42
        dw      f097A_0F4F, f097A_0F4A, f097A_0F64, f097A_0F3C, f097A_0FCA, f097A_0F64
        dw      f097A_0ECF, f097A_0F2F, f097A_0E8D, f097A_0E8D, f097A_0E8D, f097A_0E8D
        dw      f097A_0E8D, f097A_0F64
_SpellSlotScreen ENDP

; ovl1:10DE: BoolString, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _BoolString ; @ ovl1:10DE stub
_BoolString PROC FAR
        ret
_BoolString ENDP

; ovl1:1107: SaveOptionField, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _SaveOptionField ; @ ovl1:1107 stub
_SaveOptionField PROC FAR
        ret
_SaveOptionField ENDP

; ovl1:112D: DrawOptionField, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _DrawOptionField ; @ ovl1:112D stub
_DrawOptionField PROC FAR
        ret
_DrawOptionField ENDP

; ovl1:1153: BuildMenuButtons, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _BuildMenuButtons              ; @ ovl1:1153 stub
_BuildMenuButtons PROC FAR
        ret
_BuildMenuButtons ENDP

; ovl1:13F7: ShowMessage, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _ShowMessage ; @ ovl1:13F7 stub
_ShowMessage PROC FAR
        ret
_ShowMessage ENDP

; ovl1:1516: FreeFarList, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _FreeFarList ; @ ovl1:1516 stub
_FreeFarList PROC FAR
        ret
_FreeFarList ENDP

; ovl1:1551: DrawListNode, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _DrawListNode ; @ ovl1:1551 stub
_DrawListNode PROC FAR
        ret
_DrawListNode ENDP

; ovl1:1691: ScribeScrolls, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _ScribeScrolls ; @ ovl1:1691 stub
_ScribeScrolls PROC FAR
        ret
_ScribeScrolls ENDP

; ovl1:1ACD: the camp menu, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _RunCampMenu               ; @ ovl1:1ACD stub
_RunCampMenu PROC FAR
        ret
_RunCampMenu ENDP

; ovl1:243A: ChooseListEntry, now written in src/EOV1.C (the stub only keeps the symbol).
        PUBLIC  _AskYesNo ; @ ovl1:243A stub
_AskYesNo PROC FAR
        ret
_AskYesNo ENDP

        END
