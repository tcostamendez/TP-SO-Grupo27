#ifndef SHELL_H
#define SHELL_H

#define MAX_BUFFER_SIZE 1024
#define HISTORY_SIZE 10

#define INC_MOD(x, m) x = (((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

#define MAX_ARGUMENT_COUNT 10
#define MAX_ARGUMENT_SIZE 256

typedef struct {
	char *name;
	int (*function)(int argc, char * argv[]);
	char *description;
	int isBuiltIn;
} Command;


typedef struct {
	char *name;
	int (*function)(int argc, char *argv[]);
	int argc;
	char *argv[MAX_ARGUMENT_COUNT];
	int isBuiltIn;
} ParsedCommand;

#endif