; Unit at EOB.EXE 1E8B:0004 (2329 bytes, 31 functions), kind asm per the map. This is the VROOMM
; overlay manager that pages the game's overlaid modules in and out of memory: it opens the program
; file, reads its MZ header and FBOV overlay records (000F, 0134), keeps the overlay state in its own
; data segment 20B5 (loaded through cs:[4], the "PATH=" literal at 1E8B:0008) and in DGROUP 212E
; (loaded through cs:[6]), and installs a DOS interrupt handler (04D0) for overlay calls. Evidence:
; the "PATH=" literal and the MZ/FBOV parsing at 000F, the MemAlloc call at 086F (0000:72DE,
; build/re/rtl_compare.json), and the overlay stubs of EOB.EXE (WHITEPAPER 2.3). Many functions are
; still stubs; names and globals below are placeholders until proven.
;
; NOSMART: the original code has LEA of an absolute address (0250: `lea dx,[1Ch]`), which TASM's
; smart code generation would turn into a MOV; NOSMART keeps the LEA.

        .MODEL  LARGE
        EXTRN   _gOverlayIndex:BYTE      ; @ 20C1:0000 (segment 20C1, the overlay index table)
        EXTRN   _gOverlayList:BYTE       ; @ 20BF:0000 (segment 20BF, the overlay record list head)
        EXTRN   _OverlayHalt:FAR         ; @ 0000:503F (runtime library, build/re/rtl_compare.json)
        .CODE
        NOSMART                          ; 000F, 0134, 0185, 01A7, 01F3 and 0250 contain LEA of an
                                         ; absolute address or a far call to a sibling, both of which
                                         ; the smart code generator would shorten (TASM 2.0)

; 1E8B:0004: not a function but the manager's data words at the start of its code segment: the
; segment of its own data (20B5, its cs:[4]), DGROUP (212E, its cs:[6]), the "PATH=" environment
; name (1E8B:0008) and the runtime-patched word cs:[0D] at 1E8B:000D. Kept as a stub because the
; words need segment relocations this file's skeleton does not express yet.
        PUBLIC  _f1E8B_0004              ; @ 1E8B:0004 stub
_f1E8B_0004 PROC FAR
        ret
_f1E8B_0004 ENDP

; 1E8B:000F: initialise the overlay manager. It opens the program's own file (01B3, or the name in
; the far pointer argument at [bp+6] through 01A7, or the PATH search 01F3), reads and checks the MZ
; header and walks the file's FBOV records (the "FB"/"OV" signature, the record offset and length
; from the 10h-byte header at [bp-14h]) to store the overlay-file offset, its base and size at
; DGROUP:84B2/84B4 and 20B5:00A4/00A6, then swaps the overlay file open again
; (_SwapAndReopenOverlayFile), sizes the free area from the arguments at [bp+0Ah]/[bp+0Ch]
; (_InitOverlayRecords) and maps the records (_MapOverlayRecords). Returns AX = 0 on success, 0FFFEh
; for a missing file and 0FFFFh for a bad file. Evidence: the MZ 5A4Dh test, the FBOV 4246h/564Fh
; tests, the 10h header size, the 84B2/84B4 and 00A4..00A8 globals and the 01B3/01A7/01F3/0276/
; 0134/0286/02FF calls.
        PUBLIC  _InitOverlayManager      ; @ 1E8B:000F
_InitOverlayManager PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,14h
        push    ds
        mov     ds,word ptr cs:[4]
        push    si
        push    di
        cld
        cmp     word ptr ds:[0A0h],0
        jne     short init_begin
        jmp     init_ok
init_begin:
        call    _OpenOwnOverlayFile
        jae     short init_opened
        mov     ax,[bp+6]
        or      ax,[bp+8]
        jne     short init_have
        mov     word ptr [bp+6],318h
        mov     word ptr [bp+8],20C1h
init_have:
        call    _OpenNamedOverlayFile
        jae     short init_opened
        call    _OpenOverlayFileViaPath
        jae     short init_opened
        mov     ax,0FFFEh
        jmp     init_fail
init_closefail:
        mov     ah,3Eh
        int     21h
        mov     ax,0FFFFh
        jmp     init_fail
init_opened:
        mov     bx,ax
        mov     ds:[0B8h],bx
        mov     cx,14h
        call    _ReadOverlayHeaderBytes
        mov     cx,0FFFDh
        jb      short init_closefail
        xor     ax,ax
        xor     dx,dx
        cmp     word ptr [bp-14h],5A4Dh
        je      short init_mz
        mov     cx,0FFFCh
        jmp     short init_closefail
init_mz:
        mov     ax,[bp-10h]
        mov     cx,[bp-12h]
        jcxz    short init_nopara
        dec     ax
init_nopara:
        mov     dx,200h
        mul     dx
        add     ax,cx
init_seek:
        push    dx
        push    ax
        mov     cx,dx
        mov     dx,ax
        mov     ax,4200h
        int     21h
        mov     cx,10h
        call    _ReadOverlayHeaderBytes
        pop     ax
        pop     dx
        mov     cx,0FFFDh
        jb      short init_closefail
        add     ax,10h
        adc     dx,0
        cmp     word ptr [bp-14h],4246h
        mov     cx,0FFFCh
        jne     short init_closefail
        cmp     word ptr [bp-12h],564Fh
        je      short init_fbov
        add     ax,[bp-10h]
        adc     dx,[bp-0Eh]
        jmp     short init_seek
