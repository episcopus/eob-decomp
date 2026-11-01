; Unit at EOB.EXE 1C00:0001 (235 bytes, 3 functions), kind asm [H] by the map. This is the VROOMM
; overlay loader. 1C00:0000 is a lone retf (cb) and 1C00:0001 a lone 0 byte, both before the first
; function at the even offset 0002 [H data]. 1C00:0002 allocates an 0FA00h byte block (MemAlloc,
; flags 20h), loads an overlay image into it with DOS INT 21h/4B03 and reports BX = 0 on success,
; 0FFFFh on failure; and 1C00:0055 takes a far pointer to an overlay request (word 0 = the DGROUP
; offset of the request name, word +2 = the name's segment, words +4 and +6 = the entry arguments),
; loads the overlay, resizes the block 1C00:0002 loaded the overlay into, to the overlay's size
; (MemRealloc), and calls the overlay's entry through that block. DGROUP is 212Eh, as in E1A72.ASM
; (lines 81, 252, 435, 470) and E1D8F.ASM (line 78).
;
; Far calls to another unit (9A off seg) are declared at module level, after .MODEL LARGE and before
; .CODE, outside any segment, and written `call far ptr _Name` (playbook 30): a plain `call _Name`
; assembles as `push cs; call near`, and an EXTRN inside a segment of its own leaves the segment
; word 0000.

        .MODEL  LARGE

        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007 (E19BC.C)
        EXTRN   _MemRealloc:FAR         ; @ 19BC:018A (E19BC.C)
        EXTRN   g8148:WORD              ; @ ds:8148: 1 when the mouse driver handler is installed (E1A72.ASM)
        EXTRN   g814A:WORD              ; @ ds:814A: 0 disables the handler body (E1A72.ASM, E07A9.C)

        .CODE

; 1C00:0001: a lone 0 byte before the first function at the even offset 0002 (data, not code) [H].
        db      0

; 1C00:0002: load an overlay image. Allocate a 0FA00h byte far block (MemAlloc, flag 20h) and keep
; its segment at DGROUP:818Ah: MemAlloc returns MK_FP(seg + 1, 0) for flag 0x20 (19BC:0007), so its
; offset is 0 and the `or ax,dx` after the MemAlloc call leaves the segment in AX, which the
; `mov es:[bx],ax` after it stores. This 0FA00h is the block the loader reserves for the overlay; 1C00:0055 later shrinks it to
; the overlay's real size with MemRealloc (the reason for this size is not established [H]: 0FA00h is
; 64000 bytes, one 320 by 200 screen, but nothing in the bytes says that is why). Save SP and SS
; across the call (the reason is not established [H]), then load the overlay named by DS:DX through
; the INT 21h/4B03 parameter block at ES:BX = DGROUP:818Ah: word 0 is the load segment (the word at
; DGROUP:818Ah) and word 1 is the relocation factor (DGROUP:818Ch, which no file in src/ writes, so
; check it in the oracle before a port relies on it). Return BX = 0 on success or 0FFFFh on failure.
; It is called near with a pushed CS (`push cs; call`) and ends with retf, so it
; returns far; only 1C00:0055 calls it, passing BX = 818Ah, ES = DGROUP and DS:DX from the request.
        PUBLIC  _LoadOverlayImage       ; @ 1C00:0002
_LoadOverlayImage PROC NEAR
        push    bp
        mov     bp,sp
        push    dx
        push    bx
        push    ds
        push    es
        mov     ax,212Eh
        mov     ds,ax
        mov     ax,20h
        push    ax
        xor     ax,ax
        push    ax
        mov     ax,0FA00h
        push    ax
        call    far ptr _MemAlloc
        add     sp,6
        pop     es
        pop     ds
        pop     bx
        or      ax,dx
        jne     short ldok
        pop     dx
        mov     bx,0FFFFh
        jmp     short ldout
ldok:   pop     dx
        mov     es:[bx],ax
        mov     ds:[8192h],sp
        mov     ds:[8194h],ss
        mov     ax,4B03h
        int     21h
        mov     ax,212Eh
        mov     ds,ax
        mov     sp,ds:[8192h]
        mov     ss,ds:[8194h]
        mov     bx,0FFFFh
        jb      short ldout
        xor     bx,bx
ldout:  pop     bp
        retf
_LoadOverlayImage ENDP

