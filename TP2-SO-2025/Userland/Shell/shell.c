#include "shell.h"
#include "./commands/commands.h"
#include "./commands/entry_points.h"
#include "libsys.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "test_util.h"
#include <exceptions.h>
#include <stddef.h>

#ifdef ANSI_4_BIT_COLOR_SUPPORT
#include <ansiColors.h>
#endif

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

static int printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static int printNextCommand(enum REGISTERABLE_KEYS scancode);
static int deleteCharacter(enum REGISTERABLE_KEYS scancode);
static void emptyScreenBuffer(void);

static uint8_t last_command_arrowed = 0;

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {
    {.name = "block",
     .function = (int (*)(int, char **))entry_block,
     .description = "Blocks or unblocks a process",
     .isBuiltIn = 0},
    {.name = "cat",
     .function = (int (*)(int, char **))entry_cat,
     .description = "Prints the content of the file",
     .isBuiltIn = 0},
    {.name = "clear",
     .function = (int (*)(int, char **))entry_clear,
     .description = "Clears the screen",
     .isBuiltIn = 0},
    {.name = "echo",
     .function = (int (*)(int, char **))entry_echo,
     .description = "Prints the input string",
     .isBuiltIn = 0},
    {.name = "exit",
     .function = (int (*)(int, char **))_exit,
     .description = "Closes qemu window",
     .isBuiltIn = 1},
    {.name = "help",
     .function = (int (*)(int, char **))entry_help,
     .description = "Prints the available commands",
     .isBuiltIn = 0},
    {.name = "man",
     .function = (int (*)(int, char **))_man,
     .description = "Prints the description of the provided command",
     .isBuiltIn = 1},
    // {.name = "testmm",
    //  .function = (int (*)(int, char**))run_mm_test,
    //  .description = "Corre el test de stress del Memory Manager.\n\t\tUso:
    //  testmm <bytes>", .isBuiltIn=0},
    {.name = "ps",
     .function = (int (*)(int, char **))entry_ps,
     .description = "Lists all processes",
     .isBuiltIn = 0},
    {.name = "loop",
     .function = (int (*)(int, char **))entry_loop,
     .description =
         "Prints hello message every N seconds\n\t\tUso: loop <delay_seconds>",
     .isBuiltIn = 0},
    {.name = "kill",
     .function = (int (*)(int, char **))entry_kill,
     .description = "Kills a process by PID\n\t\tUso: kill <pid>",
     .isBuiltIn = 0},
    {.name = "nice",
     .function = (int (*)(int, char **))entry_nice,
     .description = "Changes process priority\n\t\tUso: nice <pid> <priority>",
     .isBuiltIn = 0},
    {.name = "mem",
     .function = (int (*)(int, char **))entry_mem,
     .description = "Shows memory status",
     .isBuiltIn = 0},
    {.name = "wc",
     .function = (int (*)(int, char **))(uint64_t)entry_wc,
     .description = "Counts inputs's total lines",
     .isBuiltIn = 0},
    {.name = "filter",
     .function = (int (*)(int, char **))entry_filter,
     .description = "Removes all vowels from input string",
     .isBuiltIn = 0}};

const int commands_size = sizeof(commands) / sizeof(Command);

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint64_t last_command_output = 0;

static int parse_command(char *buffer, char *command, ParsedCommand *parsedCommand) {
  int command_i = -1;
  char *tok = strtok(buffer, " ");
  if (tok == NULL) {
    return -1;
  }

  // Salteo pipes iniciales (si vengo de un parse anterior quedó parado en '|')
  while (tok && strcmp(tok, "|") == 0) {
    tok = strtok(NULL, " ");
  }
  if (tok == NULL) {
    return -1;
  }

  strcpy(command, tok);
  if (*command == '\0') {
    return -1;
  }
  for (int i = 0; i < commands_size; i++) {
    if (strcmp(commands[i].name, command) == 0) {
      command_i = i;
      break;
    }
  }

  if (command_i == -1) {
    return -1;
  }

  char *arg = strtok(NULL, " ");

  parsedCommand->name = commands[command_i].name;
  parsedCommand->function = commands[command_i].function;
  parsedCommand->argv[0] = commands[command_i].name;
  parsedCommand->argc = 1;

  while (arg != NULL && strcasecmp(arg, "|") != 0 &&
         parsedCommand->argc < MAX_ARGUMENT_COUNT - 1) {
    parsedCommand->argv[parsedCommand->argc] = allocMemory(strlen(arg) + 1);

    if (parsedCommand->argv[parsedCommand->argc] == NULL) {
      perror("Memory allocation failed for command argument");
      for (int i = 1; i < parsedCommand->argc; i++) {
        freeMemory(parsedCommand->argv[i]);
      }
      return -1;
    }

    strcpy(parsedCommand->argv[parsedCommand->argc], arg);
    arg = strtok(NULL, " ");
    parsedCommand->argc++;
  }

  // No avanzar un token extra: dejar strtok posicionado justo después de '|'
  parsedCommand->argv[parsedCommand->argc] = NULL;

  parsedCommand->isBuiltIn = commands[command_i].isBuiltIn;

  return 1;
}