init_fbov:
        mov     ds:[0A4h],ax
        mov     ds:[0A6h],dx
        mov     es,word ptr cs:[6]
        mov     ax,[bp-10h]
        mov     es:[84B2h],ax
        mov     ax,[bp-0Eh]
        mov     es:[84B4h],ax
        mov     ah,3Eh
        int     21h
        mov     word ptr ds:[0B8h],0
        call    far ptr _SwapAndReopenOverlayFile
        mov     ax,[bp+0Ch]
        inc     ax
        mov     ds:[0B4h],ax
        mov     ds:[0B0h],ax
        mov     bx,[bp+0Ah]
        mov     ds:[0B6h],bx
        call    _InitOverlayRecords
        mov     bx,ds:[0B6h]
        sub     bx,ds:[0B4h]
        cmp     bx,ds:[0AAh]
        jae     short init_size
        mov     cx,0FFFBh
        jmp     init_closefail
init_size:
        shr     bx,1
        shr     bx,1
        mov     ds:[0A8h],bx
        call    _MapOverlayRecords
        jae     short init_ok
        mov     bx,ds:[0B8h]
        mov     cx,0FFFFh
        jmp     short init_fail
        nop
init_ok:
        xor     ax,ax
init_fail:
        pop     di
        pop     si
        pop     ds
        mov     sp,bp
        pop     bp
        retf    8
_InitOverlayManager ENDP

; 1E8B:0134: swap and (re)open the overlay file. It saves the current INT [20B5:00A1] vector,
; installs the previously saved one from 20B5:0004:0002, stashes the one just read for the next swap,
; then closes the file handle at 20B5:00B8 and reopens the file named in the 20B5:001C buffer (or
; opens it when no handle is open). This is the pair to the shutdown path of 0185. Evidence: INT
; 21h/35h and /25h, the 20B5:00A1 interrupt number, the [0002]/[0004] vector store and the 3Eh/3Dh
; file calls.
        PUBLIC  _SwapAndReopenOverlayFile              ; @ 1E8B:0134
_SwapAndReopenOverlayFile PROC FAR
        push    bp
        mov     bp,sp
        push    ds
        mov     ds,word ptr cs:[4]
        mov     al,ds:[0A1h]
        mov     ah,35h
        int     21h
        push    es
        push    bx
        push    ds
        mov     al,ds:[0A1h]
        mov     dx,ds:[2]
        mov     ds,ds:[4]
        mov     ah,25h
        int     21h
        pop     ds
        pop     word ptr ds:[2]
        pop     word ptr ds:[4]
        cmp     word ptr ds:[0B8h],0
        je      short swap_open
        mov     bx,ds:[0B8h]
        mov     ah,3Eh
        int     21h
        mov     word ptr ds:[0B8h],0
        jmp     short swap_out
swap_open:
        lea     dx,ds:[1Ch]
        mov     ax,3D00h
        int     21h
        mov     ds:[0B8h],ax
swap_out:
        pop     ds
        pop     bp
        retf
_SwapAndReopenOverlayFile ENDP

; 1E8B:0185: shut the overlay manager down. If the overlay file handle at 20B5:00B8 is open, close it
; and restore the swapped interrupt vector through 0134, then run the two cleanup vectors kept at
; 20B5:0014 and 20B5:0012. Evidence: the [0xB8] handle test, the far call to 0134, and the two
; `push cs; call [..]` pointers; the second vector 0012 is also used by the exit path of 086F.
        PUBLIC  _ShutdownOverlayManager  ; @ 1E8B:0185
_ShutdownOverlayManager PROC FAR
        push    bp
        mov     bp,sp
        push    ds
        mov     ds,word ptr cs:[4]
        cmp     word ptr ds:[0B8h],0
        je      short shutdown_nohandle
        call    far ptr _SwapAndReopenOverlayFile
shutdown_nohandle:
        push    cs
        call    word ptr ds:[14h]
        push    cs
        call    word ptr ds:[12h]
        pop     ds
        pop     bp
        retf
_ShutdownOverlayManager ENDP

; 1E8B:01A7: open the overlay file named by this caller's far argument. It copies the name into the
; 20B5:001C buffer with 0250 (the frame and buffer are shared with the caller, 000F), keeping only
; the leaf name after the last backslash. Evidence: the pushed DS/ES, DI = 001C and the near call.
        PUBLIC  _OpenNamedOverlayFile    ; @ 1E8B:01A7
_OpenNamedOverlayFile PROC NEAR
        push    ds
        lea     di,ds:[1Ch]
        push    ds
        pop     es
        call    _CopyNameAndOpenOverlay
        pop     ds
        ret
_OpenNamedOverlayFile ENDP

; 1E8B:01B3: recover the program's own file name and open it. It walks the environment in the PSP
; (DGROUP:0075 gives the PSP segment), skips to the end of the environment, takes the following word
; (the program name in the master PSP tail) and copies it to 20B5:001C up to the last backslash, then
; calls 0250 to open it. Returns carry set on a DOS below 3 or a failed open. Evidence: INT 21h/30h
; and the DGROUP:0075 PSP load, the two-NUL scan and the 0250 call.
        PUBLIC  _OpenOwnOverlayFile      ; @ 1E8B:01B3
_OpenOwnOverlayFile PROC NEAR
        mov     ah,30h
        int     21h
        cmp     al,3
        jb      short own_out
        push    ds
        mov     ax,212Eh
        mov     ds,ax
        mov     ds,ds:[75h]
        xor     si,si
        cld
own_env:
        lodsb
        or      al,al
        jne     short own_env
        lodsb
        or      al,al
        jne     short own_env
        lodsw
        lea     di,ds:[1Ch]
        mov     ax,20B5h
        mov     es,ax
        mov     bx,di
