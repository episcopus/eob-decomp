; Unit at EOB.EXE 1C00:0001 (235 bytes, 2 functions and a lone byte at 1C00:0001), kind asm [H] by
; the map. This is the game's loader for named MZ overlay files, the graphics drivers (cga.ovl,
; tga.ovl, ega.ovl, mcga.ovl and vga.ovl, the five names at file offset 26BA1h of EOB.EXE,
; WHITEPAPER 2.5), which it loads with DOS function 4B03h (load overlay) [H]. The table
; gOverlayFiles (E07C8.C:52, ds:3074) holds all five, its five segment words are load-time fixups
; to DGROUP and its five name offsets are DGROUP:30C1h to 30E2h (see the 1C00:0055 comment); but
; WHITEPAPER 2.5 says vga.ovl is not in the copy and the VGA mode loads MCGA.OVL. It is not the
; INT 3Fh VROOMM trap: VROOMM is the Borland runtime's overlay manager, which loads the FBOV
; segments appended to EOB.EXE (WHITEPAPER 2.3), and the game's hook around it is the handler at
; 1A72:0006, installed at 07C8:008F (src/E1A72.ASM:76 to 87); this unit never touches INT 3Fh.
; The unit starts at 1C00:0001, the lone 0 byte before the first function at the even offset 0002
; [H data]; the byte before the unit, 1C00:0000, is a lone retf (cb), outside the 235 bytes.
; 1C00:0002 allocates an 0FA00h byte block (MemAlloc, flags 20h), loads an overlay image into it
; with DOS INT 21h/4B03 and reports BX = 0 on success, 0FFFFh on failure; and 1C00:0055 takes a
; far pointer to an overlay request (word 0 = the offset of the request name in the request's
; segment; word +2 = the name's segment; words +4 and +6 = a far pointer (offset, segment) to the
; table the entry's pointer walks), loads the overlay, then calls the entry at the overlay
; segment:0000 with 27h, 1A16h and that segment (1C00:0088 to 0091), resizes the block 1C00:0002
; loaded the overlay into, to the overlay's size (the entry's BX; MemRealloc, 1C00:00A8), and calls
; the pointer the entry returned (offset at 818Eh, segment at 8190h) with the request's words +4
; and +6 (1C00:00C0). The caller StartupGame's DS, the request segment (which _RunOverlayRequest
; itself loads into DS and ES at 1C00:0062 and 1C00:0071) and the name's segment are all DGROUP at
; run time (the 1C00:0055 comment gives the evidence, including the start code that sets DS). Each
; access uses the segment the code leaves in DS or ES, not one segment for all; the comment inside
; 1C00:0055 lists them.
; DGROUP is paragraph 212Eh of the load image (the value the fixups hold; the loader adds the load
; segment), as in E1A72.ASM (1A72:0010, 0180, 0308 and 033E) and E1D8F.ASM (line 78).
;
; Far calls to another unit (9A off seg) are declared at module level, after .MODEL LARGE and before
; .CODE, outside any segment, and written `call far ptr _Name` (playbook 30): a plain `call _Name`
; assembles as `push cs; call near`, and an EXTRN inside a segment of its own leaves the segment
; word 0000.

        .MODEL  LARGE

        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007 (E19BC.C)
        EXTRN   _MemRealloc:FAR         ; @ 19BC:018A (E19BC.C)
        EXTRN   gMouseHandlerInstalled:WORD ; @ ds:8148: 1 when the mouse driver handler is
                                        ;   installed (E1A72.ASM)
        EXTRN   gMouseDisabled:WORD     ; @ ds:814A: non-zero disables the mouse handler body and
                                        ;   the pointer (handler test 1A72:0185, E1BBE.ASM:89-90);
                                        ;   after start-up returns 1, Main stores 1 when the handler
                                        ;   is not installed, and `!mouse` (the command-line digit,
                                        ;   E07C8.C:121) when it is (E07C8.C:152-156, the strongest
                                        ;   evidence for the name)

        .CODE

; 1C00:0001: the unit's first byte, a lone 0 before the first function at the even offset 0002
; (data, not code) [H]; 1C00:0000, the byte before the unit, is a lone retf (cb) and is not one
; of the 235 bytes.
        db      0

