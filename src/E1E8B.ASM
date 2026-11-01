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
        NOSMART
        .CODE

        PUBLIC  _f1E8B_0004              ; @ 1E8B:0004 stub
_f1E8B_0004 PROC FAR
        ret
_f1E8B_0004 ENDP

        PUBLIC  _f1E8B_000F              ; @ 1E8B:000F stub
_f1E8B_000F PROC FAR
        ret
_f1E8B_000F ENDP

        PUBLIC  _f1E8B_0134              ; @ 1E8B:0134 stub
_f1E8B_0134 PROC FAR
        ret
_f1E8B_0134 ENDP

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
        call    far ptr _f1E8B_0134
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

        PUBLIC  _f1E8B_0286              ; @ 1E8B:0286 stub
_f1E8B_0286 PROC FAR
        ret
_f1E8B_0286 ENDP

        PUBLIC  _f1E8B_02FF              ; @ 1E8B:02FF stub
_f1E8B_02FF PROC FAR
        ret
_f1E8B_02FF ENDP

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

        PUBLIC  _f1E8B_03FD              ; @ 1E8B:03FD stub
_f1E8B_03FD PROC FAR
        ret
_f1E8B_03FD ENDP

        PUBLIC  _f1E8B_0442              ; @ 1E8B:0442 stub
_f1E8B_0442 PROC FAR
        ret
_f1E8B_0442 ENDP

        PUBLIC  _f1E8B_04D0              ; @ 1E8B:04D0 stub
_f1E8B_04D0 PROC FAR
        ret
_f1E8B_04D0 ENDP

        PUBLIC  _f1E8B_0536              ; @ 1E8B:0536 stub
_f1E8B_0536 PROC FAR
        ret
_f1E8B_0536 ENDP

        PUBLIC  _f1E8B_0580              ; @ 1E8B:0580 stub
_f1E8B_0580 PROC FAR
        ret
_f1E8B_0580 ENDP

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

        PUBLIC  _f1E8B_0613              ; @ 1E8B:0613 stub
_f1E8B_0613 PROC FAR
        ret
_f1E8B_0613 ENDP

        PUBLIC  _f1E8B_064E              ; @ 1E8B:064E stub
_f1E8B_064E PROC FAR
        ret
_f1E8B_064E ENDP

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

        PUBLIC  _f1E8B_06C0              ; @ 1E8B:06C0 stub
_f1E8B_06C0 PROC FAR
        ret
_f1E8B_06C0 ENDP

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
