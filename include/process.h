#ifndef PROCESS_H
#define PROCESS_H

#include "kernel.h"

int create_process(void (*func)(void), const char *name, priority_t priority);
void init_kernel();

#endif
