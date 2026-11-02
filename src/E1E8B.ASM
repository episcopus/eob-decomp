; Unit at EOB.EXE 1E8B:0004 (2329 bytes, 31 functions), kind asm per the map. This is the VROOMM
; overlay manager that pages the game's overlaid modules in and out of memory: it opens the program
; file, reads its MZ header and FBOV overlay records (000F, 0134), keeps the overlay state in its own
; data segment 20B5 (loaded through cs:[4], the "PATH=" literal at 1E8B:0008) and in DGROUP 212E
; (loaded through cs:[6]), and serves overlay calls through 04D0, which is entered through a DOS
; interrupt vector: nothing in this unit stores that vector (04D0 has no direct caller and no vector
; store; 0134 only swaps whatever 20B5:0002/0004 already hold), so its installer is not in this unit.
; Evidence: the "PATH=" literal and the MZ/FBOV parsing at 000F, the farmalloc call at 086F
; (0000:72DE, build/re/rtl_compare.json), and the overlay stubs of EOB.EXE (WHITEPAPER 2.3).
;
; NOSMART: the original code has LEA of an absolute address (0250: `lea dx,[1Ch]`), which TASM's
; smart code generation would turn into a MOV; NOSMART keeps the LEA.

        .MODEL  LARGE
        EXTRN   _gTlinkSegmentTable:BYTE  ; @ 20C1:0000 (segment 20C1, TLINK's segment table: 99
                                         ; entries of 8 bytes, frame paragraph at +0; WHITEPAPER 2.2)
        EXTRN   _gOverlayList:BYTE       ; @ 20BF:0000 (segment 20BF, the overlay record list head)
        EXTRN   _OverlayHalt:FAR         ; @ 0000:503F (runtime library, build/re/rtl_compare.json)
        EXTRN   _farmalloc:FAR           ; @ 0000:72DE (runtime library, build/re/rtl_compare.json)
        EXTRN   _abort:FAR               ; @ 0000:01F1 (runtime library, build/re/rtl_compare.json)
        .CODE
        NOSMART                          ; 000F, 0134, 0185, 01A7, 01F3 and 0250 contain LEA of an
                                         ; absolute address or a far call to a sibling, both of which
                                         ; the smart code generator would shorten (TASM 2.0)

; 1E8B:0004: not a function but the manager's data words at the start of its code segment: the
; segment of its own data (20B5, which the code reads as cs:[4]), DGROUP (212E, cs:[6]), the "PATH="
; environment name (1E8B:0008) and the runtime-patched word cs:[0D] at 1E8B:000D (set by 0286).
; The words are written as plain constants because the original relocations store exactly these
; segment values (EOB.EXE loads at paragraph 0). QUIRK 0004: the original unit starts at frame offset
; 4, so its cs:[4], cs:[6] and cs:[0D] land on the first word, the second word and the patch word.
; This module's code segment starts at offset 0, so the same data sit at offsets 0, 2 and 4: the code
; here reads cs:[4] as "PA" (the "PATH=" literal) and cs:[6] as "TH", not the two segment words. The
; instruction bytes are identical (the operand is the literal 4, 6 or 0D in both), and the code is
; correct only in the original frame; a separate code segment cannot carry that frame offset.
        dw      20B5h
        dw      212Eh
        db      'PATH=',0,0

