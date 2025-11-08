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
EXTERN _test_mm
EXTERN _test_prio
EXTERN _test_processes
EXTERN _test_sync
EXTERN _nice
EXTERN _ps
EXTERN _wc
EXTERN _help
EXTERN _filter
EXTERN _mvar
EXTERN _mvar_writer
EXTERN _mvar_reader
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

; TEST_MM entry point
GLOBAL entry_test_mm
entry_test_mm:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _test_mm
    ret

; TEST_PRIO entry point
GLOBAL entry_test_prio
entry_test_prio:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _test_prio
    ret

; TEST_PROCESSES entry point
GLOBAL entry_test_processes
entry_test_processes:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _test_processes
    ret

; TEST_SYNC entry point
GLOBAL entry_test_sync
entry_test_sync:
    mov rdi, rcx        ; argc in rdi
    mov rsi, r8         ; argv in rsi
    call _test_sync
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

GLOBAL entry_help
entry_help:
    mov rdi,rcx
    mov rsi, r8
    call _help
    ret


GLOBAL entry_filter
entry_filter:
    mov rdi, rcx
    mov rsi, r8
    call _filter
    ret

; MVAR main command
GLOBAL entry_mvar
entry_mvar:
    mov rdi, rcx
    mov rsi, r8
    call _mvar
    ret

; MVAR writer child
GLOBAL entry_mvar_writer
entry_mvar_writer:
    mov rdi, rcx
    mov rsi, r8
    call _mvar_writer
    ret

; MVAR reader child
GLOBAL entry_mvar_reader
entry_mvar_reader:
    mov rdi, rcx
    mov rsi, r8
    call _mvar_reader
    ret