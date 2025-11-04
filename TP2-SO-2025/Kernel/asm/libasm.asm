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

; --- INICIO: FUNCION WRAPPER DEL PROCESO ---
; Este wrapper es el "entry point" real al que saltará 'iretq'.
; Cuando llegamos aquí, popState ya restauró los registros:
;   - rdx contiene entry_point
;   - rbp contiene process_terminator
_process_wrapper:
    ; Alinear el stack (después de iretq el stack ya está alineado en teoría)
    ; pero para estar seguros, hacemos sub rsp, 8 si es necesario
    
    ; Los registros ya fueron restaurados por popState
    ; rdx = entry_point
    ; rbp = process_terminator
    
    ; Guardar en registros no volátiles para preservar a través de la llamada
    mov r12, rdx    ; r12 = entry_point
    mov r13, rbp    ; r13 = process_terminator
    
    ; Llamamos a la función principal del proceso
    ; La pila debe estar 16-byte alineada antes de call
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
; stackInit:
;     mov rax, rdi    ; rax = stack_top

;     ; 1. Forzar alineamiento de 16-bytes
;     and rax, -16    ; Alinea rax (stack_top) a 16 bytes (hacia abajo)
    
;     ; --- 2. Frame IRETQ (3 qwords) ---
;     ; Guardamos el valor actual de CS en r14
;     mov r14, cs

;     sub rax, 8
;     mov qword [rax], 0x202  ; RFLAGS
;     sub rax, 8
;     mov qword [rax], r14    ; CS (Guardamos el selector CS actual)
;     sub rax, 8
;     mov qword [rax], _process_wrapper ; RIP
    
;     ; --- 3. Frame 'pushState' (15 qwords) ---
;     ; (r15, r14, r13, r12, r11, r10, r9, r8)
;     sub rax, 8 * 8  ; Relleno para 8 registros (todos 0)
;     mov qword [rax + 8*0], 0
;     mov qword [rax + 8*1], 0
;     mov qword [rax + 8*2], 0
;     mov qword [rax + 8*3], 0
;     mov qword [rax + 8*4], 0
;     mov qword [rax + 8*5], 0
;     mov qword [rax + 8*6], 0
;     mov qword [rax + 8*7], 0
    
;     ; (rsi, rdi, rbp, rdx, rcx, rbx, rax)
;     sub rax, 8 * 7  ; Espacio para los 7 registros restantes
    
;     mov qword [rax + 8*0], 0    ; rax_val (pop rax)
;     mov qword [rax + 8*1], 0    ; rbx_val (pop rbx)
;     mov qword [rax + 8*2], rsi  ; rcx_val (pop rcx -> terminator)
;     mov qword [rax + 8*3], rdx  ; rdx_val (pop rdx -> entry_point)
;     mov qword [rax + 8*4], 0    ; rbp_val (pop rbp)
;     mov qword [rax + 8*5], 0    ; rdi_val (pop rdi)
;     mov qword [rax + 8*6], 0    ; rsi_val (pop rsi)
    
;     add rax, 8 * 14

;     ret
; ; --- FIN: FUNCION stackInit ---

stackInit:
    push rbp
    mov rbp, rsp
    
    ; rdi = stack_top
    ; rsi = entry_point
    ; rdx = process_terminator
    ; rcx = argc
    ; r8  = argv

    ; Alineamos el stack a 16 bytes
    and rdi, -16
    mov rax, rdi    ; Guardamos stack_top alineado

    ; Preparamos frame para iretq (5 valores que iretq espera)
    sub rax, 8
    mov QWORD [rax], 0x0      ; SS
    sub rax, 8
    mov QWORD [rax], rdi      ; RSP (stack original)
    sub rax, 8
    mov QWORD [rax], 0x202    ; RFLAGS (interrupts enabled)
    sub rax, 8
    mov QWORD [rax], 0x8      ; CS
    sub rax, 8
    mov QWORD [rax], _process_wrapper      ; RIP (CAMBIO: usar wrapper, no entry_point)

    ; Guardamos estado inicial de registros para popState
    ; El orden es: r15, r14, r13, r12, r11, r10, r9, r8, rsi, rdi, rbp, rdx, rcx, rbx, rax
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

    ; Retornamos el nuevo RSP (que está en rax)
    ; NO tocar rax después de esto!
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