own_copy:
        lodsb
        stosb
        or      al,al
        je      short own_done
        cmp     al,5Ch
        jne     short own_copy
        mov     bx,di
        jmp     short own_copy
own_done:
        mov     di,bx
        call    _CopyNameAndOpenOverlay
        pop     ds
own_out:
        ret
_OpenOwnOverlayFile ENDP

; 1E8B:01F3: find the overlay file through the PATH environment variable. It scans the environment
; (via DGROUP:0075, the PSP segment) for the entry whose first five bytes equal the "PATH=" literal
; at cs:0008, then tries each ';' separated directory by copying it into 20B5:001C, appending a
; backslash when needed (unless the entry ends in ':' or '\') and calling 0250 to open the file.
; Returns carry set when the file is not found. Evidence: the cs:0008 compare, the ';' 3Bh, ':' 3Ah
; and '\' 5Ch tests, and the repeated 0250 call.
        PUBLIC  _OpenOverlayFileViaPath  ; @ 1E8B:01F3
_OpenOverlayFileViaPath PROC NEAR
        push    ds
        mov     ax,212Eh
        mov     ds,ax
        mov     ds,ds:[75h]
        xor     si,si
path_search:
        mov     di,8
        push    cs
        pop     es
        mov     cx,5
        cld
        repe cmpsb
        je      short path_found
        dec     si
path_skip:
        lodsb
        or      al,al
        jne     short path_skip
        cmp     al,[si]
        jne     short path_search
path_fail:
        pop     ds
        stc
        ret
path_found:
        cmp     byte ptr [si],0
        je      short path_fail
        lea     di,ds:[1Ch]
        mov     ax,20B5h
        mov     es,ax
        xor     al,al
path_dir:
        mov     ah,al
        lodsb
        or      al,al
        je      short path_enddir
        cmp     al,3Bh
        je      short path_break
        stosb
        jmp     short path_dir
path_enddir:
        dec     si
path_break:
        cmp     ah,3Ah
        je      short path_open
        cmp     ah,5Ch
        je      short path_open
        mov     al,5Ch
        stosb
path_open:
        push    ds
        push    si
        call    _CopyNameAndOpenOverlay
        pop     si
        pop     ds
        jb      short path_found
        pop     ds
        ret
_OpenOverlayFileViaPath ENDP

; 1E8B:0250: copy the file name this caller's far argument at [bp+6] points at (at most 12 bytes)
; into the 20B5:001C buffer and open it with INT 21h/3D00. A null argument skips the copy. Returns
; carry set on failure. Evidence: the shared frame, DI = 001C, the 12-byte loop and the open call.
        PUBLIC  _CopyNameAndOpenOverlay  ; @ 1E8B:0250
_CopyNameAndOpenOverlay PROC NEAR
        lds     si,[bp+6]
        mov     ax,ds
        or      ax,si
        je      short open_name
        mov     cx,0Ch
copy_name:
        lodsb
        stosb
        or      al,al
        je      short open_name
        loop    copy_name
        sub     al,al
        stosb
open_name:
        lea     dx,ds:[1Ch]
        mov     ax,20B5h
        mov     ds,ax
        mov     ax,3D00h
        int     21h
        ret
_CopyNameAndOpenOverlay ENDP

        SMART

; 1E8B:0276: read CX bytes from the open overlay file (handle in 20B5:00B8) into the 14h-byte buffer
; at the caller's [bp-14h] (the MZ header scratch of 000F, whose frame this helper shares). Returns
; carry set on a DOS error or when fewer than CX bytes were read. Evidence: the SS:BP buffer, the
; shared frame and the two calls to this from 000F.
        PUBLIC  _ReadOverlayHeaderBytes  ; @ 1E8B:0276
_ReadOverlayHeaderBytes PROC NEAR
        push    ds
        lea     dx,[bp-14h]
        push    ss
        pop     ds
        mov     ah,3Fh
        int     21h
        pop     ds
        jb      short hdr_out
        cmp     ax,cx
hdr_out:
        ret
_ReadOverlayHeaderBytes ENDP

; 1E8B:0286: build the overlay record list and size it. It stores DGROUP:0079 + 10h (the start of
; free memory after the program) into cs:000D, then walks the record table at 20BF:00C0 in 8-byte
; steps: for each record marked word +4 bit 1 with a non-zero word +2, it points the record at its
; own segment (ES:0012), sets ES:0018 to the 4A2h marker when ES:001A is not FF, adds the DGROUP
; [00A4]:[00A6] size to ES:0004:0006, and keeps the largest total (through 0789). It ends by
; storing the largest size + 2 at 20B5:00AA. Evidence: the 20BF:00C0 table, the 4A2h marker (also
; cleared at 05FB), the 00A4/00A6 size and the 0789 call.
        NOSMART                          ; contains LEA of an absolute address (029F)
        PUBLIC  _InitOverlayRecords      ; @ 1E8B:0286
_InitOverlayRecords PROC NEAR
        mov     ax,DGROUP
        mov     es,ax
        mov     ax,es:[79h]
        add     ax,10h
        mov     word ptr cs:[0dH],ax
        mov     ax,seg _gOverlayList
        mov     es,ax
        xor     bx,bx
        xor     di,di
        lea     si,ds:[0C0h]
init_test:
        test    word ptr [si+4],2
        je      short init_next
        cmp     word ptr [si+2],0
        jne     short init_take
init_next:
        jmp     short init_adv
        nop
init_take:
        mov     ax,[si]
        push    es
        mov     es:[12h],ax
        mov     es,ax
        cmp     byte ptr es:[1ah],0FFh
        jne     short init_use
        pop     es
        jmp     short init_adv
        nop
init_use:
        pop     ax
        mov     word ptr es:[18h],4A2h
        mov     ax,ds:[0A4h]
        mov     dx,ds:[0A6h]
        add     word ptr es:[4],ax
        adc     word ptr es:[6],dx
        call    _OverlayTotalParagraphs
        cmp     bx,dx
        jae     short init_adv
        xchg    bx,dx
init_adv:
        add     si,8
        cmp     si,3D8h
        jae     short init_done
        jmp     short init_test
init_done:
        xor     ax,ax
        add     bx,2
        mov     ds:[0AAh],bx
        ret
_InitOverlayRecords ENDP
        SMART

; 1E8B:02FF: load an overlay into memory. It starts from the free segment 20B5:00B2, walks the
; overlay's own file records (ES:[0012] chain) fitting each in the free space up to 20B5:00B6, links
; them through 20B5:001C, then reloads the overlay file: it computes the 4 KB-rounded size, reads
; the image with 03C4, applies the relocations (03FD) and fills the version table (066F), then calls
; the overlay's entry through 20B5:0016 repeatedly until its successor chain at ES:001C ends.
; Returns carry set on a read or overlay failure. The tail at 03B7 is an uncalled sibling that
; converts a DX:AX byte count to a segment value. Evidence: the 00B2/00B4/00B6 free-memory words,
; the 0012/001C record chain, the 03C4/03FD/066F calls and the 20B5:0016 far pointer.
        PUBLIC  _MapOverlayRecords        ; @ 1E8B:02FF
_MapOverlayRecords PROC NEAR
        mov     cx,ds:[0B2h]
        mov     ds:[0BCh],cx
        mov     bx,cx
        mov     si,ds:[0B4h]
        mov     di,ds:[0B6h]
        push    ds
chain_next:
        mov     ds,cx
        mov     cx,ds:[12h]
        jcxz    short chain_tail
        mov     es,cx
        mov     ax,es:[4]
        mov     dx,es:[6]
        sub     ax,ds:[4]
        sbb     dx,ds:[6]
        mov     cx,10h
        div     cx
        add     ax,si
        cmp     ax,di
        ja      short chain_tail
        mov     ds:[10h],si
        mov     si,ax
        mov     cx,es
        mov     ds:[1Ch],cx
        mov     bx,ds
        jmp     short chain_next
chain_tail:
        mov     ds,bx
        mov     word ptr ds:[1Ch],0
        pop     ds
        mov     ds:[0B0h],si
        mov     ax,ds:[0B4h]
        sub     si,ax
        jne     short chain_load
        jmp     short chain_out
        nop
chain_load:
        mov     cl,4
        rol     si,cl
        mov     di,si
        and     di,0Fh
        and     si,0FFF0h
        mov     es,ds:[0BCh]
        mov     dx,es:[4]
        mov     cx,es:[6]
        call    _ReadOverlayImage
        jb      short chain_ret
        mov     ax,ds:[0BCh]
        push    ds
chain_read:
        mov     es,ax
        mov     cx,es:[0Ah]
        jcxz    short chain_ver
        call    _ApplyOverlayRelocations
chain_ver:
        call    _FillOverlayVersionTable
        mov     ax,es:[10h]
        dec     ax
        mov     ds,ax
        mov     ds:[0Eh],es
        push    es
        mov     ax,0FFFFh
        mov     bx,ax
        mov     ds,word ptr cs:[4]
        call    dword ptr ds:[16h]
        pop     es
        mov     ax,es:[1Ch]
        or      ax,ax
        jne     short chain_read
        pop     ds
chain_out:
        clc
chain_ret:
        ret
; 1E8B:03B7: uncalled sibling in this range: DX:AX bytes to a segment value.
        mov     cl,4
        shr     ax,cl
        ror     dx,cl
        and     dx,0F000h
        or      ax,dx
        ret
_MapOverlayRecords ENDP

; 1E8B:03C4: read SI:DI bytes of the overlay image from the open file into memory, four KB at a
; time. It seeks to DX:CX in the file (INT 21h/4200h), then loops reading 0FFF0h bytes (or SI when
; DI is zero) into DS:0000, advancing DS by 0FFFh to step 4 KB, until SI:DI reaches zero. Returns
; carry set on a DOS error or a short read. Evidence: the 4200h seek, AH = 3Fh read and the DS step.
        PUBLIC  _ReadOverlayImage        ; @ 1E8B:03C4
_ReadOverlayImage PROC NEAR
        push    ax
        mov     bx,ds:[0B8h]
        mov     ax,4200h
        int     21h
        pop     ax
        push    ds
        mov     ds,ax
        jmp     short img_next
img_step:
        mov     ax,ds
        add     ax,0FFFh
        mov     ds,ax
img_next:
        mov     cx,0FFF0h
        or      di,di
        jne     short img_count
        mov     cx,si
img_count:
        xor     dx,dx
        mov     ah,3Fh
        int     21h
        jb      short img_out
        cmp     ax,cx
        jb      short img_out
        sub     si,ax
        sbb     di,0
        mov     ax,si
        or      ax,di
        jne     short img_step
img_out:
        pop     ds
        ret
_ReadOverlayImage ENDP

; 1E8B:03FD: apply the CX relocations stored at the overlay's image base. It walks the CX words at
; the record's ES:0008/ES:000A (the reloc list), reads each target word through the segment 20C1
; table and, when the low bit is set, fixes it up through 0442 (the far-pointer relocation). The
; ES:0010 base is the segment the overlay was loaded at. Evidence: the ES:0008/000A address, the
; 20C1 table segment (relocated) and the 0442 call on the odd flag.
        PUBLIC  _ApplyOverlayRelocations              ; @ 1E8B:03FD
_ApplyOverlayRelocations PROC NEAR
        push    ds
        push    es
        mov     ax,es:[8]
        mov     si,ax
        and     si,0Fh
        shr     ax,1
        shr     ax,1
        shr     ax,1
        shr     ax,1
        mov     dx,es:[10h]
        add     ax,dx
        mov     ds,ax
        mov     es,dx
        shr     cx,1
        cld
fix_loop:
        lodsw
        mov     bx,ax
        mov     di,es:[bx]
        push    ds
        mov     ax,seg _gOverlayIndex
        mov     ds,ax
        mov     ax,di
        and     di,0FFF8h
        mov     dx,[di]
        mov     es:[bx],dx
        test    ax,1
        je      short fix_skip
        call    _RelocateOverlayPointer
fix_skip:
        pop     ds
        loop    fix_loop
        pop     es
        pop     ds
        ret
_ApplyOverlayRelocations ENDP

; 1E8B:0442: relocate one far pointer in the overlay image. Given DS:DX (the overlay base) and the
; pointer at ES:BX, it checks the b8 50 pattern of an overlay call fixup (the four masked bytes at
; ES:BX-1, +2, +3, +6), then replaces the ES:[BX+4] entry with its 20B5:0020 version slot. The tail
; at 04A2 is an uncalled sibling that loads a fresh overlay copy: it seeks ES:0004:0006, reads
; ES:0008+ES:000A bytes with 03C4 and applies the relocations with 03FD.
; Evidence: the 0F807h masks and b8/50 byte tests; the 03C4/03FD calls in the 04A2 tail.
        PUBLIC  _RelocateOverlayPointer              ; @ 1E8B:0442
_RelocateOverlayPointer PROC NEAR
        push    cx
        mov     ds,dx
        mov     ah,es:[bx-1]
        mov     al,ah
        and     ax,0F807h
        cmp     ah,0B8h
        jne     short rel_out
        mov     cx,ax
        mov     ah,es:[bx+2]
        mov     al,ah
        and     ax,0F807h
        cmp     ah,50h
        jne     short rel_out
        cmp     al,cl
        jne     short rel_out
        mov     dx,ax
        mov     ah,es:[bx+3]
        mov     al,ah
        and     ax,0F807h
        cmp     cx,ax
        jne     short rel_out
        mov     ah,es:[bx+6]
        mov     al,ah
        and     ax,0F807h
        cmp     ax,dx
        jne     short rel_out
        mov     di,20h
        mov     cx,ds:[0Ch]
        mov     ax,es:[bx+4]
rel_slot:
        cmp     ax,[di+2]
        je      short rel_found
        add     di,5
        loop    rel_slot
        jmp     short rel_out2
rel_found:
        mov     es:[bx+4],di
rel_out:
        pop     cx
        ret
rel_out2:
        pop     cx
        ret
; 1E8B:04A2: uncalled sibling in this range: reload an overlay image.
        mov     si,es:[8]
        xor     di,di
        add     si,es:[0Ah]
        adc     di,0
        mov     dx,es:[4]
        mov     cx,es:[6]
        mov     ax,es:[10h]
        call    _ReadOverlayImage
        jb      short reload_out
        mov     cx,es:[0Ah]
        jcxz    short reload_clc
        call    _ApplyOverlayRelocations
reload_clc:
        clc
reload_out:
        ret
_RelocateOverlayPointer ENDP

; 1E8B:04D0: the overlay call interrupt handler. An odd BP means the call came from outside the
; overlay manager, so it aborts through __OverlayHalt; otherwise it saves the registers, switches to
; the manager's data segment 20B5, pushes the request word at ES:[BX] and loads the overlay
; (_LoadOverlayRecord), either the single entry at [bp+2] or the two-word form at [bp+6]/[bp+2],
; then calls the overlay's routine through 20B5:0016 with the ES:001A bit 8 value in AX. Ends with
; IRET. Evidence: the BP parity guard, the __OverlayHalt jump, the ES:001A bit 8 and the 20B5:0016
; far pointer (the same one 02FF uses).
        PUBLIC  _OverlayCallHandler      ; @ 1E8B:04D0
_OverlayCallHandler PROC FAR
        push    bp
        mov     bp,sp
        test    bp,1
        je      short handler_ok
        jmp     far ptr _OverlayHalt
handler_ok:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    ds
        push    es
        mov     ax,20B5h
        mov     ds,ax
        sti
        les     bx,[bp+2]
        push    word ptr es:[bx]
        sub     word ptr [bp+2],2
        jne     short handler_multi
        call    _LoadOverlayRecord
        jmp     short handler_leave
handler_multi:
        add     bp,6
        mov     ax,[bp]
        xchg    word ptr [bp-6],ax
        mov     [bp],ax
        call    _LoadOverlayRecord
        mov     ax,[bp]
        xchg    word ptr [bp-6],ax
        mov     [bp],ax
handler_leave:
        pop     bx
        mov     al,es:[1Ah]
        and     al,8
        and     byte ptr es:[1Ah],0F7h
        cbw
        mov     ds,word ptr cs:[4]
        call    dword ptr ds:[16h]
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        iret
_OverlayCallHandler ENDP

; 1E8B:0536: age the overlay records after a version-record update. It bumps the 20B5:00AC counter,
; then for each record in the 20B5:00BC chain: when its ES:0010 entry is set, marks it used (ES:001B
; = 1, ES:001A |= 4); otherwise sets the unused bit (ES:001A |= 8), calls 0613 on it, updates
; ES:000E, calls the record's ES:0018 routine and relinks it through 0711. It ends
; when the byte count 0761 says no older record remains, then stores the new top at ES:0010. It
; aborts through __OverlayHalt (0000:503F) if the overlay routine fails. Evidence: the [00AC]/[00BA]
; counters, the ES:001A bit flags, the [00A8] limit and the __OverlayHalt jump.
        PUBLIC  _AgeOverlayRecords       ; @ 1E8B:0536
_AgeOverlayRecords PROC NEAR
        push    es
        inc     word ptr ds:[0BAh]
        call    _OverlayTotalParagraphs
        jmp     short age_test
age_loop:
        popf
        push    dx
        jae     short age_replace
        call    _DiscardOverlayRecords
age_replace:
        mov     es,ds:[0BCh]
        mov     ax,es:[1Ch]
        mov     ds:[0BCh],ax
        cmp     byte ptr es:[1Bh],0
        jne     short age_dec
        call    _ReleaseOverlayFpuState
        call    _OverlayCodeParagraphs
        jmp     short age_pop
age_dec:
        dec     byte ptr es:[1Bh]
        call    _MoveOverlayRecord
        call    _LinkOverlayRecord
age_pop:
        pop     dx
age_test:
        call    _OverlayBytesRemaining
        pushf
        cmp     dx,ax
        ja      short age_loop
        popf
        pop     es
        mov     ax,ds:[0B0h]
        mov     es:[10h],ax
        ret
_AgeOverlayRecords ENDP

; 1E8B:0580: bring an overlay record ES: into memory. It bumps the 20B5:00AC counter, marks the
; record used or unused (ES:001B/ES:001A), ages the records through 0536 or runs the record's
; ES:0018 routine, relinks through 0711, and then, while the overlay's entry chain at 20B5:00BC has
; records (0761 says how many bytes are left), clears or restores each record's version table (068D
; and 077D) until the remaining size reaches the 20B5:00A8 limit. A failed overlay routine jumps to
; __OverlayHalt. Evidence: the ES:001A/001B flags, the 0536/0711/064E/0761/068D/077D calls, the
; [00A8] limit and the __OverlayHalt jump.
        PUBLIC  _LoadOverlayRecord       ; @ 1E8B:0580
