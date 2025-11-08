
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
GLOBAL _exceptionHandler08     ; <-- AÑADIR
GLOBAL _exceptionHandler0D     ; <-- AÑADIR
GLOBAL _exceptionHandler0E     ; <-- AÑADIR

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

	mov rdi, %1 ; pass argument to irqDispatcher
	call irqDispatcher

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	iretq
%endmacro

; --- INICIO: REEMPLAZAR MACROS POR ESTAS VERSIONES COMPLETAS ---

; Macro para excepciones SIN código de error (ej: 0x0, 0x6)
%macro exceptionHandler_no_err 1
    cli
    
    ; Guardar TODOS los registros
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

    ; Leer RIP y RFLAGS (sin código de error)
    ; Stack: [RIP] [CS] [RFLAGS]
    mov rax, [rsp + 0x00] ; RIP
    mov [exception_register_snapshot + 0x80], rax
    mov rax, [rsp + 0x10] ; RFLAGS
    mov [exception_register_snapshot + 0x88], rax
    
    ; Pila: HW (3 pushes) -> Desalineada (...x8)
    ; 'call' (1 push) -> ALINEADA para C (...x0)
    mov rdi, %1
    mov rsi, exception_register_snapshot
    call exceptionDispatcher

.hang_no_err:
    hlt
    jmp .hang_no_err
%endmacro

; Macro para excepciones CON código de error (ej: 0x8, 0xD, 0xE)
%macro exceptionHandler_err 1
    cli
    
    ; Guardar TODOS los registros
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
    
    ; Leer RIP y RFLAGS (CON código de error)
    ; Stack: [Error] [RIP] [CS] [RFLAGS]
    mov rax, [rsp + 0x08] ; RIP (saltamos el error code)
    mov [exception_register_snapshot + 0x80], rax
    mov rax, [rsp + 0x18] ; RFLAGS (saltamos error, RIP, CS)
    mov [exception_register_snapshot + 0x88], rax
    
    ; Pila: HW (4 pushes) -> Alineada (...x0)
    ; 'call' (1 push) -> DESALINEADA para C
    
    ; --- FIX DE ALINEACIÓN ---
    sub rsp, 8
    
    mov rdi, %1
    mov rsi, exception_register_snapshot
    call exceptionDispatcher
    
.hang_err:
    hlt
    jmp .hang_err
%endmacro
; --- FIN DEL REEMPLAZO ---

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
	push rbp     ; Stack frame
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
    ; RDI (primer argumento) contiene el puntero a la estructura IDTR
    lidt [rdi]
    ret	

; --- INICIO: CAMBIO CRÍTICO ---
_irq00Handler:
    ; 1. Guardar todos los registros del Proceso A
    pushState
    ; Pila: HW(3) + pushState(15) = 18 pushes (ALINEADA, ...x0)

    ; --- INICIO: CORRECCIÓN DE ALINEACIÓN ---
    
    ; 2. Llamar a irqDispatcher (alineado)
    mov rdi, 0          ; Argumento 0 (IRQ 0)
    call irqDispatcher
    
    ; 3. Llamar a nuestro scheduler en C (alineado)
    ; Pasamos el RSP actual (que apunta a r15)
    mov rdi, rsp        
    call schedule
    
    ; 4. *** EL CAMBIO DE CONTEXTO ***
    ; 'schedule' retorna en 'rax' el NUEVO RSP
    mov rsp, rax        ; Cambiamos al stack del Proceso B
                        ; (El 'sub rsp, 8' se descarta, lo cual es correcto)

    ; 5. Enviar EOI (End of Interrupt) al PIC
    mov al, 20h
    out 20h, al

    ; 6. Restaurar los registros del Proceso B
    popState

    ; 7. Volver de la interrupción (ahora ejecutando Proceso B)
    iretq
; --- FIN: CAMBIO CRÍTICO ---

; --- INICIO: NUEVA FUNCIÓN (si no la tenías) ---
; Esta es la función que llama yield_cpu() y semWait()
; para forzar un context switch.
_force_scheduler_interrupt:
    ; Marcar que la próxima IRQ0 fue disparada por software
    mov byte [soft_irq0], 1
    int 0x20    ; Dispara manualmente _irq00Handler
    ret
; --- FIN: NUEVA FUNCIÓN ---

; Keyboard
_irq01Handler:
	pushfq
	pushState

	mov rdi, 1 ; pass argument to irqDispatcher
	call irqDispatcher

	cmp rax, REGISTER_SNAPSHOT_KEY_SCANCODE ; F12 KEY SCANCODE
	jne .skip

	popState  ; restore register values
	pushState ; preserve stack frame

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

	mov [register_snapshot + 0x08 * 0x0F], rsp ; rsp

	mov rax, [ rsp + 0x08 * 16 ] ; get the return address
	mov [register_snapshot + 0x08 * 0x10], rax ; rip

	mov rax, [ rsp + 0x08 * 15 ] ; get the rflags
	mov [register_snapshot + 0x08 * 0x11], rax ; rflags

	mov byte [register_snapshot_taken], 0x01

	.skip:
	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	add rsp, 0x08 ; remove rflags from the stack

	iretq

; System Call
; Not using the %irqHandlerMaster macro because it needs to pass the stack pointer to the syscall
_irq80Handler:
    pushState

    ; --- INICIO: CORRECCIÓN DE ALINEACIÓN ---
    push rax            ; Pushes 1 más (total 19, desalineado)
    ; --- FIN: CORRECCIÓN DE ALINEACIÓN ---

    lea rdi, [rsp + 8]
    call syscallDispatcher

    mov rbx, rax

    ; signal pic EOI (End of Interrupt)
    mov al, 20h
    out 20h, al

    mov rax, rbx

    ; --- INICIO: CORRECCIÓN DE ALINEACIÓN ---
    ; Devolvemos los 8 bytes que restamos
    add rsp, 8
    ; --- FIN: CORRECCIÓN DE ALINEACIÓN ---

    popStateButRAX
    add rsp, 8 ; skip the error code pushed by irqDispatcher
    iretq

; --- INICIO: CORRECCIÓN DE LLAMADAS ---
; Zero Division Exception (SIN código de error)
_exceptionHandler00:
    exceptionHandler_no_err 0

; Invalid Opcode Exception (SIN código de error)
_exceptionHandler06:
    exceptionHandler_no_err 6

; Double Fault Exception (CON código de error)
_exceptionHandler08:
    exceptionHandler_err 8

; General Protection Fault Exception (CON código de error)
_exceptionHandler0D:
    exceptionHandler_err 13

; Page Fault Exception (CON código de error)
_exceptionHandler0E:
    exceptionHandler_err 14
; --- FIN: CORRECCIÓN DE LLAMADAS ---

section .bss
	exception_register_snapshot resq 18
	register_snapshot resq 18
	register_snapshot_taken resb 1
    soft_irq0 resb 1

section .rodata
	REGISTER_SNAPSHOT_KEY_SCANCODE equ 0x58 ; F12 KEY SCANCODE
	USERLAND equ 0x400000 ; userland (shell module address)