GLOBAL sem_lock
GLOBAL sem_unlock

SECTION .text

; https://wiki.osdev.org/Spinlock

sem_lock:
    mov al, 1
    xchg al, BYTE [rdi]
    cmp al, 0
    jne sem_lock
    ret

sem_unlock:
    mov BYTE [rdi], 0
    ret
