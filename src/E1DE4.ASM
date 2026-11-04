; Unit at EOB.EXE 1DE4:0000 (57 bytes, 2 functions), kind asm [H] by the map. VGA retrace support:
; both functions read the Input Status Register 1 at port 3DAh and return or wait on bit 3
; (vertical retrace). Placeholder names.

        .MODEL  LARGE
        .CODE

; 1DE4:0000: AX = inportb(3DAh) & 8 (bit 3, vertical retrace), sampled once. Caller 1A65:00A7
; counts set and clear samples to measure the retrace rate.
        PUBLIC  _IsInVerticalRetrace              ; @ 1DE4:0000
_IsInVerticalRetrace PROC FAR
        push    bp
        mov     bp,sp
        mov     dx,3DAh
        in      al,dx
        and     al,8
        xor     ah,ah
        pop     bp
        ret
_IsInVerticalRetrace ENDP

; 1DE4:000D: wait on vertical retrace. [bp+6] == 1 waits for the end of retrace then its start
; (into active display), otherwise waits for the start of retrace then its end. Caller 1A16:0014.
        PUBLIC  _WaitRetrace              ; @ 1DE4:000D
_WaitRetrace PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    dx
        mov     bl,[bp+6]
        mov     dx,3DAh
        cmp     bl,1
        je      endwait
startwait:
        in      al,dx
        test    al,8
        je      startwait
endloop:
        in      al,dx
        test    al,8
        jne     endloop
        je      done
endwait:
        in      al,dx
        test    al,8
        jne     endwait
startloop:
        in      al,dx
        test    al,8
        je      startloop
done:
        pop     dx
        pop     bx
        pop     ax
        pop     bp
        ret
_WaitRetrace ENDP

        END
