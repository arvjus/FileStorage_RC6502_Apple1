; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

; Zero-Page variables
*   = $00
buffer:             .fill 32        ; command / FileEntry
buff_fe_start = buffer+2    
buff_fe_size = buffer+4
prg_start:          .addr ?         ; write, jmp_prg
prg_stop:           .addr ?         ; calculated address

; $24 - $2b used by WozMon
*   = $2d
ptr:                .addr ?         ; print_msg, read, write
flag:               .byte ?         ; flag to control execution, prg storing, loading

prefix:             .fill 14        

dat_mask:           .byte ?         ; value contains DAT bit
ms_nibble:          .byte ?         ; store nibble temporary

tmp_buffer:         .fill 10        ; variables used by conversion functions

; $4a - $ff used by A1 BASIC
*   = $4a
lomem:              .addr ?         ; BASIC program location
himem:              .addr ?
