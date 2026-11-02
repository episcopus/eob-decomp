; Unit at INTRO.EXE 0B10:0000 (139 bytes, 3 functions): far copy with normalized far pointers. It is
; the same module as EOB.EXE's 1BA8:0005 (src/E1BA8.ASM): the EOB unit starts at 1BA8:0005 (its one
; leading data byte) and this one at 0B10:0000, so the EOB module's Unpack0 is at unit offset 1 and
; this one's at unit offset 0, and their segment offsets differ by five (0005 against 0000): every
; function here sits six bytes below its EOB twin (0B10:0000 against 1BA8:0006). Unpack0 (0B10:0000)
; copies CX bytes and then BX blocks of 64 KiB, renormalizing both far pointers before the first copy
; and after every block; NormalizeFarPointer (0B10:006C) normalizes a far pointer passed by value and
; returns it in DX:AX; NormFar (0B10:007B) is the normalizer itself, given AX = offset, DX = segment
; it returns AX = offset and 0Fh, DX = segment plus offset shifted right four.
;
; Unpack0 has two callers: Unpack (0AED:00CB, format 0 only, src/I0AED.C) and ReadFromMemory
; (0B5D:1236, a plain copy from an in-memory file, not an unpacker, EOB twin 1C0E:1240).
; NormalizeFarPointer is named through its callers, which use the returned pointer as the normalized
; one: MemAlloc (094F:0007, src/I0892.C), FileRead (0B5D:03F6), FileWrite (0B5D:056E) and
; ReadFromMemory (0B5D:1236) here, and the EOB twin's callers MemAlloc (19BC:0007), FileRead
; (1C0E:0400), FileWrite (1C0E:0578), ReadFromMemory (1C0E:1240) and SaveLevelState (ovl3:11AF).
;
; The `push cs` / `call near ptr _NormFar` pairs still call a FAR PROC: the pushed CS and the near
; call's return IP are what its RETF pops, so they are correct near calls, not a mistake.

        .MODEL  LARGE
        .CODE

        PUBLIC  _Unpack0                ; @ 0B10:0000
; 0B10:0000: copy `len` bytes from the far pointer `src` to the far pointer `dst`, renormalizing both
; far pointers before the first copy and after every block. src is at [bp+6] (offset) / [bp+8]
; (segment), dst at [bp+0Ah] / [bp+0Ch]. The 32-bit len is on the stack, low word at [bp+0Eh] and
; high word at [bp+10h]; the body loads them into CX and BX, and BX is the count of 64 KiB blocks,
; the layout of the prototype in src/I0AED.C. Returns nothing.
_Unpack0 PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    di
        push    dx
        push    ds
        push    es
        push    si
        mov     si,[bp+6]
        mov     ds,[bp+8]
        mov     di,[bp+0Ah]
        mov     es,[bp+0Ch]
        mov     ax,di
        mov     dx,es
        push    cs
        call    near ptr _NormFar
        mov     di,ax
        mov     es,dx
        mov     ax,si
        mov     dx,ds
        push    cs
        call    near ptr _NormFar
        mov     si,ax
        mov     ds,dx
        mov     cx,[bp+0Eh]
        mov     bx,[bp+10h]
        shr     cx,1
        je      OddByte
        rep     movsw
OddByte:        jae     CheckBlocks
        movsb
CheckBlocks:    or      bx,bx
        je      Done
NextBlock:      mov     ax,di
        mov     dx,es
        push    cs
        call    near ptr _NormFar
        mov     di,ax
        mov     es,dx
        mov     ax,si
        mov     dx,ds
        push    cs
        call    near ptr _NormFar
        mov     si,ax
        mov     ds,dx
        mov     cx,8000h                ; 8000h words = one 64 KiB block
        rep     movsw
        dec     bx
        jne     NextBlock
Done:           pop     si
        pop     es
        pop     ds
        pop     dx
        pop     di
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
_Unpack0 ENDP

        PUBLIC  _NormalizeFarPointer    ; @ 0B10:006C
; 0B10:006C: take the far pointer at [bp+6] (offset) / [bp+8] (segment) by value, normalize it
; through _NormFar and return the normalized pointer in DX:AX. Named from its callers (see the file
; header).
_NormalizeFarPointer PROC FAR
        push    bp
        mov     bp,sp
        mov     ax,[bp+6]
        mov     dx,[bp+8]
        push    cs
        call    near ptr _NormFar
        pop     bp
        ret
_NormalizeFarPointer ENDP

        PUBLIC  _NormFar                ; @ 0B10:007B
; 0B10:007B: normalize the far pointer AX = offset, DX = segment into DX:AX: AX becomes offset & 0Fh
; and DX becomes segment plus offset shifted right four (the whole paragraphs the offset holds, 0 to
; 0FFFh). Returns the normalized pointer in DX:AX.
_NormFar PROC FAR
        push    ax
        shr     ax,1
        shr     ax,1
        shr     ax,1
        shr     ax,1
        add     dx,ax
        pop     ax
        and     ax,0Fh
        ret
_NormFar ENDP

        END
