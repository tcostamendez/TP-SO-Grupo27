#ifndef COMMANDS_H
#define COMMANDS_H

int _block(int, char **);
int _cat(int, char **);
int _clear(int, char **);
int _echo(int, char **);
int _filter(int, char **);
int _font(int, char **);
int _help(int, char **);
int _kill(int, char **);
int _loop(int, char **);
int _man(int, char **);
int _mem(int, char **);
int _nice(int, char **);
int _orphan(int, char **);
int _ps(int, char **);
int _regs(int, char **);
int _snake(int, char **);
int _spawn(int, char **);
int _time(int, char **);
int _wc(int, char **);

// Tests
int _test_mm(int, char **);
int _test_processes(int, char **);
int _test_prio(int, char **);
int _test_sync(int, char **);

#endif