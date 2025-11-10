GLOBAL sys_start_beep
GLOBAL sys_stop_beep

GLOBAL sys_fonts_text_color
GLOBAL sys_fonts_background_color
GLOBAL sys_fonts_decrease_size
GLOBAL sys_fonts_increase_size
GLOBAL sys_fonts_set_size
GLOBAL sys_clear_screen
GLOBAL sys_clear_screen_character

GLOBAL sys_hour
GLOBAL sys_minute
GLOBAL sys_second
GLOBAL sys_sleep_milis

GLOBAL sys_circle
GLOBAL sys_rectangle
GLOBAL sys_fill_video_memory

GLOBAL sys_exec

GLOBAL sys_register_key

GLOBAL sys_window_width
GLOBAL sys_window_height

GLOBAL sys_get_register_snapshot

GLOBAL sys_get_character_without_display

GLOBAL sys_malloc
GLOBAL sys_free

GLOBAL sys_create_process
GLOBAL sys_get_pid
GLOBAL sys_kill_process
GLOBAL sys_modify_priority
GLOBAL sys_ps
GLOBAL sys_block_process
GLOBAL sys_unblock_process
GLOBAL sys_yield
GLOBAL sys_wait_pid
GLOBAL sys_wait_for_children
GLOBAL sys_get_process_info

GLOBAL sys_pipe_open
;GLOBAL sys_pipe_attach

GLOBAL sys_pipe_close
GLOBAL sys_set_read_target
GLOBAL sys_set_write_target

GLOBAL sys_sem_open
GLOBAL sys_sem_close
GLOBAL sys_sem_wait
GLOBAL sys_sem_post

GLOBAL sys_shutdown
GLOBAL sys_get_memory_stats

GLOBAL sys_set_mvar_value
GLOBAL sys_get_mvar_value

; Syscall number definitions for NASM
; Keep synchronized with syscall_numbers.h

; Linux standard syscalls
%define SYS_READ  3
%define SYS_WRITE 4

; Custom syscalls - starting from 10
%define SYS_START_BEEP               10
%define SYS_STOP_BEEP                11
%define SYS_FONTS_TEXT_COLOR         12
%define SYS_FONTS_BACKGROUND_COLOR   13
%define SYS_FONTS_DECREASE_SIZE      14
%define SYS_FONTS_INCREASE_SIZE      15
%define SYS_FONTS_SET_SIZE           16
%define SYS_CLEAR_SCREEN             17
%define SYS_CLEAR_INPUT_BUFFER       18

%define SYS_HOUR                     19
%define SYS_MINUTE                   20
%define SYS_SECOND                   21

%define SYS_CIRCLE                   22
%define SYS_RECTANGLE                23
%define SYS_FILL_VIDEO_MEMORY        24

%define SYS_EXEC                     25

%define SYS_REGISTER_KEY             26

%define SYS_WINDOW_WIDTH             27
%define SYS_WINDOW_HEIGHT            28

%define SYS_SLEEP_MILIS              29

%define SYS_GET_REGISTER_SNAPSHOT    30

%define SYS_GET_CHARACTER_NO_DISPLAY 31

%define SYS_MALLOC                   32
%define SYS_FREE                     33
%define SYS_GET_MEMORY_STATS         34


%define SYS_CREATE_PROCESS           35
%define SYS_GET_PID                  36
%define SYS_KILL_PROCESS             37
%define SYS_MODIFY_PRIORITY          38
%define SYS_PS                       39
%define SYS_BLOCK_PROCESS            40
%define SYS_YIELD                    41
%define SYS_WAIT_PID                 42
%define SYS_WAIT_FOR_CHILDREN        43
%define SYS_GET_PROCESS_INFO         44

%define SYS_PIPE_OPEN                45
%define SYS_PIPE_ATTACH              46
%define SYS_PIPE_CLOSE               47
%define SYS_SET_READ_TARGET          48
%define SYS_SET_WRITE_TARGET         49

%define SYS_SEM_OPEN                 50
%define SYS_SEM_CLOSE                51
%define SYS_SEM_WAIT                 52
%define SYS_SEM_POST                 53

