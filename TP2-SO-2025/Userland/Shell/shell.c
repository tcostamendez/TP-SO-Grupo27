#include <stddef.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <exceptions.h>
#include "sys.h"
#include "test_util.h"

#ifdef ANSI_4_BIT_COLOR_SUPPORT
#include <ansiColors.h>
#endif

static void *const snakeModuleAddress = (void *)0x500000;

#define MAX_BUFFER_SIZE 1024
#define HISTORY_SIZE 10

#define INC_MOD(x, m) x = (((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

#define MAX_ARGUMENT_COUNT 10
#define MAX_ARGUMENT_SIZE 256

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

int clear(void);
int echo(void);
int exit(void);
int fontdec(void);
int font(void);
int help(void);
int history(void);
int man(void);
int snake(void);
int regs(void);
int time(void);

// Process management commands
int ps(void);
int loop(void);
int kill(void);
int nice(void);
int block(void);
int mem(void);

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);

static uint8_t last_command_arrowed = 0;

typedef struct {
  char *name;
  int (*function)(void);
  char *description;
  char isBuiltin;
} Command;

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

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {
    {.name = "clear",
     .function = (int (*)(void))(unsigned long long)clear,
     .description = "Clears the screen",
     .isBuiltin = 1},
    {.name = "divzero",
     .function = (int (*)(void))(unsigned long long)_divzero,
     .description = "Generates a division by zero exception",
     .isBuiltin=1},
    {.name = "echo",
     .function = (int (*)(void))(unsigned long long)echo,
     .description = "Prints the input string",
     .isBuiltin=1},
    {.name = "exit",
     .function = (int (*)(void))(unsigned long long)exit,
     .description = "Command exits w/ the provided exit code or 0",
     .isBuiltin = 1},
    {.name = "font",
     .function = (int (*)(void))(unsigned long long)font,
     .description =
         "Increases or decreases the font size.\n\t\t\t\tUse:\n\t\t\t\t\t  + "
         "font increase\n\t\t\t\t\t  + font decrease",
      .isBuiltin = 1},
    {.name = "help",
     .function = (int (*)(void))(unsigned long long)help,
     .description = "Prints the available commands",
     .isBuiltin = 0},
    {.name = "history",
     .function = (int (*)(void))(unsigned long long)history,
     .description = "Prints the command history",
     .isBuiltin = 1},
    {.name = "invop",
     .function = (int (*)(void))(unsigned long long)_invalidopcode,
     .description = "Generates an invalid Opcode exception",
     .isBuiltin = 1},
    {.name = "regs",
     .function = (int (*)(void))(unsigned long long)regs,
     .description = "Prints the register snapshot, if any",
     .isBuiltin=1},
    {.name = "man",
     .function = (int (*)(void))(unsigned long long)man,
     .description = "Prints the description of the provided command",
     .isBuiltin=1},
    {.name = "snake",
     .function = (int (*)(void))(unsigned long long)snake,
     .description = "Launches the snake game",
     .isBuiltin=1},
    {.name = "testmm",
     .function = (int (*)(void))(unsigned long long)run_mm_test,
     .description = "Corre el test de stress del Memory Manager.\n\t\t\t\tUso: testmm <bytes>",
     .isBuiltin=1},
    {.name = "time",
     .function = (int (*)(void))(unsigned long long)time,
     .description = "Prints the current time",
     .isBuiltin=1},
    {.name = "ps",
     .function = (int (*)(void))(unsigned long long)ps,
     .description = "Lists all processes",
     .isBuiltin=1},
    {.name = "loop",
     .function = (int (*)(void))(unsigned long long)loop,
     .description = "Prints hello message every N seconds\n\t\t\t\tUso: loop <delay_seconds>",
     .isBuiltin=1},
    {.name = "kill",
     .function = (int (*)(void))(unsigned long long)kill,
     .description = "Kills a process by PID\n\t\t\t\tUso: kill <pid>",
     .isBuiltin=1},
    {.name = "nice",
     .function = (int (*)(void))(unsigned long long)nice,
     .description = "Changes process priority\n\t\t\t\tUso: nice <pid> <priority>",
     .isBuiltin=1},
    {.name = "block",
     .function = (int (*)(void))(unsigned long long)block,
     .description = "Blocks a process\n\t\t\t\tUso: block <pid>",
     .isBuiltin=1},
    {.name = "mem",
     .function = (int (*)(void))(unsigned long long)mem,
     .description = "Shows memory status",
     .isBuiltin=1},
};
const int commands_size = sizeof(commands) / sizeof(Command);

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint64_t last_command_output = 0;

