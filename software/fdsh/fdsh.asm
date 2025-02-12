; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis


REAL_HW = 1     ; 1=Apple1 or 0=py65mon
DEBUG = 0       ; 1=Show traces in data exchange
VERSION = "0.9.0"

    .include "defs.asm"
    .include "bss.asm"

*   = $8000
    jmp dfsh

    .include "system.asm" 
    .include "delay.asm" 
    .include "uint2str.asm" 
    .include "common.asm" 
    .include "list.asm" 
    .include "read.asm" 
    .include "write.asm" 
    .include "delete.asm" 

dfsh:
    sei             ; Disable interrupts
    cld             ; Clear decimal mode
    ldx #$ff        ; Initialize stack pointer
    txs

; init ZP variables
    lda #DAT        ; A bit to test to distinguish between data and ctrl byte
    sta dat_mask
.if MOCK_HW
    lda #$97        ; RDY, not BSY and two nibbles == 'w' 
    sta DEVICE_IN
.endif    
    lda #KEY_CR
    jsr ECHO

menu:
; print prompt
    lda #'$'
    jsr ECHO
; read command dline
    ldx #0
menu_input:
    jsr KBDIN
    jsr ECHO
    sta buffer, x
    cmp #KEY_ESC
    beq exit
    cmp #KEY_CR
    beq menu_process
    inx
    jmp menu_input
; process command
menu_process:
    lda #0          ; replaece CR with 0
    sta buffer, x
    lda buffer
    cmp #KEY_L
    beq go_list
    cmp #KEY_R
    beq go_read
    cmp #KEY_X
    beq go_execute
    cmp #KEY_W
    beq go_write
    cmp #KEY_D
    beq go_delete
    cmp #KEY_B
    beq go_basic
    cmp #KEY_QM
    beq go_help
; no valid option left
    SET_PTR help
    jsr print_msg
    jmp menu
go_list:
    jsr list
    jmp menu
go_read:
    jsr read
    jmp menu
go_execute:
    jsr execute
    jmp menu
go_write:
    jsr write
    jmp menu
go_delete:
    jsr delete
    jmp menu
go_basic:
    jmp $e2b3       ; BASIC warm entry
go_help:
    SET_PTR help
    jsr print_msg
    jmp menu
   
; Exit to WozMon
exit:   
    jmp $ff00       ; WozMon entry

help:
.if REAL_HW
    .text "FlashDisk Shell v", VERSION, " by Arvid Juskaitis", 13
    .text "Help    ?", 13
    .text "Basic   B", 13
    .text "List    L[prefix]", 13
    .text "Write   W<filename>#start#stop", 13
    .text "Read    R<filename>|#block", 13
    .text "Execute X<filename>|#block", 13
    .text "Delete  D<filename>|#block", 13
.endif
    .text 0
