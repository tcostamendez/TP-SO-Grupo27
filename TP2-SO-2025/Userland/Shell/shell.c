#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exceptions.h>
#include <sys.h>

#ifdef ANSI_4_BIT_COLOR_SUPPORT
#include <ansiColors.h>
#endif

static void *const snakeModuleAddress = (void *)0x500000;

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);
static int deleteCharacter(enum REGISTERABLE_KEYS scancode, int Ctrl, int Alt, int Shift);
static void emptyScreenBuffer(void);

static uint8_t last_command_arrowed = 0;

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {{.name = "block",
  .function = (int (*)(int, char **))(uint64_t)_block,
  .description = "Blocks/unblocks the process with the provided PID",
  .isBuiltIn = 0},
 {.name = "cat",
  .function = (int (*)(int, char **))(uint64_t)_cat,
  .description = "Prints stdin as it's received",
  .isBuiltIn = 0},
 {.name = "clear",
  .function = (int (*)(int, char **))(uint64_t)_clear,
  .description = "Clears the screen",
  .isBuiltIn = 0},
 {.name = "divzero",
  .function = (int (*)(int, char **))(uint64_t)_divzero,
  .description = "Generates a division by zero exception",
  .isBuiltIn = 0},
 {.name = "echo",
  .function = (int (*)(int, char **))(uint64_t)_echo,
  .description = "Prints the input string",
  .isBuiltIn = 0},
 {.name = "exit",
  .function = (int (*)(int, char **))(uint64_t)exit,
  .description = "Kills the current shell",
  .isBuiltIn = 1},
 {.name = "filter",
  .function = (int (*)(int, char **))(uint64_t)_filter,
  .description = "Prints input filtering vocals",
  .isBuiltIn = 0},
 {.name = "font",
  .function = (int (*)(int, char **))(uint64_t)_font,
  .description = "Increases or decreases the font size\n\t\t\t\tUse:\n\t\t\t\t\t  + "
         "font increase\n\t\t\t\t\t  + font decrease",
  .isBuiltIn = 0},
 {.name = "getpid",
  .function = (int (*)(int, char **))(uint64_t)getPid,
  .description = "Prints the shell's process ID",
  .isBuiltIn = 1},
 {.name = "help",
  .function = (int (*)(int, char **))(uint64_t)_help,
  .description = "Prints the available commands",
  .isBuiltIn = 0},
 {.name = "history",                                                  // ?????????????????
  .function = (int (*)(int, char **))(uint64_t)history,               // ?????????????????
  .description = "Prints the command history",                       // ?????????????????
  .isBuiltIn = 1},
 {.name = "invop",
  .function = (int (*)(int, char **))(uint64_t)_invalidopcode,
  .description = "Generates an invalid Opcode exception",
  .isBuiltIn = 0},
 {.name = "kill",
  .function = (int (*)(int, char **))(uint64_t)_kill,
  .description = "Kills the process with the provided PID",
  .isBuiltIn = 0},
 {.name = "loop",
  .function = (int (*)(int, char **))(uint64_t)_loop,
  .description = "Loops every <milliseconds>",
  .isBuiltIn = 0},
 {.name = "man",
  .function = (int (*)(int, char **))(uint64_t)_man,
  .description = "Prints the description of the provided command",
  .isBuiltIn = 0},
 {.name = "mem",
  .function = (int (*)(int, char **))(uint64_t)_mem,
  .description = "Prints the memory status of the system",
  .isBuiltIn = 0},
 {.name = "mvar",
  .function = (int (*)(int, char **))(uint64_t)_mvar,
  .description = "Simulates the classic readers-writers problem",
  .isBuiltIn = 0},
 {.name = "nice",
  .function = (int (*)(int, char **))(uint64_t)_nice,
  .description = "Changes the priority of <pid> by <amount>",
  .isBuiltIn = 0},
 {.name = "orphan",
  .function = (int (*)(int, char **))(uint64_t)_orphan,
  .description = "Spawns a child and does not wait for it",
  .isBuiltIn = 0},
 {.name = "ps",
  .function = (int (*)(int, char **))(uint64_t)_ps,
  .description = "Lists processes' info",
  .isBuiltIn = 0},
 {.name = "regs",
  .function = (int (*)(int, char **))(uint64_t)_regs,
  .description = "Prints the register snapshot, if any",
  .isBuiltIn = 0},
 {.name = "sh",
  .function = (int (*)(int, char **))(uint64_t)main,
  .description = "Creates a new shell process",
  .isBuiltIn = 0},
 {.name = "snake",
  .function = (int (*)(int, char **))(uint64_t)_snake,
  .description = "Launches the snake game",
  .isBuiltIn = 0},
 {.name = "spawn",
  .function = (int (*)(int, char **))(uint64_t)_spawn,
  .description = "Spawns a new process and waits for the return",
  .isBuiltIn = 0},
 {.name = "test_mm",
  .function = (int (*)(int, char **))(uint64_t)_test_mm,
  .description = "Allocates and frees <amount> blocks of memory",
  .isBuiltIn = 0},
 {.name = "test_prio",
  .function = (int (*)(int, char **))(uint64_t)_test_prio,
  .description = "Creates processes with different priorities",
  .isBuiltIn = 0},
 {.name = "test_proc",
  .function = (int (*)(int, char **))(uint64_t)_test_processes,
  .description = "Creates and kills processes randomly",
  .isBuiltIn = 0},
 {.name = "test_sync",
  .function = (int (*)(int, char **))(uint64_t)_test_sync,
  .description = "Increments/decrements <n> times <using_sem>",
  .isBuiltIn = 0},
 {.name = "time",
  .function = (int (*)(int, char **))(uint64_t)_time,
  .description = "Prints the current time",
  .isBuiltIn = 0},
 {.name = "unblock",
  .function = (int (*)(int, char **))(uint64_t)_ublock,
  .description = "Unblocks the process with the provided PID",
  .isBuiltIn = 0},
 {.name = "wc",
  .function = (int (*)(int, char **))(uint64_t)_wc,
  .description = "Counts inputs's total lines",
  .isBuiltIn = 0}};

