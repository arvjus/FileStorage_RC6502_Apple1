; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

; Read file, load into memory, eventualy execute loaded program

; load Integer-BASIC stored in ProDOS format
load:
    lda #'b'                    ; Integer-BASIC in ProDOS format
    sta flag
    jmp read_request
; read regular file
read:
    lda #0                      ; regular file
    sta flag
read_request:
    lda #CMD_READ
    jsr send_request
    bcc read_request_ok         ; ok, continue
    cmp #ST_DONE
    beq read_done
    cmp #ST_ERROR
    beq read_err
read_request_ok:
    cmp #BODT
    bne read_done               ; nope, no data

; ------------------------------------------------------------------------
; receive 32 bytes- store in the buffer
read_fileentry:
    ldx #0
read_fileentry_byte:
    jsr receive_data_byte
    bcc read_fileentry_store    ; ok, continue
    cmp #ST_DONE
    beq read_done
    cmp #ST_ERROR
    beq read_err
read_fileentry_store:
    sta buffer, x
    inx
    cpx #32
    bne read_fileentry_byte     ; not done with FileEntry?
    
    lda flag
    beq read_receive_data_bytes ; process as regular file
    jsr load_basic_header
    bcs load_err                ; haven't succeeded
        
; print message, load rest of data into memory
read_receive_data_bytes:
    jsr calc_prg_stop
    jsr read_print_messages_start_stop

    ; initialize ptr with buff_fe_start, save address to prg_start
    lda buff_fe_start
    sta prg_start
    sta ptr
    lda buff_fe_start+1
    sta prg_start+1
    sta ptr+1

read_receive_data_byte:
    jsr receive_data_byte
    bcc read_prg_store          ; ok, continue
    cmp #ST_DONE
    beq read_prg_done
    cmp #ST_ERROR
    beq read_err
read_prg_store:
    ldy #$00
    sta (ptr),y
.if DEBUG == 1
    jsr PRBYTE
    lda #' '
    jsr ECHO
.endif     
    ; increment ptr
    inc ptr
    bne read_skip_high          ; if ptr low byte did not wrap, skip high byte increment
    inc ptr+1
read_skip_high:
    ; check if ptr reached prg_stop
    lda ptr+1
    cmp prg_stop+1              ; compare high byte first
    bcc read_receive_data_byte  ; if ptr+1 < prg_stop+1, continue
    bne read_prg_done           ; if ptr+1 > prg_stop+1, exit
    lda ptr
    cmp prg_stop
    bcc read_receive_data_byte  ; if ptr < prg_stop, continue

read_prg_done:
    SET_PTR read_msg3
    jsr print_msg
read_done:
    lda #CR
    jsr ECHO
    clc                         ; success
    rts

read_err:
    lda #'!'
    jsr ECHO
    sec                         ; failure
    rts

load_err:
    SET_PTR load_msg1
    jsr print_msg
    sec                         ; failure
    rts

; print messages start, stop values
read_print_messages_start_stop:
    SET_PTR read_msg1
    jsr print_msg

    SET_PTR buffer+6
    jsr print_msg

    SET_PTR read_msg2
    jsr print_msg

    lda buff_fe_start+1         ; start high
    jsr PRBYTE
    lda buff_fe_start           ; start low
    jsr PRBYTE
    lda #' '
    jsr ECHO
    lda #'-'
    jsr ECHO
    lda #' '
    jsr ECHO
    lda prg_stop+1              ; stop high
    jsr PRBYTE          
    lda prg_stop                ; stop low
    jsr PRBYTE
    lda #' '
    jsr ECHO
    rts

; handle first 512 bytes of data      
load_basic_header:
    ; check header
    jsr receive_data_byte
    bcs load_basic_err          ; byte is expected
    cmp #'A'
    bne load_basic_err          ; file signature is expected
    jsr receive_data_byte
    bcs load_basic_err          ; byte is expected
    cmp #'1'
    bne load_basic_err          ; file signature is expected
    
    ; skip another $48 bytes
    lda #2
    sta ptr                     ; we've already received 2 bytes 
load_skip_zp_loop:    
    jsr receive_data_byte
    bcs load_basic_err          ; byte is expected
    inc ptr
    lda ptr
    cmp #$4a
    bne load_skip_zp_loop

    ; load $4a - $ff data
    lda #0                      ; ZP                      
    sta ptr+1                   ; while LSB points to $4a
read_receive_zp_data_byte:
    jsr receive_data_byte
    bcs load_basic_err          ; byte is expected
    ldy #$00
    sta (ptr),y
    inc ptr
    bne read_receive_zp_data_byte   ; loop until value wrapps

    ; skip $100 - $1ff
    lda #1                      ; stack, first page        
    sta ptr+1                   ; ZP, while LSB is 0
load_skip_stack_loop:    
    jsr receive_data_byte
    bcs load_basic_err          ; byte is expected
    inc ptr
    bne load_skip_stack_loop

    ; we've read 512 ($0200) bytes, subtract this number from buff_fe_size
    lda buff_fe_size+1
    sec
    sbc #$02
    sta buff_fe_size+1
    ; done
    clc                         ; success
    rts
load_basic_err:
    sec                         ; failure
    rts

read_msg1:  .text "Reading ", 0
read_msg2:  .text " into memory ", 0
read_msg3:  .text " .. done.", 0
load_msg1:  .text "This is not BASIC program", 13, 0
