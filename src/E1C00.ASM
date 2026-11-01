; Unit at EOB.EXE 1C00:0001 (235 bytes, 3 functions), kind asm [H] by the map. The first byte is a
; pad, then the VROOMM overlay loader: allocate a 0FA00h byte block, load an overlay into it with
; DOS INT 21h/4B03, keep the block in DGROUP:818Ah and call its entry point at segment:0000.
; Placeholder names; DGROUP is 212Eh as everywhere in EOB.EXE.
;
; The externals are declared in a segment of their own so that TASM treats them as far (9A) calls:
; in .MODEL LARGE a bare `EXTRN name:FAR` is assumed to live in the module's own code segment and
; the call is written as `push cs; call near` instead.

        .MODEL  LARGE

STUB_TEXT SEGMENT PARA PUBLIC 'CODE'
        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007 (E07A9.C)
        EXTRN   _sub19BC_018A:FAR       ; @ 19BC:018A
STUB_TEXT ENDS

        .CODE

; 1C00:0001: alignment pad before 1C00:0002 (one zero byte)
        PUBLIC  _f1C00_0001             ; @ 1C00:0001
_f1C00_0001 db  0

; 1C00:0002: allocate 0FA00h bytes (MemAlloc, flags 20h), store the handle word at DGROUP:818Ah,
; then load an overlay into it with INT 21h/4B03 and return BX = 0 on success, 0FFFFh on failure.
; Called near (push cs; call) only from 1C00:0055, which passes BX = 818Ah and ES = DGROUP.
        PUBLIC  _f1C00_0002             ; @ 1C00:0002 draft
_f1C00_0002 PROC NEAR
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
        call    _MemAlloc
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
_f1C00_0002 ENDP

; 1C00:0055: [bp+6..+9] is a far pointer to an overlay request struct (caller 07C8:0254 passes
; DS:8121). DX = the struct's first word; the loader is called, then the loaded overlay's first
; entry (DGROUP:818Eh = 0, DGROUP:8190h = its segment) is called with (0, 1A16h, 27h); the result
; BX is scaled by 16 and a record is allocated (19BC:018A) and filled. AX = 0 on success, 0FFFFh on
; failure, which the caller compares with ax,-1.
        PUBLIC  _f1C00_0055             ; @ 1C00:0055 draft
_f1C00_0055 PROC FAR
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
        call    _f1C00_0002
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
        call    _sub19BC_018A
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
_f1C00_0055 ENDP

        END
