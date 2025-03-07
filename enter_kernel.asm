[bits 32]

;
; Part 4
;
; Code for jumpting to C kernel.
; Implement your routine for entering the kernel here
;
%include "print_string.asm"
extern kernel_main
section .text
global enter_kernel
SUECCSS_ENTER: db "Successfully entered mode", 0
enter_kernel:
    mov ebx, SUECCSS_ENTER
    call print_string_pm
    mov esp, 0x9000
    call kernel_main
    
    jmp $
