#ifndef SYSCALL_NUMBERS_H
#define SYSCALL_NUMBERS_H

// Linux standard syscalls
#define SYS_READ  3
#define SYS_WRITE 4

// Custom syscalls - starting from 10
#define SYS_START_BEEP               10
#define SYS_STOP_BEEP                11
#define SYS_FONTS_TEXT_COLOR         12
#define SYS_FONTS_BACKGROUND_COLOR   13
#define SYS_FONTS_DECREASE_SIZE      14
#define SYS_FONTS_INCREASE_SIZE      15
#define SYS_FONTS_SET_SIZE           16
#define SYS_CLEAR_SCREEN             17
#define SYS_CLEAR_INPUT_BUFFER       18

#define SYS_HOUR                     19
#define SYS_MINUTE                   20
#define SYS_SECOND                   21

#define SYS_CIRCLE                   22
#define SYS_RECTANGLE                23
#define SYS_FILL_VIDEO_MEMORY        24

#define SYS_EXEC                     25

#define SYS_REGISTER_KEY             26

#define SYS_WINDOW_WIDTH             27
#define SYS_WINDOW_HEIGHT            28

#define SYS_SLEEP_MILIS              29

#define SYS_GET_REGISTER_SNAPSHOT    30

#define SYS_GET_CHARACTER_NO_DISPLAY 31

#define SYS_MALLOC                   32
#define SYS_FREE                     33

#define SYS_CREATE_PROCESS           34
#define SYS_GET_PID                  35
#define SYS_KILL_PROCESS             36
#define SYS_MODIFY_PRIORITY          37
#define SYS_LIST_PROCESSES           38
#define SYS_BLOCK_PROCESS            39
#define SYS_YIELD                    40
#define SYS_WAIT_PID                 41
#define SYS_WAIT_FOR_CHILDREN        42
#define SYS_GET_PROCESS_INFO         43

#define SYS_PIPE_OPEN                44
#define SYS_PIPE_ATTACH              45
#define SYS_PIPE_CLOSE               46
#define SYS_SET_READ_TARGET          47
#define SYS_SET_WRITE_TARGET         48

#define SYS_SEM_OPEN                 49
#define SYS_SEM_CLOSE                50
#define SYS_SEM_WAIT                 51
#define SYS_SEM_POST                 52

#define SYS_SHUTDOWN                 53

#endif

