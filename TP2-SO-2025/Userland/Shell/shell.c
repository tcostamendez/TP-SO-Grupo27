#include <stddef.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <exceptions.h>
#include "libsys.h"
#include "test_util.h"
#include "shell.h"
#include "./commands/commands.h"

#ifdef ANSI_4_BIT_COLOR_SUPPORT
#include <ansiColors.h>
#endif

static void *const snakeModuleAddress = (void *)0x500000;

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

static int printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static int printNextCommand(enum REGISTERABLE_KEYS scancode);
static int deleteCharacter(enum REGISTERABLE_KEYS scancode);
static void emptyScreenBuffer(void);

static uint8_t last_command_arrowed = 0;

/*
//wrapper de test_mm
int run_mm_test(void) {
  // 1. Parseamos el siguiente token, que debería ser la cantidad de memoria
  char *max_mem_str = strtok(NULL, " ");

  if (max_mem_str == NULL) {
    printf("Error: Faltó especificar la memoria máxima.\n");
    printf("Uso: testmm <bytes>\n");
    return 1; // Devolvemos un código de error
  }

  // 2. "Falsificamos" argc y argv para pasárselos a test_mm
  char *fake_argv[] = { max_mem_str };
  uint64_t fake_argc = 1;

  // 3. Llamamos a la función de test original
  printf("Iniciando test de memoria con %s bytes...\n", max_mem_str);
  uint64_t result = test_mm(fake_argc, fake_argv);

  if (result == 0) {
      printf("Test de memoria finalizado con éxito.\n");
  } else {
      printf("Test de memoria falló!\n");
  }

  return result;
}
*/

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {
    {.name = "clear",
     .function = (int (*)(int, char**))(unsigned long long)_clear,
     .description = "Clears the screen",
     .isBuiltIn = 1},
    {.name = "echo",
     .function = (int (*)(int, char**))(unsigned long long)_echo,
     .description = "Prints the input string",
     .isBuiltIn=1},
    {.name = "exit",
     .function = (int (*)(int, char**))(unsigned long long)_exit,
     .description = "Command closes qemu",
     .isBuiltIn = 1},
    {.name = "help",
     .function = (int (*)(int, char**))(unsigned long long)_help,
     .description = "Prints the available commands",
     .isBuiltIn = 0},
    {.name = "man",
     .function = (int (*)(int, char**))(unsigned long long)_man,
     .description = "Prints the description of the provided command",
     .isBuiltIn=1},
    /*{.name = "testmm",
     .function = (int (*)(int, char**))(unsigned long long)run_mm_test,
     .description = "Corre el test de stress del Memory Manager.\n\t\t\t\tUso: testmm <bytes>",
     .isBuiltIn=1}, */
    {.name = "ps",
     .function = (int (*)(int, char**))(unsigned long long)_ps,
     .description = "Lists all processes",
     .isBuiltIn=1},
    {.name = "loop",
     .function = (int (*)(int, char**))(unsigned long long)_loop,
     .description = "Prints hello message every N seconds\n\t\t\t\tUso: loop <delay_seconds>",
     .isBuiltIn=1},
    {.name = "kill",
     .function = (int (*)(int, char**))(unsigned long long)_kill,
     .description = "Kills a process by PID\n\t\t\t\tUso: kill <pid>",
     .isBuiltIn=1},
    {.name = "nice",
     .function = (int (*)(int, char**))(unsigned long long)_nice,
     .description = "Changes process priority\n\t\t\t\tUso: nice <pid> <priority>",
     .isBuiltIn=1},
    {.name = "block",
     .function = (int (*)(int, char**))(unsigned long long)_block,
     .description = "Blocks or unblocks a process\n\t\t\t\tUso: block <pid>",
     .isBuiltIn=1},
    {.name = "mem",
     .function = (int (*)(int, char**))(unsigned long long)_mem,
     .description = "Shows memory status",
     .isBuiltIn=1},
     {.name = "wc",
      .function = (int (*)(int, char **))(uint64_t)_wc,
      .description = "Counts inputs's total lines",
      .isBuiltIn = 0}
};

const int commands_size = sizeof(commands) / sizeof(Command);

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint64_t last_command_output = 0;


static int parse_command(char *buffer, char *command, ParsedCommand *parsedCommand){
  int command_i = -1;
  strcpy(command, strtok(buffer," "));
  if(*command=='\0'){
    return -1;
  }
  for (int i=0; i<commands_size; i++){
    if(strcmp(commands[i].name, command)==0){
      command_i=i;
      break;
    }
  }

  if(command_i==-1){
    return -1;
  }

  char* arg= strtok(NULL, " ");

  parsedCommand->name=commands[command_i].name;
  parsedCommand->function=commands[command_i].function;
  parsedCommand->argv[0]= commands[command_i].name;
  parsedCommand->argc=1;

  while(arg!=NULL && strcasecmp(arg, "|") && parsedCommand->argc < MAX_ARGUMENT_COUNT - 1) {
    parsedCommand->argv[parsedCommand->argc] = allocMemory(strlen(arg)+1);
    
    if (parsedCommand->argv[parsedCommand->argc] == NULL) {
      perror("Memory allocation failed for command argument");
			freeMemory(parsedCommand->name);

			for (int i = 0; i < parsedCommand->argc; i++) {
				freeMemory(parsedCommand->argv[i]);
			}
			return -1;
    }

    strcpy(parsedCommand->argv[parsedCommand->argc],arg);
    arg = strtok(NULL, " ");
    parsedCommand->argc++;
  }

  parsedCommand->argv[parsedCommand->argc] = NULL;

	parsedCommand->isBuiltIn = commands[command_i].isBuiltIn;

	return 1;  
}