; 1E8B:000F: initialise the overlay manager. It returns success at once (the jmp at 1E8B:0025) when
; the current overlay id word 20B5:00A0 is already zero (the manager is already initialised, or no
; overlay id is set yet; which of the two is not established). Otherwise it opens the program's own
; file (01B3, or the name in the far pointer argument at [bp+6] through 01A7, or the PATH search 01F3;
; when the caller passes none, 000F substitutes 20C1:0318, the "eob.exe" string just past the 99-entry
; segment table), reads and checks the MZ header and walks the file's FBOV records (the "FB"/"OV"
; signature and the record offset and length from the 10h-byte header at [bp-14h]). On the FBOV header
; it stores the offset of the FBOV data (the file position just past the 16-byte header) at
; 20B5:00A4/00A6 and the FBOV size field (the u32 at header +4, WHITEPAPER 2.3) at DGROUP:84B2/84B4.
; It then swaps the overlay file open again (_SwapAndReopenOverlayFile), sizes the free area from the
; arguments at [bp+0Ah]/[bp+0Ch] (_InitOverlayRecords) and maps the records (_MapOverlayRecords).
; Returns AX = 0 on success and 0FFFEh when no name could be opened. The other failures close the file
; and return AX = 0FFFFh, except the _MapOverlayRecords failure exit (1E8B:011F), which jumps to
; init_fail with AX as that call left it and the reopened handle at 20B5:00B8 still open.
; QUIRK 1E8B:000F: the codes 0FFFDh, 0FFFCh and 0FFFBh that the header and free-area error paths load
; into CX are dead stores: init_closefail sets AX = 0FFFFh and never reads CX. Evidence: the MZ 5A4Dh
; test, the FBOV 4246h/564Fh tests, the 10h header size, the 84B2/84B4 and 00A4..00A8 globals and the
; 01B3/01A7/01F3/0276/0134/0286/02FF calls.
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
; installs the previously saved vector whose offset is at 20B5:0002 and segment at 20B5:0004, stashes
; the one just read for the next swap, then handles the file handle at 20B5:00B8 on one of two paths:
; when a handle is open it closes it and clears the word at 20B5:00B8 (no reopen on this path); when
; none is open it opens the file named in the 20B5:001C buffer and stores the new handle at 20B5:00B8.
; This is the pair to the shutdown path of 0185. Evidence: INT 21h/35h and /25h, the 20B5:00A1
; interrupt number, the [0002]/[0004] vector store and the 3Eh/3Dh file calls.
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
; and restore the swapped interrupt vector through 0134, then call the two near pointers at 20B5:0014
; and 20B5:0012 in the manager's own data segment. Purpose unknown: what those two pointers do; no
; function in this unit writes 20B5:0012 or 20B5:0014 (the writer is not in this unit). Evidence: the
; [0xB8] handle test, the far call to 0134 and the two `push cs; call [..]` pointers.
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

; 1E8B:01A7: open the overlay file named by this caller's far argument. It copies the whole name
; into the 20B5:001C buffer with 0250 (the frame and buffer are shared with the caller, 000F). It
; does not trim to the leaf name: only 01B3 does that. Evidence: the pushed DS/ES, DI = 001C and the
; near call.
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

; 1E8B:01B3: recover the program's own file name and open it. It walks the environment block (the word
; DGROUP:0075 holds the environment segment, `_envseg` in C0.ASM; build/re/rtl/tc100/l/r.map has
; __envseg at DGROUP:0075), skips to the end of the environment, reads and discards the 16-bit count
; that follows the environment block (DOS 3 layout), and copies the whole program path after it into
; the 20B5:001C buffer. A backslash only sets BX to the offset just past the last one; nothing is
; trimmed, the whole path is copied. It then calls 0250 with DI = that offset: when the caller's
; argument [bp+6] is null (the boot path does: 086F pushes 0,0) 0250 opens the whole path in 20B5:001C,
; and when the argument is non-null 0250 copies the caller's name over the leaf at that offset, so the
; caller's name replaces the leaf and the full path is opened. Returns carry set on a DOS below 3 or a
; failed open. Evidence: INT 21h/30h and the DGROUP:0075 environment-segment load, the two-NUL scan,
; the discarded count word and the 0250 call.
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
; block (the word DGROUP:0075 holds the environment segment, `_envseg`) for the entry whose first five
; bytes equal the "PATH=" literal
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

