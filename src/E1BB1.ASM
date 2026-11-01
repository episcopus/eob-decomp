; Unit at EOB.EXE 1BB1:0001 (208 bytes, 2 functions), kind asm [H] by the map. 1BB1:0001 is a
; single 00 byte with `segment` evidence (the unit's start marker, like 1990:0001); the one real
; function follows at 1BB1:0002.
;
; 1BB1:0002 sets the mouse pointer's shape. Two callers. f07A9_010C (07A9:010C) passes the hot spot
; (8,8) when its n is above 0 and (0,0) otherwise, and the record gIconsDDE4[n] in both cases (the
; two calls at E07A9.C:164 and :166; the table holds item icon images, E07A9.C:64-68, the comment
; just above gIconsDDE4 at :69). SetPointerShape (ovl2:0000, src/EOV2.C:206-211) passes (0,0) and
; gPtrDD00, which it has just set to gIconsDDE4[0] (EOV2.C:210-211); LoadItemData calls
; SetPointerShape at start-up (EOV2.C:451). Unit 1BBE shows what the globals are: the hide
; 1BBE:0002 subtracts the hot spot gPointerHotX/gPointerHotY (ds:8158/815A) from the last drawn
; pointer position gDrawnX/gDrawnY (ds:815C/815E) (1BBE:001E..002F), and the show 1BBE:00ED sets
; that drawn position to the mouse position gCursorX/gCursorY (ds:8138/813A) and subtracts the hot
; spot from it (1BBE:0111..012E); the shape record gPointerShapeOff/gPointerShapeSeg
; (ds:816E/8170) is what 1BBE:00ED draws through [ds:80C2]. The save-under buffer
; gPointerBackOff/gPointerBackSeg (ds:816A/816C) is the screen behind the pointer: 1BBE:0002
; restores it through [ds:804E] and 1BBE:00ED saves the background into it through [ds:80AE].
;
; The size comes from the shape record: byte SHAPE_HEIGHT_BYTE (+3) is the height in pixels, read
; at 1BB1:0034 and stored unchanged at gPointerHeight (ds:8154, 1BB1:0066). 1BBE:0002 uses it:
; 1BBE:0095 adds it to the top edge gRectTop (ds:8178) to make the bottom edge gRectBottom
; (ds:817C), and the bottom edge is clipped to 200 at 1BBE:00AB..00B3. Byte SHAPE_WIDTH_BYTE (+2)
; is read at 1BB1:003A and, plus 1 (1BB1:003E `inc dx`), stored at gPointerWidth8 (ds:8156,
; 1BB1:006A); the code shows only that the stored width is the byte plus one, so the byte's own
; meaning (a count one less than the width in 8-pixel units) is a reading [H]. 1BBE:0002 uses the
; stored width: 1BBE:005F adds it to the left edge gRectLeft (ds:8176, the x divided by 8) to make
; the right edge gRectRight (ds:817A), the left edge is skipped when it is at or below the negated
; width gNegSpriteWidth (ds:8188, 1BBE:0051..0055) or 40 or more (1BBE:0057..005A), and the right
; edge is clipped to 40 at 1BBE:0077..007E. The width and a region record share the 8-pixel column
; unit (the region's left edge at E1A29.C:21 and its width at E1A29.C:23). The stored width of 0
; means 1 column; a stored height of 0 is 0 pixels. The buffer path depends on the buffer and the
; size: with no buffer yet (gPointerBackSeg ds:816C == 0) the free is skipped (1BB1:0044 `je 0066`)
; and the size query and MemAlloc run; with a buffer and a changed size (height or width differ,
; 1BB1:004A/0050) the old buffer is freed (1BB1:0058..005C) before the query and MemAlloc; with a
; buffer and the same size the call goes straight to the show (1BB1:0050 `je 00A9`). The size
; query [ds:80C6] is called as (width, height) and its result is the new buffer size. The check
; needs LargestFreeBlock (19BC:023D) to return at least that result plus HEAP_BLOCK (16, 1BB1:0076
; `add ax,HEAP_BLOCK`), and that function already subtracts 16 from the raw largest free block
; (E19BC.C:142 `max -= 0x10`; the quote "less one 16 byte block" is its header, E19BC.C:5), so a
; raw free block of result + 32 bytes is what passes. The comparison is skipped when the block's
; high word DX is nonzero (1BB1:0080 `or dx,dx`; 1BB1:0082 `jne`), so a free block of 64 KB or
; more passes without the test. MemAlloc then allocates the result at 1BB1:0091..0094 (si minus
; the same 16).
;
; The order is what keeps the old buffer valid: 1BBE:0002 hides the pointer first, while the old
; buffer and sizes are still in place, then the globals are overwritten, the buffer is resized,
; and 1BBE:00ED shows the pointer again and draws the new shape. 1BBE:0002 does nothing when
; gMouseDisabled (ds:814A) is set; when the hide count gHideDepth (ds:8146) is nonzero it only
; increments it (a nested hide); when the count is 0 and gPointerBackSeg (ds:816C, no buffer) is 0
; it does nothing; otherwise it restores the background and increments gHideDepth. 1BBE:00ED does
; nothing when gMouseDisabled is set (1BBE:00F1), when the count is 0 on entry (1BBE:00F8), when
; the count is still nonzero after the decrement (1BBE:0103), or when gPointerBackSeg is 0
; (1BBE:010A); the decrement (1BBE:00FF) comes before the buffer test, so a show with no buffer
; still decrements. Otherwise it copies the position, saves the background and draws. The pair
; [ds:80AA] on the far pointer ds:8172/8174 and [ds:80A2] brackets only the show call at
; 1BB1:00B8 (1BB1:00B1 and 1BB1:00BD); this unit and E1BF3 only read ds:8172/8174, which
; _SetFarPointer (1A72:02FE, src/E1A72.ASM:442-443) writes and _GetFarPointer (1A72:033A) reads.
;
; The function acquires the busy flag gPointerBusy (ds:812E, a test then an increment, not a lock),
; returns 1, or 0 when the block is too small (the margin test at 1BB1:0084..0086) or MemAlloc
; fails (1BB1:009D..00A0).
;
; The unit is hand-written assembly ([H]: the map's c_like test, tools/re/map_units.py:42-46, looks
; only at the instructions after the frame and finds no C-like frame here), so the module declares
; its own segments; `ASSUME CS:NOTHING` around each direct far call is needed: removing those lines
; makes TASM turn the first `lcall` into `push cs; call` (our byte 0E where the original has 9A at
; 1BB1:0013), because TASM then assumes the far callee is in the same code segment.

        SHAPE_WIDTH_BYTE  EQU 2  ; shape record byte +2: read at 1BB1:003A, stored plus one; the
                                 ; byte's own meaning (a count one less than the width in 8-pixel
                                 ; units) is a reading [H]
        SHAPE_HEIGHT_BYTE EQU 3  ; shape record byte +3: height in pixels, read at 1BB1:0034
        HEAP_BLOCK        EQU 16 ; bytes per heap block; LargestFreeBlock subtracts one from the raw
                                 ; block (E19BC.C:142 `max -= 0x10`; the quote "less one 16 byte
                                 ; block" is its header, E19BC.C:5) and this unit adds it back for
                                 ; the margin (1BB1:0076) and removes it for MemAlloc (1BB1:0091)

        _DATA   SEGMENT WORD PUBLIC 'DATA'
        EXTRN   _gPointerBusy:WORD      ; @ ds:812E: busy flag, 0 = free; a test then an
                                        ; increment, not a lock, acquired by busy-wait
        EXTRN   _gPointerHeight:WORD    ; @ ds:8154: pointer height in pixels, from shape byte +3
        EXTRN   _gPointerWidth8:WORD    ; @ ds:8156: pointer width in 8-pixel units, shape byte +2 + 1
        EXTRN   _gPointerHotX:WORD      ; @ ds:8158: pointer hot spot, x (argument 1)
        EXTRN   _gPointerHotY:WORD      ; @ ds:815A: pointer hot spot, y (argument 2)
        EXTRN   _gPointerBackOff:WORD   ; @ ds:816A: save-under buffer, far offset
        EXTRN   _gPointerBackSeg:WORD   ; @ ds:816C: save-under buffer, far segment (0 = none)
        EXTRN   _gPointerShapeOff:WORD  ; @ ds:816E: pointer shape record, far offset
        EXTRN   _gPointerShapeSeg:WORD  ; @ ds:8170: pointer shape record, far segment
        EXTRN   g8172:WORD              ; @ ds:8172: argument of the [ds:80AA] call, offset; written by
                                        ; _SetFarPointer 1A72:02FE (src/E1A72.ASM:442), read by _GetFarPointer 1A72:033A
        EXTRN   g8174:WORD              ; @ ds:8174: argument of the [ds:80AA] call, segment; written by
                                        ; _SetFarPointer 1A72:02FE (src/E1A72.ASM:443), read by _GetFarPointer 1A72:033A
        EXTRN   g80C6:DWORD             ; @ ds:80C6: indirect far function, buffer size (width, height)
        EXTRN   g80AA:DWORD             ; @ ds:80AA: indirect far function, called with ds:8172/8174
        EXTRN   g80A2:DWORD             ; @ ds:80A2: indirect far function, no arguments
        _DATA   ENDS
        DGROUP  GROUP   _DATA

        _TEXT   SEGMENT WORD PUBLIC 'CODE'
        ASSUME  CS:_TEXT, DS:DGROUP
        db      0                       ; 1BB1:0001: the unit's start marker byte (segment evidence)
        EXTRN   _HidePointer:FAR        ; @ 1BBE:0002: hide the pointer (restore background, ++gHideDepth)
        EXTRN   _ShowPointer:FAR        ; @ 1BBE:00ED: show the pointer (--gHideDepth, save, draw)
        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007
        EXTRN   _MemFree:FAR            ; @ 19BC:0137
        EXTRN   _LargestFreeBlock:FAR         ; @ 19BC:023D: returns the largest free block (DX:AX)

; 1BB1:0002: set the mouse pointer shape to the record at (argument 3, argument 4) and the hot
; spot to (argument 1, argument 2). Allocates the save-under buffer when there is none yet or the
; sprite size changed (freeing an existing buffer of a changed size first); hides the pointer
; before overwriting the globals and shows it after, all under the busy flag gPointerBusy (ds:812E).
; Returns 1, or 0 when the largest-block test fails (1BB1:0084..0086) or MemAlloc returns no
; segment (1BB1:009D..00A0); both callers ignore the value and the prototype is void (E07A9.C:88).
        PUBLIC  _SetMousePointerShape             ; @ 1BB1:0002
_SetMousePointerShape PROC FAR
        push    bp
        mov     bp,sp
        push    dx
        push    si
        push    es
acquire:
        cmp     word ptr [_gPointerBusy],0
        jne     acquire
        inc     word ptr [_gPointerBusy]
        ASSUME  CS:NOTHING
        call    far ptr _HidePointer
        ASSUME  CS:_TEXT
        mov     ax,[bp+6]
        mov     word ptr [_gPointerHotX],ax
        mov     ax,[bp+8]
        mov     word ptr [_gPointerHotY],ax
        mov     si,[bp+0Ah]
        mov     word ptr [_gPointerShapeOff],si
        mov     es,[bp+0Ch]
        mov     word ptr [_gPointerShapeSeg],es
        xor     ah,ah
        mov     al,byte ptr es:[si+SHAPE_HEIGHT_BYTE]   ; height in pixels, stored unchanged
        xor     dh,dh
        mov     dl,byte ptr es:[si+SHAPE_WIDTH_BYTE]    ; stored width is this byte plus one (1BB1:003E)
        inc     dx
        cmp     word ptr [_gPointerBackSeg],0
        je      setsize
        cmp     ax,word ptr [_gPointerHeight]
        jne     resize
        cmp     dx,word ptr [_gPointerWidth8]
        je      ready
resize: push    ax
        push    dx
        push    word ptr [_gPointerBackSeg]
        push    word ptr [_gPointerBackOff]
        ASSUME  CS:NOTHING
        call    far ptr _MemFree
        ASSUME  CS:_TEXT
        add     sp,4
        pop     dx
        pop     ax
setsize:
        mov     word ptr [_gPointerHeight],ax
        push    ax
        mov     word ptr [_gPointerWidth8],dx
        push    dx
        call    dword ptr [g80C6]
        add     sp,4
        add     ax,HEAP_BLOCK           ; the margin LargestFreeBlock takes off (1BB1:0076)
        mov     si,ax
        ASSUME  CS:NOTHING
        call    far ptr _LargestFreeBlock
        ASSUME  CS:_TEXT
        or      dx,dx
        jne     alloc
        cmp     ax,si
        jb      failed
alloc:  mov     ax,0                    ; MemAlloc flags = 0: neither zero-fill (0x10) nor
        push    ax                      ; paragraph align (0x20) (E19BC.C:29-32, MemAlloc's header)
        xor     ax,ax
        push    ax                      ; size high word
        mov     ax,si
        sub     ax,HEAP_BLOCK           ; undo the margin so MemAlloc sees the raw block (1BB1:0091)
        push    ax                      ; size low word
        ASSUME  CS:NOTHING
        call    far ptr _MemAlloc
        ASSUME  CS:_TEXT
        add     sp,6
        cmp     dx,0
        je      failed
        mov     word ptr [_gPointerBackOff],ax
        mov     word ptr [_gPointerBackSeg],dx
ready:  push    word ptr [g8174]
        push    word ptr [g8172]
        call    dword ptr [g80AA]
        add     sp,4
        ASSUME  CS:NOTHING
        call    far ptr _ShowPointer
        ASSUME  CS:_TEXT
        call    dword ptr [g80A2]
        mov     ax,1
        jmp     short release
; QUIRK 1BB1:00C6: on failure the old buffer was already freed at resize (1BB1:0058..005C) and the
; new size is already stored, but ds:816A/816C still hold the freed pointer. A later call with the
; same size passes `je ready` (1BB1:0050) with a dangling buffer, and a later call with a different
; size (1BB1:0046/004C, `jne 0052` at 1BB1:004A) frees the already-freed block again (1BB1:0058..005C
; with ds:816A/816C), a double free. A first-time failure (ds:816C == 0) leaves it zero and the next
; call retries.
;
; QUIRK 1BB1:00C6: the failure path also skips the 1BBE:00ED call above, so the hide 1BBE:0002
; did is never undone and the pointer stays hidden until something else balances the count. The
; count can be nonzero two ways: it was already nonzero, so 1BBE:0002 only increments it (1BBE:0015
; `jne 0075` to 1BBE:00E0 `inc [8146]`, no buffer test), or ds:816C (the buffer) was nonzero on
; entry, so 1BBE:0017 passes the buffer test and increments. ds:816C is stored once (1BB1:00A5,
; only with a nonzero MemAlloc segment) and never cleared, so the count is nonzero only after a
; hide that saw a buffer. Only when ds:816C is 0 and the count is 0 does 1BBE:0002 return before
; incrementing (1BBE:0017), so the first-time failure above leaks no hide. Both paths still
; release the busy flag gPointerBusy at `release`.
failed: xor     ax,ax
release:
        dec     word ptr [_gPointerBusy]
        pop     es
        pop     si
        pop     dx
        pop     bp
        ret
_SetMousePointerShape ENDP

        _TEXT   ENDS
        END