int main() {
  clearScreen();

  // registerKey(KP_UP_KEY, printPreviousCommand);
  // registerKey(KP_DOWN_KEY, printNextCommand);
  // registerKey(BACKSPACE_KEY, deleteCharacter);
  
  buffer[0] = 0;
  buffer_dim = 0;
  int firstPid= -1;
  int requestsFg = 0;
	int firstCommandRequestedFg = 0;

  char command[MAX_ARGUMENT_SIZE]={0};
  uint16_t parsed_commands_dim = 5;
  ParsedCommand * parsedCommands= allocMemory(sizeof(ParsedCommand)*parsed_commands_dim);

  if (parsedCommands == NULL) {
		perror("Memory allocation failed for parsed commands");
		return -1;
	}

  while(1){
    printf("\e[0mshell (%d) \e[0;32m$\e[0m ", getMyPid());

		signed char c;

		while (buffer_dim < MAX_BUFFER_SIZE && (c = getchar()) != '\n') {
			putchar(c);
			command_history_buffer[buffer_dim] = c;
			buffer[buffer_dim++] = c;
		}
    
    putchar(c);

    buffer[buffer_dim] = 0;

    if( buffer_dim == MAX_BUFFER_SIZE){
      perror("\e[0;31mShell buffer overflow\e[0m\n");
      buffer[0] = buffer_dim = 0;
			while (c != '\n') c = getchar();
			continue;
    };

    int parsed = 0;
    firstCommandRequestedFg = 0;

    while(parse_command(parsed==0? buffer:NULL, command, &parsedCommands[parsed])!=-1){
    	if ((++parsed % 5 == 0) && parsed >= parsed_commands_dim) {
				ParsedCommand *newParsedCommands = allocMemory(sizeof(ParsedCommand) * (parsed_commands_dim + 5));
				if (newParsedCommands == NULL) {
					perror("Memory allocation failed for parsed commands");
					freeMemory(parsedCommands);
					return -1;
				}
				memcpy(newParsedCommands, parsedCommands, sizeof(ParsedCommand) * (parsed_commands_dim));
				freeMemory(parsedCommands);
				parsed_commands_dim += 5;
				parsedCommands = newParsedCommands;
      };
    }


    if(parsed!=0){
      strncpy(command_history[command_history_last], command_history_buffer, 255);
      command_history[command_history_last][buffer_dim]='\0';
      INC_MOD(command_history_last,HISTORY_SIZE);
      last_command_arrowed=command_history_last;
    }
    int pipes[parsed+1];
    for(int p=0; p!= parsed; p++){
      if (parsedCommands[p].isBuiltIn) {
				parsedCommands[p].function(parsedCommands[p].argc, parsedCommands[p].argv);
			} 
      //else {
			// 	if (parsed > 1 && p != parsed - 1) {
			// 		pipes[p] = openPipe();
			// 		if (pipes[p] <= 0) {
			// 			fprintf(FD_STDOUT, "\e[0;31mError opening pipe\e[0m\n");
			// 			break;
			// 		}
			// 	}
      int targets[2]= {0,1};
      int requestsForeground =1;
      int newPid = createProcess(parsedCommands[p].argc,(char **)parsedCommands[p].argv, (void *)parsedCommands[p].function, 1, targets, requestsForeground);

    }


  }



  __builtin_unreachable();
  return 0;
}

static int printPreviousCommand(enum REGISTERABLE_KEYS scancode) {

  last_command_arrowed = SUB_MOD(last_command_arrowed, 1, HISTORY_SIZE);
  if (command_history[last_command_arrowed][0] != 0) {
    fprintf(FD_STDIN, command_history[last_command_arrowed]);
  }
  return 1;
}

static int printNextCommand(enum REGISTERABLE_KEYS scancode) {

  last_command_arrowed = (last_command_arrowed + 1) % HISTORY_SIZE;
  if (command_history[last_command_arrowed][0] != 0) {
    fprintf(FD_STDIN, command_history[last_command_arrowed]);
  }
  return 1;
}

static int deleteCharacter(enum REGISTERABLE_KEYS scancode) {
	if (buffer_dim > 0) {
		//clearScreenCharacter();
		buffer_dim--;
	}
	return 1;
}

static void emptyScreenBuffer(void) {
	while (buffer_dim > 0) {
		//clearScreenCharacter();
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
