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

EXTERN register_snapshot
EXTERN register_snapshot_taken

section .text

; --- INICIO: FUNCION WRAPPER DEL PROCESO ---
; Este wrapper es el "entry point" real al que saltará 'iretq'.
; La pila (RSP) está 16-byte alineada al entrar aquí.
_process_wrapper:
    ; (RDX y RCX fueron cargados por popState)
    mov r12, rdx    ; r12 = entry_point
    mov r13, rcx    ; r13 = process_terminator
    
    ; Llamamos a la función principal del proceso (ej: test_proc_A)
    ; La pila está alineada, por lo que esta llamada es segura.
    call r12        ; call entry_point()
    
    ; Cuando la función principal retorna, llamamos al terminador
    call r13        ; call process_terminator()
    
    ; Si el terminador retorna (no debería), nos detenemos.
    cli
    hlt
; --- FIN: FUNCION WRAPPER DEL PROCESO ---


; --- INICIO: FUNCION stackInit (VERSIÓN ALINEADA) ---
; rdi: stack_top
; rsi: entry_point
; rdx: process_terminator
stackInit:
    mov rax, rdi    ; rax = stack_top

    ; 1. Forzar alineamiento de 16-bytes
    and rax, -16    ; Alinea rax (stack_top) a 16 bytes (hacia abajo)
    
    ; --- 2. Frame IRETQ (3 qwords) ---
    sub rax, 8
    mov qword [rax], 0x202  ; RFLAGS (Interrupciones habilitadas)
    sub rax, 8
    mov qword [rax], 0x08   ; CS (Selector de código)
    sub rax, 8
    mov qword [rax], _process_wrapper ; RIP (Apunta a nuestro wrapper)
    
    ; --- 3. Frame 'pushState' (15 qwords) ---
    ; (r15, r14, r13, r12, r11, r10, r9, r8)
    sub rax, 8 * 8  ; Relleno para 8 registros (todos 0)
    mov qword [rax + 8*0], 0
    mov qword [rax + 8*1], 0
    mov qword [rax + 8*2], 0
    mov qword [rax + 8*3], 0
    mov qword [rax + 8*4], 0
    mov qword [rax + 8*5], 0
    mov qword [rax + 8*6], 0
    mov qword [rax + 8*7], 0
    
    ; (rsi, rdi, rbp, rdx, rcx, rbx, rax)
    sub rax, 8 * 7  ; Espacio para los 7 registros restantes
    
    mov qword [rax + 8*0], 0    ; rsi
    mov qword [rax + 8*1], 0    ; rdi
    mov qword [rax + 8*2], 0    ; rbp
    mov qword [rax + 8*3], rsi  ; rdx (pop rdx -> entry_point)
    mov qword [rax + 8*4], rdx  ; rcx (pop rcx -> terminator)
    mov qword [rax + 8*5], 0    ; rbx
    mov qword [rax + 8*6], 0    ; rax
    
    ret
; --- FIN: FUNCION stackInit ---

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