typedef struct {
	char *name;
	int (*function)(int argc, char *argv[]);
	int argc;
	char *argv[MAX_ARGUMENT_COUNT];
	int isBuiltin;
} ParsedCommand;


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

  while(arg!=NULL && strcasecmp(arg, "|") && parsedCommand->argc < MAX_ARGUMENT_COUNT - 1){
    parsedCommand->argv[parsedCommand->argc] = malloc(strlen(arg)+1);
    if(parsedCommand->argv[parsedCommand->argc]=NULL){
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

	parsedCommand->isBuiltin = commands[command_i].isBuiltin;

	return 1;  
}


int main() {
  clear();

  registerKey(KP_UP_KEY, printPreviousCommand);
  registerKey(KP_DOWN_KEY, printNextCommand);
  registerKey(BACKSPACE_KEY, deleteCharacter);
  
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
    printf("\e[0mshell (%d) \e[0;32m$\e[0m ", getpid());

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
				ParsedCommand *newParsedCommands = malloc(sizeof(ParsedCommand) * (parsed_commands_dim + 5));
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
      if (parsedCommands[p].isBuiltin) {
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
      int newPid = createProcess((void *)parsedCommands[p].function, parsedCommands[p].argc,(char **)parsedCommands[p].argv, 1, targets, requestsForeground);

    }


  }



  __builtin_unreachable();
  return 0;
}

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode) {
  clearInputBuffer();
  last_command_arrowed = SUB_MOD(last_command_arrowed, 1, HISTORY_SIZE);
  if (command_history[last_command_arrowed][0] != 0) {
    fprintf(FD_STDIN, command_history[last_command_arrowed]);
  }
}

static void printNextCommand(enum REGISTERABLE_KEYS scancode) {
  clearInputBuffer();
  last_command_arrowed = (last_command_arrowed + 1) % HISTORY_SIZE;
  if (command_history[last_command_arrowed][0] != 0) {
    fprintf(FD_STDIN, command_history[last_command_arrowed]);
  }
}
static int deleteCharacter(enum REGISTERABLE_KEYS scancode, int Ctrl, int Alt, int Shift) {
	if (buffer_dim > 0) {
		clearScreenCharacter();
		buffer_dim--;
	}
	return 1;
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

int time(void) {
  int hour, minute, second;
  getDate(&hour, &minute, &second);
  printf("Current time: %xh %xm %xs\n", hour, minute, second);
  return 0;
}

int echo(void) {
  for (int i = strlen("echo") + 1; i < buffer_dim; i++) {
    switch (buffer[i]) {
    case '\\':
      switch (buffer[i + 1]) {
      case 'n':
        printf("\n");
        i++;
        break;
      case 'e':
#ifdef ANSI_4_BIT_COLOR_SUPPORT
        i++;
        parseANSI(buffer, &i);
#else
        while (buffer[i] != 'm')
          i++; // ignores escape code, assumes valid format
        i++;
#endif
        break;
      case 'r':
        printf("\r");
        i++;
        break;
      case '\\':
        i++;
      default:
        putchar(buffer[i]);
        break;
      }
      break;
    case '$':
      if (buffer[i + 1] == '?') {
        printf("%d", last_command_output);
        i++;
        break;
      }
    default:
      putchar(buffer[i]);
      break;
    }
  }
  printf("\n");
  return 0;
}

int help(void) {
  printf("Available commands:\n");
  for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
    printf("%s%s\t ---\t%s\n", commands[i].name,
           strlen(commands[i].name) < 4 ? "\t" : "", commands[i].description);
  }
  printf("\n");
  return 0;
}