int main() {
  clearScreen();

  registerKey(KP_UP_KEY, printPreviousCommand);
  registerKey(KP_DOWN_KEY, printNextCommand);
  registerKey(BACKSPACE_KEY, deleteCharacter);

  buffer[0] = 0;
  buffer_dim = 0;
  int firstPid = -1;
  int requestsFg = 0;
  int firstCommandRequestedFg = 0;

  char command[MAX_ARGUMENT_SIZE] = {0};
  uint16_t parsed_commands_dim = 5;
  ParsedCommand *parsedCommands =
      allocMemory(sizeof(ParsedCommand) * parsed_commands_dim);

  if (parsedCommands == NULL) {
    perror("Memory allocation failed for parsed commands");
    return -1;
  }

  while (1) {
    printf("\e[0mshell (%d) \e[0;32m$\e[0m ", getMyPid());

    signed char c;

    while (buffer_dim < MAX_BUFFER_SIZE && (c = getchar()) != '\n') {
      command_history_buffer[buffer_dim] = c;
      buffer[buffer_dim++] = c;
    }

    buffer[buffer_dim] = 0;

    if (buffer_dim == MAX_BUFFER_SIZE) {
      perror("\e[0;31mShell buffer overflow\e[0m\n");
      buffer[0] = buffer_dim = 0;
      while (c != '\n')
        c = getchar();
      continue;
    };

    int parsed = 0;
    firstCommandRequestedFg = 0;

    while (parse_command(parsed == 0 ? buffer : NULL, command,
                         &parsedCommands[parsed]) != -1) {
      if ((++parsed % 5 == 0) && parsed >= parsed_commands_dim) {
        ParsedCommand *newParsedCommands =
            allocMemory(sizeof(ParsedCommand) * (parsed_commands_dim + 5));
        if (newParsedCommands == NULL) {
          perror("Memory allocation failed for parsed commands");
          for (int p = 0; p < parsed; p++) {
            for (int i = 1; i < parsedCommands[p].argc; i++) {
              if (parsedCommands[p].argv[i])
                freeMemory(parsedCommands[p].argv[i]);
            }
          }
          freeMemory(parsedCommands);
          return -1;
        }
        memcpy(newParsedCommands, parsedCommands,
               sizeof(ParsedCommand) * (parsed_commands_dim));
        freeMemory(parsedCommands);
        parsed_commands_dim += 5;
        parsedCommands = newParsedCommands;
      }
    }

    // Check if user entered something but it wasn't a valid command
    if (parsed == 0 && buffer_dim > 0) {
      // Extract the command name that was tried
      char invalid_cmd[MAX_ARGUMENT_SIZE] = {0};
      char *cmd_tok = strtok(buffer, " ");
      if (cmd_tok != NULL) {
        strcpy(invalid_cmd, cmd_tok);
        printf("Command '%s' not found\n", invalid_cmd);
      }
      printf("For a list of valid commands type 'help'\n");
    }

    if (parsed != 0) {
      strncpy(command_history[command_history_last], command_history_buffer,
              255);
      command_history[command_history_last][buffer_dim] = '\0';
      INC_MOD(command_history_last, HISTORY_SIZE);
      last_command_arrowed = command_history_last;
    }

    if (parsed >= 1) {
      // Nueva lógica: sólo soportar comandos simples o pipeline de EXACTAMENTE dos procesos
      if (parsed == 1) {
        // Comando único
        if (parsedCommands[0].isBuiltIn) {
          parsedCommands[0].function(parsedCommands[0].argc, parsedCommands[0].argv);
        } else {
          // Soporte '&' para background en comando único
          int wantsBackground = 0;
          if (parsedCommands[0].argc > 1) {
            char *lastArg = parsedCommands[0].argv[parsedCommands[0].argc - 1];
            if (lastArg && strcmp(lastArg, "&") == 0) {
              wantsBackground = 1;
              parsedCommands[0].argv[parsedCommands[0].argc - 1] = NULL;
              parsedCommands[0].argc--;
            }
          }
          int targets[3] = {0, 1, 2};
          void (*entry_point)(int, char **) = (void (*)(int, char **))parsedCommands[0].function;
          int pid = createProcess(parsedCommands[0].argc, (char **)parsedCommands[0].argv,
                                  (void *)entry_point, 2, targets, !wantsBackground);
          if (!wantsBackground && pid > 0) {
            waitPid(pid);
          }
        }
      } else if (parsed == 2) {
        // Pipeline de dos comandos
        if (parsedCommands[0].isBuiltIn || parsedCommands[1].isBuiltIn) {
          printf("Pipelines with built-in commands are not supported.\n");
        } else {
          int pipeId = pipeOpen();
          if (pipeId < 0) {
            perror("Failed to open pipe");
          } else {
            // No soportamos '&' dentro de pipelines por simplicidad: se ignora si aparece
            for (int k = 0; k < 2; k++) {
              if (parsedCommands[k].argc > 1) {
                char *lastArg = parsedCommands[k].argv[parsedCommands[k].argc - 1];
                if (lastArg && strcmp(lastArg, "&") == 0) {
                  parsedCommands[k].argv[parsedCommands[k].argc - 1] = NULL;
                  parsedCommands[k].argc--;
                }
              }
            }
            int targetsLeft[3] = {0, pipeId, 2};    // help: STDOUT -> pipe
            int targetsRight[3] = {pipeId, 1, 2};    // filter: STDIN <- pipe
            void (*entry_left)(int, char **) = (void (*)(int, char **))parsedCommands[0].function;
            void (*entry_right)(int, char **) = (void (*)(int, char **))parsedCommands[1].function;
            int pidLeft = createProcess(parsedCommands[0].argc, (char **)parsedCommands[0].argv,
                                        (void *)entry_left, 2, targetsLeft, 0); 
            int pidRight = createProcess(parsedCommands[1].argc, (char **)parsedCommands[1].argv,
                                         (void *)entry_right, 2, targetsRight, 1);
            // Esperar ambos para mostrar salida filtrada completa antes de nuevo prompt
            if (pidLeft > 0) waitPid(pidLeft);
            if (pidRight > 0) waitPid(pidRight);
            pipeClose(pipeId);
          }
        }
      } else { // parsed > 2
        printf("Pipelines with more than 2 commands are not supported.\n");
      }
    }
    // Liberar argv alocados (argv[0] es literal de commands[])
    for (int p = 0; p < parsed; p++) {
      for (int i = 1; i < parsedCommands[p].argc; i++) {
        if (parsedCommands[p].argv[i]) {
          freeMemory(parsedCommands[p].argv[i]);
          parsedCommands[p].argv[i] = NULL;
        }
      }
      parsedCommands[p].argc = 1;
      parsedCommands[p].argv[1] = NULL;
    }
  
    buffer_dim = 0;
    buffer[0] = 0;
  }

  __builtin_unreachable();
  return 0;
}

static int printPreviousCommand(enum REGISTERABLE_KEYS scancode) {

  last_command_arrowed = SUB_MOD(last_command_arrowed, 1, HISTORY_SIZE);
  if (command_history[last_command_arrowed][0] != 0) {
    printf("%s", command_history[last_command_arrowed]);
  }
  return 1;
}

static int printNextCommand(enum REGISTERABLE_KEYS scancode) {

  last_command_arrowed = (last_command_arrowed + 1) % HISTORY_SIZE;
  if (command_history[last_command_arrowed][0] != 0) {
    printf("%s", command_history[last_command_arrowed]);
  }
  return 1;
}

static int deleteCharacter(enum REGISTERABLE_KEYS scancode) {
  if (buffer_dim > 0) {
    // clearScreenCharacter();
    buffer_dim--;
  }
  return 1;
}

static void emptyScreenBuffer(void) {
  while (buffer_dim > 0) {
    // clearScreenCharacter();
    buffer_dim--;
  }
}

int history(void) {
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
