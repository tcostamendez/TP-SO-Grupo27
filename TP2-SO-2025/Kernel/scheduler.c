#include "scheduler.h"
#include "strings.h"
#include "list.h"

ListADT ready_queue = NULL;


int cmp(Process * a, Process * b) {
    return a->priority - b->priority;
}

int nextPID= 0;
Process * proc_running= NULL;


void scheduler_init() {
    ready_queue = listCreate((int (*)(void *, void *))cmp, sizeof(Process));
    if (ready_queue == NULL) {
        return;
    }
    Process * idle = malloc(sizeof(Process));
    idle->pid=nextPID++;
    idle->name=(char*)malloc(sizeof(char)*(strlen("idle")+1));
    my_strcpy(idle->name,"idle");
    idle->stack = (uint8_t *) malloc(PROCESS_STACK_SIZE);
    idle->priority=0;
    idle->state=PROC_READY;
    listAddToTail(ready_queue,idle);
}

int queue_proc(Process * parentProcess){
    Process * newProc = mm_alloc(sizeof(Process));
    newProc->ppid = parentProcess->pid;
    newProc->pid = nextPID++;
    newProc->state = PROC_READY;
    newProc->name = (char*)mm_alloc(sizeof(char)*(strlen(parentProcess->name)+1));
    my_strcpy(newProc->name, parentProcess->name);
    newProc->stack = mm_alloc(PROCESS_STACK_SIZE);
    memcpy(newProc->stack,parentProcess->stack,PROCESS_STACK_SIZE);

    listAddTail(ready_queue, newProc);
    return newProc->pid;
}

uint64_t schedule(uint64_t current_rsp){
    if(proc_running!=NULL){
        proc_running->rsp = current_rsp;
        proc_running->state=PROC_READY;
        listAddTail(ready_queue,proc_running);
    }
    if(listGetSize(ready_queue)!=0){
        proc_running = (Process*) listRemoveFromHead(ready_queue);
        proc_running->state=PROC_RUNNING;
    }
    return proc_running ? proc_running->rsp : current_rsp;
}