; 1E8B:0276: read CX bytes from the open overlay file into the 14h-byte buffer at the caller's
; [bp-14h] (the MZ header scratch of 000F, whose frame this helper shares). The handle is whatever is
; in BX, set by the caller: 000F keeps its handle at 20B5:00B8 and moves it to BX before each call
; (this helper does not read 20B5:00B8). Returns carry set on a DOS error or when fewer than CX bytes
; were read. Evidence: the SS:BP buffer, the shared frame and the two calls to this from 000F.
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

; 1E8B:0286: build the overlay record list and size it. It reads the DGROUP word 0079 (`_psp`, the PSP
; segment: C0.ASM; build/re/rtl/tc100/l/r.map has __psp at DGROUP:0079), adds 10h and stores the result
; at cs:000D (nothing in this unit reads cs:000D). It then walks the 99-entry record table at
; 20B5:00C0 (DS:00C0; DS is 20B5, while ES
; is set to the 20BF list head) in 8-byte steps: for each entry whose word +4 has bit 1 set and whose
; word +2 is non-zero, it stores the entry's segment (word 0) into the list head's word 20BF:0012
; (the store runs while ES is still the head; no function in this unit reads 20BF:0012, so its purpose
; is unknown), switches ES to that
; record, and, when the record's byte ES:001A is not FF, sets ES:0018 to the loader offset 04A2
; (1E8B:04A2), adds the
; 32-bit FBOV data base at 20B5:00A4/00A6 to the record's file offset ES:0004:0006, and keeps the
; largest total (through 0789). It ends by storing the largest size + 2 at 20B5:00AA. Evidence: the
; 20B5:00C0 table, the 4A2h store, the 00A4/00A6 base and the 0789 call.
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

; 1E8B:02FF: load an overlay into memory. It takes the free segment 20B5:00B2 (read here, set
; outside this unit) as the start and walks the overlay's record chain: it reads the next record from
; the current record's own word 0012 (DS:0012, DS being that record), keeps the chain head in
; 20B5:00BC, and links each record to its successor through its own word +1Ch (written 0 at the end),
; fitting each in the free space up to 20B5:00B6. (20B5:001C, named in older text here, is the
; file-name buffer of 01A7, not part of this walk.) It then reloads the
; overlay file: it computes the image's byte count as SI:DI (the paragraph count times 16, no
; rounding), reads the image with 03C4, applies the relocations (03FD) and fills the version table
; (066F), then calls the overlay's entry through 20B5:0016 repeatedly until the successor chain at
; ES:001C ends. Returns carry set on a read or overlay failure. The tail at 03B7 is an uncalled
; sibling that converts a DX:AX byte count to a paragraph count. Evidence: the 00B2/00B4/00B6
; free-memory words, the 0012/001C record chain, the 03C4/03FD/066F calls and the 20B5:0016 far
; pointer.
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
; 1E8B:03B7: uncalled sibling in this range: DX:AX bytes to a paragraph count.
        mov     cl,4
        shr     ax,cl
        ror     dx,cl
        and     dx,0F000h
        or      ax,dx
        ret
_MapOverlayRecords ENDP

; 1E8B:03C4: read SI:DI bytes of the overlay image from the open file into memory, at most 0FFF0h
; bytes (64 KB minus 16) at a time. It seeks to CX:DX in the file (INT 21h/4200h: DX is the low word,
; CX the high word, as the callers load them), then loops reading 0FFF0h bytes (or SI when DI is zero)
; into DS:0000, advancing DS by 0FFFh paragraphs (0FFF0h bytes, not 4 KB) each pass, until SI:DI
; reaches zero. Returns carry set on a DOS error or a short read. Evidence: the 4200h seek, AH = 3Fh
; read and the DS step.
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
; the record's ES:0008/ES:000A (ES:0008 is the byte offset of the relocation list from the image base,
; ES:000A is the count; the caller 02FF loads ES:000A into CX). The list sits at the base ES:0010 plus
; that byte offset (the paragraph part in DS, the low four bits in SI). It reads the word at each
; relocation's ES:BX through TLINK's segment table at 20C1 (index times 8) and replaces it with the
; frame paragraph from the table; when the index's low bit is set, it then fixes the far pointer up
; through 0442, which rewrites the offset word at ES:BX+4. ES:0010 is the segment the overlay was
; loaded at. Evidence: the ES:0008/000A fields, the 20C1 table segment (relocated) and the 0442 call
; on the odd flag.
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
        mov     ax,seg _gTlinkSegmentTable
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

