; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

; Zero-Page variables
*   = $02
buffer:             .fill 32        ; command / FileEntry
buff_fe_start = buffer+2    
buff_fe_size = buffer+4

*   = $2d
prg_start:          .addr ?         ; write
prg_stop:           .addr ?         ; calculated address
ptr:                .addr ?         ; print_msg, read, write
execute_flag:       .byte ?         ; non zero to run a program after loading

prefix:             .fill 16        

dat_mask:           .byte ?         ; value contains DAT bit
ms_nibble:          .byte ?         ; store nibble temporary

uint2str_buffer:    .fill 6
uint2str_number:    .word ?
uint2str_remainder: .byte ?