_LoadOverlayRecord PROC NEAR
        inc     word ptr ds:[0ACh]
        cmp     word ptr es:[10h],0
        je      short load_unused
        mov     byte ptr es:[1Bh],1
        or      byte ptr es:[1Ah],4
        jmp     short load_after
        nop
load_unused:
        or      byte ptr es:[1Ah],8
        call    _AgeOverlayRecords
        push    ds
        dec     ax
        mov     ds,ax
        mov     ds:[0Eh],es
        pop     ds
        call    word ptr es:[18h]
        jb      short load_halt
        call    _LinkOverlayRecord
load_after:
        call    _RestoreOverlayVersionTable
        mov     al,es:[1Ah]
        and     al,3
        add     byte ptr es:[1Bh],al
        push    es
        call    _OverlayBytesRemaining
        mov     es,ds:[0BCh]
load_chain:
        mov     cx,es:[1Ch]
        jcxz    short load_out
        cmp     ax,ds:[0A8h]
        jae     short load_out
        push    cx
        push    ax
        cmp     byte ptr es:[1Bh],0
        je      short load_clear
        xor     ax,ax
        jmp     short load_add
load_clear:
        call    _ClearOverlayVersionTable
        call    _OverlayCodeParagraphs
load_add:
        pop     cx
        pop     es
        add     ax,cx
        jmp     short load_chain
