; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

; Delay routine. Taken from the Apple II ROM routine at $FCA8.
; Delay in clock cycles is 13 + 27/2 * A + 5/2 * A * A
; Changes registers: A
; Also see: chapter 3 of "Assembly Cookbook for the Apple II/IIe"
; for more details on how to use it.

;$12 - 1ms
;$c6 - 100ms
;$ff - 166ms

wait:    
    sec
wait2:   
    pha
wait3:   
    sbc   #$01
    bne   wait3
    pla                ; (13+27/2*a+5/2*a*a)
    sbc   #$01
    bne   wait2
    rts

delay_long:
    pha
    lda #$ff            ; 166ms
    jsr wait
    pla
    rts

delay_short:
    pha
    lda #$12            ; 1ms
    jsr wait
    pla
    rts