; 1C00:0002: load an overlay image. Allocate a 0FA00h byte far block (MemAlloc, flag 20h) and store
; its segment at request_seg:818Ah through ES (1C00:002F): MemAlloc returns MK_FP(seg + 1, 0) for
; flag 0x20 (19BC:0007), so its offset is 0 and `or ax,dx` leaves the segment in AX, which the
; `mov es:[bx],ax` after it stores. This 0FA00h is the block the loader reserves for the overlay;
; 1C00:0055 later shrinks it to the overlay's real size with MemRealloc (the reason for this size is
; not established [H]: 0FA00h is 64000 bytes, one 320 by 200 screen, but nothing in the bytes says
; that is why; the four overlay images in eob/ are 10082 to 12759 bytes (each file size minus its
; 512-byte MZ header), so it is enough for any of them, and vga.ovl is not in the copy). Which
; segment each access uses, and why the header's
; DGROUP:818Ah to 8194h holds (the caller StartupGame's DS is DGROUP, which 07C8:0254 `push ds`
; passes), is spelled out once in the 1C00:0055 comment.
; Save SP and SS across the call (the reason is not established [H]), then load the overlay named by
; DS:DX through the INT 21h/4B03 parameter block at ES:BX = request_seg:818Ah. Word 0 is the load
; segment (the word at 818Ah through that segment) and word 1 is the relocation factor (818Ch
; through that segment, which no file in src/ writes, so check it in the oracle before a port relies
; on it). Return BX = 0 on success or 0FFFFh on failure. The caller reaches it with `push cs; call`
; (1C00:0073 to 0074), which is a near call: it pushes CS as an extra word below the return address,
; and the function's retf pops that word as well as the return address. Only 1C00:0055 calls it,
; passing BX = 818Ah, ES = the request segment and DS:DX from the request.
        PUBLIC  _LoadOverlayImage       ; @ 1C00:0002
_LoadOverlayImage PROC NEAR    ; `push cs; call` (1C00:0073), so the `retf` pops CS
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
        jne     short loaded
        pop     dx
        mov     bx,0FFFFh
        jmp     short loadexit
loaded: pop     dx
        mov     es:[bx],ax
        mov     ds:[8192h],sp
        mov     ds:[8194h],ss
        mov     ax,4B03h
        int     21h
        mov     ax,212Eh
        mov     ds,ax
; Restore SP then SS in the original's order (1C00:0044, 0048): between the two, SS is the one DOS
; left and SP is the saved one, with interrupts enabled. The original does this and a matching file
; must keep it; a later port should load SS first. [H] on the hazard: it is not observed.
        mov     sp,ds:[8192h]
        mov     ss,ds:[8194h]
; The `jb` below tests the carry flag INT 21h 4B03h (1C00:003D) left: DOS sets carry on error (the
; DOS convention for function 4Bh [H]). The `mov bx,0FFFFh` just before the `jb` changes no flag.
        mov     bx,0FFFFh
        jb      short loadexit
        xor     bx,bx
loadexit: pop   bp
        retf
_LoadOverlayImage ENDP

