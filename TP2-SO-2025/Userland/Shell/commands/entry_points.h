#ifndef ENTRY_POINTS_H
#define ENTRY_POINTS_H

/* Entry point wrappers for process creation */
/* These functions properly extract argc/argv from registers and call the actual command functions */

extern void entry_block(void);
extern void entry_cat(void);
extern void entry_clear(void);
extern void entry_echo(void);
extern void entry_kill(void);
extern void entry_loop(void);
extern void entry_test_mm(void);
extern void entry_test_prio(void);
extern void entry_test_processes(void);
extern void entry_test_sync(void);
extern void entry_mem(void);
extern void entry_nice(void);
extern void entry_ps(void);
extern void entry_wc(void);
extern void entry_help(void);
extern void entry_filter(void);
extern void entry_mvar(void);
extern void entry_mvar_writer(void);
extern void entry_mvar_reader(void);
#endif