load_out:
        pop     es
        ret
load_halt:
        jmp     far ptr _OverlayHalt
_LoadOverlayRecord ENDP

; 1E8B:05FB: release the numeric coprocessor state of the overlay record ES: the entry at ES:0018
; holds 4A2h when the emulator (or a real FPU) installed its own replacement, so call the previous
; entry through the vector at 20B5:0010 and clear ES:0010. Evidence: the 4A2h test, the indirect
; call and the clearing store, matching the save at 02FF:02C9.
        PUBLIC  _ReleaseOverlayFpuState   ; @ 1E8B:05FB
_ReleaseOverlayFpuState PROC NEAR
        call    _ClearOverlayVersionTable
        cmp     word ptr es:[18h],4A2h
        jne     short fpu_done
        call    word ptr ds:[10h]
fpu_done:
        mov     word ptr es:[10h],0
        ret
_ReleaseOverlayFpuState ENDP

; 1E8B:0613: discard every overlay record in the 20B5:00BC chain. It walks the chain through
; ES:[001C], counts the records in CX, then walks it back (the segments are on the stack) from the
; end, unlinking each from the 20B5:00BC head via 06C0. It then resets the running size at 20B5:00B0
; to the file size 20B5:00B4. Evidence: the ES:001C chain, the 06C0/077D calls and the [00B4] reset.
        PUBLIC  _DiscardOverlayRecords   ; @ 1E8B:0613