%define SYS_SHUTDOWN                 54

%define SYS_UNBLOCK_PROCESS          55

; Shared MVar value storage (minimal kernel support for userland MVar)
%define SYS_SET_MVAR_VALUE           56
%define SYS_GET_MVAR_VALUE           57

section .text

%macro sys_int80 1
    push rbp
    mov rbp, rsp
    mov rax, %1
    int 0x80
    mov rsp, rbp
    pop rbp
    ret
%endmacro

sys_start_beep: sys_int80 SYS_START_BEEP
sys_stop_beep: sys_int80 SYS_STOP_BEEP

sys_fonts_text_color: sys_int80 SYS_FONTS_TEXT_COLOR
sys_fonts_background_color: sys_int80 SYS_FONTS_BACKGROUND_COLOR

sys_fonts_decrease_size: sys_int80 SYS_FONTS_DECREASE_SIZE
sys_fonts_increase_size: sys_int80 SYS_FONTS_INCREASE_SIZE
sys_fonts_set_size: sys_int80 SYS_FONTS_SET_SIZE
sys_clear_screen: sys_int80 SYS_CLEAR_SCREEN
sys_clear_screen_character: sys_int80 SYS_CLEAR_INPUT_BUFFER

sys_hour: sys_int80 SYS_HOUR
sys_minute: sys_int80 SYS_MINUTE
sys_second: sys_int80 SYS_SECOND

sys_circle: sys_int80 SYS_CIRCLE
sys_rectangle: sys_int80 SYS_RECTANGLE
sys_fill_video_memory: sys_int80 SYS_FILL_VIDEO_MEMORY

sys_exec: sys_int80 SYS_EXEC

sys_register_key: sys_int80 SYS_REGISTER_KEY

sys_window_width: sys_int80 SYS_WINDOW_WIDTH
sys_window_height: sys_int80 SYS_WINDOW_HEIGHT

sys_sleep_milis: sys_int80 SYS_SLEEP_MILIS

sys_get_register_snapshot: sys_int80 SYS_GET_REGISTER_SNAPSHOT

sys_get_character_without_display: sys_int80 SYS_GET_CHARACTER_NO_DISPLAY

sys_malloc: sys_int80 SYS_MALLOC
sys_free: sys_int80 SYS_FREE

sys_create_process: sys_int80 SYS_CREATE_PROCESS
sys_get_pid: sys_int80 SYS_GET_PID
sys_kill_process: sys_int80 SYS_KILL_PROCESS
sys_modify_priority: sys_int80 SYS_MODIFY_PRIORITY
sys_ps: sys_int80 SYS_PS
sys_block_process: sys_int80 SYS_BLOCK_PROCESS
sys_unblock_process: sys_int80 SYS_UNBLOCK_PROCESS
sys_yield: sys_int80 SYS_YIELD
sys_wait_pid: sys_int80 SYS_WAIT_PID
sys_wait_for_children: sys_int80 SYS_WAIT_FOR_CHILDREN
sys_pipe_open:        sys_int80 SYS_PIPE_OPEN
sys_pipe_close:       sys_int80 SYS_PIPE_CLOSE
sys_set_read_target:  sys_int80 SYS_SET_READ_TARGET
sys_set_write_target: sys_int80 SYS_SET_WRITE_TARGET
sys_sem_open:         sys_int80 SYS_SEM_OPEN
sys_sem_close:        sys_int80 SYS_SEM_CLOSE
sys_sem_wait:         sys_int80 SYS_SEM_WAIT
sys_sem_post:         sys_int80 SYS_SEM_POST
sys_get_process_info: sys_int80 SYS_GET_PROCESS_INFO
sys_shutdown:         sys_int80 SYS_SHUTDOWN

sys_get_memory_stats: sys_int80 SYS_GET_MEMORY_STATS

sys_set_mvar_value: sys_int80 SYS_SET_MVAR_VALUE
sys_get_mvar_value: sys_int80 SYS_GET_MVAR_VALUE