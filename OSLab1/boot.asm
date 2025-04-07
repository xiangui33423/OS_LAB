[org 0x7c00]      ; Bootloader is loaded into memory starting at 0x7c00

KERNEL_OFFSET equ 0x1000  ; This is the memory offset into which we will load our kernel.
KERNEL_START_ADDRESS equ 0x9000 ; This is the location to which the kernel will be loaded.

[bits 16]

global start

start:
  ; Start up code: disabling all interrupts, setting register values, setting stack pointer, etc.
  ; After that, enable interrupts, allowing them to occur again, as we'll use INT10h and INT13h
  cli
  mov ax, 0x00
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, 0x7c00
  sti

  ; BIOS stores index of boot drive it discovered in dl. Hang on to this as we will need to read
  ; from this drive later. Note that this variable is allocated in the globals section below.
  mov [BOOT_DRIVE], dl

  mov bp, KERNEL_START_ADDRESS  ; Set up the stack to load the kernel
  mov sp, bp

  mov bx, MSG_REAL_MODE         ; Print message indicating we are in real mode
  call print_string             ; This helper function is located in the print_string.asm file

  call load_kernel_from_disk    ; Uses BIOS interrupt to read kernel from disk into memory

  call switch_to_pm             ; Switch to protected mode, from which control will not return.
                                ; After the switch, enter 32-bit code at the offset BEGIN_PM.

  jmp $


;
; Part 1
;
; Code for loading kernel binary from disk into memory.
; Implement your load_kernel_from_disk code below, if disk reading fails, 
; your function should give an error message.
; Note that you must read the correct number of sectors.
;

[bits 16]
load_kernel_from_disk:

  mov bx, MSG_LOAD_KERNEL ; Print message for kernel load.
  call print_string
  
  mov bx, KERNEL_OFFSET
  mov dh, 0x1
  mov dl, [BOOT_DRIVE]

  pusha
  push dx
  mov ah, 0x2
  mov al, 25
  mov CX, 0x2
  mov dh, 0x0


  int 0x13
  pop dx

  jc load_failed          ; If carry flag is set, the read failed

  mov bx, MSG_KERNEL_LOAD_SUCCESS
  call print_string
  jmp load_done

load_failed:
  mov bx, MSG_KERNEL_LOAD_FAILED
  call print_string
  call load_kernel_from_disk
load_done:
  popa

  ret

;
; Part 2
;
; Code for setting up the global descriptor table
; Implement your GDT set up code below
;




gdt_start:
  ; Null Descriptor 
  dd 0x00000000
  dd 0x00000000
gdt_code:
  ; Code segment descriptor
  dw 0xffff     
  dw 0x0          
  db 0x0          
  db 10011010b    
  db 11001111b    
  db 0x0          
gdt_data:
  ; Data Segment Descriptor
  dw 0xffff       
  dw 0x0        
  db 0x0        
  db 10010010b  
  db 11001111b  
  db 0x0        

gdt_end:
; ...

gdt_descriptor:
; ...
  dw gdt_end - gdt_start - 1
  dd gdt_start
  

CODE_SEG equ gdt_code - gdt_start 
DATA_SEG equ gdt_data - gdt_start 
;
; Part 3
;
; Code for switching from 16-bit real mode to 32-bit protected mode.
; Implement your switch_to_pm function below.
; Feel to add other function(s) as needed, because you do need another funtion running
; in 32-bit mode, inside which BEGIN_PM must be called.
;

[bits 16]

switch_to_pm:

 
  cli
  lgdt [gdt_descriptor]

  mov eax, cr0
  or eax, 0x1
  mov cr0, eax
  jmp CODE_SEG:init_pm



[bits 32]

; Some other function(s) that set the registers and stack pointers
; At the end of the function, call BEGIN_PM
init_pm:
  mov ax, DATA_SEG
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax 
  mov ss, ax 
  mov ebp, 0x90000
  mov esp, ebp
  call BEGIN_PM


;
; Finally... This is the entry point for 32-bit code and we'll not return from it.
;

[bits 32]

BEGIN_PM:
  mov ebx, MSG_PMODE      ; Print message indicating we are in real mode.
  call print_string_pm
  call KERNEL_OFFSET      ; Begin executing the kernel.
  
  jmp $                   ; If return ever controls from the kernel, hang.


;
; Global variables and includes. No need to edit.
;
%include "print_string.asm"

BOOT_DRIVE db 0x0
SUECCSS_ENTER: db "Successfully entered mode", 0
MSG_REAL_MODE:
  db "started in 16-bit real mode", 0xa, 0xd, 0x0

MSG_PMODE:
  db "successfully landed in 32-bit protected mode." ,0x0

MSG_SSD_SUCCESS:
  db "load ssd success", 0xa, 0xd, 0x0

MSG_SSD_FAILED:
  db "load ssd failed", 0xa, 0xd, 0x0

MSG_KERNEL_LOAD_SUCCESS:
  db "load kernel success", 0xa, 0xd, 0x0

MSG_KERNEL_LOAD_FAILED:
  db "load kernel failed", 0xa, 0xd, 0x0

MSG_LOAD_KERNEL:
  db "loading kernel into memory...", 0xa, 0xd, 0x0
  
; Boot sector padding
times 510-($-$$) db 0x0
dw 0xaa55



