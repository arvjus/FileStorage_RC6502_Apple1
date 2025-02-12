; Flash Disk Shell
; Copyright (c) retain2025 Arvid Juskaitis

; ptr has to be set befor enter this
print_msg:
    ldy #0
print_msg_loop:
    lda (ptr), y
    beq print_msg_done
    jsr ECHO
    iny
    jmp print_msg_loop
print_msg_done:
    rts

; for debugging purpose
print_buffer:
    ldx #0
print_buffer_loop:
    lda buffer, x
    jsr PRBYTE
    lda #' '
    jsr ECHO
    inx
    cpx #32
    bne print_buffer_loop
    rts

; Convert ASCII hex character to binary nibble (0-15)
hex_to_bin:
    cmp #$30            ; '0'
    bcc invalid_hex
    cmp #$3a            ; '9' + 1
    bcc is_digit
    cmp #$41            ; 'A'
    bcc invalid_hex
    cmp #$47            ; 'F' + 1
    bcs invalid_hex
    sec                 ; Set carry before SBC
    sbc #$37            ; convert 'A'-'F' -> 10-15
    rts
is_digit:
    sec                 ; Set carry before SBC
    sbc #$30            ; convert '0'-'9' -> 0-9
    rts
invalid_hex:
    lda #$00            ; return 0 if invalid
    rts

; sum addresses
calc_prg_stop:
    clc
    lda buff_fe_start   ; start low
    adc buff_fe_size    ; size low
    sta prg_stop        ; stop low
    lda buff_fe_start+1 ; start high
    adc buff_fe_size+1  ; size high
    sta prg_stop+1      ; stop high
    rts

; receive a byte in A
receive_byte:
    ldy #$ff
receive_byte_load:
    lda DEVICE_IN       ; Valid only if RDY is set
    bmi received_byte
    dey
    beq receive_byte_err
    lda #$0c            ; 500uS
    jsr WAIT
    jmp receive_byte_load
received_byte:
.if DEBUG > 1
    tay
    lda #'<'
    jsr ECHO
    tya
    jsr PRBYTE
    lda #' '
    jsr ECHO
    tya
.endif     
    clc   
    rts
receive_byte_err:
    sec
    rts
    
; if C=0, A contains the value 
; if C=1, A contains status
receive_data_byte:
    jsr receive_byte
    bcs receive_data_byte_err   ; timeout
    cmp #NACK
    beq receive_data_byte_done
    jsr send_ack                ; ACK
    cmp #EODT
    beq receive_data_byte_done  ; end of data
    bit dat_mask
    beq receive_data_byte_done  ; binary is not expected
    and #$0F                    ; it's a MS nibble
    asl                 
    asl
    asl
    asl
    sta ms_nibble               ; store to ZP
    jsr receive_byte            ; get another half
    bcs receive_data_byte_err   ; timeout
    cmp #NACK
    beq receive_data_byte_done
    jsr send_ack                ; ACK
    and #$0F        
    ora ms_nibble
.if DEBUG > 1
    tay
    lda #'{'
    jsr ECHO
    tya
    jsr PRBYTE
    lda #' '
    jsr ECHO
    tya
.endif     
    clc                         ; success
    rts
receive_data_byte_done:
    lda #ST_DONE
    rts
receive_data_byte_err:
    lda #ST_ERROR
    rts
    
; wait untill BSY flag is cleared, send a single byte from A
send_byte:
    ldy #$ff
    pha
.if DEBUG > 1
    lda #'>'
    jsr ECHO
    pla
    pha
    jsr PRBYTE
    lda #' '
    jsr ECHO
    pla
    pha
.endif        
wait_not_bsy:
    lda DEVICE_IN
    and #BSY
    beq not_bsy
    dey
    beq send_byte_err
    jsr delay_short
    jmp wait_not_bsy
not_bsy:
    jsr delay_short
    pla
    sta DEVICE_OUT
send_byte_done:
    clc
    rts
send_byte_err:
    sec
    rts

; send data byte as two nibbles
send_data_byte:
    pha         ; MS nibble
    lsr
    lsr
    lsr
    lsr
    ora #DAT
    jsr send_byte
    bcs send_data_byte_err
    pla         ; LS nibble
    and #$0F
    ora #DAT
    jsr send_byte
    clc
    rts
send_data_byte_err:
    pla
    rts

; send ACK, preserve A
send_ack:
    pha
    lda #ACK
    jsr send_byte
    pla
    rts

; ------------------------------------------------------------------------
; send request to device
; at this point A must contain the command and argument is stored in the buffer 
; if C=1, A contains status
send_request:
    jsr send_byte
    bcs send_request_err    ; timeout
    jsr receive_byte        ; ACK is expected
    bcs send_request_err    ; timeout
    cmp #NACK
    beq send_request_done

    lda #BODT
    jsr send_byte
    bcs send_request_err    ; timeout
    jsr receive_byte        ; ACK is expected
    bcs send_request_err    ; timeout
    cmp #NACK
    beq send_request_done

    ; send argument from buffer[1]
    ldx #1
send_request_arg:
    lda buffer, x
    beq send_request_eodt
    jsr send_data_byte
    bcs send_request_err    ; timeout
    inx
    jmp send_request_arg;

send_request_eodt:
    lda #EODT
    jsr send_byte
    bcs send_request_err    ; timeout
.if REAL_HW
    jsr delay_long
    jsr receive_byte        ; ACK. BODT or EODT is expected
    bcs send_request_err    ; timeout
    cmp #NACK
    beq send_request_done
    cmp #EODT
    beq send_request_done   ; end of data
    cmp #ACK                ; it must be CMD_WRITE
    beq send_no_ack         ; don't ACK on ACK
.endif    
    jsr send_ack            ; ACK
send_no_ack:
    clc                     ; success
    rts
send_request_done:
    lda #ST_DONE
    rts
send_request_err:
    lda #ST_ERROR
    rts