; 1C00:0055: run an overlay request. [bp+6..+9] is a far pointer to the request (the caller, inside
; StartupGame at 07C8:0199, E07C8.C:172, passes DS:8121 at 07C8:0254): word 0 is the offset of the
; request name in the request's segment; word +2 is the name's segment; words +4 and +6 are a far
; pointer (offset then segment) to a table that the pointer the entry returns walks through DS:SI
; in word pairs (it reads `mov si,[bp+6]` and `mov ds,[bp+8]`: cga.ovl 111Bh and 111Eh, ega.ovl
; 0EF7h and 0EFAh, mcga.ovl 0BF2h and 0BF5h, tga.ovl 0F0Fh and 0F12h). Who writes DGROUP:8125h and
; 8127h (request +4 and +6) is not established; the caller bytes at 07C8:0249 to 0258 write only
; 8121h and 8123h.
; It loads the image with 1C00:0002, calls the entry at the overlay segment:0000 with 27h, 1A16h
; and that segment (1C00:0088 to 0091), resizes the block with MemRealloc (1C00:00A8), calls the
; pointer the entry returned (1C00:00C0) and, when the mouse handler is installed, clears
; gMouseDisabled (1C00:00CD to 00D4). AX = 0 on success, 0FFFFh on failure, which the caller
; compares with -1. The comment beside each block below gives its facts.
; The caller StartupGame's DS is DGROUP: it reads the DGROUP global ds:8102 through DS from its
; entry (07C8:019D, `cmp word ptr [8102h],36h`; also 07C8:01A7) and no instruction from 07C8:0199
; to the `push ds` at 07C8:0254 changes DS (no `mov ds`, `lds` or `pop ds`; checked with
; tools/re/disasm.py),
; so the DS it passes is the DS it got on entry. The start code sets that DS to DGROUP: image offset
; 0000h is `mov dx,212Eh` (BA 2E 21, the relocated word) and image offset 0014h is `mov ds,dx`
; (8E DA), at file offsets 2800h and 2814h. These two instructions are Turbo C's C0 start-up
; (STARTUP/C0.ASM in the release, docs/setup/SETUP-tc.md:24; E07C8.C:113 says "C0 startup"), the
; basis for DS = DGROUP at run time [H]. The request segment, the
; name's segment and StartupGame's DS are therefore all DGROUP at run time: the MZ fixup list has a
; load-time fixup at each of the five segment words of gOverlayFiles (ds:3076h, 307Ah, 307Eh, 3082h
; and 3086h), each holding 212Eh, the paragraph of the load image (the loader adds the load segment
; at run time, so DGROUP is that paragraph; the fixup at 1C00:000A holds the same 212Eh), so the
; caller copies DGROUP into word +2 at 07C8:0245 and 0251. The loader takes the name's segment from
; its own DS ([bp+8], the segment of the request, at 1C00:0062) and the offset from word 0
; (1C00:0065); it never reads word +2, which the caller reads back on failure, pushing [8123h] and
; [8121h] as a far pointer to PrintString at 07C8:027B to 0283. The five name offsets gOverlayFiles
; holds are 30C1h, 30C9h, 30D1h, 30D9h and 30E2h (DGROUP offsets), the file offsets 26BA1h, 26BA9h,
; 26BB1h, 26BB9h and 26BC2h (cga.ovl, tga.ovl, ega.ovl, mcga.ovl and vga.ovl) minus 23AE0h
; (DGROUP's file offset: the image starts at file offset 2800h, the start code above, so DGROUP is
; 2800h + 212E0h = 23AE0h).
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
; 1C00:0059: the segment each access uses, not one segment for all (bytes 1C00:0002 to 00C0):
; 1C00:002F (`es:[bx] = AX`, BX = 818Ah) and 1C00:0032 and 0036 (the SP and SS save) use the
; request segment, which _RunOverlayRequest sets in DS and ES at 1C00:0062 and 1C00:0071 (the
; `pop es`/`pop ds` at 1C00:0021 to 0022 restore it after the MemAlloc call). 1C00:0044 and 0048
; (the SP and SS restore) and 1C00:007C, 0081, 0085, 0098 and 00B4 use DGROUP: the 1C00:0042 reload
; `mov ax,212Eh; mov ds,ax` sets DS to DGROUP on the success path of 1C00:0002, and its `retf`
; (1C00:0054) does not restore DS; on the MemAlloc-failure return DS is the request segment that
; `pop ds` (1C00:0022) restored, which is DGROUP too, so 1C00:0002 returns with DS = DGROUP on
; every path and the caller sees DGROUP at 1C00:0077 (212Eh is the value the reload's fixup holds;
; the loader adds the load segment). The entry call (1C00:0091) leaves DS alone, so the stores at
; 1C00:0098 and 00B4 after it still use DGROUP: the four entries in eob/ write no DS (cga.ovl
; 1168h to 1184h, ega.ovl 0FBDh to 0FD1h, mcga.ovl 0C3Fh to 0C5Bh, tga.ovl 0F5Ch to 0F78h).
; 1C00:00C0 (the second `lcall [818Eh]`) reads its target
; through the request segment, which `pop ds` (1C00:00B9) restores from the word pushed at
; 1C00:006A: it calls the pointer stored at DGROUP:818Eh. These pairs hold only because the request
; segment is DGROUP (the header's claim, from 07C8:019D and 07C8:0254): the SP and SS save at
; 1C00:0032 and 0036 (request segment) and the restore at 1C00:0044 and 0048 (DGROUP); the load
; block at 1C00:007C (DGROUP) and the store at 1C00:002F (request segment); the entry pointer at
; 1C00:0098 and 00B4 (DGROUP) and the second call at 1C00:00C0 (request segment).
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
; On success this function stores 0 at 818Eh (the offset) and the overlay segment (the word at
; DGROUP:818Ah, loaded into AX at 1C00:007C) at 8190h (1C00:0081 and 0085), then pushes that
; segment, 1A16h and 27h and calls the entry at segment:0000 through [818Eh]: offset 0 jumps to the
; entry (cga.ovl `e9 65 11` at image 0000h jumps to 1168h, ega.ovl `e9 ba 0f` to 0FBDh, mcga.ovl
; `e9 3c 0c` to 0C3Fh, tga.ovl `e9 59 0f` to 0F5Ch), and the 0 is the call's target offset. The
; entry's first argument is 27h and its third the overlay segment; 1A16h is the segment of
; src/E1A16.C (WaitFrames). What the overlay does with them is not shown by the four entries in
; eob/ (cga.ovl 1168h to 1184h, ega.ovl 0FBDh to 0FD1h and its helper 0F51h, mcga.ovl 0C3Fh to
; 0C5Bh, tga.ovl 0F5Ch to 0F78h): each only does `push bp; mov bp,sp`, pushes and pops its own
; registers, and retf, with no `[bp+n]` read and no write to DS, so 27h, 1A16h and the overlay
; segment stay placeholders.
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
; The entry returns AX = its function offset and BX = its size in paragraphs, evidenced by the
; entries in eob/: cga.ovl 1168h to 1184h sets BX = 31D7h (its image size: 13271 file bytes minus
; the 512-byte MZ header), `shr bx,cl` by 4 (117Ah) then `inc bx` (117Ch) so BX = image paragraphs
; + 1, then AX = 110Fh (117Dh, the pointer's offset in the overlay segment) and retf; ega.ovl
; 0FBDh gives BX = 2762h and AX 0EEBh, mcga.ovl 0C3Fh gives 2C38h and AX 0BE6h and tga.ovl 0F5Ch
; gives 2AB0h and AX 0F03h. AX is stored at 818Eh and is the call offset of the pointer call at
; 1C00:00C0; four `shl bx,1` (1C00:009B to 00A1) make BX the MemRealloc size in bytes.
; The three `pop cx` that drop the entry's arguments leave CX = the overlay segment pushed at
; 1C00:0088, and that CX is the pointer's segment in the MemRealloc call: MemRealloc (19BC:018A,
; E19BC.C) takes (void far *ptr, unsigned size), so the call pushes the size, CX as the segment and
; then 0 as the offset (re/renames.toml records the call as (0, seg, size)), and it resizes the
; overlay's own block at segment:0000 to that size and keeps its contents.
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
; The failure test after MemRealloc (`or ax,dx; je short fail` at 1C00:00B0 to 00B2) does not see a
; failed realloc of this 0x20 block: MemRealloc's 0x20 path does `blk = realloc(...)` and then
; returns MK_FP(FP_SEG(blk) + 1, 0) (E19BC.C:142 to 145), so a NULL comes back as 0001:0000,
; non-zero: the test falls through, the code stores the segment 1 at 8190h and calls the pointer at
; 0001:AX. Shrinking a block is not expected to fail [H].
        or      ax,dx
        je      short fail
        mov     ds:[8190h],dx
; MemRealloc's DX (the new block's segment, FP_SEG(blk) + 1 on the 0x20 path, E19BC.C:142 to 145)
; is stored at 8190h at 1C00:00B4, so the second `lcall [818Eh]` (1C00:00C0) calls the pointer the
; entry returned, whose offset is the entry's AX (818Eh) and whose segment is MemRealloc's result
; (8190h); its target is read through the request segment (see above), and the segment can differ
; from the overlay segment the entry was called with if the heap moved the block [H]. It calls that
; pointer with the request's two words: `pop si` restores SI = request + 4, then `lodsw` reads word
; +4 into AX, `mov bx,ax` copies it to BX, and the second `lodsw` reads word +6 into AX, and
; `push ax; push bx` precedes `lcall [818Eh]` (1C00:00B8 to 00C0), so word +4 is the pointer's
; first argument and word +6 its second.
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
; Only the pointer call's carry flag is tested (`jb short failret` at 1C00:00C6 tests the carry
; flag the call left, which is the pointer's failure convention, not the entry's: each overlay's
; pointer clears it on success and sets it on failure (cga.ovl clc 115Ah/stc 115Dh, ega.ovl clc
; 0F36h/stc 0F39h, mcga.ovl clc 0C31h/stc 0C34h, tga.ovl clc 0F4Eh/stc 0F51h), while the entry
; sets no failure flag (cga.ovl 1168h to 1184h ends with `shr bx,cl`, `mov ax,110Fh`, `retf`);
; the two `pop cx` before it do not change the flags, and it tests neither AX nor BX). The first
; entry call's AX, BX and flags are not checked.
        jb      short failret
