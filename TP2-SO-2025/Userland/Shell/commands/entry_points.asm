; Entry point wrappers for shell commands
; These wrappers extract argc/argv from registers (as set up by stackInit)
; and call the actual command functions with proper calling convention

EXTERN _block
EXTERN _cat
EXTERN _clear
EXTERN _echo
EXTERN _kill
EXTERN _loop
EXTERN _mem
EXTERN _nice
EXTERN _ps
EXTERN _wc

SECTION .text

; Generic entry point wrapper macro
; Takes argc (in rcx) and argv (in r8) and calls the command function
; The entry points receive argc in rcx and argv in r8 from stackInit

; ECHO entry point
GLOBAL entry_echo
entry_echo:
    mov rdi, rcx        ; argc in rdi (first parameter)
    mov rsi, r8         ; argv in rsi (second parameter)
    call _echo
    ret

; CAT entry point
GLOBAL entry_cat
entry_cat:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _cat
    ret

; CLEAR entry point
GLOBAL entry_clear
entry_clear:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _clear
    ret

; BLOCK entry point
GLOBAL entry_block
entry_block:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _block
    ret

; KILL entry point
GLOBAL entry_kill
entry_kill:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _kill
    ret

; LOOP entry point
GLOBAL entry_loop
entry_loop:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _loop
    ret

; MEM entry point
GLOBAL entry_mem
entry_mem:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _mem
    ret

; NICE entry point
GLOBAL entry_nice
entry_nice:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _nice
    ret

; PS entry point
GLOBAL entry_ps
entry_ps:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _ps
    ret

; WC entry point
GLOBAL entry_wc
entry_wc:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _wc
    ret

