; Unit at EOB.EXE 1BB1:0001 (208 bytes, 2 functions), kind asm [H] by the map. 1BB1:0001 is a
; single 00 byte with `segment` evidence (the unit's start marker, like 1990:0001); the one real
; function follows at 1BB1:0002. Placeholder names.
;
; 1BB1:0002 sets up the level view/store buffer. Called from Level07A9 (07A9:010C) with the view
; origin (8,8) or (0,0) and the level's name record. It acquires the semaphore at ds:812E, tells
; 1BBE:0002, records the origin (ds:8158/815A) and the name far pointer (ds:816E/8170), reads the
; map size from the name record (byte +3 = width, byte +2 + 1 = height, so both are unsigned char
; counts), and if the buffer's size changed frees the old far buffer (ds:816A/816C) and allocates
; a new one sized through the indirect call [ds:80C6] plus 16 bytes, checking the largest free
; block (19BC:023D). It then calls [ds:80AA] on the far pointer ds:8172/8174, calls 1BBE:00ED,
; calls [ds:80A2], releases the semaphore and returns 1 (or 0 when no block can be allocated).
;
; The unit is hand-written assembly (the compiler's C functions never push dx/es after the frame,
; see tools/re/map_units.py c_like), so the module uses explicit segments like the runtime startup;
; `ASSUME CS:NOTHING` around each direct far call keeps TASM from turning `lcall` into `push cs;
; call` (which it does when it assumes the far callee is in the same code segment).

        _DATA   SEGMENT WORD PUBLIC 'DATA'
        EXTRN   g812E:WORD              ; @ ds:812E: semaphore, 0 = free (busy-wait acquire)
        EXTRN   g8154:WORD              ; @ ds:8154: buffer width already allocated
        EXTRN   g8156:WORD              ; @ ds:8156: buffer height already allocated
        EXTRN   g8158:WORD              ; @ ds:8158: view origin, x (argument 1)
        EXTRN   g815A:WORD              ; @ ds:815A: view origin, y (argument 2)
        EXTRN   g816A:WORD              ; @ ds:816A: level buffer, far offset
        EXTRN   g816C:WORD              ; @ ds:816C: level buffer, far segment (0 = none)
        EXTRN   g816E:WORD              ; @ ds:816E: name record, far offset
        EXTRN   g8170:WORD              ; @ ds:8170: name record, far segment
        EXTRN   g8172:WORD              ; @ ds:8172: far pointer argument, offset
        EXTRN   g8174:WORD              ; @ ds:8174: far pointer argument, segment
        EXTRN   g80C6:DWORD             ; @ ds:80C6: indirect far function, size of a map (H, W)
        EXTRN   g80AA:DWORD             ; @ ds:80AA: indirect far function, called with ds:8172/8174
        EXTRN   g80A2:DWORD             ; @ ds:80A2: indirect far function, no arguments
        _DATA   ENDS
        DGROUP  GROUP   _DATA

        _TEXT   SEGMENT WORD PUBLIC 'CODE'
        ASSUME  CS:_TEXT, DS:DGROUP
        db      0                       ; 1BB1:0001: the unit's start marker byte (segment evidence)
        EXTRN   _HidePointer:FAR         ; @ 1BBE:0002
        EXTRN   _ShowPointer:FAR         ; @ 1BBE:00ED
        EXTRN   _MemAlloc:FAR           ; @ 19BC:0007
        EXTRN   _MemFree:FAR            ; @ 19BC:0137
        EXTRN   _LargestFreeBlock:FAR         ; @ 19BC:023D: returns the largest free block (DX:AX)

        PUBLIC  _f1BB1_0002             ; @ 1BB1:0002
_f1BB1_0002 PROC FAR
        push    bp
        mov     bp,sp
        push    dx
        push    si
        push    es
acquire:
        cmp     word ptr [g812E],0
        jne     acquire
        inc     word ptr [g812E]
        ASSUME  CS:NOTHING
        call    far ptr _HidePointer
        ASSUME  CS:_TEXT
        mov     ax,[bp+6]
        mov     word ptr [g8158],ax
        mov     ax,[bp+8]
        mov     word ptr [g815A],ax
        mov     si,[bp+0Ah]
        mov     word ptr [g816E],si
        mov     es,[bp+0Ch]
        mov     word ptr [g8170],es
        xor     ah,ah
        mov     al,byte ptr es:[si+3]
        xor     dh,dh
        mov     dl,byte ptr es:[si+2]
        inc     dx
        cmp     word ptr [g816C],0
        je      setsize
        cmp     ax,word ptr [g8154]
        jne     resize
        cmp     dx,word ptr [g8156]
        je      ready
resize: push    ax
        push    dx
        push    word ptr [g816C]
        push    word ptr [g816A]
        ASSUME  CS:NOTHING
        call    far ptr _MemFree
        ASSUME  CS:_TEXT
        add     sp,4
        pop     dx
        pop     ax
setsize:
        mov     word ptr [g8154],ax
        push    ax
        mov     word ptr [g8156],dx
        push    dx
        call    dword ptr [g80C6]
        add     sp,4
        add     ax,16
        mov     si,ax
        ASSUME  CS:NOTHING
        call    far ptr _LargestFreeBlock
        ASSUME  CS:_TEXT
        or      dx,dx
        jne     alloc
        cmp     ax,si
        jb      failed
alloc:  mov     ax,0
        push    ax
        xor     ax,ax
        push    ax
        mov     ax,si
        sub     ax,16
        push    ax
        ASSUME  CS:NOTHING
        call    far ptr _MemAlloc
        ASSUME  CS:_TEXT
        add     sp,6
        cmp     dx,0
        je      failed
        mov     word ptr [g816A],ax
        mov     word ptr [g816C],dx
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
failed: xor     ax,ax
release:
        dec     word ptr [g812E]
        pop     es
        pop     si
        pop     dx
        pop     bp
        ret
_f1BB1_0002 ENDP

        _TEXT   ENDS
        END