const int commands_size = sizeof(commands) / sizeof(Command);

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;


static int parseCommand(char *buffer, char *command, ParsedCommand *parsedCommand) {
	int command_index = -1;
	strcpy(command, strtok(buffer, " "));
	if (*command == '\0') {
		return -1;	// Empty command
	}

	for (int i = 0; i < commands_size; i++) {
		if (strcmp(commands[i].name, command) == 0) {
			command_index = i;
			break;
		}
	}

	if (command_index == -1) {
		return -1;
	}

	char *arg = strtok(NULL, " ");

	parsedCommand->name = malloc(strlen(command) + 1);
	if (parsedCommand->name == NULL) {
		perror("Memory allocation failed for command name");
		return -1;
	}
	strcpy(parsedCommand->name, commands[command_index].name);
	parsedCommand->function = commands[command_index].function;
	parsedCommand->argv[0] = parsedCommand->name;
	parsedCommand->argc = 1;

	while (arg != NULL && strcasecmp(arg, "|") && parsedCommand->argc < MAX_ARGUMENT_COUNT - 1) {
		parsedCommand->argv[parsedCommand->argc] = malloc(strlen(arg) + 1);
		if (parsedCommand->argv[parsedCommand->argc] == NULL) {
			perror("Memory allocation failed for command argument");
			free(parsedCommand->name);
			for (int i = 0; i < parsedCommand->argc; i++) {
				free(parsedCommand->argv[i]);
			}
			return -1;
		}
		strcpy(parsedCommand->argv[parsedCommand->argc], arg);
		arg = strtok(NULL, " ");
		parsedCommand->argc++;
	}

	parsedCommand->argv[parsedCommand->argc] = NULL;

	parsedCommand->isBuiltIn = commands[command_index].isBuiltIn;

	return 1;
}