_DiscardOverlayRecords PROC NEAR
        mov     ax,ds:[0BCh]
        xor     cx,cx
disc_walk:
        inc     cx
        push    ax
        mov     es,ax
        mov     ax,es:[1Ch]
        or      ax,ax
        jne     short disc_walk
        mov     ds:[0BCh],ax
        mov     ax,ds:[0B6h]
        mov     ds:[0B0h],ax
disc_unlink:
        pop     es
        push    cx
        mov     ax,ds:[0BCh]
        mov     es:[1Ch],ax
        mov     ds:[0BCh],es
        call    _OverlayCodeParagraphs
        sub     ds:[0B0h],ax
        call    _MoveOverlayRecord
        pop     cx
        loop    disc_unlink
        mov     ax,ds:[0B4h]
        mov     ds:[0B0h],ax
        ret
_DiscardOverlayRecords ENDP

; 1E8B:064E: restore an overlay record's version table. With no version records (ES:000C = 0) it
; does nothing; when the record's marker at ES:0020 is 0EAh (a live overlay entry) it falls into
; _FillOverlayVersionTable at 066F; otherwise it strips the record's entry from the patch table
; (072F) and, when a version chain at ES:0002 exists, re-fills the table. Evidence: the ES:000C
; test, the 0EAh marker and the 072F call.
        PUBLIC  _RestoreOverlayVersionTable ; @ 1E8B:064E