; 1E8B:0442: relocate one far pointer in the overlay image. DX is the frame paragraph read from the
; 20C1 segment table in 03FD, and `mov ds,dx` makes it DS, so the version table searched below is at
; DS:0020, the record's own version table (the 5-byte entries that 066F and 068D write there); 20B5
; is not involved. The segment word of the far pointer is at ES:BX (03FD has just replaced it with the
; frame paragraph); 0442 first checks the overlay-call fixup form around it: the bytes at ES:BX-1 and
; ES:BX+3, masked with 0F807h (keeping the top five and low three bits), must both be 0B8h
; (`mov ax,<seg>`), and those at ES:BX+2 and ES:BX+6 must both be the same masked 50h (`push ax`).
; When they match, it searches the DS:0020 version slots (5 bytes each, DS:000C of them) for one whose
; +2 offset equals the offset word at ES:BX+4, and replaces that offset word with the slot's offset,
; so the call reaches the version entry. The tail at 04A2 is the record loader: 0286 stores its offset
; 04A2 at each record's ES:0018 and 0580 calls ES:0018 when the record is not loaded, and it seeks
; ES:0004:0006, reads ES:0008+ES:000A bytes with 03C4 and applies the relocations with 03FD. Evidence:
; the 0F807h masks and b8/50 byte tests; the 03C4/03FD calls in the 04A2 loader.
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
rel_out2:                               ; 1E8B:049E: the not-found exit, a second pop cx; ret
        pop     cx
        ret
; 1E8B:04A2: the record loader whose offset 0286 stores at each record's ES:0018 (0580 calls it
; through ES:0018 when the record is not loaded): reload an overlay image.
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

; 1E8B:04D0: the overlay call interrupt handler. `push bp; mov bp,sp` makes BP equal SP, so the test
; is the parity of the stack pointer: an odd SP at the test aborts through __OverlayHalt. What an odd
; SP means is not shown in this unit [H]. Otherwise it saves the registers, switches to the manager's
; data segment 20B5, pushes the request word at ES:[BX] and loads the overlay (_LoadOverlayRecord),
; either the single entry at [bp+2] or the two-word form at [bp+6]/[bp+2], then reads bit 3 (the value
; 8) of ES:001A into AX and clears that bit in the same step (`and byte ptr es:[1Ah],0F7h`) before
; calling the overlay's routine through 20B5:0016. Ends with IRET. Evidence: the SP-parity guard, the
; __OverlayHalt jump, the ES:001A bit 8 read and clear and the 20B5:0016 far pointer (the same one
; 02FF uses).
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

; 1E8B:0536: age the overlay records. It bumps the 20B5:00BA counter and takes the total paragraph
; count from 0789 in DX, then loops over the 20B5:00BC record chain: it advances the head to the
; record's ES:001C and, when 0761 signals its fallback (no current overlay), discards the whole chain
; (0613) first. For the record: when its use counter ES:001B is zero it releases the loaded copy
; (05FB) and takes its code paragraphs (077D); otherwise it decrements ES:001B, moves the record
; (06C0) and links it (0711). It repeats while the total DX exceeds the amount 0761 reports remaining,
; then stores the new top 20B5:00B0 into the caller's ES:0010. Evidence: the [00BA] counter, the
; ES:001B counter, the 0613/05FB/077D/06C0/0711 calls and the [00B0] store.
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
        call    _ReleaseOverlayRecord
        call    _OverlayCodeParagraphs
        jmp     short age_pop
age_dec:
        dec     byte ptr es:[1Bh]
        call    _MoveOverlayRecord
        call    _LinkOverlayRecord
