; Unit at EOB.EXE 1C00:0001 (235 bytes, 3 functions), kind asm [H] by the map. This is the VROOMM
; overlay loader. 1C00:0001 is a one byte alignment pad; 1C00:0002 allocates an 0FA00h byte block
; (MemAlloc, flags 20h), loads an overlay image into it with DOS INT 21h/4B03 and reports BX = 0 on
; success, 0FFFFh on failure; and 1C00:0055 takes a far pointer to an overlay request (word 0 = the
; DGROUP offset of the request name, then two words of entry arguments), loads the overlay, allocates
; a record for it and calls the overlay's entry through that block. DGROUP is 212Eh as everywhere
; in EOB.EXE.
;
; Far calls to another unit (9A off seg) are declared at module level, after .MODEL LARGE and before
; .CODE, outside any segment, and written `call far ptr _Name` (playbook 30): a plain `call _Name`
; assembles as `push cs; call near`, and an EXTRN inside a segment of its own leaves the segment
; word 0000.

        .MODEL  LARGE

        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007 (E19BC.C)
        EXTRN   _MemRealloc:FAR         ; @ 19BC:018A (E19BC.C)

        .CODE

; 1C00:0001: alignment pad before 1C00:0002 (one zero byte).
        db      0

; 1C00:0002: load an overlay image. Allocate an 0FA00h byte block of DGROUP (MemAlloc with flags
; 20h, which returns it paragraph aligned), keep its segment at DGROUP:818Ah, save SP and SS across
; the call (INT 21h/4B03 uses the caller's stack), then load the overlay named by DS:DX into the
; load block ES:BX = DGROUP:818Ah. Return BX = 0 on success or 0FFFFh on failure. It is called near
; only from 1C00:0055, which passes BX = 818Ah, ES = DGROUP and DS:DX from the request.
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

; 1C00:0055: run an overlay request. [bp+6..+9] is a far pointer to the request (caller 07C8:0254
; passes DS:8121): word 0 is the DGROUP offset the loader is given in DS:DX, and the two words at
; +4 are the entry arguments. Load the overlay image with 1C00:0002, call the loaded overlay's entry
; at segment:0000 with (0, 1A16h, 27h), scale its return by 16 and MemRealloc a record of that size
; at offset 0, then call the pointer the entry returned through the reallocated block with the two
; words from the request. AX = 0 on success, 0FFFFh on failure, which the caller compares with -1.
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
        cmp     word ptr ds:[8148h],0
        je      short done
        mov     word ptr ds:[814Ah],0
done:   xor     ax,ax
        jmp     short exit1
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