; 1C00:0055: run an overlay request. [bp+6..+9] is a far pointer to the request (the caller, inside
; main at 07C8:0199, passes DS:8121 at 07C8:0254): word 0 is the DGROUP offset the loader is given
; in DS:DX, word +2 is the name's segment (the caller copies it from DS:[bx+3076h] at 07C8:0245 and
; stores it at 07C8:0251, and reads it back on failure, pushing [8123h] and [8121h] as a far pointer
; to PrintString at 07C8:027B to 0283; the loader never reads +2 itself), and the two words at +4
; and +6 are the entry arguments. Load the overlay image with 1C00:0002. Then push the overlay
; segment (the word at DGROUP:818Ah), 1A16h, 27h and call the entry at segment:0000 (the 0 is the
; call target offset stored at 818Eh): the entry's first argument is 27h and its third is the
; overlay segment. 1A16h is the segment of src/E1A16.C (WaitFrames); that the overlay is handed a
; resident segment is evidence, what it does with it is not established, so both arguments stay
; placeholders. The entry returns AX = its function offset (stored at 818Eh) and BX = its size in
; paragraphs; four `shl bx,1` make that size in bytes. MemRealloc (19BC:018A) resizes the overlay's
; own block at segment:0000 to that size and keeps its contents. This function stores 0 at 818Eh and
; the segment at 8190h, and on success clears g814A (ds:814Ah) when g8148 (ds:8148h) is nonzero:
; g8148 is 1 when the mouse driver handler is installed (E1A72.ASM:43, set at :214, cleared at
; :200), and with g814A zero the handler body does nothing (E1A72.ASM:236), so an overlay request
; with the handler installed switches the handler body off (why is not established; E07A9.C:108
; reads zero the other way, guarding the item icon redraw). Then it calls the pointer the entry
; returned through the reallocated block with the two words from the request. AX = 0 on success,
; 0FFFFh on failure, which the caller compares with -1.
; A load failure or a failed MemRealloc or the second entry call leaves the 0FA00h block allocated;
; nothing frees it. No free on failure, not observable, because the caller prints the message, waits
; for a key (lcall 1AAD:053A at 07C8:01DA) and returns from main at once (07C8:0265 to 0277, 01D2
; to 01E1, 0575). The first entry call's AX, BX and flags are not checked; only the second call is
; (the `jb short failsub` after the second call). Who writes the entry arguments at DGROUP:8125h and
; 8127h is not
; established; the caller bytes at 07C8:0249 to 0258 write only 8121h and 8123h.
        PUBLIC  _RunOverlayRequest      ; @ 1C00:0055
_RunOverlayRequest PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    di
        push    ds
        push    dx
        push    es
        push    si
        mov     si,[bp+6]
        mov     ds,[bp+8]
        mov     dx,[si]
        add     si,4
        push    ds
        push    si
        mov     bx,818Ah
        mov     ax,ds
        mov     es,ax
        push    cs
        call    _LoadOverlayImage
        cmp     bx,-1
        je      short fail
        mov     ax,ds:[818Ah]
        xor     bx,bx
        mov     ds:[818Eh],bx
        mov     ds:[8190h],ax
        push    ax
        mov     bx,1A16h
        push    bx
        mov     bx,27h
        push    bx
        call    dword ptr ds:[818Eh]
        pop     cx
        pop     cx
        pop     cx
        mov     ds:[818Eh],ax
        shl     bx,1
        shl     bx,1
        shl     bx,1
        shl     bx,1
        push    bx
        push    cx
        xor     bx,bx
        push    bx
        call    far ptr _MemRealloc
        add     sp,6
        or      ax,dx
        je      short fail
        mov     ds:[8190h],dx
        pop     si
        pop     ds
        lodsw
        mov     bx,ax
        lodsw
        push    ax
        push    bx
        call    dword ptr ds:[818Eh]
        pop     cx
        pop     cx
        jb      short failsub
        mov     ax,212Eh
        mov     ds,ax
        cmp     word ptr [g8148],0
        je      short done
        mov     word ptr [g814A],0
done:   xor     ax,ax
        jmp     short exit1
; The failure path pops ds then si, the reverse of the success path above; both words pushed before
; the load call are discarded and the epilogue restores DS and SI.
fail:   pop     ds
        pop     si
failsub:
        mov     ax,0FFFFh
exit1:  pop     si
        pop     es
        pop     dx
        pop     ds
        pop     di
        pop     cx
        pop     bx
        pop     bp
        retf
_RunOverlayRequest ENDP

        END