; On success (the `jb short failret` at 1C00:00C6 not taken), when the mouse handler is installed
; (gMouseHandlerInstalled) the loader clears gMouseDisabled (1C00:00CD to 00D4).
; gMouseHandlerInstalled is 1 when the mouse driver handler is installed (declared at E1A72.ASM:52,
; ds:8148): set to 1 at the install (1A72:014A) and cleared when no mouse driver is found
; (1A72:011B, the no-driver path of InitMouse). gMouseDisabled is set to 1 at install (1A72:0150),
; and a non-zero value disables the handler: the INT 33h handler body skips all work on non-zero
; (its `cmp [gMouseDisabled],0; jne` at 1A72:0185) and
; HidePointer and ShowPointer do nothing on non-zero (E1BBE.ASM:89-90, 178), while SetMouseItem
; (07A9:010C, its `if (!gMouseDisabled)` test at 07A9:0148: `cmp [814A],0; je`) and E14EF.C:227
; run their pointer work only when it is zero. Clearing it after a driver overlay has loaded and the
; pointer the entry returned has run therefore turns the handler and the pointer on [H]. This clear
; is not the last word: after start-up returns 1, when g3092 is set (E07C8.C:152), Main sets the
; flag again from the mouse command-line argument, `if (!gMouseHandlerInstalled) gMouseDisabled = 1;
; else gMouseDisabled = !mouse;` (E07C8.C:153-156), so the mouse argument decides the final value.
        mov     ax,212Eh
        mov     ds,ax
        cmp     word ptr [gMouseHandlerInstalled],0
        je      short succeeded
        mov     word ptr [gMouseDisabled],0