int clear(void) {
  clearScreen();
  return 0;
}

int exit(void) {
  char *buffer = strtok(NULL, " ");
  int aux = 0;
  sscanf(buffer, "%d", &aux);
  return aux;
}

int font(void) {
  char *arg = strtok(NULL, " ");
  if (strcasecmp(arg, "increase") == 0) {
    return increaseFontSize();
  } else if (strcasecmp(arg, "decrease") == 0) {
    return decreaseFontSize();
  }

  perror("Invalid argument\n");
  return 0;
}

int man(void) {
  char *command = strtok(NULL, " ");

  if (command == NULL) {
    perror("No argument provided\n");
    return 1;
  }

  for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
    if (strcasecmp(commands[i].name, command) == 0) {
      printf("Command: %s\nInformation: %s\n", commands[i].name,
             commands[i].description);
      return 0;
    }
  }

  perror("Command not found\n");
  return 1;
}

int regs(void) {
  const static char *register_names[] = {
      "rax", "rbx", "rcx", "rdx", "rbp", "rdi", "rsi", "r8 ", "r9 ",
      "r10", "r11", "r12", "r13", "r14", "r15", "rsp", "rip", "rflags"};

  int64_t registers[18];

  uint8_t aux = getRegisterSnapshot(registers);

  if (aux == 0) {
    perror("No register snapshot available\n");
    return 1;
  }

  printf("Latest register snapshot:\n");

  for (int i = 0; i < 18; i++) {
    printf("\e[0;34m%s\e[0m: %x\n", register_names[i], registers[i]);
  }

  return 0;
}

int snake(void) { return exec(snakeModuleAddress); }

// Process management command implementations
int ps(void) {
    printf("Process List:\n");
    listProcesses();
    return 0;
}

int loop(void) {
    char *delay_str = strtok(NULL, " ");
    int delay = 1; // default 1 second
    
    if (delay_str != NULL) {
        sscanf(delay_str, "%d", &delay);
    }
    
    int pid = getMyPid();
    printf("Process %d: Hello! (delay: %d seconds)\n", pid, delay);
    
    while (1) {
        sleep(delay * 1000); // convert to milliseconds
        printf("Process %d: Hello again!\n", pid);
    }
    
    return 0;
}

int kill(void) {
    char *pid_str = strtok(NULL, " ");
    if (pid_str == NULL) {
        printf("Usage: kill <pid>\n");
        return 1;
    }
    
    int pid;
    sscanf(pid_str, "%d", &pid);
    
    int result = killProcess(pid);
    if (result == 0) {
        printf("Process %d killed successfully\n", pid);
    } else {
        printf("Failed to kill process %d\n", pid);
    }
    
    return result;
}

int nice(void) {
    char *pid_str = strtok(NULL, " ");
    char *priority_str = strtok(NULL, " ");
    
    if (pid_str == NULL || priority_str == NULL) {
        printf("Usage: nice <pid> <priority>\n");
        return 1;
    }
    
    int pid, priority;
    sscanf(pid_str, "%d", &pid);
    sscanf(priority_str, "%d", &priority);
    
    if (priority < 0 || priority > 3) {
        printf("Priority must be between 0 and 3\n");
        return 1;
    }
    
    setProcessPriority(pid, priority);
    printf("Process %d priority set to %d\n", pid, priority);
    return 0;
}

int block(void) {
    char *pid_str = strtok(NULL, " ");
    if (pid_str == NULL) {
        printf("Usage: block <pid>\n");
        return 1;
    }
    
    int pid;
    sscanf(pid_str, "%d", &pid);
    
    blockProcess(pid);
    printf("Process %d blocked\n", pid);
    return 0;
}

int mem(void) {
    // This would need memory status syscalls - for now just a placeholder
    printf("Memory status: (not implemented yet)\n");
    printf("Use 'testmm <bytes>' to test memory allocation\n");
    return 0;
}