int main() {
  clear();
  registeryKey(BACKSPACE_KEY, deleteCharacter);
  registerKey(KP_UP_KEY, printPreviousCommand);
  registerKey(KP_DOWN_KEY, printNextCommand);

  buffer[0] = 0;
	buffer_dim = 0;

	int firstPid = -1;
	int requestsForeground = 0;
	int firstCommandRequestedForeground = 0;

	char command[MAX_ARGUMENT_SIZE] = {0};
	uint16_t parsed_commands_dim = 5;
	ParsedCommand *parsedCommands = malloc(sizeof(ParsedCommand) * parsed_commands_dim);

  if (parsedCommands == NULL) {
    perror("Memory allocation failed for parsed commands");
    return -1;
  }

  while (1) {
    printf("\e[0mshell \e[0;32m$\e[0m ");

    signed char c;

    while (buffer_dim < MAX_BUFFER_SIZE && (c = getchar()) != '\n') {
      command_history_buffer[buffer_dim] = c;
      buffer[buffer_dim++] = c;
    }

    putchar(c);
  
    buffer[buffer_dim] = 0;
    command_history_buffer[buffer_dim] = 0;

    if (buffer_dim == MAX_BUFFER_SIZE) {
      perror("\e[0;31mShell buffer overflow\e[0m\n");
      buffer[0] = buffer_dim = 0;
      while (c != '\n')
        c = getchar();
      continue;
    };

    int parsed = 0;
    firstCommandRequestedForeground = 0;

    while (parseCommand(parsed == 0 ? buffer : NULL, command, &parsedCommands[parsed]) != -1) {
			if ((++parsed % 5 == 0) && parsed >= parsed_commands_dim) {
				ParsedCommand *newParsedCommands = malloc(sizeof(ParsedCommand) * (parsed_commands_dim + 5));
				if (newParsedCommands == NULL) {
					perror("Memory allocation failed for parsed commands");
					free(parsedCommands);
					return -1;
				}
				memcpy(newParsedCommands, parsedCommands, sizeof(ParsedCommand) * (parsed_commands_dim));
				free(parsedCommands);
				parsed_commands_dim += 5;
				parsedCommands = newParsedCommands;
			};
		}

    if (parsed != 0) {
			strncpy(command_history[command_history_last], command_history_buffer, 255);
			command_history[command_history_last][buffer_dim] = '\0';
			INC_MOD(command_history_last, HISTORY_SIZE);
			last_command_arrowed = command_history_last;
		}

    int pipes[parsed + 1];

    for (int p = 0; p < parsed; p++) {
			if (parsedCommands[p].isBuiltIn) {
				parsedCommands[p].function(parsedCommands[p].argc, parsedCommands[p].argv);
			} else {
				if (parsed > 1 && p != parsed - 1) {
					pipes[p] = openPipe();
					if (pipes[p] <= 0) {
						fprintf(FD_STDOUT, "\e[0;31mError opening pipe\e[0m\n");
						break;
					}
				}

				int targets[2];
				targets[READ_TARGET] = p == 0 ? PIPE_STDIN : pipes[p - 1];
				targets[WRITE_TARGET] = (p == parsed - 1) ? PIPE_STDOUT : pipes[p];

				char *lastArg = ((char **)parsedCommands[p].argv)[parsedCommands[p].argc - 1];
				int wantsBackground = !strcmp(lastArg, "&");

				if (p == 0) {
					requestsForeground = !wantsBackground;
				} else {
					requestsForeground = 0;
				}

				if (wantsBackground) {
					parsedCommands[p].argc--;
				}

				if (requestsForeground) {
					firstCommandRequestedForeground = 1;
				}

				int newPid = proc((void *)parsedCommands[p].function, parsedCommands[p].argc,
								  (char **)parsedCommands[p].argv, 1, targets, requestsForeground);

				if (p == 0) {
					if (newPid > 0) {
						firstPid = newPid;
					}
				}
			}
		}

    // If the command is not found, ignore \n
		if (firstPid <= 0 && parsed <= 0 && buffer_dim > 0) {
			fprintf(FD_STDOUT, "\e[0;33mCommand not found:\e[0m %s\n", command);
		} else if (firstCommandRequestedForeground) {
			waitpid(firstPid);
		}

    buffer[0] = buffer_dim = 0;
  }

  __builtin_unreachable();
  return 0;
}

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode) {
  last_command_arrowed = SUB_MOD(last_command_arrowed, 1, HISTORY_SIZE);
	int currentCommandLenght = strlen(command_history[last_command_arrowed]);
	emptyScreenBuffer();
	if (command_history[last_command_arrowed][0] != 0) {
		strcpy(buffer, command_history[last_command_arrowed]);
		buffer_dim = currentCommandLenght;
		fprintf(FD_STDOUT, "%s", command_history[last_command_arrowed]);
	}
	return 1;
}

static void printNextCommand(enum REGISTERABLE_KEYS scancode) {
  last_command_arrowed = (last_command_arrowed + 1) % HISTORY_SIZE;
	int currentCommandLenght = strlen(command_history[last_command_arrowed]);
	emptyScreenBuffer();
	if (command_history[last_command_arrowed][0] != 0) {
		strcpy(buffer, command_history[last_command_arrowed]);
		buffer_dim = currentCommandLenght;
		fprintf(FD_STDOUT, "%s", command_history[last_command_arrowed]);
	}
	return 1;
}

static int deleteCharacter(enum REGISTERABLE_KEYS scancode, int Ctrl, int Alt, int Shift) {
	if (buffer_dim > 0) {
		clearScreenCharacter();
		buffer_dim--;
	}
	return 1;
}

static void emptyScreenBuffer(void) {
	while (buffer_dim > 0) {
		clearScreenCharacter();
		buffer_dim--;
	}
}


int history(nt argc, char **argv) {
  if (argc > 1) {
		perror("Invalid amount of aruments\n");
		return 1;
	}

	uint8_t last = command_history_last;
	DEC_MOD(last, HISTORY_SIZE);
	uint8_t i = 0;
	while (i < HISTORY_SIZE && command_history[last][0] != 0) {
		printf("%d. %s\n", i, command_history[last]);
		DEC_MOD(last, HISTORY_SIZE);
		i++;
	}
	return 0;
}

int exit(int argc, char **argv) {
	if (argc > 1) {
		perror("Invalid amount of aruments\n");
		return 1;
	}

	int ret = kill(getpid());
	if (ret == -1) {
		perror("Error killing process\n");
		return 1;
	}
	return 0;
}

int getPid(int argc, char **argv) {
	if (argc > 1) {
		perror("Invalid amount of aruments\n");
		return 1;
	}

	int pid = getpid();
	if (pid < 0) {
		perror("Error getting pid\n");
		return 1;
	}
	printf("%d\n", pid);
	return 0;
}