succeeded: xor  ax,ax
        jmp     short returnpath
; fail: is reached from two places: a failed overlay load (the `cmp bx,-1; je short fail` at
; 1C00:007A) and a failed MemRealloc (`or ax,dx; je short fail` at 1C00:00B2); both go through the
; same ds/si swap below and fall through into the shared failure return at 1C00:00E0. A load
; failure, a failed realloc or the pointer call leaves the 0FA00h block allocated; nothing
; frees it. That is not observable, because the caller prints the message, waits for a key
; (lcall 1AAD:053A at 07C8:01DA) and returns to Main through the shared epilogue of StartupGame at
; 07C8:0575 (pop si, pop bp, retf; 07C8:0265 to 0277, 01D2 to 01E1); Main then runs ShutdownGame
; (E07C8.C:160, 07C8:058A), which ends in exit(0) (E07C8.C:345), so the block is never used again.
; The failure path pops ds then si (1C00:00DE, 00DF), the reverse of the success path above: `pop
; ds` takes the word pushed as si (1C00:006B) and `pop si` takes the word pushed as ds (1C00:006A),
; so ds and si swap values. It is harmless, because the epilogue pops at 1C00:00E3 to 00E6 restore
; si and ds from the entry pushes; both words pushed before the load call are discarded.
fail:   pop     ds
        pop     si
failret:                        ; the shared 0FFFFh failure return (1C00:00E0): `fail:` falls into
                                ; it, and the pointer call's carry set jumps here (the
                                ; `jb short failret` at 1C00:00C6)
        mov     ax,0FFFFh
returnpath:                     ; the shared return before the epilogue pops (1C00:00E3)
        pop     si
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
