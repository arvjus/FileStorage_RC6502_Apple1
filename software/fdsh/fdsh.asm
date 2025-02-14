; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

REAL_HW = 1     ; 1=Apple1 or 0=py65mon
DEBUG = 0       ; 1=Show traces in data exchange
VERSION = "0.9.3"

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
    lda #DAT        
    sta dat_mask    ; A bit to test to distinguish between data and ctrl byte
    lda #0
    sta prefix      ; clear prefix buffer
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
    cmp #KEY_BS
    beq menu_input_back
    cmp #KEY_ESC
    beq exit
    cmp #KEY_CR
    beq menu_process
    inx
    jmp menu_input
menu_input_back:
    cpx #0
    beq menu_input
    dex 
    jmp menu_input
; process command
menu_process:
    lda #0          ; replaece CR with 0
    sta buffer, x
    lda buffer      ; 1st cmd byte
    ldx buffer+1    ; 2nd cmd byte
    ldy #0          ; index in cmd_table

search_command:
    lda cmd_table,y
    beq unknown_cmd ; end of cmd_table
    cmp buffer      ; cmp first character
    bne next_cmd
    lda cmd_table+1,y
    cmp buffer+1    ; cmp second character
    bne next_cmd

    ; jump to address
    lda cmd_table+2, y
    sta ptr
    lda cmd_table+3, y
    sta ptr+1
    jmp (ptr)

next_cmd:
    iny
    iny
    iny
    iny
    bne search_command

unknown_cmd:
    SET_PTR help
    jsr print_msg
    jmp menu
do_list:
    jsr list
    jmp menu
do_load:
    jsr read
    jmp menu
do_exec_prog:
    jsr execute
    jmp menu
do_exec_basic:
    jsr read
    jmp $e2b3       ; BASIC warm entry
do_store:
    jsr write
    jmp menu
do_remove:
    jsr delete
    jmp menu

; Exit to WozMon
exit:   
    jmp $ff00       ; WozMon entry

; Copies up to 14 bytes from buffer+1 to prefix, appends '/' if buffer+2 is not null
prefix_copy:
    ldx #2
    ldy #0
    lda buffer,x
    beq prefix_null_terminate
prefix_copy_loop:
    lda buffer,x
    beq prefix_append_slash
    sta prefix,y
    inx
    iny
    cpy #14
    bne prefix_copy_loop
prefix_append_slash:
    cpy #0
    beq prefix_null_terminate
    lda #$2F            ; '/'
    sta prefix,y
    iny
prefix_null_terminate:
    lda #$00
    sta prefix,y
    jmp menu

; Command table format:
; 2 bytes: command prefix
; 2 bytes: jump address (low/high)
cmd_table:
    .byte 'C', 'D', <prefix_copy, >prefix_copy
    .byte 'L', 'S', <do_list, >do_list
    .byte 'L', 'D', <do_load, >do_load
    .byte 'X', 'P', <do_exec_prog, >do_exec_prog
    .byte 'X', 'B', <do_exec_basic, >do_exec_basic
    .byte 'S', 'T', <do_store, >do_store
    .byte 'R', 'M', <do_remove, >do_remove
    .byte 0          ; End of table marker

help:
.if REAL_HW
    .text "FlashDisk Shell v", VERSION, " by Arvid Juskaitis", 13
    .text "CD        CD[directory]", 13
    .text "List      LS[prefix]", 13
    .text "Store     ST<filename>#start#stop", 13
    .text "Load      LD<filename>|#block", 13
    .text "ExecProg  XP<filename>|#block", 13
    .text "ExecBasic XB<filename>|#block", 13
    .text "Remove    RM<filename>|#block", 13
.endif
    .text 0
