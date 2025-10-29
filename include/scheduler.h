#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel.h"

int find_next_process();
void scheduler();

void sys_yield();
void sys_sleep(int ms);
void sys_exit();

#endif