_RestoreOverlayVersionTable PROC NEAR
        cmp     word ptr es:[0Ch],0
        jne     short rest_have
        ret
rest_have:
        cmp     byte ptr es:[20h],0EAh
        je      short ver_ret
        mov     cx,es:[2]
        jcxz    _FillOverlayVersionTable
        mov     ax,es:[10h]
        mov     dx,es
        call    _PatchOverlayEntryVector
_RestoreOverlayVersionTable ENDP

; 1E8B:066F: fill the CX overlay-version records at ES:0020 with the entry-point far pointer
; (0EAh, the offset at ES:0010 and the base segment ES), so a later load can find the overlay's
; entry. Evidence: the 0EAh marker byte, the stores of ES:0010 and DI = 0020 stepping by 5.
        PUBLIC  _FillOverlayVersionTable  ; @ 1E8B:066F
_FillOverlayVersionTable PROC NEAR
        mov     bx,es:[10h]
        mov     cx,es:[0Ch]
        mov     di,20h
        cld
fill_ver:
        mov     dx,es:[di+2]
        mov     al,0EAh
        stosb
        mov     ax,dx
        stosw
        mov     ax,bx
        stosw
        loop    fill_ver
ver_ret:
        ret
_FillOverlayVersionTable ENDP

; 1E8B:068D: clear the overlay's own saved state. Unless the record's marker at ES:0020 is CDh, it
; first removes the overlay's entry from the patch table through 072F, clears ES:0002, then rewrites
; the CX version records at ES:0020 with the old entry at ES:0020, the current DGROUP:00A0 and a
; zero marker. Evidence: the CDh test, the 072F call and the DI = 0020 loop.
        PUBLIC  _ClearOverlayVersionTable ; @ 1E8B:068D
_ClearOverlayVersionTable PROC NEAR
        cmp     byte ptr es:[20h],0CDh
        je      short clr_out
        mov     ax,es
        mov     dx,es:[10h]
        xor     cx,cx
        call    _PatchOverlayEntryVector
        mov     es:[2],cx
        mov     cx,es:[0Ch]
        mov     di,20h
        cld
clr_ver:
        mov     dx,es:[di+1]
        mov     ax,ds:[0A0h]
        stosw
        mov     ax,dx
        stosw
        xor     al,al
        stosb
        loop    clr_ver
clr_out:
        ret
_ClearOverlayVersionTable ENDP

; 1E8B:06C0: move the overlay record ES: in memory. It moves ES:0008 bytes (in words) from the old
; base ES:0010 to the new base 20B5:00B0, forward or backward (STD) depending on the direction, then
; updates ES:000E and, unless ES:0020 is CDh, strips the record from the patch table (073B) and
; clears the CX version records at ES:0023. Evidence: the ES:0008 length, the ES:0010/DGROUP:00B0
; bases, the rep movsw direction choice and the 073B call.
        PUBLIC  _MoveOverlayRecord       ; @ 1E8B:06C0
_MoveOverlayRecord PROC NEAR
        mov     ax,ds:[0B0h]
        mov     dx,es:[10h]
        mov     es:[10h],ax
        mov     cx,es:[8]
        inc     cx
        shr     cx,1
        xor     si,si
        cld
        cmp     ax,dx
        jb      short move_forward
        mov     si,cx
        dec     si
        shl     si,1
        std
move_forward:
        mov     di,si
        push    ds
        push    es
        mov     ds,dx
        mov     es,ax
        rep movsw
        cld
        dec     ax
        mov     ds,ax
        pop     es
        mov     ds:[0Eh],es
        inc     ax
        pop     ds
        cmp     byte ptr es:[20h],0CDh
        je      short move_out
        call    _FindOverlaySlot
        mov     cx,es:[0Ch]
        mov     di,23h
        cld
move_label:
        stosw
        add     di,3
        loop    move_label
move_out:
        ret
_MoveOverlayRecord ENDP

; 1E8B:0711: add the paragraph count of the overlay record ES to the running total at 20B5:00B0,
; then link that record into the tail of the list held at 20BF:001C (walking the [001C] successor
; chain by loading each successor segment into DS until a null link). Evidence: the 077D call and
; the add to [0B0], the 20BF segment and the [001C] chain walk.
        PUBLIC  _LinkOverlayRecord        ; @ 1E8B:0711
_LinkOverlayRecord PROC NEAR
        call    _OverlayCodeParagraphs
        add     word ptr ds:[0B0h],ax
        push    ds
        mov     ax,20BFh
