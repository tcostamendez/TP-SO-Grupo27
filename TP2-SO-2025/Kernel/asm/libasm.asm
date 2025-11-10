GLOBAL cpuVendor
GLOBAL getKeyboardBuffer

GLOBAL getSecond
GLOBAL getMinute
GLOBAL getHour

GLOBAL setPITMode
GLOBAL setPITFrequency
GLOBAL setSpeaker

GLOBAL getRegisterSnapshot

GLOBAL stackInit
GLOBAL _process_wrapper

GLOBAL outw

EXTERN register_snapshot
EXTERN register_snapshot_taken


section .text
_process_wrapper:
    ; rdx = entry_point
    ; rbp = process_terminator
    
    mov r12, rdx 
    mov r13, rbp
    
    call r12        ; call entry_point()
    call r13        ; call process_terminator()
    
    cli
    hlt

stackInit:
    push rbp
    mov rbp, rsp

    ; Align stack to 16 bytes
    and rdi, -16
    mov rax, rdi

    ; Prepare frame for iretq (5 values expected by iretq)
    sub rax, 8
    mov QWORD [rax], 0x0      ; SS
    sub rax, 8
    mov QWORD [rax], rdi      ; RSP (stack original)
    sub rax, 8
    mov QWORD [rax], 0x202    ; RFLAGS (interrupts enabled)
    sub rax, 8
    mov QWORD [rax], 0x8      ; CS
    sub rax, 8
    mov QWORD [rax], _process_wrapper 

    ; Register save order: r15, r14, r13, r12, r11, r10, r9, r8, rsi, rdi, rbp, rdx, rcx, rbx, rax
    sub rax, 8
    mov QWORD [rax], 0        ; rax
    sub rax, 8
    mov QWORD [rax], 0        ; rbx
    sub rax, 8
    mov QWORD [rax], rcx      ; rcx (argc)
    sub rax, 8
    mov QWORD [rax], rsi      ; rdx (entry_point) - _process_wrapper lo toma en r12
    sub rax, 8
    mov QWORD [rax], rdx      ; rbp (process_terminator) - _process_wrapper lo toma en r13
    sub rax, 8
    mov QWORD [rax], 0        ; rdi
    sub rax, 8
    mov QWORD [rax], 0        ; rsi
    sub rax, 8
    mov QWORD [rax], r8       ; r8 (argv)
    sub rax, 8
    mov QWORD [rax], 0        ; r9
    sub rax, 8
    mov QWORD [rax], 0        ; r10
    sub rax, 8
    mov QWORD [rax], 0        ; r11
    sub rax, 8
    mov QWORD [rax], 0        ; r12
    sub rax, 8
    mov QWORD [rax], 0        ; r13
    sub rax, 8
    mov QWORD [rax], 0        ; r14
    sub rax, 8
    mov QWORD [rax], 0        ; r15

    mov rsp, rbp
    pop rbp
    ret   ; rax contiene el nuevo RSP
getKeyboardBuffer:
	push rbp
	mov rbp, rsp

	in al, 60h

	mov rsp, rbp
	pop rbp

	ret


getSecond:
	push rbp
	mov rbp, rsp

	mov al, 0
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


getMinute:
	push rbp
	mov rbp, rsp

	mov al, 2
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


getHour:
	push rbp
	mov rbp, rsp

	mov al, 4
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


setPITMode:
	push rbp
	mov rbp, rsp
	
	mov rax, rdi
	out 0x43, al

	mov rsp, rbp
	pop rbp

	ret


setPITFrequency:
	push rbp
	mov rbp, rsp

	mov rax, rdi
	out 0x42, al
	mov al, ah
	out 0x42, al

	mov rsp, rbp
	pop rbp
	ret  


setSpeaker:
	push rbp
	mov rbp, rsp

	cmp rdi, 0
	je .off

	.on:
	in al, 0x61
	or al, 0x03
	out 0x61, al
	jmp .end

	.off:
	in al, 0x61
	and al, 0xFC
	out 0x61, al

	.end:
	mov rsp, rbp
	pop rbp
	ret

; void outw(uint16_t port, uint16_t val)
; Writes a word (2 bytes) to an I/O port
outw:
    mov rdx, rdi    ; First argument (port)
    mov rax, rsi    ; Second argument (value)
    out dx, ax      ; Send word to port
    ret