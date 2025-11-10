
GLOBAL _cli
GLOBAL _sti
GLOBAL _hlt
GLOBAL _force_scheduler_interrupt
GLOBAL soft_irq0

GLOBAL picMasterMask
GLOBAL picSlaveMask

GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _irq80Handler

GLOBAL _exceptionHandler00
GLOBAL _exceptionHandler06
GLOBAL _exceptionHandler08     
GLOBAL _exceptionHandler0D      
GLOBAL _exceptionHandler0E    

GLOBAL register_snapshot
GLOBAL register_snapshot_taken

GLOBAL _load_idt_asm

EXTERN irqDispatcher
EXTERN syscallDispatcher
EXTERN exceptionDispatcher
EXTERN getStackBase
EXTERN schedule

SECTION .text

%macro pushState 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popStateButRAX 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
%endmacro

%macro popState 0
	popStateButRAX
	pop rax
%endmacro

%macro irqHandlerMaster 1
	pushState

	mov rdi, %1 
	call irqDispatcher

	mov al, 20h
	out 20h, al

	popState
	iretq
%endmacro

%macro exceptionHandler_no_err 1
    cli
    
    mov [exception_register_snapshot + 0x00], rax
    mov [exception_register_snapshot + 0x08], rbx
    mov [exception_register_snapshot + 0x10], rcx
    mov [exception_register_snapshot + 0x18], rdx
    mov [exception_register_snapshot + 0x20], rbp
    mov [exception_register_snapshot + 0x28], rdi
    mov [exception_register_snapshot + 0x30], rsi
    mov [exception_register_snapshot + 0x38], r8
    mov [exception_register_snapshot + 0x40], r9
    mov [exception_register_snapshot + 0x48], r10
    mov [exception_register_snapshot + 0x50], r11
    mov [exception_register_snapshot + 0x58], r12
    mov [exception_register_snapshot + 0x60], r13
    mov [exception_register_snapshot + 0x68], r14
    mov [exception_register_snapshot + 0x70], r15
    mov [exception_register_snapshot + 0x78], rsp

    mov rax, [rsp + 0x00] 
    mov [exception_register_snapshot + 0x80], rax
    mov rax, [rsp + 0x10] 
    mov [exception_register_snapshot + 0x88], rax
    
    mov rdi, %1
    mov rsi, exception_register_snapshot
    call exceptionDispatcher

.hang_no_err:
    hlt
    jmp .hang_no_err
%endmacro

%macro exceptionHandler_err 1
    cli
    
    mov [exception_register_snapshot + 0x00], rax
    mov [exception_register_snapshot + 0x08], rbx
    mov [exception_register_snapshot + 0x10], rcx
    mov [exception_register_snapshot + 0x18], rdx
    mov [exception_register_snapshot + 0x20], rbp
    mov [exception_register_snapshot + 0x28], rdi
    mov [exception_register_snapshot + 0x30], rsi
    mov [exception_register_snapshot + 0x38], r8
    mov [exception_register_snapshot + 0x40], r9
    mov [exception_register_snapshot + 0x48], r10
    mov [exception_register_snapshot + 0x50], r11
    mov [exception_register_snapshot + 0x58], r12
    mov [exception_register_snapshot + 0x60], r13
    mov [exception_register_snapshot + 0x68], r14
    mov [exception_register_snapshot + 0x70], r15
    mov [exception_register_snapshot + 0x78], rsp
    
    mov rax, [rsp + 0x08] 
    mov [exception_register_snapshot + 0x80], rax
    mov rax, [rsp + 0x18] 
    mov [exception_register_snapshot + 0x88], rax
    sub rsp, 8
    
    mov rdi, %1
    mov rsi, exception_register_snapshot
    call exceptionDispatcher
    
.hang_err:
    hlt
    jmp .hang_err
%endmacro

_hlt:
	sti
	hlt
	ret

_cli:
	cli
	ret

_sti:
	sti
	ret

picMasterMask:
	push rbp     
	mov rbp, rsp
	mov rax, rdi

	out 0x21, al ; https://wiki.osdev.org/8259_PIC#Masking

	mov rsp, rbp
	pop rbp
	ret

picSlaveMask:
	push rbp
	mov rbp, rsp
	mov rax, rdi

	out 0xA1, al ; https://wiki.osdev.org/8259_PIC#Masking

	mov rsp, rbp
	pop rbp
	ret

_load_idt_asm:
    lidt [rdi]
    ret	

_irq00Handler:
    pushState

    mov rdi, 0 
    call irqDispatcher
    
    mov rdi, rsp        
    call schedule
    mov rsp, rax  

    mov al, 20h
    out 20h, al

    popState

    iretq

_force_scheduler_interrupt:
    mov byte [soft_irq0], 1
    int 0x20    
    ret

_irq01Handler:
	pushfq
	pushState

	mov rdi, 1 
	call irqDispatcher

	cmp rax, REGISTER_SNAPSHOT_KEY_SCANCODE 
	jne .skip

	popState  
	pushState 

	mov [register_snapshot + 0x08 * 0x00], rax
	mov [register_snapshot + 0x08 * 0x01], rbx
	mov [register_snapshot + 0x08 * 0x02], rcx
	mov [register_snapshot + 0x08 * 0x03], rdx
	mov [register_snapshot + 0x08 * 0x04], rbp
	mov [register_snapshot + 0x08 * 0x05], rdi
	mov [register_snapshot + 0x08 * 0x06], rsi
	mov [register_snapshot + 0x08 * 0x07], r8
	mov [register_snapshot + 0x08 * 0x08], r9
	mov [register_snapshot + 0x08 * 0x09], r10
	mov [register_snapshot + 0x08 * 0x0A], r11
	mov [register_snapshot + 0x08 * 0x0B], r12
	mov [register_snapshot + 0x08 * 0x0C], r13
	mov [register_snapshot + 0x08 * 0x0D], r14
	mov [register_snapshot + 0x08 * 0x0E], r15

	mov [register_snapshot + 0x08 * 0x0F], rsp 

	mov rax, [ rsp + 0x08 * 16 ] 
	mov [register_snapshot + 0x08 * 0x10], rax 

	mov rax, [ rsp + 0x08 * 15 ] 
	mov [register_snapshot + 0x08 * 0x11], rax 

	mov byte [register_snapshot_taken], 0x01

	.skip:
	mov al, 20h
	out 20h, al

	popState
	add rsp, 0x08 

	iretq

_irq80Handler:
    pushState

    push rax             

    lea rdi, [rsp + 8]
    call syscallDispatcher

    mov rbx, rax

    mov al, 20h
    out 20h, al

    mov rax, rbx

    add rsp, 8

    popStateButRAX
    add rsp, 8 
    iretq

_exceptionHandler00:
    exceptionHandler_no_err 0

_exceptionHandler06:
    exceptionHandler_no_err 6

_exceptionHandler08:
    exceptionHandler_err 8

_exceptionHandler0D:
    exceptionHandler_err 13

_exceptionHandler0E:
    exceptionHandler_err 14

section .bss
	exception_register_snapshot resq 18
	register_snapshot resq 18
	register_snapshot_taken resb 1
    soft_irq0 resb 1

section .rodata
	REGISTER_SNAPSHOT_KEY_SCANCODE equ 0x58 
	USERLAND equ 0x400000 