rec_tail:
        mov     ds,ax
        mov     ax,ds:[1Ch]
        or      ax,ax
        jne     short rec_tail
        mov     ds:[1Ch],es
        mov     es:[1Ch],ax
        pop     ds
        ret
_LinkOverlayRecord ENDP

; 1E8B:072F: find in the table at SS:BP (BP = CX * 2 into the table) the slot whose ES:0010 equals
; DX, save the old entry pointer at SS:[BP+4] into the far function pointer 20B5:0010, and if BX is
; non-zero patch that slot to CX. Evidence: the 073B search, the SS:[BX+2] exchange with CX and the
; 20B5:0010 pointer used by 05FB.
        PUBLIC  _PatchOverlayEntryVector  ; @ 1E8B:072F
_PatchOverlayEntryVector PROC NEAR
        call    _FindOverlaySlot
        or      bx,bx
        je      short patch_out
        xchg    word ptr ss:[bx+2],cx
patch_out:
        ret
_PatchOverlayEntryVector ENDP

; 1E8B:073B: walk the overlay table at SS:BP backward for the slot whose far segment at SS:[BP+4]
; equals DX; BX = the first matching slot whose far offset at SS:[BP+2] is zero (the free one), and
; its SS:[BP+4] is set to AX. Entries whose low bit is set (SS:[BP] odd) are skipped one level down.
; Evidence: the shr word,1 / jb pair, the DX compare and the AX store.
        PUBLIC  _FindOverlaySlot          ; @ 1E8B:073B
_FindOverlaySlot PROC NEAR
        xor     bx,bx
        push    cx
        push    bp
        jmp     short slot_scan
slot_adv:
        shl     cx,1
        mov     bp,cx
slot_scan:
        mov     cx,[bp]
        shr     cx,1
        je      short slot_out
        jb      short slot_adv
        cmp     dx,[bp+4]
        jne     short slot_adv
        mov     [bp+4],ax
        or      bx,bx
        jne     short slot_adv
        mov     bx,bp
        jmp     short slot_adv
slot_out:
        pop     bp
        pop     cx
        ret
_FindOverlaySlot ENDP

; 1E8B:0761: return in AX how many bytes of the current overlay segment (ES:0010) remain before the
; top of the allocated area at 20B5:00B0, and carry set when the result is a fallback computed from
; the file size at 20B5:00B6 instead. Evidence: the [00BC] selector, the two subtractions from
; [00B0] and the STC on the fallback path.
        PUBLIC  _OverlayBytesRemaining   ; @ 1E8B:0761
_OverlayBytesRemaining PROC NEAR
        mov     ax,ds:[0BCh]
        or      ax,ax
        je      short rem_fallback
        mov     es,ax
        mov     ax,es:[10h]
        sub     ax,ds:[0B0h]
        jae     short rem_out
rem_fallback:
        mov     ax,ds:[0B6h]
        sub     ax,ds:[0B0h]
        stc
rem_out:
        ret
_OverlayBytesRemaining ENDP

; 1E8B:077D: return in AX the number of paragraphs the overlay record ES: occupies for its code:
; (size at ES:0008 plus the 10h byte header plus 0Fh rounding) shifted right by 4. Evidence: the
; ES:0008 load, the +11h and the 4-bit shift shared with 0789.
        PUBLIC  _OverlayCodeParagraphs   ; @ 1E8B:077D
_OverlayCodeParagraphs PROC NEAR
        mov     ax,es:[8]
        add     ax,11h
        mov     cl,4
        shr     ax,cl
        ret
_OverlayCodeParagraphs ENDP

; 1E8B:0789: return in DX:AX the total size in paragraphs of the overlay record ES (code through
; _OverlayCodeParagraphs, data from ES:000A plus 0Fh). The trailing code at 0789+18 through 079A is a
; second, uncalled routine 07A1 that checks whether AX is a loaded overlay segment in 20B5:00B4 to
; 00B6, returns its base segment in ES or AX = 0 with carry set; it shares this function's range.
; Evidence: the two shifts and the ES:000A load; the 07A1 range check against [00B4]/[00B6].
        PUBLIC  _OverlayTotalParagraphs  ; @ 1E8B:0789
_OverlayTotalParagraphs PROC NEAR
        mov     cl,4
        mov     ax,es:[8]
        add     ax,11h
        shr     ax,cl
        mov     dx,es:[0Ah]
        add     dx,0Fh
        shr     dx,cl
        add     dx,ax
        ret
; 1E8B:07A1: uncalled sibling in this range: resolve AX to a loaded overlay record.
        push    ds
        push    es
        mov     ds,word ptr cs:[4]
        cmp     ax,ds:[0B4h]
        jb      short rec_notloaded
        cmp     ax,ds:[0B6h]
        jae     short rec_notloaded
        dec     ax
        mov     es,ax
        mov     es,es:[0Eh]
        inc     ax
        mov     bx,ds:[0A0h]
        cmp     bx,es:[0]
        jne     short rec_notloaded
        cmp     ax,es:[10h]
        mov     ax,es
        je      short rec_loaded
rec_notloaded:
        xor     ax,ax
        stc
rec_loaded:
        pop     es
        pop     ds
        retf
_OverlayTotalParagraphs ENDP

        PUBLIC  _f1E8B_07D7              ; @ 1E8B:07D7 stub
_f1E8B_07D7 PROC FAR
        ret
_f1E8B_07D7 ENDP

        PUBLIC  _f1E8B_086F              ; @ 1E8B:086F stub
_f1E8B_086F PROC FAR
        ret
_f1E8B_086F ENDP

        END