age_pop:
        pop     dx
age_test:
        call    _OverlayParagraphsRemaining
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
; ES:0018 routine, relinks through 0711, and then restores the version table once (064E). It then
; walks the overlay's entry chain at 20B5:00BC while 0761 reports paragraphs remaining below the
; 20B5:00A8 limit: for each record whose use counter ES:001B is zero it clears that record's version
; table (068D) and adds its code paragraphs (077D); a non-zero counter adds nothing. A failed overlay
; routine jumps to __OverlayHalt. Evidence: the ES:001A/001B flags, the 0536/0711/064E/0761/068D/077D
; calls, the [00A8] limit and the __OverlayHalt jump.
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
        call    _OverlayParagraphsRemaining
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

; 1E8B:05FB: release an overlay record's loaded copy. It clears the record's version table (068D)
; and, when the record's routine word ES:0018 is the loader offset 04A2 (1E8B:04A2, set by 0286 when
; the record is built), makes a near call through the offset stored at 20B5:0010 (`call word ptr
; ds:[10h]`: a near call in this unit's code segment, not a far call through that address); it then
; clears the record's base segment ES:0010 so the record reads as not loaded. Called from 0536 when
; the record's use counter ES:001B reaches zero. "Release hook" is a guess: the evidence is only that
; 20B5:0010 is called here and that ES:0010 is then cleared. Evidence: the 04A2 loader-offset test,
; the 20B5:0010 near call and the clearing store (the 04A2h store is at 0286, not in 02FF).
        PUBLIC  _ReleaseOverlayRecord   ; @ 1E8B:05FB
_ReleaseOverlayRecord PROC NEAR
        call    _ClearOverlayVersionTable
        cmp     word ptr es:[18h],4A2h
        jne     short rel_done
        call    word ptr ds:[10h]
rel_done:
        mov     word ptr es:[10h],0
        ret
_ReleaseOverlayRecord ENDP

; 1E8B:0613: discard every overlay record in the 20B5:00BC chain. It walks the chain through
; ES:001C, counts the records in CX, then walks it back from the end (the segments are on the stack):
; it relinks each record onto the 20B5:00BC head inline (ES:001C = the current head, then 00BC = ES),
; which is the unlink, and calls 06C0 to move the record's image, subtracting its code paragraphs
; (077D) from 20B5:00B0 as it goes. It first sets 20B5:00B0 to 20B5:00B6 (the free area's top) and at
; the end sets it to 20B5:00B4 (the free area's base). Evidence: the ES:001C chain, the inline relink,
; the 06C0/077D calls and the [00B6]/[00B4] stores.
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

; 1E8B:064E: restore an overlay record's version table. With no version records (ES:000C zero) it does
; nothing. When the record's marker ES:0020 is 0EAh (already a live far entry) it skips the fill and
; returns. Otherwise it fills the version table (066F): when the saved slot offset ES:0002 is zero it
; jumps straight to the fill, and when it is not zero it first calls 072F, which restores that saved
; offset into the matching slot (072F exchanges the slot's current offset with CX = ES:0002), and then
; falls through into the fill. Evidence: the ES:000C test, the 0EAh marker and the 072F call.
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

; 1E8B:066F: fill the CX overlay-version records at ES:0020. Each 5-byte entry becomes a live far
; jump, 0EAh (the JMP FAR opcode) followed by the offset the old entry held (read from +2) and the
; record's base segment ES:0010, so a later load can reach the overlay's entry. This is the reverse of
; 068D, which rewrites the same entry into the 5-byte stub form 0CDh, 3Fh, offset, 0 (the Turbo Pascal
; overlay stub of WHITEPAPER 2.3). Evidence: the 0EAh byte, the stores from ES:0010 and the DI = 0020
; step of 5.
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

; 1E8B:068D: clear the overlay's own saved state. Unless the record's marker ES:0020 is already CDh,
; it first calls 072F (AX = ES, DX = ES:0010), which retargets the matching slot and returns the
; slot's previous offset in CX; it saves that offset in ES:0002 (zero when no slot matched), then
; rewrites the CX version records at ES:0020 into the 5-byte stub form: the word at 20B5:00A0, whose
; bytes are 0CDh, 3Fh (the Turbo Pascal overlay stub of WHITEPAPER 2.3), then the live entry's offset
; read from +1, then a zero byte. This is the reverse of 066F. Evidence: the CDh test, the 072F call,
; the ES:0002 store and the DI = 0020 loop.
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
; writes the record's segment ES at word 0Eh of the header paragraph just below the new base (DS =
; new base - 1; 07A1 reads that back link). Unless ES:0020 is CDh it then calls 073B with AX = the new
; base and DX = the old base, which retargets the slots whose segment is the old base to the new one,
; and stores the new base (AX) in the segment word of each version record (ES:0023, then +5 each), so
; the far jumps 066F wrote point at the new base. Evidence: the ES:0008 length, the ES:0010/20B5:00B0
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

; 1E8B:072F: patch an overlay's entry in the far-pointer table on the stack. It calls 073B, which
; walks the table at SS:BP and, for every slot whose segment SS:[BP+4] equals DX, stores AX into that
; slot's SS:[BP+4] and returns BX = the first matching slot (or zero). When BX is nonzero, 072F swaps
; that slot's offset SS:[BX+2] with CX, so the slot gets CX and the old offset comes back in CX. The
; two callers pass the (segment, record) pair the other way round: 064E (restore) puts AX = ES:0010,
; DX = ES, while 068D (clear) puts AX = ES, DX = ES:0010. 072F writes no far pointer itself; the
; 20B5:0010 pointer is read by 05FB. Evidence: the 073B call and the SS:[BX+2]/CX exchange.
        PUBLIC  _PatchOverlayEntryVector  ; @ 1E8B:072F
_PatchOverlayEntryVector PROC NEAR
        call    _FindOverlaySlot
        or      bx,bx
        je      short patch_out
        xchg    word ptr ss:[bx+2],cx
patch_out:
        ret
_PatchOverlayEntryVector ENDP

; 1E8B:073B: walk the overlay slot table (the caller's SS:BP) and, for every slot whose far segment
; at SS:[BP+4] equals DX, store AX into that SS:[BP+4] (every matching slot, not only the first); BX
; is the first matching slot, or zero when none matched. The walk follows the link word at each slot
; (`mov bp,cx` from [bp]); a slot whose link word has its low bit set steps one level down. It never
; tests SS:[BP+2]. Evidence: the shr word,1 / jb pair, the DX compare against SS:[BP+4] and the AX
; store; BX is set once, from the first match.
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

; 1E8B:0761: with the chain head 20B5:00BC non-zero, return in AX the current record's base (its
; ES:0010) minus the running top 20B5:00B0: the paragraphs from 00B0 up to that base. When the
; subtraction borrows, because the base is below 00B0 (the case for a record already linked, since
; 0711 adds its size to 00B0), it returns the free paragraphs 20B5:00B6 minus 20B5:00B0 with carry
; set. With no current record (00BC zero) it goes straight to the fallback. Both operands are segment
; (paragraph) values, so AX counts paragraphs. Evidence: the [00BC] selector, the two subtractions
; from [00B0] and the STC on the fallback path.
        PUBLIC  _OverlayParagraphsRemaining   ; @ 1E8B:0761
_OverlayParagraphsRemaining PROC NEAR
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
_OverlayParagraphsRemaining ENDP

; 1E8B:077D: return in AX the number of paragraphs the overlay record ES: occupies for its code:
; (the size at ES:0008 plus 11h) shifted right by 4. The 11h is the 10h record header plus one more
; byte; why the extra byte is there is not established. Evidence: the ES:0008 load, the +11h and the
; 4-bit shift shared with 0789.
        PUBLIC  _OverlayCodeParagraphs   ; @ 1E8B:077D
_OverlayCodeParagraphs PROC NEAR
        mov     ax,es:[8]
        add     ax,11h
        mov     cl,4
        shr     ax,cl
        ret
_OverlayCodeParagraphs ENDP

; 1E8B:0789: size an overlay record ES in paragraphs. AX gets the code paragraphs (ES:0008 plus 11h,
; shifted right by 4, the same as 077D) and DX gets the total: that plus the data paragraphs (ES:000A
; plus 0Fh, shifted right by 4). Callers use DX (0286 keeps the largest total). The trailing code from
; 07A1 is a second, uncalled in this unit, routine that checks whether AX is a loaded overlay segment
; in 20B5:00B4 to 00B6: it finds the record through the header back link at word 0Eh, tests the
; current overlay id 20B5:00A0 against the record's ES:0000 and AX against the record's ES:0010, and
; returns the record segment in AX (`mov ax,es`) on a match, or AX = 0 with carry set otherwise (ES is
; popped before the RETF); it shares this function's range. Evidence: the two shifts and the
; ES:0008/000A loads; the 07A1 range check against [00B4]/[00B6] and the back-link/ES:0000/ES:0010
; tests.
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

; 1E8B:07D7: drop stale overlay slots. Uncalled in this unit (no direct caller in the packet). It
; walks the caller's slot table at SS:BP (CX on entry is the table end; the caller supplies it, `cmp
; bx,cx`) for a slot whose segment SS:[bx+4] is below the base 20B5:00B4, whose offset SS:[bx+2] is
; zero, and whose record's word ES:0000 equals the current overlay id 20B5:00A0; it sets the found
; byte [bp-2] and clears that record's ES:0002. When a record was marked, it walks the table again
; and, for a slot below 00B4 with a non-zero SS:[bx+2] whose record id matches and whose ES:0002 is
; still zero, it moves the SS:[bx+2] value into ES:0002 and clears the slot. A slot word with the low
; bit set (SS:[bx] odd) steps one level down. Evidence: the SS:BP table, the [00A0] id / [00B4] base
; comparisons, the [bp-2] found byte and the ES:[0]/ES:[2] patch.
        PUBLIC  _DropStaleOverlaySlots   ; @ 1E8B:07D7
_DropStaleOverlaySlots PROC FAR
        push    bp
        mov     bp,sp
        sub     sp,2
        mov     byte ptr [bp-2],0
        push    ds
        mov     ds,word ptr cs:[4]
        push    si
        push    di
        mov     bx,bp
        mov     ax,ds:[0A0h]
        mov     dx,ds:[0B4h]
        jmp     short drop_scan
drop_advance:
        shl     si,1
        mov     bx,si
drop_scan:
        cmp     bx,cx
        jae     short drop_check
        mov     si,ss:[bx]
        shr     si,1
        je      short drop_check
        jb      short drop_advance
        mov     di,ss:[bx+4]
        cmp     di,dx
        jae     short drop_advance
        mov     es,di
        mov     di,ss:[bx+2]
        or      di,di
        jne     short drop_advance
        cmp     word ptr es:[di],ax
        jne     short drop_advance
        mov     byte ptr [bp-2],1
        mov     word ptr es:[di+2],di
        jmp     short drop_advance
drop_check:
        cmp     byte ptr [bp-2],1
        je      short drop_second
        jmp     short drop_out
        nop
drop_next:
        shl     si,1
        mov     bx,si
drop_second:
        mov     si,ss:[bx]
        shr     si,1
        je      short drop_out
        jb      short drop_next
        mov     di,ss:[bx+4]
        cmp     di,dx
        jae     short drop_next
        mov     es,di
        mov     di,ss:[bx+2]
        or      di,di
        je      short drop_next
        cmp     word ptr es:[0],ax
        jne     short drop_next
        cmp     word ptr es:[2],0
        jne     short drop_next
        xchg    word ptr es:[2],di
        mov     word ptr ss:[bx+2],di
        jmp     short drop_next
drop_out:
        pop     di
        pop     si
        pop     ds
        mov     sp,bp
        pop     bp
        retf
_DropStaleOverlaySlots ENDP

; 1E8B:086F: the overlay manager entry, reached from the overlay stubs rather than from this unit (the
; packet finds no direct caller). Given a record whose segment is in [bp+8] with id ES:0000 equal to
; the current overlay id 20B5:00A0, it loads the record (_LoadOverlayRecord) when the record has no
; loaded copy (ES:0010 zero) or its version marker ES:0020 is CDh, and on that load path calls the
; record's routine through the 20B5:0016 far pointer. When ES:0010 is set and ES:0020 is not CDh it
; returns ES:0010 without loading or calling: the call through 20B5:0016 is on the load path only. On
; the reached exits it stores ES:0010 in [bp+8]. The two one-byte returns after entry_out are unused
; tails. 08C9 is a second entry of this same PROC, the program's overlay-memory bootstrap: it
; runs _InitOverlayRecords, forms a paragraph count as (20B5:00AA doubled when 00AA is at least
; DGROUP:3088, otherwise DGROUP:3088) plus one, allocates it with _farmalloc, and calls
; _InitOverlayManager over the block. That call pushes DX, BX and then two zero words, so
; _InitOverlayManager sees [bp+6]/[bp+8] = 0 (the name far pointer, null) and [bp+0Ah] = BX,
; [bp+0Ch] = DX. On failure the bootstrap jumps to _abort. Evidence: the ES:0000 id test, the
; ES:0010/ES:0020 markers, the 20B5:0016 far pointer, the DGROUP:3088 word and the
; _farmalloc/_InitOverlayManager calls.
        NOSMART                          ; 086F does a far call to a sibling (090B)
        PUBLIC  _OverlayManagerEntry     ; @ 1E8B:086F
_OverlayManagerEntry PROC FAR
        push    bp
        mov     bp,sp
        push    ds
        mov     ds,word ptr cs:[4]
        mov     es,[bp+8]
        mov     cx,ds:[0A0h]
        cmp     cx,es:[0]
        jne     short entry_out
        cmp     word ptr es:[10h],0
        je      short entry_load
        cmp     byte ptr es:[20h],0CDh
        jne     short entry_call
entry_load:
        mov     word ptr [bp+8],0
        push    si
        push    di
        call    _LoadOverlayRecord
        pop     di
        pop     si
        mov     bx,[bp+6]
        push    es
        mov     al,es:[1Ah]
        and     al,8
        and     byte ptr es:[1Ah],0F7h
        cbw
        mov     ds,word ptr cs:[4]
        call    dword ptr ds:[16h]
        pop     es
entry_call:
        mov     ax,es:[10h]
        mov     [bp+8],ax
entry_out:
        pop     ds
        pop     bp
        retf
        retn
        retf
; 1E8B:08C9: allocate the overlay memory and start the manager.
        push    ds
        push    si
        push    di
        mov     ax,20B5h
        mov     ds,ax
        call    _InitOverlayRecords
        mov     bx,ds:[0AAh]
        mov     ax,212Eh
        mov     ds,ax
        cmp     bx,ds:[3088h]
        jae     short boot_have
        mov     bx,ds:[3088h]
        jmp     short boot_size
boot_have:
        shl     bx,1
boot_size:
        inc     bx
        push    bx
        mov     ax,10h
        mul     bx
        push    dx
        push    ax
        call    far ptr _farmalloc
        pop     bx
        pop     bx
        pop     bx
        mov     cx,ax
        or      cx,dx
        jcxz    short boot_abort
        inc     dx
        add     bx,dx
        push    dx
        push    bx
        xor     ax,ax
        push    ax
        push    ax
        call    far ptr _InitOverlayManager
        or      ax,ax
        jne     short boot_abort
        pop     di
        pop     si
        pop     ds
        retf
boot_abort:
        jmp     far ptr _abort
_OverlayManagerEntry ENDP
        SMART

        END
